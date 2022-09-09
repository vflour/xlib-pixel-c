#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vector.h"

Vector2 addV(Vector2 v1, Vector2 v2){
    Vector2 vsum = {
        .x = v1.x+v2.x,
        .y = v1.y+v2.y
    };
    return vsum;
}

int dotV(Vector2 v1, Vector2 v2){
    return v1.x*v2.x+v1.y*v2.y;
}

