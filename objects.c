#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sprite.h"
#include "objects.h"

PixelSprite* sprites;

void getSpritesToRender(){
    int numSprites = 1;
    sprites = malloc(numSprites*sizeof(PixelSprite));  
    sprites[0] = readSprite("sprites/01_dish.png");

    return sprites;
}

Object* getObjects(){
    int numObjects = 2;
    Object* objects = malloc(numObjects*sizeof(Object));

    Object o1 = {
        .sprite = &sprites[0],
        .x = 50,
        .y = 20,
        .vx = 0,
        .vy = 0,
    };
    objects[0] = o1;

    Object o2 = {
        .sprite = &sprites[0],
        .x = 100,
        .y = 50,
        .vx = 2,
        .vy = 2,
    };
    objects[1] = o2;

    return objects;
}

void drawObjects(Object* objects, Display *dpy, Drawable drawable, GC gc){
    for(int i = 0; i < 2; i++){
        drawSprite(objects[i].sprite, objects[i].x, objects[i].y, dpy, drawable, gc);
    }
}
