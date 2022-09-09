#ifndef SPRITE_H_
#define SPRITE_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "png.h"

#define MAX_SPRITE_SIZE (unsigned short)65535

typedef struct {
    long pixels[MAX_SPRITE_SIZE];
    int columns;
}PixelSprite;

PixelSprite readSprite(char path[]);

void clearSprite(PixelSprite *sprite, short offsetX, short offsetY, Display *dpy, Drawable drawable, GC gc, long background);
void drawSprite(PixelSprite *sprite, short offsetX, short offsetY, Display *dpy, Drawable drawable, GC gc);
unsigned long RGB(int r, int g, int b);

#endif
