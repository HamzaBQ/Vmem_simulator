#include "os.h"
#include "targa_creator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define LOADBMP_IMPLEMENTATION
#include "loadbmp.h"

Color gray       ={155,155,155};
Color dark_gray  ={55,55,55};
Color white      ={255,255,255};
Color black      ={0,0,0};
Color red        ={0,0,255};
Color dark_red   ={0,0,70};
Color green      ={0,255,0};
Color dark_green ={0,70,0};
Color blue       ={255,0,0};
Color dark_blue  ={70,0,00};

//couleur de chaque processus
Color processes_color[MAX_PROCESSES]={
    {255,255,0},
    {0,255,255},
    {255,0,255},
    {80,40,255},
    {170,60,200},
    {60,200,100},
    {0,70,255},
    {255,70,0}};

Image font_image;

Image create_image(int w,int h)
{
    Image result;
    result.buffer = (Color*)malloc(w*h*sizeof(Color));
    if(!result.buffer)
        printf("**== COULDN't alloc mem for image\n");
    result.w      = w;
    result.h      = h;
    return result;
}

void free_image(Image image)
{
    free(image.buffer);
}

void create_font_image()
{
    int err = loadbmp_decode_file("font.bmp",(unsigned char**)&font_image.buffer,&font_image.w,&font_image.h,LOADBMP_RGB);
    if(err)
    {
        printf("Failed to load font; %d\n",err);
        font_image.buffer = NULL;
    }
}

void draw_rectangle(Image* image,int x,int y,int w,int h,Color col)
{
    if(x<0)x=0;
    if(y<0)y=0;
    if(x>image->w || y>image->h)return;
    
    if(x+w>image->w)w = image->w-x;
    if(y+h>image->h)h = image->h-y;

    for(int i=x;i<x+w;i++)
        for(int j=y;j<y+h;j++)
            image->buffer[j*image->w+i]=col;
}

void draw_vertical_line(Image* image,int x,int y1,int y2,Color col)
{
    int th = 1;
    if(y1>y2)
    {
        int t=y1;
        y1=y2;
        y2=t;
    }
    draw_rectangle(image,x-th, y1, 2*th, y2-y1, col);
}

void draw_horizontal_line(Image* image,int y,int x1,int x2,Color col)
{
    int th = 1;
    if(x1>x2)
    {
        int t=x1;
        x1=x2;
        x2=t;
    }
    draw_rectangle(image,x1, y-th, x2-x1,2*th, col);
}

void clear_image(Image* image,Color col)
{
    for(int i=0;i<image->w;i++)
        for(int j=0;j<image->h;j++)
            image->buffer[j*image->w+i]=col;
}

Image draw_process_mem(int ps)
{
    Process_Mem* psmem      = proc_mem + ps;
    FreeBlock_List* fb      = psmem->freeblocks;
    AllocatedBlock_List* ab = psmem->allocblocks;

    int page_width  = 200;
    int page_height = 30;
    int xoffset     = 10;
    int yoffset     = 30;

    Image result = create_image(1024,100);
    clear_image(&result,white);

    //dessiner l'espace des pages
    draw_rectangle(&result,xoffset,yoffset,1024-5*xoffset,page_height,dark_gray);
    //dessiner les trois points
    {
        int x = 1024-4*xoffset + 10;
        int y = yoffset+page_height/2;
        int a = 4;
        draw_rectangle(&result,x-a/2,y-a/2,a,a,dark_gray);
        x+=10;
        draw_rectangle(&result,x-a/2,y-a/2,a,a,dark_gray);
        x+=10;
        draw_rectangle(&result,x-a/2,y-a/2,a,a,dark_gray);
    }

    //dessin des bloques vides
    while(fb)
    {
        int page_id    = fb->addr/PAGE_SIZE;
        int in_mem     = psmem->page_table[page_id].in_mem;
        int x          = (int)(((float)fb->addr/PAGE_SIZE)*page_width) + xoffset;
        int w          = (int)(((float)fb->size/PAGE_SIZE)*page_width);
        int y          = yoffset;
        Color col      = dark_green;
        if(in_mem) col = green;
        draw_rectangle(&result,x,y,w,page_height,col);
        draw_vertical_line(&result,x+w,yoffset-2,yoffset+page_height+2,col);
        fb = fb->next;
    }

    //dessin des bloques alloués
    while(ab)
    {
        int num_page_in_block = ab->size/PAGE_SIZE;
        int rest = ab->size - num_page_in_block*PAGE_SIZE;
        for(int i=0;i<num_page_in_block;i++)
        {
            int page_id    = (ab->addr+i*PAGE_SIZE)/PAGE_SIZE;
            int in_mem     = psmem->page_table[page_id].in_mem;
            int x          = (int)(((float)(ab->addr+i*PAGE_SIZE)/PAGE_SIZE)*page_width) + xoffset;
            int w          = page_width;
            int y          = yoffset;
            Color col      = dark_red;
            if(in_mem) col = red;
            draw_rectangle(&result,x,y,w,page_height,col);
            draw_vertical_line(&result,x+w,yoffset-2,yoffset+page_height+2,col);
        }
        if(rest!=0)
        {
            int page_id    = (ab->addr+num_page_in_block*PAGE_SIZE)/PAGE_SIZE;
            int in_mem     = psmem->page_table[page_id].in_mem;
            int x          = (int)(((float)(ab->addr+num_page_in_block*PAGE_SIZE)/PAGE_SIZE)*page_width) + xoffset;
            int w          = (int)(((float)rest/PAGE_SIZE)*page_width);
            int y          = yoffset;
            Color col      = dark_red;
            if(in_mem) col = red;
            draw_rectangle(&result,x,y,w,page_height,col);
            draw_vertical_line(&result,x+w,yoffset-2,yoffset+page_height+2,col);
        }
        ab = ab->next;
    }


    //dessin des bars indicateurs de limites de pages
    {
        int x=xoffset;
        do
        {
            draw_vertical_line(&result,x,yoffset-3,yoffset+page_height,black);
            x+=page_width;
        }while(x<1024-5*xoffset);
    }

    return result;
}

Image draw_system_mem()
{
    int page_width  = 200;
    int page_height = 30;
    int xoffset     = 10;
    int yoffset     = 30;

    Image result = create_image(1024,270);
    clear_image(&result,white);

    int num_frames_to_show = (1024-2*xoffset)/page_width;
    if(num_frames_to_show>MAX_PHYSICAL_FRAMES)
        num_frames_to_show = MAX_PHYSICAL_FRAMES;

    //dessin de l'espace des frames
    draw_rectangle(&result,xoffset,yoffset,num_frames_to_show*page_width,page_height,black);
    //dessin des frames
    for(int i=0;i<num_frames_to_show;i++)
    {
        Color col = black;
        if(frames[i].page_id!=-1)
            col = processes_color[frames[i].process_id];
        draw_rectangle(&result,xoffset+i*page_width,yoffset,page_width,page_height,col);

        if(frames[i].page_id!=-1)
        {
            char frame_desc[128];
#ifdef USE_LRU
            sprintf(frame_desc,"ps%d\npage%d\nlastused%d",
                    frames[i].process_id,frames[i].page_id,
                    frames[i].last_used);
#else
            sprintf(frame_desc,"ps%d\npage%d\nrefcount%x",
                    frames[i].process_id,frames[i].page_id,
                    frames[i].ref_count);
#endif
            draw_text(&result,frame_desc,xoffset+i*page_width,yoffset+page_height+3);
        }
    }
    
    //dessin des bars indicateurs de limites de pages    
    {
        for(int i=0;i<num_frames_to_show;i++)
            draw_vertical_line(&result,xoffset+i*page_width,yoffset-3,yoffset+page_height,black);        
    }
    return result;
}


void draw_text(Image* img, char* str,int x,int y)
{
    if(font_image.buffer == NULL)
    {
        return;
    }
    int xi = x;
    while(*str)
    {
        if(*str=='\n')
        {
            y+=CHAR_DIM;
            x=xi;
        }
        else
        {
            draw_char(img, *str,x,y);
            x+=CHAR_DIM;
        }
        str++;
    }
}

void draw_char(Image* img,char c,int x,int y)
{
    int cx=(c%16)*CHAR_DIM,cy=(c/16)*CHAR_DIM;
    //copie du charactère à partir de font_image vers l'image en question
    for(int i=0;i<CHAR_DIM;i++)
        for(int j=0;j<CHAR_DIM;j++)
            img->buffer[(x+i)+(y+j)*img->w] =
                font_image.buffer[(cx+i)+(cy+j)*font_image.w];
}


unsigned char stegano_insert(unsigned char c, int bit)
{
    //insert la valeur de 'bit' dans le bit de poids le plus faible
    //de c
    c = (c&(unsigned char)0xFE) | (unsigned char)bit;
    return c;
}

void add_stegano(Image* img,int client_id,int process_id)
{
    //insére des informations concernant l'image par stéganographie
    client_id = 55;
    if(process_id==-1)
    {
        int type = 0;//sys_mem_shot
        unsigned char C = client_id;
        int bits_needed = 8+1;//nombre de bit à insérer
        //valeur des bits à insérer
        int* bits_values=(int*)malloc(sizeof(int)*bits_needed);
        int n = 0;
        bits_values[n++] = type;
        //client_id
        for(int i=0;i<8;i++)
            bits_values[n++] = ((C&((unsigned char)1<<i)))>>i;
        
        unsigned char* buffer = (unsigned char*)img->buffer;
        //insertion des bits en question dans l'image
        for(int i=0;i<bits_needed;i++)
        {
            *buffer = stegano_insert(*buffer,bits_values[i]);
            buffer++;
        }
    }
    else
    {
        int type = 1;//ps_mem_shot
        unsigned char C = client_id;
        unsigned char P = process_id;
        int bits_needed = 8+8+1;
        //valeur des bits à insérer
        int* bits_values=(int*)malloc(sizeof(int)*bits_needed);
        int n = 0;
        bits_values[n++] = type;
        //client_id
        for(int i=0;i<8;i++)
            bits_values[n++] = (C&((unsigned char)1<<i))>>i;
        //process_id
        for(int i=0;i<8;i++)
            bits_values[n++] = ((P&((unsigned char)1<<i)))>>i;
        
        unsigned char* buffer = (unsigned char*)img->buffer;
        //insertion des bits en question dans l'image
        for(int i=0;i<bits_needed;i++)
        {
            *buffer = stegano_insert(*buffer,bits_values[i]);
            buffer++;
        }
    }
}
