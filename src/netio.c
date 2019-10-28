#include "netio.h"
#include "request_queue.h"
#include "targa_creator.h"
#include "client.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

//de charge d'envoyer les réponses au client
void* net_responder(void* d)
{
    for(;;)
    {
        //dépiller la réponse
        NetResponse* netresponse = pop_netresponse();
        printf("#NEW NET RESPONSE\n");
        //header du fichier targa
        Targa_Header header;
        header.id_length = 0;
        header.map_type = 0;
        header.image_type = 2;     // uncompressed RGB image
        header.map_first = 0;
        header.map_length = 0;
        header.map_entry_size = 0;
        header.x = 0;
        header.y = 0;
        header.width = netresponse->img.w;
        header.height = netresponse->img.h;
        header.bits_per_pixel = 24;
        header.misc = 0x20;// scan from upper left corner

        int datasize   = sizeof(header) + netresponse->img.w*netresponse->img.h*sizeof(Color);
        //envoie de la taille de réponse
        int sent_bytes = send(netresponse->client_sock, &datasize, 4, 0);
        if(sent_bytes!=4)
            printf("#net_respond: datasize not sent correctly sent_bytes=%d\n",sent_bytes);

        //enovie du header
        sent_bytes = send(netresponse->client_sock, &header,sizeof(header), 0);
        if(sent_bytes!=sizeof(header))
            printf("#net_respond: targa header not sent correctly\n");

        //envoie des pixels
        datasize-=sizeof(header);
        int bytes_sent_so_far=0;
        
        while(bytes_sent_so_far<datasize)
        {
            sent_bytes = send(netresponse->client_sock,
                 netresponse->img.buffer+bytes_sent_so_far,
                              datasize-bytes_sent_so_far,0);
            printf("-sent_bytes %d\n",sent_bytes);
            bytes_sent_so_far+=sent_bytes;
        }
        printf("#net_respond: sending finished\n");
        //free image and netresponse
        free(netresponse->img.buffer);
        free(netresponse);
    }
}

//écoute le réseau pour des requêtes distantes
//accepte la connexion et reçoit la requête
//puis il empille dans la Request_Queue
void* net_listner(void* d)
{
    int socket_desc, client_sock;
    struct sockaddr_in server,client;
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("#net_listner: Could not create socket\n");
    }
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 22101 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        printf("#net_listner: bind failed.\n");
        return NULL;
    }

    while(1)
    {
        //Listen
        listen(socket_desc , 3);

        //Accept and incoming connection
        printf("#net_listner: Listening ....\n");
        int c = sizeof(struct sockaddr_in);

        //accept connection from an incoming client
        client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);
        if (client_sock < 0)
        {
            printf("#net_listner: accept failed\n");
            continue;
        }
        printf("#net_listner: Connection accepted\n");

        //read request
        Client_Request client_request;
        {
            int read_size;
            read_size = recv(client_sock , &client_request, sizeof(Client_Request) , 0) ;
            if(read_size!=sizeof(Client_Request))
                printf("#net_listner: didn't receive whole request\n");
        }

        printf("request type %d, ps %d, client %d\n",
               client_request.type,
               client_request.process_id,
               client_request.client_id);
        
        Request* new_req    = (Request*)malloc(sizeof(Request));
        new_req->type       = client_request.type;
        new_req->process_id = client_request.process_id;
        new_req->client_sock= client_sock;
        //empiller la requête
        push_request(new_req);
        
        printf("#net_listner: Request pushed into queue\n");
    }
}
