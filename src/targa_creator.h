#ifndef TARGA_CREATOR_H
#define TARGA_CREATOR_H

#define CHAR_DIM 16//largeur et longeur d'un caractère dans l'image font_image

#pragma pack(push,1)
typedef struct
{
    unsigned char b,g,r;//ordre de stockage des couleurs dans une image TARGA
}Color;
#pragma pack(pop)

typedef struct
{
    Color* buffer;
    int w,h;
}Image;

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

extern Color white;
extern Color black;    
extern Color red;   
extern Color dark_red;  
extern Color green;  
extern Color dark_green;
extern Color blue;      
extern Color dark_blue; 
extern Color gray;
extern Color dark_gray;
extern Color processes_color[MAX_PROCESSES];

extern Image font_image;

Image create_image(int w,int h);
void  free_image(Image image);
void draw_rectagle(Image* image,int x,int y,int w,int h,Color col);
void draw_vertical_line(Image* image,int x,int y1,int y2,Color col);
void draw_horizontal_line(Image* image,int y,int x1,int x2,Color col);
void clear_image(Image* image,Color col);
Image draw_process_mem(int ps);
Image draw_system_mem();
void create_font_image();

void draw_text(Image* img, char* str,int x,int y);
void draw_char(Image* img, char c,int x,int y);

void add_stegano(Image* img,int client_id,int process_id);
#endif
