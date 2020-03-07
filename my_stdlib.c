#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>

#include "my_stdlib.h"
#include "custom_unistd.h"
#include "definitions.h"
void d(){ heap_dump_debug_information(); getchar(); }

pthread_mutex_t mutex;
pthread_mutexattr_t attr;
heap_control_t heap;
mem_fence_t fence;

void *heap_malloc(long int size, const char* file, long int  line){
  if ( size <= 0 )
    return NULL;

  pthread_mutex_lock(&mutex);
  assert( heap_validate() == 0 );

  size += 2*FENCE_SIZE;         //need to set fences
  block_control_t *new_block = find_fitting_block(size);  // find best fitting free block

  if ( new_block == NULL ){
    new_block = sbrk_request(size);     //try to alloc more memory for heap and init it
    errno = 0;
    if ( new_block == NULL ){
      pthread_mutex_unlock(&mutex);
      return NULL;
    }
  } else init_new_block(new_block,size);    // init existing free block

  heap.alloc_size += new_block->size;
  write_block_info(new_block, file, line);
  new_block->crs = check_sum(new_block);

  log("[ %s:%lu ] Malloc: Allocate new block (%p) size: %ld\n", file, line, (void *)new_block, new_block->size);
  pthread_mutex_unlock(&mutex);
  return (void *)( (char *)new_block + BLOCK_C_SIZE + FENCE_SIZE );
}
void *find_fitting_block(long int size){
  block_control_t *temp = heap.first_block->next;
  while ( !(temp->free && temp->size >= size) && temp != heap.last_block )    //looking for first match
    temp = temp->next;
  if ( temp == heap.last_block )
    return NULL;

  block_control_t *ret = temp;
  while( temp != heap.last_block ){      //looking for the best match
    if ( ret->size == size )
      break;
    if ( temp->free && temp->size >= size && ret->size > temp->size )
      ret = temp;
    temp = temp->next;
  }
  return (void *)ret;
}
void * sbrk_request(long int size){
  block_control_t *last = heap.last_block->prev;

  if ( last->free ){    //if last user block is free, use it to alloc
      long int req_size = size - last->size;
      custom_sbrk(req_size);
      if ( errno != 0 ){
        return NULL;}

      heap.last_block = (block_control_t *)( (char *)custom_sbrk(0) - BLOCK_C_SIZE );
      heap.last_block->prev = last;
      heap.last_block->next = NULL;
      heap.last_block->free = false;
      heap.last_block->size = 0;
      heap.last_block->crs = check_sum(heap.last_block);

      heap.size += req_size;
      heap.free_size -= last->size;

      last->next = heap.last_block;
      last->size = size - 2*FENCE_SIZE;
      last->free = false;

      init_fences(last);

      log(" --- Widening heap by %lu bytes (custom_sbrk used)\n", req_size);
      return (void *)(last);
  } else {          //else create new one
      long int req_size = size + BLOCK_C_SIZE;
      custom_sbrk(req_size);
      if ( errno != 0 ){
        return NULL;}

      block_control_t *new_block = heap.last_block;
      heap.last_block = (block_control_t *)( (char *)custom_sbrk(0) - BLOCK_C_SIZE );
      heap.last_block->prev = new_block;
      heap.last_block->next = NULL;
      heap.last_block->free = false;
      heap.last_block->size = 0;
      heap.last_block->crs = check_sum(heap.last_block);

      heap.size += req_size;
      heap.blocks_num++;

      last->next = new_block;
      last->crs = check_sum(last);

      new_block->next = heap.last_block;
      new_block->prev = last;
      new_block->size = size - 2*FENCE_SIZE;
      new_block->free = false;

      init_fences(new_block);

      log(" --- New Widening heap by %lu bytes (custom_sbrk used)\n", req_size);
      return (void *)(new_block);
  }

}
void *init_new_block(block_control_t *block, long int size){
  long int rest_of_free_size = block->size - size;
  if ( rest_of_free_size < MIN_SIZE-WORD_SIZE+1 ){   //if rest_of_free_size < block_t + 2*FENCES + 1bit, then alloc whole block
    block->free = false;
    block->size -= 2*FENCE_SIZE;
    block->free = false;

    init_fences(block);
    heap.free_size -= block->size + 2*FENCE_SIZE;
    return (void *)block;
  } else {                               // else create new free block next to this allocating one

    block_control_t *next_block = (block_control_t *)( (char *)block + BLOCK_C_SIZE + size );
    next_block->prev = block;
    next_block->next = block->next;
    block->next->prev = next_block;
    block->next = next_block;

    block->size = size - 2*FENCE_SIZE;
    block->free = false;
    next_block->size = rest_of_free_size - BLOCK_C_SIZE;
    next_block->free = true;
    write_block_info(next_block, __FILE__, (__LINE__) - 13);
    next_block->crs = check_sum(next_block);


    heap.free_size -= BLOCK_C_SIZE + size;
    heap.blocks_num++;
    return (void *)block;
  }
}

void *heap_calloc(long int number, long int size, const char* file, long int line){
  if ( number<=0 || size<=0 )
    return NULL;
  long int max = LONG_MAX/number;
  if ( size > max )
    return NULL;

  block_control_t *new_block = heap_malloc(number*size,file,line);
  if ( new_block == NULL ) return NULL;
  long int s = heap_get_block_size(new_block);
  memset(new_block,0,s);

  return (void *)new_block;
}

void heap_free(void *memblock, const char* file, long int line){
  if ( memblock == NULL )
    return;

  pthread_mutex_lock(&mutex);
  assert( heap_validate() == 0 );
  assert( get_pointer_type(memblock) == pointer_valid );

  block_control_t *block = (block_control_t *)( (char *)memblock-BLOCK_C_SIZE-FENCE_SIZE );
  if ( block->free ){
    pthread_mutex_unlock(&mutex);
    return;
  }

  block->free = true;
  heap.freed_size += block->size;       //freed as much as allocate
  block->size += 2*FENCE_SIZE;          //but new size is bigger by 2*FENCE_SIZE, cause fences are useless in free block
  heap.free_size += block->size;        //adding free_size to metadatas with 2*FENCES_SIZE
  log("[ %s:%lu ] Free: Free block (%p) size: %lu\n", file, line, (void *)block, block->size - 2*FENCE_SIZE);
  write_block_info(block, file, line);
  try_attach_adj(block);
  try_release_to_sbrk();
  errno = 0;
  pthread_mutex_unlock(&mutex);
}
void try_attach_adj(block_control_t *block){
  block_control_t *temp = block->next;
  while( temp->free ){
    temp->next->prev = block;
    block->next = temp->next;
    temp->next->crs = check_sum(temp->next);

    block->size += BLOCK_C_SIZE + temp->size;
    heap.free_size += BLOCK_C_SIZE;
    heap.blocks_num--;

    log(" --- Attached block (%p) to block (%p) attached size: %lu\n", (void *)temp, (void *)block, BLOCK_C_SIZE + temp->size);
    temp = temp->next;
  }

  temp = block->prev;
  while ( temp->free ){
    block->next->prev = temp;
    temp->next = block->next;
    temp->next->crs = check_sum(temp->next);

    temp->size += BLOCK_C_SIZE + block->size;
    heap.free_size += BLOCK_C_SIZE;
    heap.blocks_num--;

    log(" --- Attached block (%p) to block (%p) attached size: %lu\n", (void *)block, (void *)temp, BLOCK_C_SIZE + block->size);
    temp = temp->prev;
  }
}
void try_release_to_sbrk(){
    block_control_t *temp = heap.last_block->prev;
    if ( temp->free ){
      block_control_t *prev = temp->prev;

      long int rel_size = temp->size + BLOCK_C_SIZE;
      custom_sbrk(-rel_size);
      if ( errno != 0 )
        return;

      heap.last_block = temp;
      heap.last_block->size = 0;
      heap.last_block->free = false;
      heap.last_block->prev = prev;
      prev->next = heap.last_block;
      prev->crs = check_sum(prev);
      heap.last_block->crs = check_sum(heap.last_block);

      heap.free_size -= rel_size-BLOCK_C_SIZE;
      heap.size -= rel_size;
      heap.blocks_num--;

      log(" --- Releasing %lu bytes to sbrk\n", rel_size);
    }

}

void *heap_realloc(void* memblock, long int size, const char* file, long int line){
  if ( memblock == NULL )
    return heap_malloc(size,file,line);
  if ( size == 0 ){
    heap_free(memblock,file,line);
    return NULL;
  }

  pthread_mutex_lock(&mutex);
  assert( get_pointer_type(memblock) == pointer_valid );
  assert( heap_validate() == 0 );

  block_control_t *block = (block_control_t *)( (char *)memblock - BLOCK_C_SIZE - FENCE_SIZE );
  if ( block->size == size ){
      pthread_mutex_unlock(&mutex); return (void *)( memblock ); }
  if ( block->free ){
      pthread_mutex_unlock(&mutex); return NULL; }

  block_control_t *temp = block->next;
  if ( size > block->size ){      //user want to increase actual memory

    long int resize = size - block->size;
    if ( temp->free ){     //try to resize actual block
        long int no_destroy_size = temp->size - 2*FENCE_SIZE - 1;      //available memory without destroying next block
        if ( resize <= no_destroy_size ){
          log("[ %s:%lu ] Realloc: Reallocate(Next-destroyed: No) block (%p) old size: %lu  new size: %lu\n", file, line, (void *)block, block->size, size);
          write_block_info(block,file,line);
          return realloc_no_destroy(block,size);
        }
        int destroy_size = temp->size + BLOCK_C_SIZE; //available memory with destroying next block
        if ( resize <= destroy_size ){
          log("[ %s:%lu ] Realloc: Reallocate(Next-destroyed: Yes) block (%p) old size: %lu   new size: %lu\n", file, line, (void *)block, block->size, size);
          write_block_info(block,file,line);
          return realloc_destroy(block,size);
        }
    }

    temp = heap_malloc(size,file,line);
    if ( temp == NULL ){
      return NULL;
    }
    memcpy( temp, (char *)block+BLOCK_C_SIZE+FENCE_SIZE, block->size );   //copy data to new location
    write_block_info(block,file,line);
    h_free((char *)block+BLOCK_C_SIZE+FENCE_SIZE);
    return (void *)temp;

  } else {                        //user want to decrease actural memory

    long int resize = block->size - size;
    if ( temp->free ){
      log("[ %s:%lu ] Realloc: Reallocate(Next-shifting) block (%p) old size: %lu   new size: %lu\n", file, line, (void *)block, block->size, size);
      write_block_info(block,file,line);
      return realloc_decrease_free(block,size);
    } else if ( resize >= MIN_SIZE - WORD_SIZE + 1 ){
      log("[ %s:%lu ] Realloc: Reallocate(New-creating) block (%p) old size: %lu   new size: %lu\n", file, line, (void *)block, block->size, size);
      write_block_info(block,file,line);
      return realloc_decrease_occupied(block,size);
    }
  }


  pthread_mutex_unlock(&mutex);
  return NULL;
}
void *realloc_no_destroy(block_control_t *block, long int size){
  block_control_t *temp = block->next;
  block_control_t *temp_next = temp->next;
  long int resize = size - block->size;

  temp = (block_control_t *)( (char *)temp + resize );
  temp->free = true;
  temp->size = block->next->size - resize;
  temp->next = temp_next;
  temp->prev = block;

  temp_next->prev = temp;
  temp_next->crs = check_sum(temp_next);

  block->next = temp;
  block->size += resize;
  init_fences(block);
  block->crs = check_sum(block);

  heap.free_size -= resize;
  heap.alloc_size += resize;

  pthread_mutex_unlock(&mutex);
  return (void *)( (char *)block + BLOCK_C_SIZE + FENCE_SIZE );
}
void *realloc_destroy(block_control_t *block, long int size){
  block_control_t *temp = block->next;
  block_control_t *temp_next = temp->next;
  long int destroy_size = temp->size + BLOCK_C_SIZE;

  block->next = temp_next;
  block->size += destroy_size;
  init_fences(block);
  block->crs = check_sum(block);

  temp_next->prev = block;
  temp_next->crs = check_sum(temp->next);

  heap.free_size -= temp->size;
  heap.alloc_size += destroy_size;
  heap.blocks_num--;

  pthread_mutex_unlock(&mutex);
  return (void *)( (char *)block + BLOCK_C_SIZE + FENCE_SIZE );
}
void *realloc_decrease_free(block_control_t *block, long int size){
  block_control_t *temp = block->next;
  block_control_t *temp_next = temp->next;
  long int free_size = temp->size;
  long int resize = block->size - size;

  temp = (block_control_t *)( (char *)temp - resize );
  temp->next = temp_next;
  temp->prev = block;
  temp->free = true;
  temp->size = free_size + resize;

  temp_next->prev = temp;
  temp_next->crs = check_sum(temp_next);

  block->next = temp;
  block->size = size;
  init_fences(block);
  block->crs = check_sum(block);

  heap.free_size += resize;
  heap.freed_size += resize;

  pthread_mutex_unlock(&mutex);
  return (void *)( (char *)block + BLOCK_C_SIZE + FENCE_SIZE );
}
void *realloc_decrease_occupied(block_control_t *block, long int size){
  long int resize = block->size - size;
  block_control_t *new_block = (block_control_t *)( (char *)block + size + BLOCK_C_SIZE + FENCE_SIZE);

  new_block->size = resize - BLOCK_C_SIZE;
  new_block->next = block->next;
  new_block->prev = block;
  new_block->free = true;
  write_block_info(new_block, __FILE__, __LINE__);

  block->next = new_block;
  block->size = size;
  init_fences(block);
  block->crs = check_sum(block);

  heap.free_size += resize - BLOCK_C_SIZE;
  heap.freed_size += resize;
  heap.blocks_num++;

  pthread_mutex_unlock(&mutex);
  return (void *)( (char *)block + BLOCK_C_SIZE + FENCE_SIZE );
}

/*######### Aligned version #############*/

void* heap_malloc_aligned(long int  size, const char* file, long int line){
  if ( size <= 0 )
    return NULL;

  pthread_mutex_lock(&mutex);
  assert( heap_validate() == 0 );

  size += 2*FENCE_SIZE;      //need to set fences
  block_control_t *prev_block = NULL;
  block_control_t *new_block = find_fitting_block_aligned(size,&prev_block);

  if ( new_block == NULL ){
    new_block = sbrk_request_aligned(size);
    errno = 0;
    if ( new_block == NULL ){
      pthread_mutex_unlock(&mutex);
      return NULL;
    }
  } else init_new_block_aligned(size,new_block,prev_block);

  heap.alloc_size += new_block->size;
  log("[ %s:%lu ] Aligned_alloc: Allocate new block (%p) size: %lu\n", file, line, (void *)new_block, new_block->size);
  write_block_info(new_block,file,line);
  new_block->crs = check_sum(new_block);

  block_control_t *data_start = (block_control_t *)( (char *)new_block + BLOCK_C_SIZE + FENCE_SIZE );
  pthread_mutex_unlock(&mutex);
  if ( ( (intptr_t)data_start & (intptr_t)(PAGE_SIZE - 1) ) == 0 )
    return  ( void *)( data_start );
  else return NULL;
}
void *find_fitting_block_aligned(long int  size, block_control_t **prev_block){
  block_control_t *shifted_mem = (block_control_t *)( (char *)heap.first_block + PAGE_SIZE - BLOCK_C_SIZE - FENCE_SIZE );
  block_control_t *temp = heap_get_data_block_start(shifted_mem);
  intptr_t *last_cell = ( intptr_t *)( (char *)shifted_mem + FENCE_SIZE + size );

  while ( temp != NULL ){
    temp = (block_control_t *)( (char *)temp - BLOCK_C_SIZE - FENCE_SIZE );
    if ( temp->free && (intptr_t *)temp->next >= (intptr_t *)last_cell ){
      *prev_block = temp;
      return ( void *)shifted_mem;
    }

    last_cell = ( intptr_t *)( (char *)last_cell + PAGE_SIZE );
    shifted_mem = (block_control_t *)( (char *)shifted_mem + PAGE_SIZE );
    temp = heap_get_data_block_start(shifted_mem);
  }

  return NULL;
}
void *init_new_block_aligned(long int  size, block_control_t *block, block_control_t *prev_block){
  long int prev_rest = (char *)block - (char *)prev_block;
  long int prev_size = prev_block->size;
  intptr_t *last_cell_prev = (intptr_t *)( (char *)prev_block + prev_size + 2*FENCE_SIZE + BLOCK_C_SIZE );
  intptr_t *last_cell_new = (intptr_t *)( (char *)block + size + BLOCK_C_SIZE );
  block_control_t *next_block = prev_block->next;

  if ( prev_rest < MIN_SIZE-WORD_SIZE+1 ){      //need to destroy previous block
    heap.alloc_size += prev_rest;
    heap.free_size -= prev_rest;
    heap.blocks_num--;

    prev_block->prev->size += prev_rest;
    prev_block->prev->next = block;
    init_fences(prev_block->prev);
    prev_block->prev->crs = check_sum(prev_block->prev);

    block->prev = prev_block->prev;

    if ( prev_rest )  //log if there was any memory
      log(" --- Destroyed previous block\n");
  } else {
    block->prev = prev_block;
    prev_block->next = block;
    prev_block->size = prev_rest - BLOCK_C_SIZE;
    heap.free_size -= BLOCK_C_SIZE;
  }

  long int next_rest = (char *)last_cell_prev - (char *)last_cell_new;
  if ( next_rest >= MIN_SIZE-WORD_SIZE+1 ){         //creating new block
    block_control_t *new_block = (block_control_t *)last_cell_new;
    new_block->size = next_rest - BLOCK_C_SIZE - 2*FENCE_SIZE;
    new_block->free = true;
    new_block->next = next_block;
    new_block->prev = block;

    next_block->prev = new_block;
    next_block->crs = check_sum(next_block);

    block->next = new_block;
    block->size = size - 2*FENCE_SIZE;

    heap.blocks_num++;
    heap.free_size -= BLOCK_C_SIZE;

    log(" --- Creating next block\n");
  } else{
    block->next = next_block;
    next_block->prev = block;
    block->size += size + next_rest;
    heap.free_size -= next_rest;
  }

  heap.blocks_num++;
  heap.free_size -= size;

  block->free = false;
  init_fences(block);

  return (void *)block;
}
void *sbrk_request_aligned(long int  size){
  block_control_t *temp = heap.last_block;
  block_control_t *last = temp->prev;
  long int additional_size = (intptr_t)temp % PAGE_SIZE    ?    PAGE_SIZE - ((intptr_t)temp % PAGE_SIZE)    :   0;    //aligment to PAGE_SIZE
  additional_size -= BLOCK_C_SIZE + 1;
  long int req_size = size + additional_size + BLOCK_C_SIZE;
  custom_sbrk(req_size);
  if ( errno != 0 )
    return NULL;

  block_control_t *new_block = (block_control_t *)( (char *)heap.last_block + additional_size );
  heap.last_block = (block_control_t *)( (char *)custom_sbrk(0) - BLOCK_C_SIZE );
  heap.last_block->prev = new_block;
  heap.last_block->next = NULL;
  heap.last_block->size = 0;
  heap.last_block->free = false;

  new_block->next = heap.last_block;
  new_block->size = size-2*FENCE_SIZE;
  new_block->free = false;
  heap.size += req_size;
  heap.blocks_num++;

  if ( additional_size < MIN_SIZE-WORD_SIZE+1 ){
    last->size += additional_size;
    last->next = new_block;

    new_block->prev = last;

    heap.alloc_size += additional_size;
  } else {
    block_control_t *prev_block = (block_control_t *)temp;
    prev_block->size = additional_size - BLOCK_C_SIZE;
    prev_block->free = true;
    prev_block->next = new_block;
    prev_block->prev = last;

    last->next = prev_block;
    new_block->prev = prev_block;

    heap.blocks_num++;
    heap.free_size += prev_block->size;
    log(" --- Creating free block for aligment\n");
    write_block_info(prev_block,__FILE__,__LINE__);
  }

  init_fences(new_block);
  init_fences(last);
  heap.last_block->crs = check_sum(heap.last_block);
  last->crs = check_sum(last);

  log(" --- Widening heap by %lu bytes (custom_sbrk used)\n", req_size);
  return (void *)new_block;
}

void *heap_calloc_aligned(long int  number, long int size, const char* file, long int line){
  if ( number<=0 || size<=0 )
    return NULL;
  long int max = LONG_MAX/number;
  if ( size > max )
    return NULL;

  block_control_t *new_block = heap_malloc_aligned(number*size,file,line);
  if ( new_block == NULL ) return NULL;
  long int s = heap_get_block_size(new_block);
  memset(new_block,0,s);
  return (void *)new_block;
}

void *heap_realloc_aligned(void* memblock, long int size, const char* file, long int line){
  if ( memblock == NULL )
    return heap_malloc_aligned(size,file,line);
  if ( size == 0 ){
    heap_free(memblock,file,line);
    return NULL;
  }

  pthread_mutex_lock(&mutex);
  assert( get_pointer_type(memblock) == pointer_valid );
  assert( heap_validate() == 0 );

  block_control_t *block = (block_control_t *)( (char *)memblock - BLOCK_C_SIZE - FENCE_SIZE );
  if ( block->size == size ){
      pthread_mutex_unlock(&mutex); return (void *)( memblock ); }
  if ( block->free ){
      pthread_mutex_unlock(&mutex); return NULL; }

  block_control_t *temp = block->next;
  if ( size > block->size ){      //user want to increase actual memory

    long int resize = size - block->size;
    if ( temp->free ){     //try to resize actual block
        long int no_destroy_size = temp->size - 2*FENCE_SIZE - 1;      //available memory without destroying next block
        if ( resize <= no_destroy_size ){
          log("[ %s:%lu ] Realloc: Reallocate(Next-destroyed: No) block (%p) old size: %lu  new size: %lu\n", file, line, (void *)block, block->size, size);
          write_block_info(block,file,line);
          return realloc_no_destroy(block,size);
        }
        int destroy_size = temp->size + BLOCK_C_SIZE; //available memory with destroying next block
        if ( resize <= destroy_size ){
          log("[ %s:%lu ] Realloc: Reallocate(Next-destroyed: Yes) block (%p) old size: %lu   new size: %lu\n", file, line, (void *)block, block->size, size);
          write_block_info(block,file,line);
          return realloc_destroy(block,size);
        }
    }

    temp = heap_malloc_aligned(size,file,line);
    if ( temp == NULL ){
      return NULL;
    }
    memcpy( temp, (char *)block+BLOCK_C_SIZE+FENCE_SIZE, block->size );   //copy data to new location
    write_block_info(block,file,line);
    heap_free((char *)block+BLOCK_C_SIZE+FENCE_SIZE,__FILE__,__LINE__);
    return (void *)temp;

  } else {                        //user want to decrease actural memory

    long int resize = block->size - size;
    if ( temp->free ){
      log("[ %s:%lu ] Realloc: Reallocate(Next-shifting) block (%p) old size: %lu   new size: %lu\n", file, line, (void *)block, block->size, size);
      write_block_info(block,file,line);
      return realloc_decrease_free(block,size);
    } else if ( resize >= MIN_SIZE - WORD_SIZE + 1 ){
      log("[ %s:%lu ] Realloc: Reallocate(New-creating) block (%p) old size: %lu   new size: %lu\n", file, line, (void *)block, block->size, size);
      write_block_info(block,file,line);
      return realloc_decrease_occupied(block,size);
    }
  }


  pthread_mutex_unlock(&mutex);
  return NULL;
}

/*#########################################################*/
/*################## Additional functions #################*/
/*#########################################################*/

int check_sum(block_control_t *block){
  char *start = (char *)(block);
  int sum = 0;
  for (int i=0; i<BLOCK_C_SIZE - sizeof(int); ++i){   // - sizeof(int) cause we skipping crs field in counting
    sum += *(start + i);
  }
  return sum;
}
void *get_left_fence(block_control_t *block){
  return (uint8_t *)( (char *)block + BLOCK_C_SIZE );
}
void *get_right_fence(block_control_t *block){
  return (uint8_t *)( (char *)block + BLOCK_C_SIZE + block->size + FENCE_SIZE );
}
void init_fences(block_control_t *block){
  if ( heap.first_block == block || heap.last_block == block )
    return;
  uint8_t *fence_left = get_left_fence(block);       //init fences
  memcpy(fence_left, fence.left, FENCE_SIZE);
  uint8_t *fence_right = get_right_fence(block);
  memcpy(fence_right, fence.right, FENCE_SIZE);
}
void write_block_info(block_control_t *block, const char *filename, int line){
  if ( heap.first_block == block || heap.last_block == block )
    return;
  #if DEBUG
    strncpy(block->filename,filename,FN_SIZE);
    block->line_number = line;
    block->crs = check_sum(block);
  #endif
}

void heap_dump_debug_information(){
  printf("\n### Heap Information:\n");
  printf("   All info:\n");
  printf("    Heap first block: (%p)\n", (void *)heap.first_block);
  printf("    Heap last block: (%p)\n", (void *)heap.last_block );
  printf("    Heap total size: %lu\n", heap.size);
  printf("    Heap occupied size: %lu\n" , heap.size - heap.free_size );
  printf("    Heap free size: %lu\n", heap.free_size);
  printf("    Heap blocks num: %lu\n", heap.blocks_num);
  printf("    Heap biggest free block: %d\n", 0 );

  printf("   User info:\n");
  printf("    Heap allocated size: %lu\n", heap.alloc_size);
  printf("    Heap freed size: %lu\n\n", heap.freed_size);

  printf("    Block list: \n");
  int i=1;
  if ( heap.first_block != NULL ){
    block_control_t *temp =  heap.first_block->next;
    while(temp != heap.last_block){
      printf("      - ");
      #if DEBUG
        printf("[ %s.%u ] ", temp->filename, temp->line_number );
      #endif
      printf("Block.%d    prev:%p (%p) %p:next    ....size: %lu....free: %s\n",i++, (void *)temp->prev, (void *)temp,  (void *)temp->next, temp->size, temp->free ? "true" : "false" );
      temp = temp->next;
    } printf("\n");
  }
}
int heap_setup(){
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&mutex, &attr);

  pthread_mutex_lock(&mutex);
  log("### Start info: \n   BLOCK_C_SIZE: %lu\n   FENCE_SIZE: %lu\n   FENCE_UNIT_NUM: %d\n\n",
                            BLOCK_C_SIZE,         FENCE_SIZE,         FENCE_UNIT_NUM );
  unsigned int size = 2*BLOCK_C_SIZE;
  heap.first_block = (block_control_t *)custom_sbrk(size);
  if ( errno != 0 ){
    pthread_mutex_unlock(&mutex);
    return -1;
  }
  heap.last_block = (block_control_t *)( (char *)custom_sbrk(0) - BLOCK_C_SIZE );

  srand(time(NULL));
  for (int i=0; i<FENCE_SIZE; ++i){     //init fences
    fence.left[i] = rand();
    fence.right[i] = rand();
  }

  heap.size = size;
  heap.blocks_num = 2;                        //first and last
  heap.free_size = heap.alloc_size = heap.freed_size = 0;

  heap.first_block->next = heap.last_block;   //first block init
  heap.first_block->prev = NULL;
  heap.first_block->size = 0;
  heap.first_block->free = false;
  heap.first_block->crs = check_sum(heap.first_block);

  heap.last_block->next = NULL;               //last block init
  heap.last_block->prev = heap.first_block;
  heap.last_block->size = 0;
  heap.last_block->free = false;
  heap.last_block->crs = check_sum(heap.last_block);

  pthread_mutex_unlock(&mutex);
  return 0;
}

long int heap_get_free_space(){ return heap.free_size; }
long int heap_get_used_space(){ return heap.size - heap.free_size; }
long int heap_get_used_blocks_count(){
  if ( heap.first_block == NULL )
    return 0;
  pthread_mutex_lock(&mutex);
  block_control_t *temp = heap.first_block->next;
  long int used_blocks = 0;
  while( temp != heap.last_block ){
    if ( !temp->free )
      used_blocks++;
    temp = temp->next;
  }
  pthread_mutex_unlock(&mutex);
  return used_blocks;
}
long int heap_get_free_gaps_count() {
  if ( heap.first_block == NULL )
    return 0;
  pthread_mutex_lock(&mutex);
  block_control_t *temp = heap.first_block->next;
  long int free_blocks = 0;
  while( temp != heap.last_block ){
    if ( temp->free && temp->size >= MIN_SIZE-BLOCK_C_SIZE )
      free_blocks++;
    temp = temp->next;
  }
  pthread_mutex_unlock(&mutex);
  return free_blocks;
}
long int heap_get_largest_used_block_size() {
  if ( heap.first_block == NULL )
    return 0;
  pthread_mutex_lock(&mutex);
  long int ret = 0;
  block_control_t *temp = heap.first_block->next;
  while (temp != heap.last_block){
    if ( !temp->free && temp->size > ret )
      ret = temp->size;
    temp = temp->next;
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}
long int heap_get_largest_free_area(){
  if ( heap.first_block == NULL )
    return 0;
  pthread_mutex_lock(&mutex);
  long int ret = 0;
  block_control_t *temp = heap.first_block->next;
  while (temp != heap.last_block){
    if ( temp->free && temp->size > ret )
      ret = temp->size;
    temp = temp->next;
  }
  pthread_mutex_unlock(&mutex);
  return ret;
}

enum pointer_type_t get_pointer_type(const void* pointer){
  if ( pointer == NULL )
    return pointer_null;

  block_control_t *memblock = heap_get_data_block_start(pointer);
  pthread_mutex_lock(&mutex);

  if ( memblock == NULL ){
    pthread_mutex_unlock(&mutex); return pointer_out_of_heap; }
  block_control_t *block = (block_control_t *)( (char *)memblock - BLOCK_C_SIZE - FENCE_SIZE );
  if ( (void *)block == pointer ){
    pthread_mutex_unlock(&mutex); return pointer_control_block; }
  else if ( (void *)memblock  == pointer ){
    pthread_mutex_unlock(&mutex); return pointer_valid; }
  else if ( pointer > (void *)memblock ){
    if ( block->free ) {
      pthread_mutex_unlock(&mutex); return pointer_unallocated; }
    else {
      pthread_mutex_unlock(&mutex); return pointer_inside_data_block; }
  }
  else {
    pthread_mutex_unlock(&mutex); return pointer_control_block; }
}
void* heap_get_data_block_start(const void* pointer){
  if ( pointer == NULL )
    return NULL;
  void *last_block_finish = (void *)( (char *)heap.first_block + BLOCK_C_SIZE );
  if ( heap.first_block == NULL || pointer < last_block_finish || pointer >= (void *)heap.last_block )
    return NULL;

  pthread_mutex_lock(&mutex);
  block_control_t *temp = heap.first_block->next;
  while( (void *)temp < pointer )
    temp = temp->next;
  pthread_mutex_unlock(&mutex);

  if ( (void *)temp == pointer )
    return (void *)( (char *)temp + BLOCK_C_SIZE + FENCE_SIZE );
  temp = temp->prev;
  return (void *)( (char *)temp + BLOCK_C_SIZE + FENCE_SIZE );
}
long int heap_get_block_size(const void* memblock){
  if ( heap.first_block == NULL )
    return 0;
  if ( get_pointer_type(memblock) != pointer_valid )
    return 0;
  block_control_t *block = (block_control_t *)( (char *)memblock - BLOCK_C_SIZE - FENCE_SIZE );
  return block->size;
}

int heap_validate(void){
  pthread_mutex_lock(&mutex);
  block_control_t *first = heap.first_block;
  block_control_t *last = heap.last_block;
  if ( !first || !last ) {
    pthread_mutex_unlock(&mutex); return -1; }
  if ( last != (block_control_t *)( (char *)custom_sbrk(0) - BLOCK_C_SIZE ) ) {
    pthread_mutex_unlock(&mutex); return -2; }
  if ( (char *)( (char *)last + BLOCK_C_SIZE ) - (char *)first != heap.size ) {
    pthread_mutex_unlock(&mutex); return -3; }
  if ( first->crs != check_sum(first) ) {
    pthread_mutex_unlock(&mutex); return -4; }
  if ( last->crs != check_sum(last) ) {
    pthread_mutex_unlock(&mutex); return -5; }

  long int blocks_used = 0;
  long int blocks_free = 0;
  long int free_size = 0;
  long int used_size = 0;
  block_control_t *t = first->next;
  uint8_t *fence_l, *fence_r;

  while ( t != last ){
    if ( !t->free ){
      if ( t->crs != check_sum(t) ) {
        pthread_mutex_unlock(&mutex); return -6; }
      fence_l = get_left_fence(t);
      fence_r = get_right_fence(t);
      if ( memcmp(&fence.left,fence_l,FENCE_SIZE)!=0 ) {
        pthread_mutex_unlock(&mutex); return -7; }
      if ( memcmp(&fence.right,fence_r,FENCE_SIZE)!=0) {
        pthread_mutex_unlock(&mutex); return -8; }
      used_size += t->size;
      blocks_used++;
    } else {
      free_size += t->size;
      blocks_free++;
    }
    t = t->next;
  }

  if ( blocks_used + blocks_free + 2 != heap.blocks_num ) {
    pthread_mutex_unlock(&mutex); return -9; }
  if ( free_size != heap.free_size ) {
    pthread_mutex_unlock(&mutex); return -10; }
  if ( free_size + used_size + heap.blocks_num*BLOCK_C_SIZE + blocks_used*(2*FENCE_SIZE)!= heap.size ) {
    pthread_mutex_unlock(&mutex); return -11; }

  pthread_mutex_unlock(&mutex);
  return 0;
}
int heap_destroy(){
  assert( heap_validate() == 0 );
  pthread_mutex_lock(&mutex);
  custom_sbrk(-2*BLOCK_C_SIZE);
  if ( errno != 0 ){
    pthread_mutex_unlock(&mutex); return -1;
  }
  heap.size = heap.free_size = heap.blocks_num = 0;
  heap.first_block = heap.last_block = NULL;
  heap.alloc_size = heap.freed_size = 0;
  pthread_mutex_unlock(&mutex);
  return 0;
}
