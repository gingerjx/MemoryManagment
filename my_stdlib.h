#if !defined(_MYSTDLIB_H_)
#define _MYSTDLIB_H_
#include <stddef.h>

enum pointer_type_t{
  pointer_null,
  pointer_out_of_heap,
  pointer_control_block,
  pointer_inside_data_block,
  pointer_unallocated,
  pointer_valid
};

void *heap_malloc(long int size, const char* file, long int  line);
#define h_malloc(_size)   heap_malloc((_size), __FILE__, __LINE__);

void *heap_calloc(long int  number, long int  size, const char* file, long int  line);
#define h_calloc(_num,_size)  heap_calloc((_num), (_size), __FILE__, __LINE__);

void heap_free(void* memblock, const char* file, long int  line);
#define h_free(_memblock)   heap_free( (_memblock), __FILE__, __LINE__);

void *heap_realloc(void* memblock, long int  size, const char* file, long int  line);
#define h_realloc(_memblock,_size)   heap_realloc( (_memblock), (_size), __FILE__, __LINE__ );

/*######### Aligned version #############*/

void* heap_malloc_aligned(long int  size, const char* file, long int  line);
#define h_malloc_aligned(_size)   heap_malloc_aligned((_size), __FILE__, __LINE__);

void *heap_calloc_aligned(long int  number, long int  size, const char* file, long int  line);
#define h_calloc_aligned(_num,_size)  heap_calloc_aligned((_num), (_size), __FILE__, __LINE__);

void *heap_realloc_aligned(void* memblock, long int  size, const char* file, long int  line);
#define h_realloc_aligned(_memblock,_size)   heap_realloc_aligned( (_memblock), (_size), __FILE__, __LINE__ );




/*#########################################################*/
/*################## Additional functions #################*/
/*#########################################################*/

int heap_setup();
void heap_dump_debug_information();

long int  heap_get_free_space();
long int  heap_get_used_space();
long int  heap_get_used_blocks_count();
long int  heap_get_free_gaps_count();
long int  heap_get_largest_used_block_size();
long int  heap_get_largest_free_area();

enum pointer_type_t get_pointer_type(const void* pointer);
void* heap_get_data_block_start(const void* pointer);
long int  heap_get_block_size(const void* memblock);

int heap_validate(void);

int heap_destroy();


#endif // _MYSTDLIB_H_
