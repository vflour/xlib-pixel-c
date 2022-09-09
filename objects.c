#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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
        .pos = {
            .x = 100,
            .y = 100,
        },
        .velocity = {
            .x = 0,
            .y = 0,
        },
        
    };
    objects[0] = o1;

    Object o2 = {
        .sprite = &sprites[0],
        .pos = {
            .x = 100,
            .y = 50,
        },
        .velocity = {
            .x = 1,
            .y = 0,
        },
        
    };
    objects[1] = o2;

    return objects;
}

void stepVelocity(Object* o){
    o->pos.x = o->pos.x + o->velocity.x;
    o->pos.y = o->pos.y + o->velocity.y;
}

void drawObjects(Object* objects, Display *dpy, Drawable drawable, GC gc){
    XClearWindow(dpy, drawable);
    for(int i = 0; i < 2; i++){
        drawSprite(objects[i].sprite, objects[i].pos.x, objects[i].pos.y, dpy, drawable, gc);
    }
}

void stepObjects(Object* objects){
    for(int i = 0; i < 2; i++){
        stepVelocity(&objects[i]);
    }
}

void freeObjects(Object* objects){
    free(sprites);
    free(objects);
    printf("Freed up object memory \n");
}
