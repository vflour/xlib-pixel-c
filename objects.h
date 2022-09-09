#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sprite.h"
#include "vector.h"

typedef struct{
    PixelSprite* sprite;
    Vector2 pos;
    Vector2 velocity;
}Object;

void drawObjects(Object* objects, Display *dpy, Drawable drawable, GC gc);
Object* getObjects();
void getSpritesToRender();
void freeObjects(Object* objects);
void stepObjects(Object* objects);