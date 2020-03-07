#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "my_stdlib.h"
#include "unit_tests.h"
#include "definitions.h"

void debug() { heap_dump_debug_information(); getchar(); }
void *thread_function(void *);

int main(){
  #if 1
  //######## Unit tests ########//
  run_tests();
  #endif
  #if 1
  //######## Official main test ########//
  int status = heap_setup();
		assert(status == 0);

		// parametry pustej sterty
		size_t free_bytes = heap_get_free_space();
		size_t used_bytes = heap_get_used_space();

		void* p1 = h_malloc(8 * 1024 * 1024); // 8MB
		void* p2 = h_malloc(8 * 1024 * 1024); // 8MB
		void* p3 = h_malloc(8 * 1024 * 1024); // 8MB
		void* p4 = h_malloc(45 * 1024 * 1024); // 45MB
		assert(p1 != NULL); // malloc musi się udać
		assert(p2 != NULL); // malloc musi się udać
		assert(p3 != NULL); // malloc musi się udać
		assert(p4 == NULL); // nie ma prawa zadziałać
	    // Ostatnia alokacja, na 45MB nie może się powieść,
	    // ponieważ sterta nie może być aż tak
	    // wielka (brak pamięci w systemie operacyjnym).

		status = heap_validate();
		assert(status == 0); // sterta nie może być uszkodzona

		// zaalokowano 3 bloki
		assert(heap_get_used_blocks_count() == 3);

		// zajęto 24MB sterty; te 2000 bajtów powinno
	    // wystarczyć na wewnętrzne struktury sterty
		assert(
	        heap_get_used_space() >= 24 * 1024 * 1024 &&
	        heap_get_used_space() <= 24 * 1024 * 1024 + 2000
	        );

		// zwolnij pamięć
		h_free(p1);
		h_free(p2);
		h_free(p3);

		// wszystko powinno wrócić do normy
		assert(heap_get_free_space() == free_bytes);
		assert(heap_get_used_space() == used_bytes);

		// już nie ma bloków
		assert(heap_get_used_blocks_count() == 0);
		status = heap_destroy();
		assert(status == 0);
  #endif
  #if 1
    //######## Multithread safe main test ########//
  	assert(heap_setup() == 0);// my main

  	pthread_t thread[4];
  	for (int i=0; i<2; ++i)
  		pthread_create(thread+i,NULL,thread_function,NULL);
  	for (int i=0; i<2; ++i)
  		pthread_join(thread[i],NULL);


  	status = heap_destroy();
  	assert(status == 0);
  #endif
  return 0;
}

void *thread_function(void *p){
	int *datas[100];
	for (int i=0; i<100; ++i){
		datas[i] = (int *)h_malloc(sizeof(int));
		*datas[i] = i;
    usleep(500);
	}
	int sum=0;
	for (int i=0; i<100; ++i)
		sum += *datas[i];
	assert( sum == 4950 );
	for (int i=0; i<100; ++i){
		h_free(datas[i]);
    usleep(500);
  }
  return NULL;
}
