/*
 * proxy.c - ICS Web proxy
 *
 *
 */

#include "csapp.h"
#include <stdarg.h>
#include <sys/select.h>

/*
 * The server_fd or client_fd or server_rio or client_rio is the server indeed or the client indeed.
 * I almost confused the server and the client when I wrote the code.
 * So I have to write here to remind myself.
 */
/*
 * Function prototypes
 */

int parse_uri(char *uri, char *target_addr, char *path, char *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size);
int acceptClient(char *client_hostname, char *client_port, int listenfd);
int connectToServerAndSend(char *hostname, char *port, char *method, char *pathname, char *versio, rio_t *rio_client,
                           rio_t *rio);
int sendRequest(int serverfd, char *request_line, rio_t *rio_client, int content_length, char *method);
int sendResponse(int clientfd, rio_t *rio_server, char *response_line, int content_length);
int parse_request_line(char *buf, char *method, char *uri, char *version)
{
    return sscanf(buf, "%s %s %s", method, uri, version);
}

struct arg_t {
    int connfd;
    struct sockaddr_in clientaddr;
} thread_arg_t;

sem_t log_mutex;
/*
 * Rio error checking wrappers
 */
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t rc;

    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        fprintf(stderr, "Rio_readnb error\n");
        return 0;
    }
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t rc;

    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        fprintf(stderr, "Rio_readlineb error\n");
        return 0;
    }
    return rc;
}

int Rio_writen_w(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n) {
        fprintf(stderr, "Rio_writen error\n");
        return -1;
    }
    return 0;
}

void thread_routine(void *thread_arg);
/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    Signal(SIGPIPE, SIG_IGN); // ignore SIGPIPE (broken pipe error)

    int listenfd, connfd;
    int clientlen = sizeof(struct sockaddr_storage);
    pthread_t tid;
    struct arg_t *thread_arg;

    listenfd = Open_listenfd(argv[1]);

    /* init sem */
    Sem_init(&log_mutex, 0, 1);

    while (1) {
        thread_arg = Malloc(sizeof(struct arg_t));
        connfd = Accept(listenfd, (SA *)&thread_arg->clientaddr, &clientlen);
        thread_arg->connfd = connfd;
        if (pthread_create(&tid, NULL, &thread_routine, thread_arg) != 0) {
            Free(thread_arg);
            fprintf(stderr, "pthread_create error\n");
            exit(0);
        }
    }
    Close(listenfd);
    exit(0);
}

void thread_routine(void *thread_arg)
{

    Pthread_detach(pthread_self());
    // printf("thread_routine created.\n");
    // Get request from client  method uri version
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE], port[MAXLINE];
    rio_t rio_client, rio_server;
    char request_line[MAXLINE];
    memset(request_line, 0, sizeof(request_line));
    
    // Get request from client
    Rio_readinitb(&rio_client, ((struct arg_t *)thread_arg)->connfd);
    if (Rio_readlineb_w(&rio_client, buf, MAXLINE) <= 0) {
        Close(((struct arg_t *)thread_arg)->connfd);
        Free(thread_arg);
        return;
    }
    if (parse_request_line(buf, method, uri, version) < 3) {
        // printf("parse_request_line failed\n");
        Close(((struct arg_t *)thread_arg)->connfd);
        Free(thread_arg);
        return;
    }
    // // printf("method: %s, uri: %s, version: %s\n", method, uri, version);
    // printf("method is %s\n", method);
    parse_uri(uri, hostname, pathname, port);
    if (pathname[0] == '\0') {
        strcpy(pathname, "/");
    }
    // // printf("hostname: %s, pathname: %s, port: %s\n", hostname, pathname, port);

    sprintf(request_line, "%s %s %s\r\n", method, pathname, version);
    // get content_length
    int content_length = 0;
    while (Rio_readlineb_w(&rio_client, buf, MAXLINE) > 0) {
        if (strcmp(buf, "\r\n") == 0) {
            sprintf(request_line, "%s%s", request_line, buf);
            break;
        }
        if (strstr(buf, "Content-Length") != NULL) {
            sscanf(buf, "Content-Length: %d", &content_length);
        }
        // add request line to request
        sprintf(request_line, "%s%s", request_line, buf);
    }
    // // printf("request line: %s", request_line);

    // if the last char in request_line is not '\n', we assume that the request is invalid
    if (request_line[strlen(request_line) - 1] != '\n') {
        // printf("invalid request\n");
        Close(((struct arg_t *)thread_arg)->connfd);
        Free(thread_arg);
        return;
    }

    // connect to server and send request
    int serverfd = open_clientfd(hostname, port);
    if (serverfd < 0) {
        // printf("open_clientfd failed\n");
        Close(((struct arg_t *)thread_arg)->connfd);
        Free(thread_arg);
        return;
    }
    rio_readinitb(&rio_server, serverfd);
    if (sendRequest(serverfd, request_line, &rio_client, content_length, method) < 0) {
        // printf("sendRequest failed\n");
        Close(serverfd);
        Close(((struct arg_t *)thread_arg)->connfd);
        Free(thread_arg);
        return;
    }
    int response_length = 0;
    char response_line[MAXLINE];
    memset(response_line, 0, sizeof(response_line));

    // read response header from server
    int byteReceived = 0;
    int TotalByteReceived = 0;
    memset(buf, 0, MAXLINE);
    while ((byteReceived = Rio_readlineb_w(&rio_server, buf, MAXLINE)) > 0) {
        if (strcmp(buf, "\r\n") == 0) {
            sprintf(response_line, "%s%s", response_line, buf);
            break;
        }
        if (strstr(buf, "Content-Length") != NULL) {
            sscanf(buf, "Content-Length: %d", &response_length);
        }
        // add response line to response
        sprintf(response_line, "%s%s", response_line, buf);
        TotalByteReceived += byteReceived;
    }
    // printf("response header from server:\n %s", response_line);
    // printf("\nend of response header\n");
    // send response to client
    byteReceived = sendResponse(((struct arg_t *)thread_arg)->connfd, &rio_server, response_line, response_length);
    if (byteReceived < 0) {
        // printf("sendResponse failed\n");
        Close(serverfd);
        Close(((struct arg_t *)thread_arg)->connfd);
        Free(thread_arg);
        return;
    }
    // printf("send response to client success\n");
    TotalByteReceived += byteReceived;
    TotalByteReceived += 2;
    // printf("TotalByteReceived: %d\n", TotalByteReceived);
    // printf("close serverfd: %d\n", serverfd);
    Close(serverfd);
    Close(((struct arg_t *)thread_arg)->connfd);
    char logstring[MAXLINE];
    memset(logstring, 0, sizeof(logstring));
    format_log_entry(logstring, &((struct arg_t *)thread_arg)->clientaddr, uri, TotalByteReceived);
    P(&log_mutex);
    printf("%s\n", logstring);
    V(&log_mutex);
    Free(thread_arg);
}

int sendRequest(int serverfd, char *request_line, rio_t *rio_client, int content_length, char *method)
{
    char buf[MAXLINE];
    // printf("when send request: serverfd: %d\n", serverfd);
    if (Rio_writen_w(serverfd, request_line, strlen(request_line)) < 0) {
        // printf("Rio_writen_w failed\n");
        return -1;
    }
    // // printf("send request line: %s", request_line);
    if (strcasecmp(method, "GET") != 0) {
        int n = 0;

        while (n < content_length) {
            if (Rio_readnb_w(rio_client, buf, 1) <= 0) {
                return -1;
            }
            n++;

            if (Rio_writen_w(serverfd, buf, 1) < 0) {
                return -1;
            }
        }
    }
    // printf("\nsend request success\n");
    return 0;
}

int sendResponse(int clientfd, rio_t *rio_server, char *response_line, int content_length)
{
    // printf("send response to client, the clientfd is %d\n", clientfd);
    char buf[MAXLINE];
    if (Rio_writen_w(clientfd, response_line, strlen(response_line)) < 0) {
        return -1;
    }
    if (content_length > 0) {
        int n = 0;
        while (n < content_length) {
            if (Rio_readnb_w(rio_server, buf, 1) == 0) {
                return 0;
            }

            n++;
            Rio_writen_w(clientfd, buf, 1);
        }
    }

    return content_length;
}
/*
 * parse_uri - URI parser
 *
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, char *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    if (hostend == NULL)
        return -1;
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';

    /* Extract the port number */
    if (*hostend == ':') {
        char *p = hostend + 1;
        while (isdigit(*p))
            *port++ = *p++;
        *port = '\0';
    } else {
        strcpy(port, "80");
    }

    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
        pathname[0] = '\0';
    } else {
        // pathbegin++;
        strcpy(pathname, pathbegin);
        /* modify */
        /* if (strlen(pathname) == 0) {
            strcpy(pathname, "/");
        } */
    }

    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), the number of bytes
 * from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, size_t size)
{
    time_t now;
    char time_str[MAXLINE];
    char host[INET_ADDRSTRLEN];

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    if (inet_ntop(AF_INET, &sockaddr->sin_addr, host, sizeof(host)) == NULL)
        unix_error("Convert sockaddr_in to string representation failed\n");

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %s %s %zu", time_str, host, uri, size);
}
