#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "request_queue.h"
#include "os.h"
#include "client.h"
#include "netio.h"

#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

//processus P1
void* P1(void* d)
{
    int ps = 0;
    int ptr1 = my_malloc(ps,3000);
    int ptr2 = my_malloc(ps,3000);
    int ptr3 = my_malloc(ps,2000);
    sleep(5);
    int ptr4 = my_malloc(ps,1000);
    for(int i=0;i<10;i++)
    {
        if(i%2)
        read_addr(ps,ptr1);
        else
        {
            read_addr(ps,ptr3);
            read_addr(ps,ptr3);
        }
    }
    sleep(5);
    my_free(ps,ptr4);
    sleep(5);
    my_free(ps,ptr1);
}

//Processus P2
void* P2(void* d)
{
    int ps = 1;
    int ptr1 = my_malloc(ps,5000);
    int ptr2 = my_malloc(ps,3000);
    int ptr3 = my_malloc(ps,2024);
    sleep(7);
    for(int i=0;i<20;i++)
    {
        if(i%2)
        read_addr(ps,ptr3);
        else
        {
            read_addr(ps,ptr1);
            read_addr(ps,ptr1);
        }
    }
    sleep(5);
    read_addr(ps,ptr2);
    sleep(5);
    my_free(ps,ptr1);
    my_free(ps,ptr2);
}

#ifndef USE_LRU
//dans le cas d'utilisation du viellissement, l'os a besoin de mettre
//le bit referenced de chaque frame.. ceci ce fait à l'aide d'une requête
//MMU_TICK que le main_thread envoie de manière périodique
void send_mmu_tick()
{
    Request* new_req    = (Request*)malloc(sizeof(Request));
    new_req->type       = MMU_TICK;
    push_request(new_req);
}
#endif
int main(char** argv,char argc)
{
    init_request_queue();
    init_netresponse_queue();

#ifndef USE_LRU
    //installation du timer pour le MMU_TICK
    struct sigaction sa;
    struct itimerval timer;
    memset (&sa, 0, sizeof (sa));
    sa.sa_handler = &send_mmu_tick;
    sigaction (SIGALRM, &sa, NULL);
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 500000;//500msec
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 500000;
    setitimer (ITIMER_REAL, &timer, NULL);
#endif
    sem_init(&os_inited_signal,0,0);
    pthread_t os_thread;
    //lancer le thread os
    int ret = pthread_create(&os_thread,NULL,os_proc,NULL);    
    if(ret)
    {
        printf("Error(os_thread creation) - pthread_create() return code: %d\n",ret);
        return 0;
    }
    //l'os vient de terminer la phase d'initiaition,
    //on peut lancer les Pi maintenant
    sem_wait(&os_inited_signal);

    //lancer le thread NetResponder
    pthread_t netresponse_thread;
    pthread_create(&netresponse_thread,NULL,net_responder,NULL);
    //lancement des Pi
    pthread_t p1_thread,p2_thread;
    pthread_create(&p1_thread,NULL,P1,NULL);
    pthread_create(&p2_thread,NULL,P2,NULL);
    //écouter le réseau
    net_listner(NULL);
    
    while(1);
    //pthread_join(p1_thread,NULL);
    //pthread_cancel(os_thread);

    return 0;
}
