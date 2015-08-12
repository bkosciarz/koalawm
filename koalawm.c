#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#define LENGTH(x) ((int)sizeof(x)/(int)sizeof(*x)) //only works on staticly allocated memory
#define true 1
#define false 0
#define bool short

typedef union{
    const char ** com;
	const int i;
} Arg;

/* 
 * key structure that holds the modifiers pressed, the keysym, 
 * a corresponding function and it's arguments
 */
typedef struct {
    uint32_t modifier;    
    xcb_keysym_t keysym;
	void (*fptr)(const Arg *); //send null terminated array of void* args
	const Arg arg;
} key;


/*
 * Window structure that holds coordinates for managing windows on screen
 * The next and previous pointers are for maintaining position in the linked list
 */

typedef struct {
	int x; // change these to whatever XCB's coordinates are in
	int y; // ^
	int height;
	int width; //^

	struct window_t * w_next;
	struct window_t * w_prev; //DLL?
} window_t;

typedef struct {
	char * name; //name for current workspace
	int num_windows;
	int tiling_mode; //for future when multiple tiling modes

    window_t * w_head;
	window_t * w_tail;

	struct desktop_t * d_next;
	struct desktop_t * d_prev;
} desktop_t;

typedef struct {
	uint16_t width; //size of screen
	uint16_t height;
    bool panel; //show panel
    int num_desktops;

    desktop_t * d_head;
	desktop_t * d_tail;
} master_t;

/*variables*/
static bool running = true;
static xcb_connection_t *dpy;
static xcb_screen_t *screen;
static master_t * master;

/*functions*/
void initKeys(void);
void handleKeyPress(xcb_generic_event_t *event);
int init(void);
void run(void);
void quit();
void launch(const Arg *arg);
void initStructs(void);
void initDesktop(desktop_t * d_node);

#include "config.h"

int main(void)
{
    init();
	run();
	return 0;
}

void initKeys(void)
{
	//get keycode from keysym
	xcb_key_symbols_t 	*keysyms;
	xcb_keycode_t 		*keycode;

    xcb_ungrab_key(dpy, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

	if (!(keysyms = xcb_key_symbols_alloc(dpy)))
		exit(1);

	int i, j;
	for(i = 0; i < LENGTH(keys); ++i)
	{
        keycode = xcb_key_symbols_get_keycode(keysyms, keys[i].keysym);

        for(j = 0; keycode[j] != XCB_NO_SYMBOL; ++j)
        {
			//does not account for other modifiers such as caps or numlock
        	xcb_grab_key(dpy, 1, screen->root, keys[i].modifier, 
        				keycode[j], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }
        printf("initkey %d\n", i);
        free(keycode);
	}
	xcb_key_symbols_free(keysyms); //free allocation
}

void handleKeyPress(xcb_generic_event_t *event)
{
	xcb_key_press_event_t 	*ev 	= (xcb_key_press_event_t *)event;
	xcb_keycode_t 			keycode = ev->detail;

	//get keysym from keycode
	xcb_key_symbols_t 	*keysyms;
	xcb_keysym_t 		 keysym;

	if (!(keysyms = xcb_key_symbols_alloc(dpy)))
        exit(1);
    keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0);
    xcb_key_symbols_free(keysyms);

    int i;
    for(i = 0; i < LENGTH(keys); ++i)
    {
    	if(keysym == keys[i].keysym)
    	{
    		keys[i].fptr(&keys[i].arg);
    		break;
    	}
    }
}


/*
 * returns 0 on success
 */
int init(void)
{

//	xcb_drawable_t win;
//	xcb_drawable_t root;
    /* Open a connection to the X server that uses the value 
	 * of the DISPLAY ev and sets the screen to 0
	 */
	dpy = xcb_connect(NULL, NULL);
    if(xcb_connection_has_error(dpy))
		return 1; //there was an error with the connection
	//should look into err.h^^^

	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;

	initKeys();
	initStructs();

	return 0; 
}

void initStructs(void)
{
    master = malloc(sizeof(master_t));
    master->width = screen->width_in_pixels;
	master->height = screen->height_in_pixels;
	master->panel = SHOW_PANEL;
	master->num_desktops = NUM_DESKTOPS;

	desktop_t * d_temp1 = (desktop_t *)malloc(sizeof(desktop_t));
    desktop_t * d_temp2;
	d_temp1->d_prev = NULL;
	master->d_head = d_temp1;
 
	/* create linked list of desktops*/
	int i;
	for(i = 0; i < master->num_desktops-1; i++)
	{
		initDesktop(d_temp1);
		d_temp2 = (desktop_t *)malloc(sizeof(desktop_t));
		d_temp1->d_next = d_temp2;
		d_temp2->d_prev = d_temp1;
        d_temp1 = d_temp2;
	}
	initDesktop(d_temp1);
    d_temp1->d_next = NULL;
	master->d_tail = d_temp1;

	//need to initialize the desktop structs
}

void initDesktop(desktop_t * d_node)
{
	if(d_node)
	{
        //d_node->name = "tempname";
        d_node->num_windows = 0;
        d_node->w_head = NULL;
        d_node->w_tail = NULL;
        //would be cool if there was some default windows that we could launch
        //d_node->tiling_mode = somekindofdefault;
	}
}

void run(void)
{
	xcb_generic_event_t *event;
	while(running) 
	{
		xcb_flush(dpy);
		event = xcb_wait_for_event(dpy);
        switch(event->response_type & ~0x80) 
		{
			case XCB_KEY_PRESS:		
			{
				handleKeyPress(event);
				break;
			}
		    default:
		        break;	   
		}
	}
}

void quit()
{
	running = false;
	xcb_disconnect(dpy);
}

void launch(const Arg *arg)
{
	fork();
	execv((char*)(arg->com[0]), (char**)(arg->com));
}
