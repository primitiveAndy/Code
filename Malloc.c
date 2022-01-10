/* Andy Sustaita
   Glenda Roberston
*/
#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct _block *)(ptr) - 1)
static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;
/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );
}
struct _block
{
   size_t  size;         /* Size of the allocated _block of memory in bytes */
   struct _block *prev;  /* Pointer to the previous _block of allcated memory   */
   struct _block *next;  /* Pointer to the next _block of allcated memory   */
   bool   free;          /* Is this _block free?                     */
   char   padding[3];
};
struct _block *freeList = NULL; /* Free list to track the _blocks available */
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
   struct _block *curr = freeList;
#if defined FIT && FIT == 0
   /* First fit */
   while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
#endif
#if defined BEST && BEST == 0
    struct _block *temp = curr;
    /*Gives a starting place for pointer */
    while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
   /*Finding the smallest size*/
   while(temp && temp->next != NULL)
   {
      temp = temp->next;
      if(temp->free && temp->size >=size)
      {
         if(temp->size < curr->size)
         {
            *last = temp;
            curr = temp;
         }
      }
   }
#endif
#if defined WORST && WORST == 0
       struct _block *temp = curr;
    /*Gives a starting place for pointer */
    while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr  = curr->next;
   }
   /*Finding the biggest size*/
   while(temp && temp->next != NULL)
   {
      temp = temp->next;
      if(temp->free && temp->size >=size)
      {
         if(temp->size >= curr->size)
         {
            *last = temp;
            curr = temp;
         }
      }
   }
#endif
#if defined NEXT && NEXT == 0
   struct _block *start = curr;
   /*starting place */
    while (curr && !(curr->free && curr->size >= size))
   {
      *last = curr;
      curr = curr->next;
   }
   while(curr && curr->next != start && !(curr->free && curr->size >= size))
   {
       *last=curr;
       start = curr;
       curr = curr->next;
       if(curr==NULL)
       {
         curr=freeList;
       }
  }
#endif
   return curr;
}
/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size)
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);
   assert(curr == prev);
   /* OS allocation failed */
   if (curr == (struct _block *)-1)
   {
      return NULL;
   }
   /* Update freeList if not set */
   if (freeList == NULL)
   {
      freeList = curr;
   }
   /* Attach new _block to prev _block */
   if (last)
   {
      last->next = curr;
   }
   /* Update _block metadata */
   curr->size = size;
   curr->next = NULL;
   curr->free = false;
   num_blocks ++;
   num_requested +=size;
   max_heap +=size;
   num_grows ++;
   return curr;
}
/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{
   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }
   /* Align to multiple of 4 */
   size = ALIGN4(size);
   /* Handle 0 size */
   if (size == 0)
   {
      return NULL;
   }
   /* Look for free _block */
   struct _block *last = freeList;
   struct _block *next = findFreeBlock(&last, size);
   /* TODO: Split free _block if possible */
   if (next != NULL)
   {
     next->size = next->size - size - sizeof(struct _block);
     num_splits ++;
     num_reuses ++;
   }
   /* Could not find free _block, so grow heap */
   if (next == NULL)
   {
      next = growHeap(last, size);
   }
   //hooks up prev to equal to last
    //next->prev=last;
   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL)
   {
      return NULL;
      num_requested +=size;
   }
   /* Mark _block as in use */
   next->free = false;
   /* Return data address associated with _block */
   num_mallocs ++;
   return BLOCK_DATA(next);
}
/*
 * \brief calloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes and size of array
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *calloc(size_t Csize, size_t num)
{
   void *blockC;
   size_t size;
   /* Align to multiple of num */
   size = num *Csize;
   /*Use malloc for logic */
   blockC = malloc(size);
   num_mallocs --;
   /*check that block was allocated*/
   if(!blockC)
   {
      return NULL;
   }
   /*set calloc to all zeros*/
   memset(blockC, 0, size);
   /* Return data address associated with _block */
   return blockC;
}
/*
 * \brief realloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes and size of array
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *realloc(void *block, size_t size)
{
   struct _block *reBlock;
   void *retVal;
   /*check that parameters are valid */
   if(!block || !size)
   {
      return malloc(size);
   }
   /*New Size*/
   reBlock = (struct _block*) block - 1; //why wont this work??
   /* If the reallocation in less than or same as size then dont reallocate*/
   if(reBlock->size >= size)
   {
      return block;
   }
   /*Getting new  memory */
   retVal = malloc(size);
   num_mallocs --;
   /*Reallocating */
   if(retVal)
   {
      memcpy(retVal, block, reBlock->size);
      free(block);
      num_frees --;
   }
   max_heap+=size;
   /* Return data address associated with _block */
   return retVal;
}
/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
   if (ptr == NULL)
   {
      return;
   }
   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;
   num_frees ++;
   /* TODO: Coalesce free _blocks if needed */
   if(curr->prev != NULL && curr->prev->free)
   {
      curr->prev->size = curr->prev->size + curr->size + sizeof(struct _block);
      curr->prev->next = curr->next;
      num_coalesces++;
   }
   if(curr->next != NULL && curr->next->free)
   {
      curr->size = curr->size + curr->next->size +sizeof(struct _block);
      curr->next = curr->next->next;
      num_coalesces++;
   }
}
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
