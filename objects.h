#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sprite.h"

typedef struct{
    PixelSprite* sprite;
    int x, y;
    int vx, vy;
}Object;

void drawObjects(Object* objects, Display *dpy, Drawable drawable, GC gc);
Object* getObjects();
void getSpritesToRender();