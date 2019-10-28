#ifndef OS_H
#define OS_H

//paramètres de l'OS
#define PAGE_SIZE 4096
#define MAX_PHYSICAL_FRAMES 3
#define MAX_PROCESS_PAGES 1024
#define MAX_PROCESSES 8

#include "request_queue.h"

//choix d'algorithme à utiliser
#define USE_LRU //enlever pour avoir le Viellissement à la place du LRU
#define USE_BEST_FIT //enlever pour avoir WORST-FIT à la place du BEST-FIT

#ifdef USE_LRU
//compteur des opérations d'acces memoire
extern int mem_access_counter;
#endif

typedef struct
{
    int process_id;
    int page_id;
#ifdef USE_LRU
    int last_used;
#else
    unsigned int referenced;
    unsigned int ref_count;
#endif
}Frame;

typedef struct
{
    int free;
    int in_mem;
    int frame_id;//frame number
} Page;

struct FreeBlock_List
{
    int addr;
    int size;
    struct FreeBlock_List* next;
};
typedef struct FreeBlock_List FreeBlock_List;

struct AllocatedBlock_List
{
    int addr;
    int size;
    struct AllocatedBlock_List* next;
};
typedef struct AllocatedBlock_List AllocatedBlock_List;

typedef struct
{
    Page page_table[MAX_PROCESS_PAGES];
    //for my_malloc, my_free
    FreeBlock_List* freeblocks;
    AllocatedBlock_List* allocblocks;
    sem_t alloc_signal;
}Process_Mem;

extern Process_Mem proc_mem[MAX_PROCESSES];
extern Frame frames[MAX_PHYSICAL_FRAMES];
extern sem_t os_inited_signal;

int  get_available_frame();
int  get_available_page(int ps);
int  swap_out();
void swap_in(int ps,int av_page);
int  get_newpage(int ps);
void release_page(int ps,int pg);

int  my_malloc(int ps,int rsize);
void my_free(int ps,int ptr);
void read_addr(int ps,int ptr);
void write_addr(int ps,int ptr);

int  malloc_internal(int ps,int rsize);
void free_internal(int ps,int ptr);
void read_addr_internal(int ps,int ptr);
void write_addr_internal(int ps,int ptr);


void try_merge_freeblocks(FreeBlock_List** freeblocks);

AllocatedBlock_List* new_allocblock(int addr,int size);
FreeBlock_List*      new_freeblock(int addr,int size);

void add_freeblock(FreeBlock_List** freeblocks,int addr,int size);
void add_allocblock(AllocatedBlock_List** allocblocks,int addr,int size);

void* os_proc(void* data);

#endif
