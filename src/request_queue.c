#include "request_queue.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

//global request queue
Request_Queue request_queue;

int init_request_queue()
{
    request_queue.head = request_queue.tail = 0;
    pthread_mutex_init(&request_queue.access_mutex,0);
    sem_init(&request_queue.work_semaphore,0,0);
    return 1;
}

void push_request(Request* request)
{
    Request_Queue_Entry* new_entry = (Request_Queue_Entry*) malloc(sizeof(Request_Queue_Entry));
    new_entry->next    = 0;
    new_entry->request = request;

    pthread_mutex_lock(&request_queue.access_mutex);
    if(request_queue.head==0)
    {
        request_queue.tail = request_queue.head = new_entry;
    }
    else
    {
        request_queue.tail->next = new_entry;
        request_queue.tail = new_entry;
    }
    pthread_mutex_unlock(&request_queue.access_mutex);
    
    sem_post(&request_queue.work_semaphore);
}

Request* pop_request()
{
    sem_wait(&request_queue.work_semaphore);
    
    Request_Queue_Entry* entry = request_queue.head;
    request_queue.head        = request_queue.head->next;
    
    Request* result;
    result = entry->request;
    //free entry memory
    free(entry);

    return result;
}

NetResponse_Queue netresponse_queue;

int init_netresponse_queue()
{
    netresponse_queue.head = netresponse_queue.tail = 0;
    //pthread_mutex_init(&netresponse_queue.access_mutex,0);
    sem_init(&netresponse_queue.work_semaphore,0,0);
    return 1;
}

void  push_netresponse(NetResponse* netresponse)
{
    NetResponse_Queue_Entry* new_entry = (NetResponse_Queue_Entry*) malloc(sizeof(NetResponse_Queue_Entry));
    new_entry->next    = 0;
    new_entry->netresponse = netresponse;

    //pthread_mutex_lock(&netresponse_queue.access_mutex);
    if(netresponse_queue.head==0)
    {
        netresponse_queue.tail = netresponse_queue.head = new_entry;
    }
    else
    {
        netresponse_queue.tail->next = new_entry;
        netresponse_queue.tail = new_entry;
    }
    //pthread_mutex_unlock(&netresponse_queue.access_mutex);
    
    sem_post(&netresponse_queue.work_semaphore);
}

NetResponse* pop_netresponse()
{
    sem_wait(&netresponse_queue.work_semaphore);
    
    NetResponse_Queue_Entry* entry = netresponse_queue.head;
    netresponse_queue.head         = netresponse_queue.head->next;
    
    NetResponse* result;
    result = entry->netresponse;
    //free entry memory
    free(entry);

    return result;
}
