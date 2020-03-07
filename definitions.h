#if !defined(_DEFINITIONS_H_)
#define _DEFINITIONS_H_
#include <stdint.h>
#include <stdbool.h>

#define FENCE_UNIT_NUM 1
typedef struct{
  uint8_t left[FENCE_UNIT_NUM];
  uint8_t right[FENCE_UNIT_NUM];
} mem_fence_t;


#define FN_SIZE 30
#define DEBUG 1
#if DEBUG
  #define log(...)  printf(__VA_ARGS__);
  struct block_control_t{
    char filename[FN_SIZE];
    unsigned int  line_number;
    struct block_control_t *next;
    struct block_control_t *prev;
    long int  size;
    bool free;
    int crs;          //control_sum
  };
#else
  #define log(...)  printf("");
  struct block_control_t{
    struct block_control_t *next;
    struct block_control_t *prev;
    long int  size;
    bool free;
    int crs;          //control_sum
  };
#endif

typedef struct  block_control_t block_control_t;


typedef struct{
  long int  size;            //actual total size got by custom_sbrk
  long int  free_size;       //actual free size
  long int  alloc_size;      //whole memory allocated in time programs
  long int  freed_size;      //whole memory freed in time programs
  long int  blocks_num;      //actual number of blocks
  block_control_t *first_block;
  block_control_t *last_block;
} heap_control_t;

#define PAGE_SIZE 4096 //4KB
#define HEAP_C_SIZE (sizeof(heap_control_t))
#define BLOCK_C_SIZE (sizeof(block_control_t))
#define FENCE_SIZE (sizeof(uint8_t)*FENCE_UNIT_NUM)
#define WORD_SIZE ((sizeof(void *) * CHAR_BIT)/8)
#define MIN_SIZE (WORD_SIZE + BLOCK_C_SIZE + 2*FENCE_SIZE)

int check_sum(block_control_t *block);
void *get_left_fence(block_control_t *block);
void *get_right_fence(block_control_t *block);
void init_fences(block_control_t *block);
void write_block_info(block_control_t *block, const char *filename, int line);

void *find_fitting_block(long int  size);
void * sbrk_request(long int  size);
void *init_new_block(block_control_t *block, long int  size);

void try_attach_adj(block_control_t *block);
void try_release_to_sbrk();

void *realloc_no_destroy(block_control_t *block, long int  size);
void *realloc_destroy(block_control_t *block, long int  size);
void *realloc_decrease_free(block_control_t *block, long int  size);
void *realloc_decrease_occupied(block_control_t *block, long int  size);

/*######### Aligned version #############*/

void *find_fitting_block_aligned(long int  size, block_control_t **prev_block);
void *init_new_block_aligned(long int  size, block_control_t *block, block_control_t *prec_block);
void *sbrk_request_aligned(long int  size);

#endif // _DEFINITIONS_H_
