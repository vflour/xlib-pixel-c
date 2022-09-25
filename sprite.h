#ifndef SPRITE_H_
#define SPRITE_H_

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "cute_aseprite.h"
#include <X11/Xos.h>
#include "vector.h"
#include "png.h"

#define MAX_SPRITE_SIZE (unsigned short)65535

typedef struct {
    long pixels[MAX_SPRITE_SIZE];
    int columns;
    int rows;
}PixelSprite;

/// @brief Group the sprites into animatable ones
typedef struct {
    PixelSprite* sprites;
    /// Store each anim group sequentially, based on the length of each one
    char* groups;
    char groupCount;
}SpriteGroup;

typedef struct{
    SpriteGroup* sprites;
}SpritePack;

PixelSprite readSprite(char* path);
SpriteGroup readSpriteGroup(char * path);

void clearSprite(PixelSprite *sprite, short offsetX, short offsetY, Display *dpy, Drawable drawable, GC gc, long background);
void drawSprite(PixelSprite *sprite, short offsetX, short offsetY, Display *dpy, Drawable drawable, GC gc, long background);
unsigned long RGB(int r, int g, int b);

#endif
