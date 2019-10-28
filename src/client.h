#ifndef CLIENT_H
#define CLIENT_H

#include "request_queue.h"

typedef struct
{
    RequestType type;
    int client_id;
    int process_id;
} Client_Request;

#endif
