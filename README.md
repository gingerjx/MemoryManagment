# MemoryManagment
  Project provides implementation of thread-safe functions for dynamic allocation. <br>
  Custom_sbrk derive memory from system, which we will manage. <br>
  User should use functions only from 'my_stdlib.h':
  - <b>'void* h_malloc(_size)'</b> - works like malloc
  - <b>'void* h_calloc(_size)'</b> - works like calloc
  - <b>'void* h_free(_size)'</b> - works like free
  - <b>'void* h_realloc(_size)'</b> - works like realloc
  - <b>'void* h_'*'_aligned(_size)'</b> - the only difference between base functions is that we will allocate at the start of page memory
  - <b>'int heap_setup()'</b> - initialize heap and return error code (0 - correct)
  - <b>'void heap_dump_debug_information()'</b> - display information about our heap
  - <b>long int  heap_get_free_space()</b> - return number of free bytes on the heap
  - <b>long int  heap_get_used_space()</b> - return number of used bytes on the heap
  - <b>long int  heap_get_used_blocks_count()</b> - return number of used block on the heap
  - <b>long int  heap_get_free_gaps_count()</b> - return number of free block on the heap
  - <b>long int  heap_get_largest_used_block_size()</b> - return number of bytes of the largest used block on the heap
  - <b>long int  heap_get_largest_free_area()</b> - return number of bytes of the largest free block on the heap
  - <b>enum pointer_type_t get_pointer_type(const void* pointer)</b> - return type of pointer:
    - pointer_null
    - pointer_out_of_heap
    - pointer_control_block
    - pointer_inside_data_block
    - pointer_unallocated
    - pointer_valid
  - <b>void* heap_get_data_block_start(const void* pointer)</b> - return pointer to begin of the block,
  which any byte points to
  - <b>long int  heap_get_block_size(const void* memblock)</b> - return pointer block size
  - <b>int heap_validate(void)</b> - check correctness of heap (user can write out of pointer size etc.). Return error-code (0 - correct)
  - <b>int heap_destroy()</b> - clear heap, should be used at the end, return error-code (0 - correct)

## More details

For every allocation there is used control block, space for data and fences. <br>
Control block stores metadata about allocation: size, free or not, pointer to next and previous block and control sum. Pointers to the block helps us moving through heap. Control sum is used to check if control block is overwritten (user can do it by using pointers foolishly). Fences control bounds of space data
intended for the user. <br>
All these mechanisms helps us validate our heap where we check number of blocks, their size, control sum, fences correctness etc. <br>
When user allocate some memory, program try to find best fitting free block, otherwise heap size is expanded by request to system ('custom_sbrk'). <br>
Program is tested by unit tests for each function ('unit_test.c'), and simple program where we perform easy allocations and frees. Additionally there is part for thread-safe test, where we run 4 threads at the same time and similar as before they execute simple allocations and frees.
