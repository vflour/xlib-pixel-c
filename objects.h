#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sprite.h"

typedef struct{
    SpriteGroup* sprite;
    Vector2 pos;
    Vector2 velocity;
    char step;
    char state;
}Object;

typedef struct{
    Object* objects;
    int count;
}ObjectGroup;

void drawObjects(ObjectGroup objects, Display *dpy, Drawable drawable, GC gc, long bg);
ObjectGroup getObjects();
ObjectGroup getMap(char * path);
void getSpritesToRender();
void freeObjects(ObjectGroup objects);