#ifndef SPRITE_H_
#define SPRITE_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "png.h"

#define MAX_SPRITE_SIZE (unsigned short)65535

struct pixelSprite {
    long pixels[MAX_SPRITE_SIZE];
    int columns;
};

struct pixelSprite readSprite(char path[]);
void drawSprite(struct pixelSprite sprite, Display *dpy, Drawable drawable, GC gc);
unsigned long RGB(int r, int g, int b);

#endif
