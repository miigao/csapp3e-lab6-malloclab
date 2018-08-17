/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * miigao
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "faux_pas",
    /* First member's full name */
    "miigao",
    /* First member's email address */
    "*****@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT (8)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define MAX(x, y) ((x)>(y)?(x):(y))
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define PACK(size, alloc) ((size)|(alloc))
#define GET(p) (*(unsigned int *)(p))
#define GET_NEXT(p) (*(unsigned int **)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define PUT_NEXT(p, _address) (*(unsigned int **)(p) = (_address))
#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define EXPONENT 8
#define THRESHOLD (1 << EXPONENT)
static char *heap_listp;
static unsigned int **array_p;
int array_size;
int mm_check(void);
void *find_start(size_t size)
{
	int e = EXPONENT;
	unsigned int **p = array_p;
	if (size <= THRESHOLD)
		p += (size/ALIGNMENT - 2);
	else if (size <= 4096) {
		while((1 << e) < size){
			e++;
		}
		p += (THRESHOLD/ALIGNMENT - 2) + (e - EXPONENT);
	}
	else
		p += array_size/WSIZE - 1;
	return p;
}
void *find_fit(size_t size)
{
	unsigned int **p = (unsigned int **)find_start(size);
	unsigned int *pp;
	pp = GET_NEXT(p);
	p++;
	while (1) {
		while (pp != NULL && GET_SIZE(HDRP(pp)) < size) {
			pp = GET_NEXT(pp);
		}
		if (pp == NULL && p != (array_p + array_size/WSIZE)) {
			pp = GET_NEXT(p);
			p++;
		}
		else break;
	}
	return (void *)pp;
}
void delete_from_array(void * bp)
{
	unsigned int *pre = NULL;
	unsigned int *p = (unsigned int *)find_start(GET_SIZE(HDRP(bp)));
	p = GET_NEXT(p);
	while (p != bp) {
		pre = p;
		p = GET_NEXT(p);
	}
	if (pre == NULL) {
		PUT(find_start(GET_SIZE(HDRP(bp))), GET(p));
		PUT_NEXT(p, NULL);
	}
	else {
		PUT(pre, GET(p));
		PUT_NEXT(p, NULL);
	}
}
void back_to_array(void * bp, size_t size)
{
	//void *pre = NULL;
	void *p = find_start(size);
	if (GET_NEXT(p) == NULL) {		//empty
		PUT_NEXT(bp, NULL);
		PUT(p, (long)bp);
	}
	else if (GET_SIZE(HDRP(GET_NEXT(p))) >= GET_SIZE(HDRP(bp))) {		//insert at front
		PUT(bp, GET(p));
		PUT(p, (long)bp);
	}
	else {
		p = GET_NEXT(p);
		while (GET_NEXT(p) != NULL && GET_SIZE(HDRP(p)) < GET_SIZE(HDRP(bp))) {
			p = GET_NEXT(p);
		}
		if (GET_NEXT(p) == NULL) {
			PUT_NEXT(bp, NULL);
			PUT(p, (long)bp);
		}
		else {
			PUT(bp, (long)p);
			PUT(p, (long)bp);
		}
	}
}
void place(void *bp, size_t size, int in_array)
{
	void *pp = NULL;
	size_t tmp_size = GET_SIZE(HDRP(bp));
	if (in_array)
		delete_from_array(bp);
	if (tmp_size >= size + DSIZE) {
		PUT(HDRP(bp), PACK(size, 1));
		PUT(FTRP(bp), PACK(size, 1));
		pp = (char *)bp + size;
		PUT(HDRP(pp), PACK(tmp_size - size, 0));
		PUT(FTRP(pp), PACK(tmp_size - size, 0));
		back_to_array(pp, tmp_size - size);
	}
	else {
		PUT(HDRP(bp), PACK(tmp_size, 1));
		PUT(FTRP(bp), PACK(tmp_size, 1));
	}
}
void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));
	if (prev_alloc && next_alloc) {
		back_to_array(bp, size);
		return bp;
	}
	else if (prev_alloc && !next_alloc) {
		delete_from_array(NEXT_BLKP(bp));
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}
	else if (!prev_alloc && next_alloc) {
		delete_from_array(PREV_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	else {
		delete_from_array(PREV_BLKP(bp));
		delete_from_array(NEXT_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	back_to_array(bp, size);
	return bp;
}
void *extend_heap(size_t words)
{
	char *bp;
	size_t size;
	size = (words % 2) ? (words + 1)*WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size)) == -1)	//extension failure
		return NULL;
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
	return coalesce(bp);
}
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
	unsigned int **p;
	int prologue_size;
	array_size = THRESHOLD/2 + (12 - EXPONENT)*4;
	array_size = DSIZE * ((array_size + (DSIZE - 1)) / DSIZE);
	prologue_size = array_size + DSIZE;
	if ((heap_listp = mem_sbrk(prologue_size + DSIZE)) == (void *)-1)
		return -1;
	PUT(heap_listp, 0);		// padding for alignment
	PUT(heap_listp + (1*WSIZE), PACK(prologue_size, 1));
	PUT(heap_listp + (2*WSIZE + array_size), PACK(prologue_size, 1));
	PUT(heap_listp + (3*WSIZE + array_size), PACK(0, 1));	//Epilogue header
	heap_listp += (2*WSIZE);
	array_p = (unsigned int **)heap_listp;
	for (p = array_p; p < array_p + array_size/WSIZE; p++) {
		*p = NULL;
	}
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
	size_t extendsize;
	char *bp;
	if (size == 0)
		return NULL;
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize, 1);
		return bp;
	}
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, asize, 1);
	return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));
	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE;
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
/*
 * mm_check - A heap checker
 */
int mm_check(void)
{
	unsigned int **p1, *p2;
	int yes;
	printf("heap_listp:%p\n", heap_listp);
	for (p1 = array_p; p1 < array_p + array_size/WSIZE; p1++) {
		yes = 0;
		for (p2 = GET_NEXT(p1); p2 != NULL; p2 = GET_NEXT(p2)) {
			yes = 1;
			printf("%d ", GET_SIZE(HDRP(p2)));
		}
		if (yes) {
			printf(" address: %p", p1);
			printf("\n");
		}
	}
	return 0;
}












