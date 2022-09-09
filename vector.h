#include <X11/Xlib.h>
#include <X11/Xutil.h>

typedef struct{
    int x,y;
}Vector2;

Vector2 addV(Vector2 v1, Vector2 v2);
int dotV(Vector2 v1, Vector2 v2);
Vector2 unitV(Vector2 v1);
