#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <X11/Xlib.h>

void createWindow(){
    Display *dpy;
    int screen;
    Window win;
    XEvent event;
    GC gc;
    
	dpy = XOpenDisplay((char *)0);
   	screen = DefaultScreen(dpy);
	long black = BlackPixel(dpy,screen);	/* get color black */
	long white = WhitePixel(dpy, screen);  /* get color white */

    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 200, 300, 5, black, white);

	/* this routine determines which types of input are allowed in
	   the input.  see the appropriate section for details...
	*/
	XSelectInput(dpy, win, ExposureMask|ButtonPressMask|KeyPressMask);

	/* create the Graphics Context */
    gc=XCreateGC(dpy, win, 0,0);        

	/* here is another routine to set the foreground and background
	   colors _currently_ in use in the window.
	*/
	XSetBackground(dpy,gc,white);
	XSetForeground(dpy,gc,black);

	/* clear the window and bring it on top of the other windows */
	XClearWindow(dpy, win);
	XMapRaised(dpy, win);

    XMapRaised(dpy, win);
    XFlush(dpy);
    while(1){
        XNextEvent(dpy, &event);
    }
}

bool isBelowSize(char *input){
    int maxSize = 10;
    int size = 0;

    for(int i = 0; i<size; i++){
        size++;
    }
    return size <= maxSize;
}

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

int main(){
    askForName();
    createWindow();
    printf("Goodbye \n");
    return 0;
}