#include <stdio.h>
#include <stdlib.h>


#pragma pack(push,1)
typedef struct {
    char id_length;
    char map_type;
    char image_type;
    short map_first;
    short map_length;
    char map_entry_size;
    short x;
    short y;
    short width;
    short height;
    char bits_per_pixel;
    char misc;
} Targa_Header;
#pragma pack(pop)

//retourne la valeur du bit de poids le plus faible
int stegano_extract_bit(unsigned char c)
{
    return c&1;
}

int main(char argc,char** argv)
{
    if(argc<2)
    {
        printf("fichier targa non specifie\n");
        printf("usage: stegano_extractor nom_fichier.tga\n");
        return 0;
    }
    //chargement du fichier en question
    FILE* file = fopen(argv[1],"rb");
    //lecture du contenu du fichier
    fseek(file,0,SEEK_END);
    int filesize=ftell(file);
    fseek(file,0,SEEK_SET);
    unsigned char* file_content = (unsigned char*)malloc(filesize);
    fread(file_content,filesize,1,file);
    printf("filesize %d\n",filesize);

    //pointeur vers les données des pixels
    unsigned char* pixels = file_content + sizeof(Targa_Header);

    //## EXTRACTION DES DONNEES
    int image_type = stegano_extract_bit(*pixels);
    printf("image_type %d\n",image_type);
    pixels++;
    if(image_type)
    {//ps_mem_shot
        unsigned char client_id=0;
        //reconstruction du client_id
        for(int i=0;i<8;i++)
        {
            client_id = client_id | (stegano_extract_bit(*pixels)<<i);
            pixels++;
        }
        unsigned char process_id=0;
        //recontruction du process_id
        for(int i=0;i<8;i++)
        {
            process_id = process_id | (stegano_extract_bit(*pixels)<<i);
            pixels++;
        }

        printf("L'image represente un representation de la memoire du processus %d, demande par le client %d\n",process_id,client_id);
    }
    else
    {//global_mem_shot
        unsigned char client_id=0;
        //reconstruction du client_id
        for(int i=0;i<8;i++)
        {
            client_id = client_id | (stegano_extract_bit(*pixels)<<i);
            pixels++;
        }
        printf("L'image represente une representation de la memoire global du systeme, demande par le client %d\n",client_id);
    }
    free(file_content);
    fclose(file);
    return 0;
}
