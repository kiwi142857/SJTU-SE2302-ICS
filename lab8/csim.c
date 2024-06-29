/* student ID : 522031910299 */
/* student Name : 陈启炜 */
/* 思路： ## Part A

整体思路：面向对象建立Cache的模型，Cache中划分为Set的数组，个数为2^s个，Set中包含E条Line。
除此之外还必须包含Cache的信息：s,E,b,hits,misses,evivtions的次数，useLog（表示使用时间戳的计数）。
Line中包含valid，tag，lastUsed信息（应该要有个Block）。
init part和free part：malloc和free相应构建。

Load函数：首先用位运算计算setIndex，去相应的Set中，通过Tag匹配对应的Line。
查看Valid情况，判断hit/miss。
遍历Line观察是否有invalid line可以replace，若无，则eviction++ && replace by the lastUsed info.

mainPart 处理argv 参数：模仿之前的Lab。
*/
#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int valid;
    unsigned long tag;
    int lastUsed;
} Line;

typedef struct {
    Line *lines;
} Set;

typedef struct {
    Set *sets;
    int s, E, b;
    int hit, miss, eviction;
    int useLog;
} Cache;

Cache cache;

int vobase = 0;
void initCache(int s, int E, int b)
{
    cache.s = s;
    cache.E = E;
    cache.b = b;

    cache.hit = 0;
    cache.miss = 0;
    cache.eviction = 0;
    cache.useLog = 0;

    int S = 1 << s;
    cache.sets = (Set *)malloc(S * sizeof(Set));
    for (int i = 0; i < S; i++) {
        cache.sets[i].lines = (Line *)malloc(E * sizeof(Line));
        for (int j = 0; j < E; j++) {
            cache.sets[i].lines[j].valid = 0;
            cache.sets[i].lines[j].tag = 0;
            cache.sets[i].lines[j].lastUsed = 0;
        }
    }
}

void freeCache()
{
    int S = 1 << cache.s;
    for (int i = 0; i < S; i++) {
        free(cache.sets[i].lines);
    }
    free(cache.sets);
}

void load(unsigned long address)
{
    unsigned long tag = address >> (cache.s + cache.b);
    unsigned long setIndex = (address >> cache.b) & ((1 << cache.s) - 1);

    Set set = cache.sets[setIndex];
    for (int i = 0; i < cache.E; i++) {
        Line line = set.lines[i];
        if (line.valid && line.tag == tag) {
            line.lastUsed = cache.useLog++;
            set.lines[i] = line;
            if (vobase)
                printf("%s \n", "hit");
            cache.hit++;
            return;
        }
    }

    cache.miss++;
    if (vobase)
        printf("%s \n", "miss");
    for (int i = 0; i < cache.E; i++) {
        Line line = set.lines[i];
        if (!line.valid) {
            line.valid = 1;
            line.tag = tag;
            line.lastUsed = cache.useLog++;
            set.lines[i] = line;
            return;
        }
    }

    cache.eviction++;
    if (vobase)
        printf("%s \n", "eviction");
    int min = 0;
    for (int i = 1; i < cache.E; i++) {
        if (set.lines[i].lastUsed < set.lines[min].lastUsed) {
            min = i;
        }
    }
    set.lines[min].tag = tag;
    set.lines[min].lastUsed = cache.useLog++;
}

void store(unsigned long address)
{
    load(address);
}

void printHelp()
{
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n");
    printf("Options:\n"
           "  -h         Print this help message.\n"
           "  -v         Optional verbose flag."
           "  -s <num>   Number of set index bits.\n"
           "  -E <num>   Number of lines per set.\n"
           "  -b <num>   Number of block offset bits.\n"
           "  -t <file>  Trace file.\n"
           "\n"
           "Examples:\n"
           "  linux>  ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n"
           "  linux>  ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

/* Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile> */

int main(int argc, char **argv)
{
    int s, E, b;
    char *tracefile;
    char opt;

    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
        case 'h':
            printHelp();
            exit(0);
        case 'v':
            vobase = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            tracefile = optarg;
            break;
        default:
            printHelp();
            exit(1);
        }
    }

    // Initialize the cache
    initCache(s, E, b);

    // Read the trace file
    FILE *fp = fopen(tracefile, "r");
    if (!fp) {
        printf("Error: Cannot open file %s\n", tracefile);
        return 1;
    }

    char buf[100];
    while (fgets(buf, 100, fp)) {
        if (buf[0] == 'I')
            continue;
        char type;
        unsigned long address;
        int size;
        sscanf(buf, " %c %lx,%d", &type, &address, &size);
        switch (type) {
        case 'L':
            load(address);
            break;
        case 'S':
            store(address);
            break;
        case 'M':
            load(address);
            store(address);
            break;
        default:
            break;
        }
    }

    fclose(fp);
    freeCache();
    printSummary(cache.hit, cache.miss, cache.eviction);
    return 0;
}
