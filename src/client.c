#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "client.h"
#include "targa_creator.h"

int main(int argc , char *argv[])
{
    int ID = 0;
    if(argc<2)
    {
        printf("vous devez indiquer l'ID du client en params\n");
        return 0;
    }
    else
        ID = atoi(argv[1]);
    printf("Launch client, ID = %d\n",ID);
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[2000];
    
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 22101 );
    
    int img_seq = 0;
    int out_gif_seq = 0;
    while(1)
    {
        printf("Entrez le nombre de votre choix:\n");
        printf("1- image representant la memoire du systeme\n");
        printf("2- image representant la memoire d'un processus\n");
        printf("3- creer un gif des images reçu\n");
        printf("4- quitter\n");
        printf("Votre choix:");
        int choix=0;
        scanf("%d",&choix);
        
        int send_a_req = 0;
        int rcv_count=0;
        Client_Request request;
        if(choix==4)
            break;
        if(choix==1)
        {
            request.type       = GLOBAL_MEM_SHOT;
            request.client_id  = ID;
            request.process_id = -1;
            send_a_req = 1;
        }
        else if(choix==2)
        {
            printf("Entrez l'identifiant du processus: \n");
            scanf("%d",&request.process_id);
            request.type       = PROCESS_MEM_SHOT;
            request.client_id  = ID;
            send_a_req = 1;
        }
        else if(choix==3)
        {
            //création de la commande à faire passer à system
            char cmd[4096]={};
            int insert_idx = 0;
            char cmd_begin[] = "convert -loop 0 -delay 200 ";
            strcpy(cmd,cmd_begin);
            insert_idx+=strlen(cmd_begin);
            for(int i=0;i<img_seq;i++)
            {
                char str_in[64]={};
                sprintf(str_in,"client%d_%d.tga ",ID,i);
                strcpy(cmd+insert_idx,str_in);
                insert_idx+=strlen(str_in);
            }
            char outfile[64]={};
            sprintf(outfile,"client%d_out%d.gif",ID,out_gif_seq++);
            strcpy(cmd+insert_idx,outfile);
            printf("launch cmd \"%s\"\n",cmd);
            system(cmd);
        }
        
        if(send_a_req)
        {
            //Create socket
            sock = socket(AF_INET , SOCK_STREAM , 0);
            if (sock == -1)
            {
                printf("Could not create socket\n");
                exit(5);
            }
            printf("Socket created\n");
            
            //Connect to remote server
            if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
            {
                perror("connect failed. Error");
                continue;
            }
            
            printf("Connected\n");
            
            //Send some data
            if( send(sock , &request , sizeof(Client_Request) , 0) < 0)
            {
                printf("Send failed");
                continue;
            }
            printf("Request Sent\n");
            
            //
            struct timeval tv;
            tv.tv_sec = 6;  /* 30 Secs Timeout */
            tv.tv_usec = 0;  // Not init'ing this can cause strange errors
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
            //receive the verdict
            int response_datasize=0;
            if( ( rcv_count=recv(sock,&response_datasize,4,0 ))<0){
                printf("recv datasize failed");
                continue;
            }
            if(rcv_count!=4)
                printf("rcv_count for datasize !=4\n");
            
            char* response = (char*)malloc(response_datasize);
            int bytes_recv_so_far = 0;
            int rcv_success = 1;
            while(bytes_recv_so_far<response_datasize)
            {
                if( ( rcv_count=recv(sock,response+bytes_recv_so_far,
                                     response_datasize - bytes_recv_so_far,0 ))<0)
                {
                    printf("recv response failed");
                    rcv_success = 0;
                    break;
                }
                bytes_recv_so_far+=rcv_count;
            }
            if(rcv_success)
            {
                //image réponse reçue
                //stockage dans un fichier
                char filename[64];
                sprintf(filename,"client%d_%d.tga",ID,img_seq++);
                FILE* targa_file = fopen(filename,"wb");
                fwrite(response,response_datasize,1,targa_file);
                fclose(targa_file);
            }
        }
        close(sock);
    }
    return 0;
}
