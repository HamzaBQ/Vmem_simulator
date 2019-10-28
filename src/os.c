#include "os.h"
#include "targa_creator.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "loadbmp.h"


#ifdef USE_LRU
int mem_access_counter = 0;
#endif

Process_Mem proc_mem[MAX_PROCESSES];
Frame frames[MAX_PHYSICAL_FRAMES];

sem_t os_inited_signal;
int PAD = 0;
void DO_PAD()
{
    for(int i=0;i<PAD;i++)printf(" ");
}


void init_os()
{
    for(int i=0;i<MAX_PHYSICAL_FRAMES;i++)
    {
        frames[i].page_id    = -1;
#ifdef USE_LRU
        frames[i].last_used = 0;
#else
        frames[i].referenced = 0;
        frames[i].ref_count   = 0;
#endif
    }
    for(int j=0;j<MAX_PROCESSES;j++)
    {
        for(int i=0;i<MAX_PROCESS_PAGES;i++)
        {
            proc_mem[j].page_table[i].in_mem   = 0;
            proc_mem[j].page_table[i].frame_id = -1;
            proc_mem[j].page_table[i].free     = 1;
        }
        proc_mem[j].freeblocks  = 0;
        proc_mem[j].allocblocks = 0;
        sem_init(&proc_mem[j].alloc_signal,0,0);
    }
    //signal au main thread que l'os a complété son initiation
    sem_post(&os_inited_signal);

    create_font_image();
}

//thread os
void* os_proc(void* data)
{
    init_os();
    
    for(;;)
    {
        Request* req = pop_request();
        printf("###NEW REQUEST###\n");
        switch(req->type)
        {
            case MALLOC:
            {
                *req->alloc_ptr = malloc_internal(req->process_id, req->rsize);
                sem_post(&proc_mem[req->process_id].alloc_signal);
            }break;
            case MFREE:
            {
                free_internal(req->process_id, req->free_ptr);
            }break;
            case MREAD:
            {
                read_addr_internal(req->process_id,req->access_ptr);
            }break;
            case MWRITE:
            {
                write_addr_internal(req->process_id,req->access_ptr);
            }break;
            case GLOBAL_MEM_SHOT:
            {
                printf("DRAW_SYS_MEM\n");
                //création de l'image
                Image img = draw_system_mem();
                draw_text(&img,"memoire systeme",0,0);
                add_stegano(&img,req->client_id,-1);
                // empiller netresponse
                NetResponse* netresponse = (NetResponse*)malloc(sizeof(NetResponse));
                netresponse->img = img;
                netresponse->client_sock = req->client_sock;
                push_netresponse(netresponse);
            }break;
            case PROCESS_MEM_SHOT:
            {
                if(req->process_id<MAX_PROCESSES)
                {
                    printf("DRAW_PSMEM ps:%d\n",req->process_id);
                    //création de l'image
                    Image img = draw_process_mem(req->process_id);

                    char title[128]={};
                    sprintf(title,"memoire processus n %d",req->process_id);
                    draw_text(&img,title,0,0);
                    add_stegano(&img,req->client_id,req->process_id);
                    
                    //empiller netresponse
                    NetResponse* netresponse = (NetResponse*)malloc(sizeof(NetResponse));
                    netresponse->img = img;
                    netresponse->client_sock = req->client_sock;
                    push_netresponse(netresponse);
                }
            }break;
            case MMU_TICK:
            {
#ifndef USE_LRU
                printf("MMU_TICK\n");
                for(int i=0;i<MAX_PHYSICAL_FRAMES;i++)
                {
                    frames[i].ref_count = frames[i].ref_count>>1;
                    frames[i].ref_count |= (frames[i].referenced<<31);
                
					frames[i].referenced = 0;
				}
				
#endif
            }break;
        }
        //free request mem
        free(req);
    }
    return NULL;
}


int  my_malloc(int ps,int rsize)
{
    int result;
    Request* new_req    = (Request*)malloc(sizeof(Request));
    new_req->type       = MALLOC;
    new_req->alloc_ptr  = &result;
    new_req->rsize      = rsize;
    new_req->process_id = ps;
    push_request(new_req);
    //attend le signal de l'os pour retourner l'adresse au processus
    sem_wait(&proc_mem[ps].alloc_signal);
    return result;
}

void my_free(int ps,int ptr)
{
    Request* new_req    = (Request*)malloc(sizeof(Request));
    new_req->type       = MFREE;
    new_req->free_ptr   = ptr;
    new_req->process_id = ps;
    push_request(new_req);
}

void read_addr(int ps,int ptr)
{
    Request* new_req    = (Request*)malloc(sizeof(Request));
    new_req->type       = MREAD;
    new_req->access_ptr = ptr;
    new_req->process_id = ps;
    push_request(new_req);
}

void write_addr(int ps,int ptr)
{
    Request* new_req    = (Request*)malloc(sizeof(Request));
    new_req->type       = MWRITE;
    new_req->access_ptr = ptr;
    new_req->process_id = ps;
    push_request(new_req);
}

int swap_out()
{
    PAD+=2;
    DO_PAD();
    printf("SWAP_OUT:\n");
    int min_i=0;
    //choix de la frame à décharger
#ifdef USE_LRU
    for(int i=0;i<MAX_PHYSICAL_FRAMES;i++)
    {
        if(frames[i].last_used<frames[min_i].last_used)
            min_i=i;
    }
#else
    for(int i=0;i<MAX_PHYSICAL_FRAMES;i++)
    {
        if(frames[i].ref_count<frames[min_i].ref_count)
            min_i=i;
    }
#endif

    int pg = frames[min_i].page_id;
    int ps = frames[min_i].process_id;
    
    DO_PAD();
    printf("- frame %d (associated page %d of process %d) swapped out\n", min_i,pg,ps);
    //mise à jour de la page table
    proc_mem[ps].page_table[pg].in_mem = 0;
    proc_mem[ps].page_table[pg].frame_id = -1;
    
    PAD-=2;
    return min_i;
}

int get_available_frame()
{
    // recherche et renvoie une frame libre
    int r = -1;
    for(int i=0;i<MAX_PHYSICAL_FRAMES;i++)
    {
        if(frames[i].page_id==-1)
        {
            r = i;
            break;
        }
    }
    //aucune frame libre, on fait appel à swap_out pour décharger une
    if(r==-1)
    {
        return swap_out();
    }
    
    return r;
}

void swap_in(int ps,int av_page)
{
    //associe la page d'un processus à une frame physique libre
    PAD+=2;
    DO_PAD();
    printf("SWAP IN: PS %d\n",ps);
    int av_frame = get_available_frame();
    
    DO_PAD();
    printf("-for page %d",av_page);
    printf(" -> assoc_frame %d\n",av_frame);

    //mise à jour de la page_table
    proc_mem[ps].page_table[av_page].in_mem   = 1;
    proc_mem[ps].page_table[av_page].frame_id = av_frame;
    
    frames[av_frame].page_id    = av_page;
    frames[av_frame].process_id = ps;
#ifndef USE_LRU
    frames[av_frame].referenced = 0;
    frames[av_frame].ref_count = 0;
#endif
    PAD-=2;
}

int get_available_page(int ps)
{
    //retourne une page libre dand l'espace d'adressage virtuel d'un processus
    int r = -1;
    for(int i=0;i<MAX_PROCESS_PAGES;i++)
    {
        if(proc_mem[ps].page_table[i].free)
        {
            r = i;
            break;
        }
    }
    assert(r!=-1);
    return r;
}

int get_newpage(int ps)
{
    //retourne l'id d'une page libre et mit à jour la page table
    PAD+=2;
    DO_PAD();
    printf("GET_NEWPAGE: process = %d \n",ps);
    int av_page  = get_available_page(ps);
    DO_PAD();
    printf("- page %d\n",av_page);
    
    proc_mem[ps].page_table[av_page].in_mem = 0;
    proc_mem[ps].page_table[av_page].free   = 0;
    proc_mem[ps].page_table[av_page].frame_id     = -1;
    
    PAD-=2;
    return av_page;
}

void release_page(int ps,int pg)
{
    PAD+=2;
    DO_PAD();
    printf("RELEASE_PAGE: page %d,process %d \n",pg,ps);

    if(proc_mem[ps].page_table[pg].in_mem)
    {
        int f = proc_mem[ps].page_table[pg].frame_id;
        frames[f].page_id = -1;
        frames[f].process_id = -1;
    }
    proc_mem[ps].page_table[pg].in_mem   = 0;
    proc_mem[ps].page_table[pg].free     = 1;
    proc_mem[ps].page_table[pg].frame_id = -1;
    
    PAD-=2;
}

//#### fonction de manipulation des liste chainées représentant
//#### les bloques vide et utilisé (ajout, création, ...)
FreeBlock_List* new_freeblock(int addr,int size)
{
    FreeBlock_List* r = (FreeBlock_List*)malloc(sizeof(FreeBlock_List));
    r->addr = addr;
    r->size = size;
    r->next = 0;
    return r;
}

AllocatedBlock_List* new_allocblock(int addr,int size)
{
    AllocatedBlock_List* r = (AllocatedBlock_List*)malloc(sizeof(AllocatedBlock_List));
    r->addr = addr;
    r->size = size;
    r->next = 0;
    return r;
}

void add_allocblock(AllocatedBlock_List** allocblocks,int addr,int size)
{
    if(size<1)
        return;
    
    AllocatedBlock_List* ab = new_allocblock(addr,size);
    AllocatedBlock_List* p = *allocblocks;
    if(!*allocblocks)
    {
        *allocblocks = ab;
        return;
    }
    while(p->next)
    {
        p = p->next;
    }
    p->next = ab;
}

void add_freeblock(FreeBlock_List** freeblocks,int addr,int size)
{
    //l'insertion se fait en gardant un ordre croissant des adresses
    //des bloques vide
    if(size<1)
        return;
    
    FreeBlock_List* fb = new_freeblock(addr,size);
    FreeBlock_List* p = *freeblocks;
    //liste vide
    if(!p)
    {
        *freeblocks = fb;
        return;
    }
    
    if(addr < p->addr)
    {
        fb->next = p;
        *freeblocks = fb;
        return;
    }
    while(p->next)
    {
        if(addr < p->next->addr)
            break;
        p = p->next;
    }
    //insérer entre p1 et p1->next
    if(p->next)
    {
        fb->next = p->next;
    }
    p->next = fb;
}

void try_merge_freeblocks(FreeBlock_List** freeblocks)
{
    FreeBlock_List* p = *freeblocks;
    if(!p)return;
    
    while(p->next)
    {
        if(p->addr+p->size == p->next->addr)
        {
            //fusionner p et p->next
            FreeBlock_List* nextnext = p->next->next;
            p->size +=p->next->size;
            free(p->next);
            p->next = nextnext;
            //on ne change pas p, vu la possiblité de faire une autre fusion
            //avec le bloque qui vient après
        }
        else
        {
            p = p->next;
        }
    }
}

void release_free_pages(int ps,FreeBlock_List** freeblocks)
{
    FreeBlock_List* p = *freeblocks;
    FreeBlock_List* prev = 0;
    if(!p)return;
    printf("#TRY RELEASE FREE PAGES\n");
    while(p)
    {
        //si le bloque contient des pages entières, ces pages doivent
        //être libéré.. des bloques vide sont créér pour ce qui reste
        int on_page_bndry= (((p->addr/PAGE_SIZE)*PAGE_SIZE) == p->addr);
        int num_free_pages=0;
        int first_page = 0;
        if(on_page_bndry)
        {
            num_free_pages  = p->size/PAGE_SIZE;
            first_page = p->addr/PAGE_SIZE;
 
            int free_rest = p->size - num_free_pages*PAGE_SIZE;
            //delete this block
            if(!prev)//if first block
                *freeblocks = (*freeblocks)->next;
            else
            {
                prev->next = p->next;
            }
            add_freeblock(freeblocks,
                          (first_page+num_free_pages)*PAGE_SIZE,
                          free_rest);
        }
        else
        {
            int next_page_addr = ((p->addr/PAGE_SIZE)+1)*PAGE_SIZE;
            num_free_pages  = (p->size - (next_page_addr-p->addr))/PAGE_SIZE;
            first_page = (p->addr+PAGE_SIZE-1)/PAGE_SIZE;
 
            if(num_free_pages)
            {
                int free_rest_front = next_page_addr - p->addr;
                int free_rest_end   = p->size - free_rest_front
                    - num_free_pages*PAGE_SIZE;
                //delete this block
                if(!prev)//if first block
                    *freeblocks = (*freeblocks)->next;
                else
                {
                    prev->next = p->next;
                }
                add_freeblock(freeblocks,p->addr,free_rest_front);
                add_freeblock(freeblocks,
                              next_page_addr+num_free_pages*PAGE_SIZE,
                              free_rest_end);
            }
        }

        for(int i=0;i<num_free_pages;i++)
        {
            release_page(ps,first_page+i);
        }
        prev = p;
        p=p->next;
        
    }
}

int malloc_internal(int ps,int rsize)
{
    PAD+=2;
    DO_PAD();
    printf("MY_MALLOC: process %d, rsize %d\n",ps,rsize);
    
    FreeBlock_List* p = proc_mem[ps].freeblocks;
    FreeBlock_List* prev = 0;
    int f = 0;
    FreeBlock_List* chosed_fb = 0;
    int chosed_fb_size = 0;
    //y'a-t-il un bloque avec une taille < rsize
    //chosed_fb pointe vers le bloque choisi selon Best_fit ou Worst-fit
    while(p)
    {
        if(p->size>=rsize)
        {
#ifdef USE_BEST_FIT
            if(chosed_fb)
            {
                if(chosed_fb->size > p->size)
                    chosed_fb = p;
            }
            else
                chosed_fb = p;
#else
            if(chosed_fb)
            {
                if(chosed_fb->size < p->size)
                    chosed_fb = p;
            }
            else
                chosed_fb = p;
#endif
        }
        
        prev = p;
        p = p->next;
    }

    p=chosed_fb;

    //aucun bloque trouvé
    if(!p)
    {//acquérir le nombre pages nécessaire pour allouer rsize
        DO_PAD();
        printf("-no free block found\n");
        int rest = rsize%PAGE_SIZE;
        int pages_needed = rsize/PAGE_SIZE + (rest!=0);
        int first_page = get_newpage(ps);
        for(int i=0;i<pages_needed-1;i++)
            get_newpage(ps);
        int addr = first_page*PAGE_SIZE;
        add_allocblock(&proc_mem[ps].allocblocks,addr,rsize);

        if(rest!=0)
        {
            int last_page = first_page + pages_needed - 1;
            int last_page_addr = last_page*PAGE_SIZE;
            printf("add freeblock addr:%d,size:%d\n",last_page_addr + rest,PAGE_SIZE - rest);
            add_freeblock(&proc_mem[ps].freeblocks,last_page_addr + rest,PAGE_SIZE - rest);
        }
        DO_PAD();
        printf("-returned addr = %d\n",addr);
        PAD-=2;
        return addr;
    }
    //bloque trouvé
    int addr = p->addr;
    add_allocblock(&proc_mem[ps].allocblocks,p->addr,rsize);
    if(p->size == rsize)
    {
        //supprimer le bloque vide
        if(!prev)//first node
        {
            proc_mem[ps].freeblocks = proc_mem[ps].freeblocks->next;
        }
        else
        {
            prev->next = p->next;//supprimer p de la liste chainée
            free(p);
        }
    }
    else
    {
        //on procède par ajustement de p->addr et p->size et on garde le bloque
        p->addr = p->addr + rsize;
        p->size = p->size - rsize;
    }
    DO_PAD();
    printf("-returned addr = %d\n",addr);
    PAD-=2;
    return addr;
    
}
void free_internal(int ps,int ptr)
{
    PAD+=2;
    DO_PAD();
    printf("MY_FREE: process %d\n",ps);
    assert(proc_mem[ps].allocblocks);
    AllocatedBlock_List* p = proc_mem[ps].allocblocks;
    AllocatedBlock_List* prev = 0;
    //recherche du bloque en question
    while(p)
    {
        if(p->addr == ptr)
            break;
        prev = p;
        p = p->next;
    }
    //bloque non trouvé
    if(!p)
    {
        DO_PAD();
        printf("-trying to free block(ptr = %d) that wasn't allocated\n",ptr);
        return;
    }
    
    add_freeblock(&proc_mem[ps].freeblocks, p->addr,p->size);
    //suppimer le block alloué
    if(!prev)//premier noeuds
    {
        proc_mem[ps].allocblocks = proc_mem[ps].allocblocks->next;
    }
    else
    {
        prev->next = p->next;//supprimer p de la liste chainée
        free(p);
    }
    
    try_merge_freeblocks(&proc_mem[ps].freeblocks);
    release_free_pages(ps,&proc_mem[ps].freeblocks);
    DO_PAD();
    printf("- ptr %d freed success\n",ptr);
    PAD-=2;
}

void read_addr_internal(int ps,int ptr)
{
    PAD+=2;
    DO_PAD();
    printf("read_ADDR %d process %d\n", ptr,ps);
    
    int page = ptr/PAGE_SIZE;

    // violation d'acces, cette page n'a jamais été alloué
    assert(proc_mem[ps].page_table[page].free==0);
    //page non présente en mémoire -> swap in
    if(!proc_mem[ps].page_table[page].in_mem)
    {
        swap_in(ps,page);
    }
    //mise à jour des différent variable d'état pour LRU et viellissement
#ifdef USE_LRU
    mem_access_counter++;
    frames[proc_mem[ps].page_table[page].frame_id].last_used = mem_access_counter;
#else
    frames[proc_mem[ps].page_table[page].frame_id].referenced = 1;
#endif
    PAD-=2;
}

void write_addr_internal(int ps,int ptr)
{
    PAD+=2;
    DO_PAD();
    printf("write ADDR %d process %d\n", ptr, ps);
    
    int page = ptr/PAGE_SIZE;

    // violation d'acces, cette page n'a jamais été alloué    
    assert(proc_mem[ps].page_table[page].free==0);
    //page non présente en mémoire -> swap in
    if(!proc_mem[ps].page_table[page].in_mem)
    {
        swap_in(ps,page);
    }
    //mise à jour des différent variable d'état pour LRU et viellissement
#ifdef USE_LRU
    mem_access_counter++;
    frames[proc_mem[ps].page_table[page].frame_id].last_used = mem_access_counter;
#else
    frames[proc_mem[ps].page_table[page].frame_id].referenced = 1;
#endif
    PAD-=2;
}
