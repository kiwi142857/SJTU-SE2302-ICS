/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    /* create the initial empty heap */
    heap_listp = mem_sbrk(ALIGN(LISTLENGTH * WSIZE) + 4 * WSIZE);
    if (heap_listp == (void *)-1)
        return -1;

    /* initialize the free list */
    free_listps = heap_listp;
    for (int i = 0; i < LISTLENGTH; ++i) {
        PUT(heap_listp + i * WSIZE, 0);
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