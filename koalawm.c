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

typedef struct window_t {
	int x; // change these to whatever XCB's coordinates are in
	int y; // ^
	int height;
	int width;
	xcb_window_t xcb_window;

	struct window_t * w_next;
	struct window_t * w_prev;
} window_t;

typedef struct desktop_t {
	char * name; //name for current workspace
	int num_windows;
	int tiling_mode; //for future when multiple tiling modes, fibonnaci amirite bart.

    window_t * w_head;
	window_t * w_tail;
} desktop_t;


// YO WHY LINKED LIST OF DESKTOPS, Array is ezier as it is static
typedef struct master_t {
	uint16_t width; //size of screen
	uint16_t height;
    bool panel; //show panel
    int num_desktops;
    int currDesktop;

    desktop_t * desktops;
} master_t;

/*variables*/
static bool running = true;
static xcb_connection_t *dpy;
static xcb_screen_t *screen;
static xcb_window_t root;
static master_t * master;

/*functions*/
void initKeys(void);
void handleKeyPress(xcb_generic_event_t *event);
void handleMapRequest(xcb_generic_event_t *event);
int init(void);
void run(void);
void quit();
void launch(const Arg *arg);
void initStructs(void);
void cleanup(void);
void addWindowToDesktop(int desktopNum, xcb_window_t window, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

#include "config.h"

int main(void)
{
    init();
	run();
	cleanup();
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

void handleMapRequest(xcb_generic_event_t *event)
{
	xcb_map_request_event_t   *ev = (xcb_map_request_event_t *)event;

	xcb_window_t 			  window = ev->window;

	//TODO: functions
	uint32_t x = 300;
	uint32_t y = 300;
	uint32_t width = 300; // call function to determine width
	uint32_t height = 300; // call function to determine height
	uint32_t border = 10;

	addWindowToDesktop(master->currDesktop, window, x, y, width, height);

	uint16_t configWindowMask = XCB_CONFIG_WINDOW_X | 
								XCB_CONFIG_WINDOW_Y | 
								XCB_CONFIG_WINDOW_WIDTH | 
								XCB_CONFIG_WINDOW_HEIGHT | 
								XCB_CONFIG_WINDOW_BORDER_WIDTH;

	const uint32_t configValues[] = { x, y, width, height, border };
	xcb_configure_window(dpy, 
						 window,
						 configWindowMask,
						 configValues);

	xcb_map_window(dpy, window);

	//CALL FUNCTION HERE TO FIX THE REST OF THE WINDOWS ON THE DESKTOP

	xcb_flush(dpy);
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
		return 1; //there was an error with the NUM_DESKTOPSconnection
	//should look into err.h^^^

	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	root = screen->root;

	unsigned int values[1] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT|
                              XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY|
                              XCB_EVENT_MASK_PROPERTY_CHANGE|
                              XCB_EVENT_MASK_BUTTON_PRESS};

    //TODO: error checking using xcb_request_check
	xcb_change_window_attributes(dpy, root, XCB_CW_EVENT_MASK, values);
	xcb_flush(dpy);

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
	master->currDesktop = 0;

	master->desktops = (desktop_t *)malloc(sizeof(desktop_t)*NUM_DESKTOPS);
	//need to initialize the desktop structs
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
			case XCB_MAP_REQUEST:
			{
				handleMapRequest(event);
				break;
			}
			case XCB_CONFIGURE_REQUEST:
			{
				break;
			}
			case XCB_CLIENT_MESSAGE:
			{
				break;
			}
		    default:
		        break;	   
		}
	}
}

void cleanup(void)
{
	int i;
	window_t * iter;
	for(i = 0; i < NUM_DESKTOPS; ++i)
	{
		iter = master->desktops[i].w_head;
		while(iter != NULL)
		{
			window_t * next = iter->w_next;
			free(iter);
			iter = next;
		}
	}

	free(master->desktops);
	free(master);

}

void quit(void)
{
	running = false;
	xcb_disconnect(dpy);
}

void launch(const Arg *arg)
{
	fork();
	execv((char*)(arg->com[0]), (char**)(arg->com));
}

desktop_t * getDesktop(int desktopNum)
{
	if(desktopNum >= 0 && desktopNum < NUM_DESKTOPS)
	{
		return &master->desktops[master->currDesktop];
	}
	else return NULL;
}

void addWindowToDesktop(int desktopNum, xcb_window_t window, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	// init new window struct
	window_t * newWindow = (window_t *)malloc(sizeof(window_t));
	newWindow->xcb_window = window;
	newWindow->x = x;
	newWindow->y = y;
	newWindow->width = width;
	newWindow->height = height;
	newWindow->w_next = NULL;
	newWindow->w_prev = NULL;

	desktop_t * desktop = getDesktop(desktopNum);
	if(desktop)
	{
		//add to head of linked list
		if(desktop->w_head == NULL)
		{
			desktop->w_head = newWindow;
		}
		else
		{
			window_t * prevHead = desktop->w_head;
			newWindow->w_next = prevHead;
			prevHead->w_prev = newWindow;
			desktop->w_head = newWindow;
		}
	}


}



