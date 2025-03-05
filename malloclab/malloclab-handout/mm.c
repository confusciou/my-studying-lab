/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

//分配器独有的全局变量，指向序言快（的下一个块）
static char* heap_listp;
static char* cur_listp;
/* single word (4) or double word (8) alignment */
#define W_size 4
#define D_size 8

#define ALIGNMENT 8
#define chunksize (1<<12)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

//Max
#define Max(x,y) ((x)>(y)?(x):(y))

//块头部内容
#define Head(size,alloc) ((size)|(alloc))

// hp 指向块头部标记位    bp 指向块的有效载荷部的开始

//强制转换
#define getp(hp) (*(unsigned int*)(hp))
//操作块(32)
#define Put(hp,val) (*(unsigned int*)(hp) = (val))

//获取块大小
#define Get_size(hp) (*(unsigned int*)(hp) & ~0x7)
//获取块状态
#define Get_alloc(hp) (*(unsigned int *)(hp) & 0x1)
//获取头，脚 指针
#define Get_head(bp) ((char*)(bp) - W_size)  //我才明白为什么这里是char，因为定义大小用的是字节
#define Get_foot(bp) ((char*)(bp) + Get_size(Get_head(bp)) - D_size)
//next pre
#define Get_next(bp) ((char*)(bp) + Get_size(Get_head(bp)))
#define Get_pre(bp) ((char*)(bp) - Get_size((char*)(bp) - D_size))
// 按pdf推荐，准备一个check函数//
void mm_check(void){
/*1，free列表是否有误,是否包含所有空闲块
  2,是否有相邻空闲块
  3，分配块是否重叠
  4，指针是否有效
*/

}
static void* coalesce(void* bp);
static void* extend_heap(size_t words);
static void mm_place(char* bp, size_t asize);
static void* find_fit(size_t size);
/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if((heap_listp = mem_sbrk(4*W_size)) == (void*)(-1)){
      return -1;
    }
    //
    // printf("\nmm_init:\n");
    // printf("heap start == %p\n",heap_listp);
    //
    Put(heap_listp,0);
    Put(heap_listp + W_size,Head(D_size,1));
    Put(heap_listp + 2*W_size,Head(D_size,1));//序言块
    Put(heap_listp + 3*W_size,Head(0,1)); //结尾块
    heap_listp += 2*W_size;
    cur_listp = heap_listp;
    //
    // printf("heap_listp == %p\n",heap_listp);
    //
    if(extend_heap(chunksize/W_size) == (void*)-1){
      return -1;
    }
    return 0;

}
void* extend_heap(size_t words){
    char *bp;
    size_t asize = (words%2)?(words+1)*W_size : words*W_size;
    if((void*)(bp = mem_sbrk(asize)) == (void*)-1){
      return (void*)-1;
    }
    //mem_sbrk 返回原来的堆顶指针
    Put(Get_head(bp),Head(asize,0));
    Put(Get_foot(bp),Head(asize,0));
    Put(Get_head(Get_next(bp)),Head(0,1));
    //清除及设置终止块
    //拼合终止块和扩展块
    // printf("extend_size = %d , brk = %p\n",asize,bp);
    return coalesce(bp);
}

//空块，与前后合并（如果前后有空）
void* coalesce(void* bp){
  size_t pre_alloc = Get_alloc(Get_foot(Get_pre(bp)));
  size_t next_alloc = Get_alloc(Get_foot(Get_next(bp)));
  size_t size = Get_size(Get_head(bp));

  if(pre_alloc && next_alloc){
    return bp;
  }
  else if(!pre_alloc && next_alloc){
    bp = Get_pre(bp);
    size += Get_size(Get_head(bp));
    Put(Get_head(bp),Head(size,0));
    Put(Get_foot(bp),Head(size,0));
    return bp;
  }
  else if(pre_alloc && !next_alloc){
    size += Get_size(Get_head(Get_next(bp)));
    Put(Get_head(bp),Head(size,0));
    Put(Get_foot(bp),Head(size,0));
    return bp;
  }
  else{
    size += Get_size(Get_head(Get_pre(bp))) + Get_size(Get_head(Get_next(bp)));
    bp = Get_pre(bp);
    Put(Get_head(bp),Head(size,0));
    Put(Get_foot(bp),Head(size,0));
    return bp;
  }
  return NULL;
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
//查找匹配块
void* find_fit(size_t asize){
    //next次适配
  // void *bp;
  // for(bp = cur_listp; Get_size(Get_head(bp)) > 0; bp = Get_next(bp)){
  //   if(!Get_alloc(Get_head(bp)) && (asize <= Get_size(Get_head(bp)))){
  //     cur_listp = bp;
  //     return bp;
  //   }
  // }
  // for(bp = heap_listp; bp != cur_listp; bp = Get_next(bp)){
  //   if(!Get_alloc(Get_head(bp)) && (asize <= Get_size(Get_head(bp)))){
  //     cur_listp = bp;
  //     return bp;
  //   }
  // }
  // return NULL;
  for (char* bp = heap_listp; Get_size(Get_head(bp)) > 0; bp = Get_next(bp))
    {
        if (!Get_alloc(Get_head(bp)) && Get_size(Get_head(bp)) >= asize)
        {
            return bp;
        }
    }
    return NULL;
}
void mm_place(char* bp, size_t asize){
    size_t b_size = Get_size(Get_head(bp));
    // //
    // printf("b_size = %d\n",b_size);
    // printf("a_size = %d\n",asize);
    // //
    if((b_size - asize) >= 2*D_size){
        Put(Get_head(bp),Head(asize,1));
        Put(Get_foot(bp),Head(asize,1));
        bp = Get_next(bp);
        Put(Get_head(bp),Head((b_size-asize),0));
        Put(Get_foot(bp),Head((b_size-asize),0));
    }
    else{
      Put(Get_head(bp),Head(b_size,1));
      Put(Get_foot(bp),Head(b_size,1));
    }
    // printf("mm_place == %p\n",bp);
}
void *mm_malloc(size_t size)
{
    char* bp;
    // size_t asize = ALIGN(size + SIZE_T_SIZE);
    size_t asize;

    if(size == 0){
        return NULL;
    }
    if(size <= D_size){
      asize = 2*D_size;
    }
    else{
      asize = D_size * ((size + (D_size) + (D_size-1)) / D_size);
    }

    size_t extend_size;

    // printf("-----one malloc-----\n");
    // printf("asize == %d\n",asize);

    if((bp = find_fit(asize)) != NULL){
      mm_place(bp,asize);
      return bp;
    }
    extend_size = Max(asize,chunksize);
    // if((bp = extend_heap(extend_size/sizeof(size_t))) == NULL){
    if((bp = extend_heap(extend_size/W_size)) == NULL){
      return NULL;
    }
    mm_place(bp,asize);
    return bp;
 }

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
/*检测，ptr指向已分配块
  没有返回
  可以追加测试
  */
    size_t size = Get_size(Get_head(bp));
    Put(Get_head(bp),Head(size,0));
    Put(Get_foot(bp),Head(size,0));
    coalesce(bp);

}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    // void *oldptr = ptr;
    // void *newptr;
    // size_t copySize;    //旧内存块的大小
    
    // newptr = mm_malloc(size);
    // if (newptr == NULL)
    //   return NULL;
    // copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    // if (size < copySize)
    //   copySize = size;
    // memcpy(newptr, oldptr, copySize);   //复制copeSize大小的数据至newptr中
    // mm_free(oldptr);
    // return newptr;
  size_t old_size, new_size, extendsize;
    void *old_ptr, *new_ptr;

    if (ptr == NULL) {
        return mm_malloc(size);
    }
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    new_size = ALIGN(size + 2*W_size);
    old_size = Get_size(Get_head(ptr));
    old_ptr = ptr;
    if (old_size >= new_size) {
        if (old_size - new_size >= 2*D_size) {  //分割内存块
            mm_place(old_ptr, new_size);
            return old_ptr;
        } else {   //剩余块小于最小块大小，不分割
            return old_ptr;
        }
    } else {  //释放原内存块，寻找新内存块
        if ((new_ptr = find_fit(new_size)) == NULL) {  //无合适内存块
            extendsize = Max(new_size, chunksize);
            if ((new_ptr = extend_heap(extendsize)) == NULL)   //拓展堆空间
                return NULL;
        }
        mm_place(new_ptr, new_size);
        memcpy(new_ptr, old_ptr, old_size - 2*W_size);
        mm_free(old_ptr);
        return new_ptr;
    }
}













