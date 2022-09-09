#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "png.h"

#define PNG_DEBUG 3
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "sprite.h"

/// @brief Converts RGB values into an unsigned long val
/// @param r
/// @param g 
/// @param b 
/// @return long value color
unsigned long RGB(int r, int g, int b){
    return b + (g<<8) + (r<<16);
}

void abort_(const char * s, ...)
{
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	fprintf(stderr, "\n");
	va_end(args);
	abort();
}

/// @brief Draws the sprite onto a drawable
/// @param sprite 
/// @param dpy 
/// @param drawable 
/// @param gc 
void drawSprite(PixelSprite *sprite, short offsetX, short offsetY, Display *dpy, Drawable drawable, GC gc){
    for (unsigned short i = 0; i < MAX_SPRITE_SIZE; i++){
        short x = i%sprite->columns+offsetX;
        short y = i/sprite->columns+offsetY;

        if(sprite->pixels[i]!=0){
            XSetForeground(dpy, gc, sprite->pixels[i]);
            XDrawPoint(dpy, drawable, gc, x, y);
        }
    }
}

void clearSprite(PixelSprite *sprite, short offsetX, short offsetY, Display *dpy, Drawable drawable, GC gc, long background){
    XSetForeground(dpy, gc, background);
    for (unsigned short i = 0; i < MAX_SPRITE_SIZE; i++){
        short x = i%sprite->columns+offsetX;
        short y = i/sprite->columns+offsetY;

        if(sprite->pixels[i]!=0){
            XDrawPoint(dpy, drawable, gc, x, y);
        }
    }
}

/// @brief Reads a png file from a path.
/// @param file_name 
/// @param width 
/// @param height 
/// @return Pixel buffer for the reader
png_bytep* readPng(char* file_name, int *width, int * height){
    png_bytep * row_pointers;
    int x, y;

    png_byte color_type;
    png_byte bit_depth;

    png_structp png_ptr;
    png_infop info_ptr;
    int number_of_passes;

    char header[8];    // 8 is the maximum size that can be checked

    /* open file and test for it being a png */
    FILE *fp = fopen(file_name, "rb");
    if (!fp)
	    abort_("[read_png_file] File %s could not be opened for reading", file_name);
    fread(header, 1, 8, fp);
    if (png_sig_cmp(header, 0, 8))
	    abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


    /* initialize stuff */
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    if (!png_ptr)
	    abort_("[read_png_file] png_create_read_struct failed");

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr)
	    abort_("[read_png_file] png_create_info_struct failed");

    if (setjmp(png_jmpbuf(png_ptr)))
	    abort_("[read_png_file] Error during init_io");

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);

    png_read_info(png_ptr, info_ptr);

    *width = png_get_image_width(png_ptr, info_ptr);
    *height = png_get_image_height(png_ptr, info_ptr);

    int widthVal = *width;
    int heightVal = *height;

    color_type = png_get_color_type(png_ptr, info_ptr);
    bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);

    /* read file */
    if (setjmp(png_jmpbuf(png_ptr)))
	    abort_("[read_png_file] Error during read_image");

    row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * (widthVal));

    for (y=0; y<heightVal; y++)
	    row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

    png_read_image(png_ptr, row_pointers);
    fclose(fp);

    return row_pointers;
}

/// Converts the pixel buffer into a sprite
PixelSprite processFile(png_bytep * row_pointers, int width, int height){
    PixelSprite sprite;
    sprite.columns = width;

    /// Go through each row
    for (int y=0; y< height; y++) {
        png_byte* row = row_pointers[y];
        for (int x=0; x< width; x++) {
            png_byte* ptr = &(row[x*4]);
            short index = y*(width) + x; 
            sprite.pixels[index] = RGB(ptr[0], ptr[1], ptr[2]);
        }
    }
    return sprite;
}

/// @brief Get the path of a sprite and draw it
PixelSprite readSprite(char * path){
    int width, height;
    png_bytep * row_pointers = readPng(path, &width, &height);

    PixelSprite sprite = processFile(row_pointers, width, height);
    return sprite;
}