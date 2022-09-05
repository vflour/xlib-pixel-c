#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "sprite.h"

Display *dpy;
int screen;
Window win;
Window childWin;
XEvent event;
GC gc;
long black;
long white;

void initWindow();
void closeWindow();
void drawWindow();

void testSprite(){
    struct pixelSprite sprite = {
        .pixels = {(0)},
        .columns = 3
    };
    drawSprite(sprite, dpy, win, gc);
}

void initWindow(){
	dpy = XOpenDisplay((char *)0);
   	screen = DefaultScreen(dpy);

    black = BlackPixel(dpy, screen);	/* get color black */
    white = WhitePixel(dpy, screen);  /* get color white */

    win = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 0, 0, 500, 500, 1, black, white);
    XSetStandardProperties(dpy, win, "PixelController", "", None, NULL, 0, NULL);
    XSelectInput(dpy, win, ExposureMask | ButtonPressMask | KeyPressMask);
    gc = XCreateGC(dpy, win, 0, 0);
    XSetBackground(dpy, gc, white);
    XSetForeground(dpy, gc, black);

    childWin = XCreateSimpleWindow(dpy, win, 20, 20, 200, 100, 1, black, white);

    XClearWindow(dpy, win);
    XMapRaised(dpy, win);

}

void closeWindow(){
    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    printf("Goodbye \n");
    exit(0);
}

void drawWindow(){
    XClearWindow(dpy, win);
}

void eventLoop(){
    char text[255];
    KeySym key;

    struct pixelSprite spriteRef = readSprite("sprites/01_dish.png");
    printf("%d\n", spriteRef.columns);
    drawSprite(spriteRef, dpy, win, gc);

    while(1){
        XNextEvent(dpy, &event);
        if(event.type == Expose && event.xexpose.count == 0){
            drawWindow();
            drawSprite(spriteRef, dpy, win, gc);
        }
        if(event.type == KeyPress && XLookupString(&event.xkey, text, 255, &key, 0)==1){
            if(text[0] == 'q') {
                closeWindow();
            }
        }
        if (event.type == ButtonPress){
            int x = event.xbutton.x; int y = event.xbutton.y;
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%d, %d", x, y);
            XDrawString(dpy, win, gc, x, y, buffer, strlen(buffer));

        }
    }
}

// Returns true if the string length is below ten characters
bool isBelowSize(char input[]){
    int maxSize = 10;
    int size = (int)(strlen(input));
    return size <= maxSize;
}

/// Prompt user through terminal to enter their name
void askForName(){
    unsigned char maxStringSize = 255;
    char userInput[maxStringSize];
    bool valid = false;

    while(!valid){
        printf("Enter your name \n");
        scanf("%128s", userInput);
        valid = isBelowSize(userInput);
        if (!valid){
            printf("\nPlease enter a correct value \n");
        }
    }
}

// Main program
int main(){
    //askForName();
    initWindow();
    eventLoop();
    return 0;
}