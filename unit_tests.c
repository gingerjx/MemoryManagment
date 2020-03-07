#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "my_stdlib.h"
#include "definitions.h"
#include "unit_tests.h"

/* ######### Tested on x64 machine ############ */


/*########## GLOBAL VARIABLES AND USEFUL FUNCTIONS ##########*/

const unsigned int tests_num = 9;
unsigned short passed = 0;
unsigned short finished = 0;
char err_msg[100];

long int  free_space;
long int  used_space;
long int  used_blocks;
long int  free_gaps;
long int  largest_used_block;
long int  largest_free_area;

void init_values(){
  free_space = heap_get_free_space();
  used_space = heap_get_used_space();
  used_blocks = heap_get_used_blocks_count();
  free_gaps = heap_get_free_gaps_count();
  largest_used_block = heap_get_largest_used_block_size();
  largest_free_area = heap_get_largest_free_area();
}
int check_values(){
  if ( heap_get_free_space() != free_space )
    return 0;
  if ( heap_get_used_space() != used_space )
    return 0;
  if ( heap_get_used_blocks_count() != used_blocks )
    return 0;
  if ( heap_get_free_gaps_count() != free_gaps )
    return 0;
  if ( heap_get_largest_used_block_size() != largest_used_block )
    return 0;
  if ( heap_get_largest_free_area() != largest_free_area )
    return 0;
  return 1;
}

int error(int expr, const char *msg, unsigned int test_line){
  if ( expr == 0 ){
    printf("[line:%u] %s\n", test_line, msg);
    return 1;
  }
  return 0;
}
void dbg(){ heap_dump_debug_information(); getchar(); }

/*########## TEST 1 ##########*/
/*# Checking valid of *_get_* functions #*/

void test_1(){
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test1. Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr[10];
  int allocated = 0;
  for ( int i=0; i<10; ++i ){
    ptr[i] = h_malloc(sizeof(int)*(i+1));
    allocated += sizeof(int)*(i+1);
    if ( error( ptr[i]!=NULL, "test1. Malloc returned NULL, but it should return pointer", __LINE__ ) )
      return;
  }
  h_free(ptr[4]);
  allocated -= 5*sizeof(int) + 2*FENCE_SIZE;
/* ############## */

  /* ## Gets for used blocks ## */
  int correct_used_space = allocated + BLOCK_C_SIZE*12 + 10*2*FENCE_SIZE;
  sprintf(err_msg,"test1. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;

  int correct_used_blocks = 9;
  sprintf(err_msg,"test1. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;

  int correct_largest_used = 40;
  sprintf(err_msg,"test1. Function should return %d but it returned %ld\n", correct_largest_used, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  int correct_block_size = 4;  //of ptr[0]
  sprintf(err_msg,"test1. Function should return %d but it returned %ld\n", correct_block_size, heap_get_block_size(ptr[0]) );
  if ( error( heap_get_block_size(ptr[0])==correct_block_size , err_msg, __LINE__ ) )
    return;

  /* ## Gets for free block ## */
  int correct_free_space = 22;
  sprintf(err_msg,"test1. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_space() );
  if ( error( heap_get_free_space()==correct_free_space , err_msg, __LINE__ ) )
    return;

  int correct_free_blocks = 1;
  sprintf(err_msg,"test1. Function should return %d but it returned %ld\n", correct_free_blocks, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_blocks , err_msg, __LINE__ ) )
    return;

  int correct_largest_free = 22;
  sprintf(err_msg,"test1. Function should return %d but it returned %ld\n", correct_largest_free, heap_get_largest_free_area() );
  if ( error( heap_get_largest_free_area()==correct_largest_free , err_msg, __LINE__ ) )
    return;

/* ################ */

  for ( int i=0; i<10; ++i ) {    //free memory
    if ( i != 4 ) h_free(ptr[i]); }

/* #### FINISH TEST ###### */

  if ( error( check_values(), "test1. Initialize values does not match with end values", __LINE__ ) ){
    printf(" ### TEST 1: %s\n", "Failed");
    return;
  }
  if ( error( heap_destroy()==0, "test1. Heap destroy failed", __LINE__ ) )
    return;
  finished++;
  passed++;
  printf(" ### TEST 1: %s\n", "Passed");
}

/*########## TEST 2 ##########*/
/*# Checking valid of get_pointer_type  and heap_get_data_block_start function #*/

void test_2(){
  if ( finished != 1 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test2. Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr[3];
  for ( int i=0; i<3; ++i ){                                  //
    ptr[i] = h_malloc(sizeof(int)*(i+1));
    if ( error( ptr[i]!=NULL, "test2. Malloc returned NULL, but it should return pointer", __LINE__ ) )
      return;
  }
  h_free(ptr[1]);
  char enums[6][30] = { "pointer_null", "pointer_out_of_heap", "pointer_control_block", "pointer_inside_data_block", "pointer_unallocated", "pointer_valid" };

/* ################ */

  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_null], enums[get_pointer_type(NULL)] );
  if ( error( get_pointer_type(NULL)==pointer_null , err_msg, __LINE__ ) )
    return;


  int ptr3_size = heap_get_block_size(ptr[2]);
  intptr_t *ptr_out_of_heap = (intptr_t *)( (char *)ptr[2] + ptr3_size + FENCE_SIZE );
  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_out_of_heap], enums[get_pointer_type( ptr_out_of_heap )] );
  if ( error( get_pointer_type( ptr_out_of_heap )==pointer_out_of_heap , err_msg, __LINE__ ) )
    return;

  ptr_out_of_heap = (intptr_t *)( (char *)ptr[0] - BLOCK_C_SIZE - FENCE_SIZE - 1);
  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_out_of_heap], enums[get_pointer_type( ptr_out_of_heap )] );
  if ( error( get_pointer_type( ptr_out_of_heap )==pointer_out_of_heap , err_msg, __LINE__ ) )
    return;

  ptr_out_of_heap = (intptr_t *)( (char *)ptr[0] - BLOCK_C_SIZE - 9999 );
  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_out_of_heap], enums[get_pointer_type( ptr_out_of_heap )] );
  if ( error( get_pointer_type( ptr_out_of_heap )==pointer_out_of_heap , err_msg, __LINE__ ) )
    return;

  intptr_t *ptr_control_block = (intptr_t *)( (char *)ptr[1] - BLOCK_C_SIZE - FENCE_SIZE);
  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_control_block], enums[get_pointer_type( ptr_control_block )] );
  if ( error( get_pointer_type( ptr_control_block )==pointer_control_block , err_msg, __LINE__ ) )
    return;

  ptr_control_block = (intptr_t *)( (char *)ptr[0] +  heap_get_block_size(ptr[0]) + FENCE_SIZE );
  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_control_block], enums[get_pointer_type( ptr_control_block )] );
  if ( error( get_pointer_type( ptr_control_block )==pointer_control_block , err_msg, __LINE__ ) )
    return;

  intptr_t *ptr_ins_data = (intptr_t *)( (char *)ptr[0] + 2);
  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_inside_data_block], enums[get_pointer_type( ptr_ins_data )] );
  if ( error( get_pointer_type( ptr_ins_data )==pointer_inside_data_block , err_msg, __LINE__ ) )
    return;

  intptr_t *ptr_unalloc = (intptr_t *)( (char *)ptr[1] + 4);
  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_unallocated], enums[get_pointer_type( ptr_unalloc )] );
  if ( error( get_pointer_type( ptr_unalloc )==pointer_unallocated , err_msg, __LINE__ ) )
     return;

  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_valid], enums[get_pointer_type( ptr[0] )] );
  if ( error( get_pointer_type( ptr[0] )==pointer_valid , err_msg, __LINE__ ) )
    return;

  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_valid], enums[get_pointer_type( ptr[1] )] );
  if ( error( get_pointer_type( ptr[1] )==pointer_valid , err_msg, __LINE__ ) )
    return;

  sprintf(err_msg,"test2. Function should return %s but it returned %s\n", enums[pointer_valid], enums[get_pointer_type( ptr[2] )] );
  if ( error( get_pointer_type( ptr[2] )==pointer_valid , err_msg, __LINE__ ) )
    return;

  sprintf(err_msg,"test2. Function should return %p but it returned %p\n", (void *)( ptr[2] ), heap_get_data_block_start( (char *)ptr[2]+5 ) );
  if ( error( heap_get_data_block_start( (char *)ptr[2]+5 )==ptr[2] , err_msg, __LINE__ ) )
    return;

/* ################ */

  h_free(ptr[0]);     //free rest of memory
  h_free(ptr[2]);

/* #### FINISH TEST ###### */

  if ( error( check_values(), "test2. Initialize values does not match with end values", __LINE__ ) ){
    printf(" ### TEST 2: %s\n", "Failed");
    return;
  }
  if ( error( heap_destroy()==0, "test2. Heap destroy failed", __LINE__ ) )
    return;
  finished++;
  passed++;
  printf(" ### TEST 2: %s\n", "Passed");
}

/*########## TEST 3 ##########*/
/*# Checking valid of malloc function #*/

void test_3(){
  if ( finished != 2 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test3. Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr1 = (int *)h_malloc( 0 );
  int *ptr2 = (int *)h_malloc( -53 );
  char *ptr3 = (char *)h_malloc( strlen("This is pointer 3")+1 );   
  char *ptr4 = (char *)h_malloc( 16 * 1024 * 1024 );
  char *ptr5 = (char *)h_malloc( 16 * 1024 * 1024 );
  char *ptr6 = (char *)h_malloc( 64 * 1024 * 1024 );
  strcpy(ptr3,"This is pointer 3");
  strcpy(ptr4,"Next one have number 4");
  strcpy(ptr5,"And the last one is 5");

/* ################ */

  if ( error(ptr1==NULL, "test3. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;
  if ( error(ptr2==NULL, "test3. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test3. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr4!=NULL, "test3. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr5!=NULL, "test3. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr6==NULL, "test3. Malloc returned pointer, but it should returned NULL", __LINE__ ) )    //custom_sbrk won't allocate more memory
    return;

  long int correct_used_size = 24 + 2*(16 * 1024 * 1024) + BLOCK_C_SIZE*3 + BLOCK_C_SIZE*2; //allocated size + 3*control blocks + 2*fences
  int correct_used_blocks = 3;
  long int correct_largest_used = 16 * 1024 * 1024;

  sprintf(err_msg,"test3. Function should return %ld but it returned %ld\n", correct_used_size, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_size , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test3. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test3. Function should return %ld but it returned %ld\n", correct_used_size, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  int correct_data = !strcmp(ptr3,"This is pointer 3") + !strcmp(ptr4,"Next one have number 4") + !strcmp(ptr5,"And the last one is 5");
  sprintf(err_msg,"test3. Only %d/3 strings are correct\n", correct_data );
  if ( error( correct_data==3 , err_msg, __LINE__ ) )
    return;

/* ################ */

  h_free(ptr3);
  h_free(ptr4);
  h_free(ptr5);

/* #### FINISH TEST ###### */

  if ( error( check_values(), "test3. Initialize values does not match with end values", __LINE__ ) ){
    printf(" ### TEST 3: %s\n", "Failed");
    return;
  }
  if ( error( heap_destroy()==0, "test3. Heap destroy failed", __LINE__ ) )
    return;
  finished++;
  passed++;
  printf(" ### TEST 3: %s\n", "Passed");
}

/*########## TEST 4 ##########*/
/*# Checking valid of calloc function #*/

void test_4(){
  if ( finished != 3 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test4. Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr1 = (int *)h_calloc( 0,1 );
  int *ptr2 = (int *)h_calloc( -21,1 );
  int *ptr3 = (int *)h_calloc( sizeof(int),1 );       //alloc 4[B]
  double *ptr4 = (double *)h_calloc( sizeof(double),1 );  //alloc 8[B]
  short *ptr5 = (short *)h_calloc( sizeof(short),40 );    //alloc 80[B]
  float *ptr6 = (float *)h_calloc( sizeof(float),1024 );  //alloc 4096[B]
  unsigned int *ptr7 = (unsigned int *)h_calloc( 128 * 1024,1024 );

/* ################ */

  if ( error(ptr1==NULL, "test4. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;
  if ( error(ptr2==NULL, "test4. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test4. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr4!=NULL, "test4. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr5!=NULL, "test4. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr6!=NULL, "test4. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr7==NULL, "test4. Malloc returned pointer, but it should returned NULL", __LINE__ ) )    //custom_sbrk won't allocate more memory
    return;

  long int correct_used_size = 4 + 8 + 80 + 4096 + BLOCK_C_SIZE*6 + 4*2*FENCE_SIZE;   //allocated size + 4*control block + 2*fences
  int correct_used_blocks = 4;
  long int correct_largest_used = 4096;

  sprintf(err_msg,"test4. Function should return %ld but it returned %ld", correct_used_size, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_size , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test4. Function should return %d but it returned %ld", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test4. Function should return %ld but it returned %ld", correct_used_size, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  int correct_data = 0;
  correct_data +=  !(*ptr3) + !(*ptr4);
  for ( int i=0; i<40; ++i)
    correct_data += !(*(ptr5+i));
  for ( int i=0; i<1024; ++i )
    correct_data += !(*(ptr6+i));
  if ( error( correct_data==1066 , "test4. Not all datas are zeros", __LINE__ ) )
    return;

/* ################ */

  h_free(ptr3);
  h_free(ptr4);
  h_free(ptr5);
  h_free(ptr6);

/* #### FINISH TEST ###### */

  if ( error( check_values(), "test4. Initialize values does not match with end values", __LINE__ ) ){
    printf(" ### TEST 4: %s\n", "Failed");
    return;
  }
  if ( error( heap_destroy()==0, "test4. Heap destroy failed", __LINE__ ) )
    return;
  finished++;
  passed++;
  printf(" ### TEST 4: %s\n", "Passed");
}

/*########## TEST 5 ##########*/
/*# Checking valid of free function #*/

void test_5(){
  if ( finished != 4 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test5 Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr1 = (int *)h_malloc( sizeof(int)*4 );
  int *ptr2 = (int *)h_malloc( sizeof(int)*40 );
  int *ptr3 = (int *)h_malloc( sizeof(int)*400 );
  int *ptr4 = (int *)h_malloc( sizeof(int)*4000 );
  int *ptr5 = (int *)h_malloc( sizeof(int)*40000 );

/* ################ */

  if ( error(ptr1!=NULL, "test5. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr2!=NULL, "test5. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test5. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr4!=NULL, "test5. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr5!=NULL, "test5. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;

  h_free(ptr2);
  h_free(ptr4);
  int correct_free_space =  (40 + 4000)*sizeof(int) + 2*2*FENCE_SIZE;
  int correct_free_blocks = 2;
  int correct_largest_free = 4000*sizeof(int) + 2*FENCE_SIZE;

  sprintf(err_msg,"test5. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_space() );
  if ( error( heap_get_free_space()==correct_free_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test5. Function should return %d but it returned %ld\n", correct_free_blocks, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test5. Function should return %d but it returned %ld\n", correct_largest_free, heap_get_largest_free_area() );
  if ( error( heap_get_largest_free_area()==correct_largest_free , err_msg, __LINE__ ) )
    return;

  h_free(NULL);
  h_free(ptr5);
  correct_free_space = 40*sizeof(int) + 2*FENCE_SIZE;
  correct_free_blocks = 1;
  correct_largest_free = correct_free_space;
  int correct_used_space = (4+400)*sizeof(int) + BLOCK_C_SIZE*5 + 2*2*FENCE_SIZE;

  sprintf(err_msg,"test5. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_space() );
  if ( error( heap_get_free_space()==correct_free_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test5. Function should return %d but it returned %ld\n", correct_free_blocks, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test5. Function should return %d but it returned %ld\n", correct_largest_free, heap_get_largest_free_area() );
  if ( error( heap_get_largest_free_area()==correct_largest_free , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test5. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;

/* ################ */

  h_free(ptr1);
  h_free(ptr3);

/* #### FINISH TEST ###### */

  if ( error( check_values(), "test5. Initialize values does not match with end values", __LINE__ ) ){
    printf(" ### TEST 5: %s\n", "Failed");
    return;
  }
  if ( error( heap_destroy()==0, "test5. Heap destroy failed", __LINE__ ) )
    return;
  finished++;
  passed++;
  printf(" ### TEST 5: %s\n", "Passed");

}

/*########## TEST 6 ##########*/
/*# Checking valid of realloc function #*/

void test_6(){
  if ( finished != 5 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test6 Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr1 = (int *)h_realloc(NULL,sizeof(int)*4);
  int *ptr2 = (int *)h_realloc(NULL,sizeof(int)*40);
  int *ptr3 = (int *)h_realloc(NULL,sizeof(int)*400);
  int *ptr4 = (int *)h_realloc(NULL,sizeof(int)*4000);
  int *ptr5 = (int *)h_realloc(NULL,sizeof(int)*40000);
  *ptr1 = 123;
  ptr2[34] = 34545;
  ptr3[67] = 804;
  ptr4[190] = 547;
  ptr5[3999] = -23211;

/* ################ */
  if ( error(ptr1!=NULL, "test6. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr2!=NULL, "test6. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test6. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;
  if ( error(ptr4!=NULL, "test6. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;
  if ( error(ptr5!=NULL, "test6. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;

  int correct_used_space = (4+40+400+4000+40000)*sizeof(int) + BLOCK_C_SIZE*7 + 5*2*FENCE_SIZE;
  int correct_used_blocks = 5;
  int correct_largest_used = 40000*sizeof(int);

  sprintf(err_msg,"test6. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test6. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test6. Function should return %d but it returned %ld\n", correct_largest_used, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  /* ## After realloc ## */
  ptr1 = h_realloc(ptr1,1024);
  correct_used_space += 1024 + BLOCK_C_SIZE - sizeof(int)*4;
  int correct_free_space = 1;

  sprintf(err_msg,"test6. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test6. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test6. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test6. Function should return %d but it returned %ld\n", correct_largest_used, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  int correct_data = 0;
  correct_data += (*ptr1 == 123) + (ptr2[34] == 34545) + (ptr3[67] == 804) + (ptr4[190] == 547) + (ptr5[3999] == -23211);
  sprintf(err_msg,"test6. Only %d/5 datas are corret\n", correct_data );
  if ( error( correct_data==5 , err_msg, __LINE__ ) )
    return;

/* ################ */

  h_realloc(ptr1,0);
  h_realloc(ptr2,0);
  h_realloc(ptr3,0);
  h_realloc(ptr4,0);
  h_realloc(ptr5,0);

/* #### FINISH TEST ##### */

  if ( error( check_values(), "test6. Initialize values does not match with end values", __LINE__ ) ){
    printf(" ### TEST 6: %s\n", "Failed");
    return;
  }
  if ( error( heap_destroy()==0, "test6. Heap destroy failed", __LINE__ ) )
    return;
  finished++;
  passed++;
  printf(" ### TEST 6: %s\n", "Passed");

}

/*########## TEST 7 ##########*/
/*# Checking valid of malloc_aligned function #*/

void test_7(){
  if ( finished != 6 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test7. Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr1 = (int *)h_malloc_aligned(sizeof(int)*23);
  int *ptr2 = (int *)h_malloc_aligned(sizeof(int)*4);
  int *ptr3 = (int *)h_malloc_aligned(sizeof(int)*78);

  assert( ( (intptr_t)ptr1 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  assert( ( (intptr_t)ptr2 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  assert( ( (intptr_t)ptr3 & (intptr_t)(PAGE_SIZE - 1) ) == 0);

  *ptr1 = 579;
  ptr2[3] = 34;
  ptr3[57] = 21;

/* ################ */

  if ( error(ptr1!=NULL, "test7. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr2!=NULL, "test7. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test7. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;

  /* ## checking free gaps created during aligment ## */
  int free_block_1 = PAGE_SIZE - BLOCK_C_SIZE*3 - FENCE_SIZE;
  int free_block_2 = PAGE_SIZE - ( BLOCK_C_SIZE*2 + sizeof(int)*23 + 2*FENCE_SIZE );
  int free_block_3 = PAGE_SIZE - ( BLOCK_C_SIZE*2 + sizeof(int)*4 + 2*FENCE_SIZE );
  int correct_free_space =  free_block_1 + free_block_2 + free_block_3;
  int correct_free_blocks = 3;
  int correct_largest_free = free_block_3;

  sprintf(err_msg,"test7. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_space() );
  if ( error( heap_get_free_space()==correct_free_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test7. Function should return %d but it returned %ld\n", correct_free_blocks, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test7. Function should return %d but it returned %ld\n", correct_largest_free, heap_get_largest_free_area() );
  if ( error( heap_get_largest_free_area()==correct_largest_free , err_msg, __LINE__ ) )
    return;


  int used_block_1 = sizeof(int)*23 + 2*FENCE_SIZE;
  int used_block_2 = sizeof(int)*4  + 2*FENCE_SIZE;
  int used_block_3 = sizeof(int)*78 + 2*FENCE_SIZE;
  int correct_used_space =  used_block_1 + used_block_2 + used_block_3 + BLOCK_C_SIZE*8;
  int correct_used_blocks = 3;
  int correct_largest_used = used_block_3 - 2*FENCE_SIZE;

  sprintf(err_msg,"test7. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test7. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test7. Function should return %d but it returned %ld\n", correct_largest_used, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  int *null_ptr = h_malloc_aligned(1024*1024*96);
  if ( error(null_ptr==NULL, "test7. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;

  int correct_data = 0;
  correct_data += (*ptr1 == 579) + (ptr2[3] == 34) + (ptr3[57] == 21);
  sprintf(err_msg,"test7. Only %d/3 datas are corret\n", correct_data );
  if ( error( correct_data==3 , err_msg, __LINE__ ) )
    return;

/* ################ */

  h_free(ptr1);
  h_free(ptr2);
  h_free(ptr3);

/* #### FINISH TEST ##### */

if ( error( check_values(), "test7. Initialize values does not match with end values", __LINE__ ) ){
  printf(" ### TEST 7: %s\n", "Failed");
  return;
}
if ( error( heap_destroy()==0, "test7. Heap destroy failed", __LINE__ ) )
  return;
finished++;
passed++;
printf(" ### TEST 7: %s\n", "Passed");

}

/*########## TEST 8 ##########*/
/*# Checking valid of calloc_aligned function #*/

void test_8(){
  if ( finished != 7 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test8. Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr1 = (int *)h_calloc_aligned(sizeof(int),23);
  int *ptr2 = (int *)h_calloc_aligned(sizeof(int),4);
  int *ptr3 = (int *)h_calloc_aligned(sizeof(int),78);

  assert( ( (intptr_t)ptr1 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  assert( ( (intptr_t)ptr2 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  assert( ( (intptr_t)ptr3 & (intptr_t)(PAGE_SIZE - 1) ) == 0);

/* ################ */

  if ( error(ptr1!=NULL, "test8. Calloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr2!=NULL, "test8. Calloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test8. Calloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;

  /* ## checking created free gaps during aligment ## */
  int free_block_1 = PAGE_SIZE - BLOCK_C_SIZE*3 - FENCE_SIZE;
  int free_block_2 = PAGE_SIZE - ( BLOCK_C_SIZE*2 + sizeof(int)*23 + 2*FENCE_SIZE );
  int free_block_3 = PAGE_SIZE - ( BLOCK_C_SIZE*2 + sizeof(int)*4 + 2*FENCE_SIZE );
  int correct_free_space =  free_block_1 + free_block_2 + free_block_3;
  int correct_free_blocks = 3;
  int correct_largest_free = free_block_3;

  sprintf(err_msg,"test8. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_space() );
  if ( error( heap_get_free_space()==correct_free_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test8. Function should return %d but it returned %ld\n", correct_free_blocks, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test8. Function should return %d but it returned %ld\n", correct_largest_free, heap_get_largest_free_area() );
  if ( error( heap_get_largest_free_area()==correct_largest_free , err_msg, __LINE__ ) )
    return;

  int used_block_1 = sizeof(int)*23 + 2*FENCE_SIZE;
  int used_block_2 = sizeof(int)*4 + 2*FENCE_SIZE;
  int used_block_3 = sizeof(int)*78 + 2*FENCE_SIZE;
  int correct_used_space =  used_block_1 + used_block_2 + used_block_3 + BLOCK_C_SIZE*8;
  int correct_used_blocks = 3;
  int correct_largest_used = used_block_3 - 2*FENCE_SIZE ;

  sprintf(err_msg,"test8. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test8. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test8. Function should return %d but it returned %ld\n", correct_largest_used, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  int *null_ptr = h_calloc_aligned(1024,1024*96);
  if ( error(null_ptr==NULL, "test8. Calloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;

  short correct_data = 0;
  for ( int i=0; i<23; ++i )
    correct_data += !(ptr1[i]);
  for ( int i=0; i<4; ++i )
    correct_data += !*(ptr2 +i);
  for ( int i=0; i<78; ++i )
    correct_data += !(ptr3[i]);
  sprintf(err_msg,"test8. Not all datas are zeros\n" );
  if ( error( correct_data==78+4+23 , err_msg, __LINE__ ) )
    return;

/* ################ */

  h_free(ptr1);
  h_free(ptr2);
  h_free(ptr3);

/* #### FINISH TEST ##### */

if ( error( check_values(), "test8. Initialize values does not match with end values", __LINE__ ) ){
  printf(" ### TEST 8: %s\n", "Failed");
  return;
}
if ( error( heap_destroy()==0, "test8. Heap destroy failed", __LINE__ ) )
  return;
finished++;
passed++;
printf(" ### TEST 8: %s\n", "Passed");

}

/*########## TEST 9 ##########*/
/*# Checking valid of realloc_aligned function #*/

void test_9(){
  if ( finished != 8 )
    return;
  printf(" ### START\n");
  if ( error( heap_setup()==0, "test9. Heap initialization failed", __LINE__ ) )
    return;
  init_values();

/* #### START TEST ###### */

  int *ptr1 = (int *)h_realloc_aligned(NULL,sizeof(int)*23);
  int *ptr2 = (int *)h_realloc_aligned(NULL,sizeof(int)*4);
  int *ptr3 = (int *)h_realloc_aligned(NULL,sizeof(int)*78);

  assert( ( (intptr_t)ptr1 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  assert( ( (intptr_t)ptr2 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  assert( ( (intptr_t)ptr3 & (intptr_t)(PAGE_SIZE - 1) ) == 0);

  *ptr1 = 123;
  ptr2[3] = 34545;
  ptr3[67] = 804;

/* ################ */

  if ( error(ptr1!=NULL, "test9. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr2!=NULL, "test9. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test9. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;

  /* ## checking created free gaps during aligment ## */
  int free_block_1 = PAGE_SIZE - BLOCK_C_SIZE*3 - FENCE_SIZE;
  int free_block_2 = PAGE_SIZE - ( BLOCK_C_SIZE*2 + sizeof(int)*23 + 2*FENCE_SIZE ); //+4 cause of aligment to WORD_SIZE
  int free_block_3 = PAGE_SIZE - ( BLOCK_C_SIZE*2 + sizeof(int)*4 + 2*FENCE_SIZE);
  int correct_free_space =  free_block_1 + free_block_2 + free_block_3;
  int correct_free_blocks = 3;
  int correct_largest_free = free_block_3;

  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_space() );
  if ( error( heap_get_free_space()==correct_free_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_free_blocks, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_largest_free, heap_get_largest_free_area() );
  if ( error( heap_get_largest_free_area()==correct_largest_free , err_msg, __LINE__ ) )
    return;

  int used_block_1 = sizeof(int)*23 + 2*FENCE_SIZE;
  int used_block_2 = sizeof(int)*4 + 2*FENCE_SIZE;
  int used_block_3 = sizeof(int)*78 + 2*FENCE_SIZE;
  int correct_used_space =  used_block_1 + used_block_2 + used_block_3 + BLOCK_C_SIZE*8;
  int correct_used_blocks = 3;
  int correct_largest_used = used_block_3 - 2*FENCE_SIZE;

  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_largest_used, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;



  /* ## After realloc ## */

  ptr1 = h_realloc_aligned(ptr1,1024);
  ptr2 = h_realloc_aligned(ptr2,1024*24);
  int *ptr_null = h_realloc_aligned(ptr3,1024*96*1024);

  assert( ( (intptr_t)ptr1 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  assert( ( (intptr_t)ptr2 & (intptr_t)(PAGE_SIZE - 1) ) == 0);
  
  if ( error(ptr1!=NULL, "test9. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr2!=NULL, "test9. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr3!=NULL, "test9. Malloc returned NULL, but it should returned pointer", __LINE__ ) )
    return;
  if ( error(ptr_null==NULL, "test9. Malloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;

  /* ## checking created free gaps during aligment ## */
  free_block_1 = PAGE_SIZE - BLOCK_C_SIZE*3 - FENCE_SIZE;
  free_block_2 = 2*PAGE_SIZE - BLOCK_C_SIZE*2 - 1024 - 2*FENCE_SIZE;
  free_block_3 = PAGE_SIZE - (sizeof(int)*78) - 2*BLOCK_C_SIZE - 2*FENCE_SIZE;
  correct_free_space =  free_block_1 + free_block_2 + free_block_3;
  correct_free_blocks = 3;
  correct_largest_free = free_block_2;

  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_free_space, heap_get_free_space() );
  if ( error( heap_get_free_space()==correct_free_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_free_blocks, heap_get_free_gaps_count() );
  if ( error( heap_get_free_gaps_count()==correct_free_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_largest_free, heap_get_largest_free_area() );
  if ( error( heap_get_largest_free_area()==correct_largest_free , err_msg, __LINE__ ) )
    return;

  used_block_1 = 1024 + 2*FENCE_SIZE;
  used_block_2 = 1024*24  + 2*FENCE_SIZE;
  used_block_3 = sizeof(int)*78 + 2*FENCE_SIZE;
  correct_used_space =  used_block_1 + used_block_2 + used_block_3 + BLOCK_C_SIZE*8;
  correct_used_blocks = 3;
  correct_largest_used = used_block_2 -2*FENCE_SIZE;

  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_used_space, heap_get_used_space() );
  if ( error( heap_get_used_space()==correct_used_space , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_used_blocks, heap_get_used_blocks_count() );
  if ( error( heap_get_used_blocks_count()==correct_used_blocks , err_msg, __LINE__ ) )
    return;
  sprintf(err_msg,"test9. Function should return %d but it returned %ld\n", correct_largest_used, heap_get_largest_used_block_size() );
  if ( error( heap_get_largest_used_block_size()==correct_largest_used , err_msg, __LINE__ ) )
    return;

  int *null_ptr = h_realloc_aligned(NULL,1024*1024*96);
  if ( error(null_ptr==NULL, "test9. Realloc returned pointer, but it should returned NULL", __LINE__ ) )
    return;

  int correct_data = 0;
  correct_data += (*ptr1 == 123) + (ptr2[3] == 34545) + (ptr3[67] == 804);
  sprintf(err_msg,"test9. Only %d/3 datas are corret\n", correct_data );
  if ( error( correct_data==3 , err_msg, __LINE__ ) )
    return;

/* ################ */

  h_realloc_aligned(ptr1,0);
  h_realloc_aligned(ptr2,0);
  h_realloc_aligned(ptr3,0);

/* #### FINISH TEST ##### */

  if ( error( check_values(), "test9. Initialize values does not match with end values", __LINE__ ) ){
    printf(" ### TEST 9: %s\n", "Failed");
    return;
  }
  if ( error( heap_destroy()==0, "test9. Heap destroy failed", __LINE__ ) )
    return;
  finished++;
  passed++;
  printf(" ### TEST 9: %s\n", "Passed");

}


void run_tests(){
  printf("\n");
  test_1();
  test_2();
  test_3();
  test_4();
  test_5();
  test_6();
  test_7();
  test_8();
  test_9();
  printf("\n\n --- %u/%u tests done\n", finished, tests_num);
  printf(" --- %u/%u tests passed\n", passed, tests_num);
  return;
}
