#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sprite.h"
#include "vector.h"

typedef struct{
    SpriteGroup* sprite;
    Vector2 pos;
    Vector2 velocity;
    char step;
    char state;
}Object;

void drawObjects(Object* objects, Display *dpy, Drawable drawable, GC gc, long bg);
Object* getObjects();
void getSpritesToRender();
void freeObjects(Object* objects);