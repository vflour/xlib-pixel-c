#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sprite.h"
#include "objects.h"
#include "cute_tiled.h"

SpriteGroup* sprites;
SpritePack mapTiles;

void getSpritesToRender(){
    int numSprites = 1;
    sprites = malloc(numSprites*sizeof(SpriteGroup));  
    sprites[0] = readSpriteGroup("sprites/Blue_witch/B_witch_transparent.ase");

    return sprites;
}

ObjectGroup getObjects(){
    ObjectGroup map = getMap("sprites/map-main.tmj");
    
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
    return map;
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
        int stateIndex = objects[i].state;
        int group = spriteGroup->groups[stateIndex];
        char prevStep = positive_modulo(objects[i].step - 1, group);
        char index = getSpriteIndex(spriteGroup, prevStep, objects[i].state);

        PixelSprite* sprite = &spriteGroup->sprites[index];

        //clearSprite(sprite, objects[i].pos.x, objects[i].pos.y, dpy, drawable, gc, bg);
        stepVelocity(&objects[i]);
    }
    //usleep(1000);
    for(int i=0; i<objectGroup.count; i++){
        SpriteGroup* spriteGroup = objects[i].sprite;
        PixelSprite* sprite = &spriteGroup->sprites[getSpriteIndex(spriteGroup, objects[i].step, objects[i].state)];

        drawSprite(sprite, objects[i].pos.x, objects[i].pos.y, dpy, drawable, gc, bg);
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

PixelSprite getSubSprite(PixelSprite mainSprite, int w, int h, int i){
    PixelSprite subSprite;
    subSprite.columns = w;
    subSprite.rows = h;
    
    int totalXCells = mainSprite.columns/w;
    int totalYCells = mainSprite.rows/h;

    int start_column = (i%totalXCells)*w;
    int end_column = start_column+w;
    int start_row = (i/totalXCells)*h;
    int end_row = start_row+h;

    int index = 0;
    for (int x = start_column; x<end_column; x++){
        for(int y = start_row; y<end_row; y++){
            int subIndex = y*mainSprite.columns+x;
            subSprite.pixels[index] = mainSprite.pixels[subIndex];
            index++;
        }
    }
    
    return subSprite;
}

SpritePack getMapTiles(cute_tiled_map_t* map){
    printf("Started\n");
    cute_tiled_tileset_t* tileset = map->tilesets;
    int total_tilecount = 0;

    // get the total amount of tiles
    while(tileset){
        total_tilecount += tileset->tilecount;
        tileset = tileset->next;
    }
    tileset = map->tilesets;
    printf("%d\n", total_tilecount);

    // allocate to total amount of tiles
    SpriteGroup* sprites = calloc(total_tilecount, sizeof(SpriteGroup));
    printf("malloc done\n");
    
    // loop through tilesets again to create pixels
    int tile_index = 0;
    while(tileset){
        /// read and create the tile sprites
        cute_tiled_tile_descriptor_t * tiles = tileset->tiles;
        // add tiles directly
        char* imagePath = tileset->image.ptr;
        if(imagePath){
            char totalPath[128] = "sprites/";
            strcat(totalPath,imagePath);

            PixelSprite imageSprite = readSprite(totalPath); 
            for(int i = 0; i<tileset->tilecount; i++){
                PixelSprite subSprite = getSubSprite(imageSprite, tileset->tilewidth, tileset->tileheight, i);

                PixelSprite *spriteRef = malloc(sizeof(PixelSprite));
                spriteRef[0] = subSprite;
                int empty[] = {1};

                sprites[tile_index].sprites = spriteRef;
                sprites[tile_index].groups = empty;
                sprites[tile_index].groupCount = 1;
                tile_index++;
            }
        }
        tileset = tileset->next;
    }
    printf("Created sprite \n");
    SpritePack pack = {
        .sprites = sprites,
    };
    return pack;
}

ObjectGroup getMap(char * path){
    cute_tiled_map_t* map = cute_tiled_load_map_from_file(path, NULL);

    // get map width and height
    int w = map->width;
    int h = map->height;

    // get the tiles
    mapTiles = getMapTiles(map);

    // loop through tilecount in all layers to get the size of the map
    cute_tiled_layer_t* layer = map->layers;
    int total_data = 0;
    while (layer)
    {
        total_data += layer->data_count;
        layer = layer->next;
    }
    layer = map->layers; // reset
    // loop over the map's layers
    Object* mapObjects = calloc(total_data, sizeof(Object));
    int tile_count = 0;
    while (layer)
    {
        int* data = layer->data;
        int data_count = layer->data_count;
        for(int i = 0; i<data_count;i++){
            int id = data[i]-1;
            if(id>=0){
                SpriteGroup* sprite = &mapTiles.sprites[id];

                int x = (i%w)*map->tilewidth;
                int y = (i/h)*map->tileheight;
                Object mapObject = {
                    .sprite = sprite,
                    .pos = {
                        .x = x,
                        .y = y,
                    },
                    .velocity = {
                        .x = 0,
                        .y = 0,
                    },
                    .state = 0,
                    .step = 0,
                };


                printf("x,y %d %d \n", x, y);
                mapObjects[tile_count] = mapObject;            

                tile_count++;
            }
        }
        layer = layer->next;
    }
    cute_tiled_free_map(map);

    ObjectGroup group = {
        .objects = mapObjects,
        .count = tile_count
    };
    return group;
}
