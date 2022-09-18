#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sprite.h"
#include "objects.h"

SpriteGroup* sprites;

void getSpritesToRender(){
    int numSprites = 1;
    sprites = malloc(numSprites*sizeof(SpriteGroup));  
    sprites[0] = readSpriteGroup("sprites/Blue_witch/B_witch_transparent.ase");

    return sprites;
}

ObjectGroup getObjects(){
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
        .step = 0,
        .state = 0,
        
    };
    objects[0] = o1;

    Object o2 = {
        .sprite = &sprites[0],
        .pos = {
            .x = 100,
            .y = 50,
        },
        .velocity = {
            .x = 0,
            .y = 0,
        },
        .step = 0,
        .state = 0,
        
    };
    objects[1] = o2;

    ObjectGroup group = {
        .objects = objects,
        .count = numObjects
    };
    return group;
}

void stepVelocity(Object* o){
    o->pos.x = o->pos.x + o->velocity.x;
    o->pos.y = o->pos.y + o->velocity.y;
}

char getSpriteIndex(SpriteGroup * spriteGroup, char step, char state){
    // calculate the amount of frames from the previous groups
    int index = 0;
    for(char i = 0; i < state; i++){
        index = index + spriteGroup->groups[i];
    }
    // get the frame index
    return index+step;
}

int positive_modulo(int i, int n) {
    return (i % n + n) % n;
}

void drawObjects(ObjectGroup objectGroup, Display *dpy, Drawable drawable, GC gc, long bg){
    Object* objects = objectGroup.objects;
    usleep(50000);
    for(int i = 0; i < objectGroup.count; i++){
        SpriteGroup* spriteGroup = objects[i].sprite;
        char prevStep = positive_modulo(objects[i].step - 1, spriteGroup->groups[objects[i].state]);
        char index = getSpriteIndex(spriteGroup, prevStep, objects[i].state);
        PixelSprite* sprite = &spriteGroup->sprites[index];

        clearSprite(sprite, objects[i].pos.x, objects[i].pos.y, dpy, drawable, gc, bg);
        stepVelocity(&objects[i]);
    }
    usleep(1000);
    for(int i=0; i<objectGroup.count; i++){
        SpriteGroup* spriteGroup = objects[i].sprite;
        PixelSprite* sprite = &spriteGroup->sprites[getSpriteIndex(spriteGroup, objects[i].step, objects[i].state)];

        drawSprite(sprite, objects[i].pos.x, objects[i].pos.y, dpy, drawable, gc);
        // add to state
        objects[i].step = (objects[i].step + 1) % (spriteGroup->groups[objects[i].state]);
    }
    XFlush(dpy);
}

void freeObjects(ObjectGroup objects){
    free(objects.objects);
    
    free(sprites[0].sprites);
    free(sprites);
    printf("Freed up object memory \n");
}
