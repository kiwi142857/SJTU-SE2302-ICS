#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"
#include "mm.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

/* word size and double words size */
#define WSIZE 4
#define DSIZE 8

/* heap extend size */
#define INITCHUNKSIZE (1 << 6)
#define CHUNKSIZE (1 << 12)
#define REALLOCCHUNKSIZE (1 << 6)

/* max and min */
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define MIN(x, y) ((x) < (y) ? (x) : (y))

/* pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* read and write a word */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* read the size and allocated fields */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* transform between address and offset */
#define GET_OFFSET(bp) ((char *)bp - (char *)heap_listp)
#define GET_ADDR(offset) (heap_listp + offset)

/* compute address of header, footer, predecessor and successor */
#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define PRED(bp) (bp)
#define SUCC(bp) ((bp) + WSIZE)

/* compute address of previous and next blocks according to address */
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE(((char *)(bp)-DSIZE)))
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp)-WSIZE)))

/* compute address of previous and next free block in the list */
#define PREV_FREEP(bp) ((GET(PRED(bp))) ? (GET_ADDR(GET(PRED(bp)))) : NULL)
#define NEXT_FREEP(bp) ((GET(SUCC(bp))) ? (GET_ADDR(GET(SUCC(bp)))) : NULL)

/* length of segregated free list */
#define LISTLENGTH 16
#define LINEARLENGTH 5
#define LINEARMAX ((1 << (LINEARLENGTH - 3)) - 2)

/* if size > BIGSIZE, place at right side */
#define BIGSIZE (108)

/* global variables */
static void *heap_listp;
static void **free_listps;

/* helper functions */
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void *place(void *bp, size_t asize);
static void *insert_freeblk(void *bp);
static void *remove_freeblk(void *bp);
static int get_free_list_index(size_t size);

static int verbose = 1;
int mm_check(void);

/*
 *  mm_check - check the consistency of the heap
 */
int mm_check(void)
{
    /* Is every block in the free list marked as free? */
    /* Do the pointers in the free list point to valid free blocks? */
    for (int i = 0; i < LISTLENGTH; ++i) {
        void *bp = free_listps[i];
        while (bp) {
            if (GET_ALLOC(HDRP(bp))) {
                fprintf(stderr, "Error: Block in free list is not marked as free\n");
                return -1;
            }
            bp = NEXT_FREEP(bp);
        }
    }

    /* Are there any contiguous free blocks that somehow escaped coalescing? */
    void *bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) > 0) {
        if (!GET_ALLOC(HDRP(bp)) || !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) {
            fprintf(stderr, "Error: Contiguous free blocks escaped coalescing\n");
            return -1;
        }
        bp = NEXT_BLKP(bp);
    }

    /* Is every free block actually in the free list? */
    bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) > 0) {
        void *freebp = NULL;
        for (int i = 0; i < LISTLENGTH; ++i) {
            freebp = free_listps[i];
            while (freebp) {
                if (freebp == bp)
                    break;
                freebp = NEXT_FREEP(freebp);
            }
            if (freebp)
                break;
        }
        if (!freebp) {
            fprintf(stderr, "Error: Free block not in free list\n");
            return -1;
        }
    }

    /* Do any allocated blocks overlap? */
    bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) > 0) {
        if (!GET_ALLOC(HDRP(bp))) {
            bp = NEXT_BLKP(bp);
            continue;
        }
        void *next = NEXT_BLKP(bp);
        if (GET_SIZE(HDRP(bp)) > 0 && !GET_ALLOC(HDRP(next)) && (char *)bp + GET_SIZE(HDRP(bp)) > (char *)next) {
            fprintf(stderr, "Error: Allocated blocks overlap\n");
            return -1;
        }
        bp = next;
    }

    /* Do the pointers in a heap block point to valid heap addresses? */
    bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) > 0) {
        if (PREV_BLKP(bp) < heap_listp || PREV_BLKP(bp) > mem_heap_hi() || NEXT_BLKP(bp) < heap_listp ||
            NEXT_BLKP(bp) > mem_heap_hi()) {
            fprintf(stderr, "Error: Pointers in a heap block point to invalid heap addresses\n");
            return -1;
        }
        bp = NEXT_BLKP(bp);
    }

    return 0;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    
    if(verbose) {
        printf("Initializing heap...\n");
    }

    /* create the initial empty heap */
    heap_listp = mem_sbrk(ALIGN(LISTLENGTH * WSIZE) + 4 * WSIZE);
    if (heap_listp == (void *)-1)
        return -1;

    /* initialize the free list */
    free_listps = heap_listp;
    for (int i = 0; i < LISTLENGTH; ++i) {
        PUT(free_listps + i * WSIZE, 0);
    }

    /* initialize the heap */
    heap_listp += ALIGN(LISTLENGTH * WSIZE) + 2 * WSIZE;
    PUT(HDRP(heap_listp), PACK(DSIZE, 1));
    PUT(FTRP(heap_listp), PACK(DSIZE, 1));
    PUT(HDRP(NEXT_BLKP(heap_listp)), PACK(0, 1));

    /* extend the empty heap with a free block */
    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;

    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if(verbose) {
        printf("Allocating %d bytes...\n", size);
    }
    if (!size)
        return NULL;

    /* find a block of asize */
    size_t asize = ALIGN(size) + DSIZE;
    void *bp = find_fit(asize);

    /* if not found, extend */
    if (bp == NULL) {
        size_t exsize = MAX(asize, CHUNKSIZE);
        bp = extend_heap(exsize);
        if (bp == NULL)
            return NULL;
    }

    /* place asize in the block found */
    bp = place(bp, asize);

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if(verbose) {
        printf("Freeing block %p...\n", ptr);
    }
    if (!ptr)
        return;

    /* set the allocated bit to 0 */
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));

    /* coalesce and insert to free list */
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    if(verbose) {
        printf("Reallocating block %p to %d bytes...\n", ptr, size);
    }
    /* if ptr is NULL, just malloc it */
    if (!ptr)
        return mm_malloc(size);
    /* if size is 0, just free it */
    if (!size) {
        mm_free(ptr);
        return NULL;
    }

    size_t newsize = ALIGN(size) + DSIZE; /* new actual size */
    size_t oldsize = GET_SIZE(HDRP(ptr)); /* old block size */
    size_t remainsize =
        GET_ALLOC(HDRP(NEXT_BLKP(ptr))) ? 0 : GET_SIZE(HDRP(NEXT_BLKP(ptr))); /* remain size after the block */

    /* case: size not change, directly return */
    if (newsize == oldsize)
        return ptr;

    /* case: size change smaller */
    if (newsize < oldsize) {
        /* if remain size < smallest block size, directly return */
        if (oldsize - newsize + remainsize < DSIZE * 2)
            return ptr;

        /* else place at left side */
        PUT(HDRP(ptr), PACK(newsize, 1));
        PUT(FTRP(ptr), PACK(newsize, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize - newsize, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(oldsize - newsize, 0));
        coalesce(NEXT_BLKP(ptr));
        return ptr;
    }

    /* case: size change larger */
    int can_extend = (remainsize && !GET_SIZE(HDRP(NEXT_BLKP(NEXT_BLKP(ptr))))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)));

    /* if no enough remain size but can extend, extend and get new remain size */
    if (remainsize < newsize - oldsize && can_extend) {
        extend_heap(MAX(newsize - oldsize - remainsize, REALLOCCHUNKSIZE));
        remainsize = GET_SIZE(HDRP(NEXT_BLKP(ptr)));
    }

    /* if remain size enough */
    if (remainsize >= newsize - oldsize) {
        /* if new remain size < smallest block size, simply coalesce */
        if (oldsize + remainsize - newsize < 16) {
            remove_freeblk(NEXT_BLKP(ptr));
            PUT(HDRP(ptr), PACK(oldsize + remainsize, 1));
            PUT(FTRP(ptr), PACK(oldsize + remainsize, 1));
            return ptr;
        }

        /* else place at left side */
        remove_freeblk(NEXT_BLKP(ptr));
        PUT(HDRP(ptr), PACK(newsize, 1));
        PUT(FTRP(ptr), PACK(newsize, 1));
        PUT(HDRP(NEXT_BLKP(ptr)), PACK(oldsize + remainsize - newsize, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(oldsize + remainsize - newsize, 0));
        insert_freeblk(NEXT_BLKP(ptr));
        return ptr;
    }

    /* else malloc a new block */
    void *newptr = mm_malloc(size);
    if (!newptr)
        return NULL;
    memcpy(newptr, ptr, oldsize - DSIZE);
    mm_free(ptr);

    return newptr;
}

/*
 * extend_heap - extend heap to contain larger size
 */
static void *extend_heap(size_t size)
{
    if(verbose) {
        printf("Extending heap by %d bytes...\n", size);
    }
    /* allocate an even number of words to maintain alignment */
    size = ALIGN(size);
    char *bp = mem_sbrk(size);
    if (bp == (void *)-1)
        return NULL;

    /* initialize free bock header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */

    /* coalesce if the previous block was free */
    return coalesce(bp);
}

/*
 * coalesce - insert current block to free list and coalesce adjecent free blocks
 */
static void *coalesce(void *bp)
{
    if(verbose) {
        printf("Coalescing block %p...\n", bp);
    }
    void *prev = PREV_BLKP(bp);
    void *next = NEXT_BLKP(bp);
    size_t prev_alloc = GET_ALLOC(HDRP(prev));
    size_t next_alloc = GET_ALLOC(HDRP(next));
    size_t size = GET_SIZE(HDRP(bp));

    /* case: no coalesce */
    if (prev_alloc && next_alloc) {
        insert_freeblk(bp);
    }
    /* case: coalesce with next block */
    else if (prev_alloc && !next_alloc) {
        remove_freeblk(next);
        size += GET_SIZE(HDRP(next));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_freeblk(bp);
    }
    /* case: coalesce with previous block */
    else if (!prev_alloc && next_alloc) {
        remove_freeblk(prev);
        size += GET_SIZE(HDRP(prev));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(prev), PACK(size, 0));
        insert_freeblk(prev);
        bp = prev;
    }
    /* case: coalesce with both next block and previous block*/
    else {
        remove_freeblk(next);
        remove_freeblk(prev);
        size += GET_SIZE(HDRP(next)) + GET_SIZE(HDRP(prev));
        PUT(HDRP(prev), PACK(size, 0));
        PUT(FTRP(next), PACK(size, 0));
        insert_freeblk(prev);
        bp = prev;
    }

    return bp;
}

/*
 * find_fit - find befitting free blocks for malloc
 */
void *find_fit(size_t asize)
{   
    if(verbose) {
        printf("Finding fit for %d bytes...\n", asize);
    }
    if (!asize)
        return NULL;

    /* get the first idx to search */
    int idx = get_free_list_index(asize);

    /* search each free list after */
    for (int i = idx; i < LISTLENGTH; ++i) {
        void *bp = free_listps + i * WSIZE;
        while (bp) {
            if (GET_SIZE(HDRP(bp)) >= asize)
                return bp;

            bp = NEXT_FREEP(bp);
        }
    }

    return NULL;
}

/*
 * place - place the given size to given free block
 */
void *place(void *bp, size_t asize)
{
    if(verbose) {
        printf("Placing %d bytes in block %p...\n", asize, bp);
    }
    size_t size = GET_SIZE(HDRP(bp));
    size_t remain = size - asize;

    /* remove given free block */
    remove_freeblk(bp);

    /* if remain size < smallest size, no division */
    if (remain < 16) {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));
        return bp;
    }

    /* if big size, place at right side */
    else if (asize > BIGSIZE) {
        PUT(HDRP(bp), PACK(remain, 0));
        PUT(FTRP(bp), PACK(remain, 0));
        void *next = NEXT_BLKP(bp);
        PUT(HDRP(next), PACK(asize, 1));
        PUT(FTRP(next), PACK(asize, 1));
        insert_freeblk(bp);
        return next;
    }

    /* if small size, place at left size */
    else {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        void *next = NEXT_BLKP(bp);
        PUT(HDRP(next), PACK(remain, 0));
        PUT(FTRP(next), PACK(remain, 0));
        insert_freeblk(next);
        return bp;
    }
}

/*
 * insert_freeblk - insert given block to suitable free block list
 */
static void *insert_freeblk(void *bp)
{
    if(verbose) {
        printf("Inserting block %p to free list...\n", bp);
    }
    size_t size = GET_SIZE(HDRP(bp));

    /* get the free list to insert */
    int index = get_free_list_index(size);
    void *free_listpp = free_listps + index * WSIZE;
    /* whether the first block */
    if(GET(free_listpp) == 0) {
        PUT(free_listpp, GET_OFFSET(bp));
        return bp;
    } else {
        void *next = GET_ADDR(GET(free_listpp));
        PUT(PRED(bp), 0);
        PUT(SUCC(bp), GET(free_listpp));
        PUT(PRED(next), GET_OFFSET(bp));
        PUT(free_listpp, GET_OFFSET(bp));
    }

    return bp;
}

/*
 * remove_freeblk - remove given block from suitable free block list
 */
static void *remove_freeblk(void *bp)
{
    if(verbose) {
        printf("Removing block %p from free list...\n", bp);
    }
    size_t size = GET_SIZE(HDRP(bp));
    int index = get_free_list_index(size);
    void *free_listpp = free_listps + index * WSIZE;

    void *pred = PREV_FREEP(bp);
    void *succ = NEXT_FREEP(bp);

    if (pred) {
        PUT(SUCC(pred), GET(SUCC(bp)));
    } else {
        PUT(free_listpp, GET(SUCC(bp)));
    }

    if (succ) {
        PUT(PRED(succ), GET(PRED(bp)));
    }

    return bp;
}

/*
 * get_free_listp - get free list by size
 */
static void **get_free_listpp(size_t size)
{
    if(verbose) {
        printf("Getting free list for %d bytes...\n", size);
    }
    /* linear part */
    size_t linear_index = size / DSIZE - 2;
    if (linear_index < LINEARMAX) {
        return &free_listps[linear_index];
    }

    /* power of 2 part */
    size_t search = (size - 1) >> LINEARLENGTH;
    for (int i = LINEARMAX; i < LISTLENGTH - 1; ++i) {
        if (!search)
            return &free_listps[i];

        search >>= 1;
    }

    /* largest ones */
    return &free_listps[LISTLENGTH - 1];
}

/*
* get_free_list_index - get free list index by size
*/
static int get_free_list_index(size_t size)
{
    if(verbose) {
        printf("Getting free list index for %d bytes...\n", size);
    }
    /* linear part */
    size_t linear_index = size / DSIZE - 2;
    if (linear_index < LINEARMAX) {
        return linear_index;
    }

    /* power of 2 part */
    size_t search = (size - 1) >> LINEARLENGTH;
    for (int i = LINEARMAX; i < LISTLENGTH - 1; ++i) {
        if (!search)
            return i;

        search >>= 1;
    }

    /* largest ones */
    return LISTLENGTH - 1;
}