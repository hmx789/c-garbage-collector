#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

struct memory_region{
  size_t * start;
  size_t * end;
};

struct memory_region global_mem;
struct memory_region heap_mem;
struct memory_region stack_mem;

void walk_region_and_mark(void* start, void* end);

//how many ptrs into the heap we have
#define INDEX_SIZE 1000
void* heapindex[INDEX_SIZE];


//grabbing the address and size of the global memory region from proc 
void init_global_range(){
  char file[100];
  char * line=NULL;
  size_t n=0;
  size_t read_bytes=0;
  size_t start, end;

  sprintf(file, "/proc/%d/maps", getpid());
  FILE * mapfile  = fopen(file, "r");
  if (mapfile==NULL){
    perror("opening maps file failed\n");
    exit(-1);
  }

  int counter=0;
  while ((read_bytes = getline(&line, &n, mapfile)) != -1) {
    if (strstr(line, "hw4")!=NULL){
      ++counter;
      if (counter==3){
        sscanf(line, "%lx-%lx", &start, &end);
        global_mem.start=(size_t*)start;
        // with a regular address space, our globals spill over into the heap
        global_mem.end=malloc(256);
        free(global_mem.end);
      }
    }
    else if (read_bytes > 0 && counter==3) {
      if(strstr(line,"heap")==NULL) {
        // with a randomized address space, our globals spill over into an unnamed segment directly following the globals
        sscanf(line, "%lx-%lx", &start, &end);
        printf("found an extra segment, ending at %zx\n",end);						
        global_mem.end=(size_t*)end;
      }
      break;
    }
  }
  fclose(mapfile);
}


//marking related operations

int is_marked(size_t* chunk) {
  return ((*chunk) & 0x2) > 0;
}

void mark(size_t* chunk) {
  (*chunk)|=0x2;
}

void clear_mark(size_t* chunk) {
  (*chunk)&=(~0x2);
}

// chunk related operations

#define chunk_size(c)  ((*((size_t*)c))& ~(size_t)3 ) 
void* next_chunk(void* c) { 
  if(chunk_size(c) == 0) {
    printf("Panic, chunk is of zero size.\n");
  }
  if((c+chunk_size(c)) < sbrk(0))	//is pointer in the heap,then return
    return ((void*)c+chunk_size(c));
  else 
    return 0;
}
int in_use(void *c) { 
  //when it is odd it will return 1 otherwise 0
	return (next_chunk(c) && ((*(size_t*)next_chunk(c)) & 1));
}


// index related operations

#define IND_INTERVAL ((sbrk(0) - (void*)(heap_mem.start - 1)) / INDEX_SIZE)
void build_heap_index() {
  // TODO


}

void sweep() {
  size_t * curr = heap_mem.start - 1;
	size_t* nextChunk = NULL;

  while (curr < (size_t*)sbrk(0) && curr > 0 ) {	// need to keep calling sbrk(0) b/c heap size changes
		nextChunk = (size_t*)next_chunk(curr);        // Need to save next chunk b/c could be freed
		if (is_marked(curr) && in_use(curr)) {				// if its allocated and marked
				clear_mark(curr);
				curr = nextChunk;
				continue;
		}
		if (in_use(curr) && !is_marked(curr) ) {    // if allocated and not marked
				free(curr+1);
				curr = nextChunk;
				continue;
		}
		curr = nextChunk;
  }
  return;
}

//determine if what "looks" like a pointer actually points to a block in the heap
size_t * is_pointer(size_t * ptr) {
  if (ptr == NULL) {
		return NULL;
	}																				// TODO
	if (ptr < heap_mem.start) {
		return NULL;
  }
	if (ptr > heap_mem.end) {
		return NULL;
	}

 size_t * currentChunk = heap_mem.start - 1;
 size_t* heapEnd = heap_mem.end;
 while (currentChunk < heapEnd) {
	if (ptr > currentChunk) {			// if ptr > chunk could be in that chunk
		if ( ptr < (size_t*)next_chunk(currentChunk) || (size_t*)next_chunk(currentChunk) == 0 ) { // checking if inside currentChunk
			return currentChunk;
		}
	}
	currentChunk = next_chunk(currentChunk);
 }
 return NULL;
}


void recursiveMark(size_t s) {
	size_t* c = is_pointer((size_t*)s);
	if (c == NULL) {
		return;
	}
	if (is_marked(c) == 1) {
    return;
	}
	mark(c);

	int length = (int)chunk_size(c)/8;
	int i = 1;
	for(i = 1; i < length-1; i++) {	
		recursiveMark(c[i]);
	}
	return;
}

void walk_region_and_mark(void* start, void* end) {
  // TODO
	
	if (end == NULL) {
		return;
	}
	if (start == NULL) {
		return;
	}

	//Type casting the pointer I need to use

  size_t* regionStart = (size_t*)start;
  size_t* regionEnd = (size_t*)end;
	
	size_t* curr = regionStart;

	while (curr != regionEnd) {
		recursiveMark(*curr);
		curr++;
  }	
}

// standard initialization 

void init_gc() {
  size_t stack_var;
  init_global_range();
  heap_mem.start=malloc(512);
  //since the heap grows down, the end is found first
  stack_mem.end=(size_t *)&stack_var;
}

void gc() {
  size_t stack_var;
  heap_mem.end=sbrk(0);
  //grows down, so start is a lower address
  stack_mem.start=(size_t *)&stack_var;

  // build the index that makes determining valid ptrs easier
  // implementing this smart function for collecting garbage can get bonus;
  // if you can't figure it out, just comment out this function.
  // walk_region_and_mark and sweep are enough for this project.
  //  build_heap_index();

  //walk memory regions
  walk_region_and_mark(global_mem.start,global_mem.end);
  walk_region_and_mark(stack_mem.start,stack_mem.end);
  sweep();
}
