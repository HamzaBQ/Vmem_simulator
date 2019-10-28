#ifndef REQUEST_QUEUE_H
#define REQUEST_QUEUE_H

#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#include "os.h"
#include "targa_creator.h"

//type de la requete
typedef enum
{
    MALLOC,
    MFREE,
    MREAD,
    MWRITE,
    GLOBAL_MEM_SHOT,
    PROCESS_MEM_SHOT,
    MMU_TICK
} RequestType;

typedef struct
{
    RequestType type;
    int process_id;
    union
    {
        int free_ptr;//pointeur à libérer
        int access_ptr;//pointeur à lire/écrire
        int client_sock;//socket utilisé pour répondre le client
        int client_id;//id du client qui fait la demande
        struct
        {
            int* alloc_ptr;//pointeur qui reçevra la valeur retourner par l'os dans le cas de MALLOC
            int rsize;//taille de la mémoire demandée
        };
     };
} Request;

struct Request_Queue_Entry
{
    Request* request;//requete
    struct Request_Queue_Entry* next;//suivant
};
typedef struct Request_Queue_Entry Request_Queue_Entry;
            
typedef struct
{
    Request_Queue_Entry* head;
    Request_Queue_Entry* tail;
    //mutex controlant l'acces à la file
    pthread_mutex_t   access_mutex;
    //semaphore signalant l'existance d'une requete dans la queue
    //qui peut être dépiler
    sem_t             work_semaphore;
    
}Request_Queue;

extern Request_Queue request_queue;

int      init_request_queue();
void     push_request(Request* request);
Request* pop_request();

//##File des réponses réseau ##

typedef struct
{
    Image img;//image à retourner au client
    int client_sock;
}NetResponse;
struct NetResponse_Queue_Entry
{
    NetResponse* netresponse;
    struct NetResponse_Queue_Entry* next;
};
typedef struct NetResponse_Queue_Entry NetResponse_Queue_Entry;

typedef struct
{
    NetResponse_Queue_Entry* head;
    NetResponse_Queue_Entry* tail;
    //mutex controlant l'acces à la file
    pthread_mutex_t   access_mutex;
    //semaphore signalant l'existance d'une réponse dans la queue
    sem_t             work_semaphore;
    
}NetResponse_Queue;

extern NetResponse_Queue netresponse_queue;

int           init_netresponse_queue();
void          push_netresponse(NetResponse* netresponse);
NetResponse*  pop_netresponse();


#endif
