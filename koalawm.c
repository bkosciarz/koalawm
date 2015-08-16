#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_atom.h>
#include <xcb/xcb_icccm.h>
#include <X11/keysym.h>

#define LENGTH(x) ((int)sizeof(x)/(int)sizeof(*x)) //only works on staticly allocated memory
#define true 1
#define false 0
#define bool short
#define NUM_EVENTS XCB_NO_OPERATION

enum { WM_PROTOCOLS, WM_DELETE_WINDOW, WM_COUNT };
static char *WM_ATOM_NAME[]   = { "WM_PROTOCOLS", "WM_DELETE_WINDOW" };

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

static xcb_atom_t wmatoms[WM_COUNT];

/*
 * Event enum and array to simplify handling events
 */
static void (*events[NUM_EVENTS])(xcb_generic_event_t *ev);


/*functions*/
int init(void);
void initEvents(void);
void initAtoms(void);
void initKeys(void);
void initStructs(void);

void run(void);
void handleKeyPress(xcb_generic_event_t *event);
void handleMapRequest(xcb_generic_event_t *event);
void addWindowToDesktop(int desktopNum, xcb_window_t window, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
desktop_t * getDesktop(int desktopNum);
void configureWindow(xcb_window_t window, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void launch(const Arg *arg);

void quit();
void cleanup(void);


#include "config.h"

int main(void)
{
    init();
	run();
	cleanup();
	return 0;
}

/*
 * returns 0 on success
 */
int init(void)
{
    /* 
     * Open a connection to the X server that uses the value 
	 * of the DISPLAY ev and sets the screen to 0
	 */

	xcb_generic_error_t *error; // var reused to catch errors through xcb_request_check
	if(xcb_connection_has_error(dpy = xcb_connect(NULL, NULL)))
		exit(1);

	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
	root = screen->root;

	unsigned int values[1] = {XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT|
                              XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY|
                              XCB_EVENT_MASK_PROPERTY_CHANGE|
                              XCB_EVENT_MASK_BUTTON_PRESS};

	error = xcb_request_check(dpy, xcb_change_window_attributes(dpy, root, XCB_CW_EVENT_MASK, values));
	if(error) exit(1); //everything won't work if root window properties are not changed
	xcb_flush(dpy);

	initEvents();
	initAtoms();
	initKeys();
	initStructs();

	return 0; 
}


/*
 * initializes the event jump table
 */ 
void initEvents(void)
{
	//setup event array
	for(unsigned int i = 0; i < NUM_EVENTS; ++i)
		events[i] = NULL;
	events[XCB_KEY_PRESS] 	= handleKeyPress;
	events[XCB_MAP_REQUEST] = handleMapRequest;
}


/*
 * Wrapper function to initialize atoms
 */
void initAtoms(void)
{
	//setup delete window atom
	xcb_intern_atom_cookie_t cookies[WM_COUNT];
    xcb_intern_atom_reply_t  *reply;

    for (unsigned int i = 0; i < WM_COUNT; ++i)
    	cookies[i] = xcb_intern_atom(dpy, 0, strlen(WM_ATOM_NAME[i]), WM_ATOM_NAME[i]);
    for (unsigned int i = 0; i < WM_COUNT; ++i) {
    	reply = xcb_intern_atom_reply(dpy, cookies[i], NULL);
    	if(reply) {
    		wmatoms[i] = reply->atom;
    		free(reply);
    	}
    	else
    		printf("Got rekt getting %s atom\n", WM_ATOM_NAME[i]);
    }

}

void initKeys(void)
{
	//get keycode from keysym
	xcb_key_symbols_t 	*keysyms;
	xcb_keycode_t 		*keycode;

    xcb_ungrab_key(dpy, XCB_GRAB_ANY, screen->root, XCB_MOD_MASK_ANY);

	if (!(keysyms = xcb_key_symbols_alloc(dpy)))
		exit(1);

	for(unsigned int i = 0; i < LENGTH(keys); ++i)
	{
        keycode = xcb_key_symbols_get_keycode(keysyms, keys[i].keysym);

        for(unsigned int j = 0; keycode[j] != XCB_NO_SYMBOL; ++j)
        {
			//does not account for other modifiers such as caps or numlock
        	xcb_grab_key(dpy, 1, screen->root, keys[i].modifier, 
        				keycode[j], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }
        free(keycode);

	}
	xcb_key_symbols_free(keysyms); //free allocation
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
	for(int i = 0; i < NUM_DESKTOPS; ++i)
	{
		master->desktops[i].w_head = NULL;
		master->desktops[i].w_tail = NULL;
	}
}

void run(void)
{
	xcb_generic_event_t *event;
	while(running) 
	{
		xcb_flush(dpy);
		if((event = xcb_wait_for_event(dpy))) {
			if(events[event->response_type & ~0x80])
				events[event->response_type & ~0x80](event);
			free(event);
		}
	}
}

void handleKeyPress(xcb_generic_event_t *event)
{
	xcb_key_press_event_t 	*ev 	= (xcb_key_press_event_t *)event;
	xcb_keycode_t 			keycode = ev->detail;

	//get keysym from keycode
	xcb_key_symbols_t 	*keysyms;
	xcb_keysym_t 		 keysym;

	if (!(keysyms = xcb_key_symbols_alloc(dpy))) //allocate array of keysyms
        exit(1);
    keysym = xcb_key_symbols_get_keysym(keysyms, keycode, 0); // get the keysym from the keycode and keysyms
    xcb_key_symbols_free(keysyms);

    // iterate through key array until it finds the matching keysym, then calls function associated
    for(unsigned int i = 0; i < LENGTH(keys); ++i)
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

	//TODO: HANDLING OF TILING
	uint32_t x = 300;
	uint32_t y = 300;
	uint32_t width = 300; // call function to determine width
	uint32_t height = 300; // call function to determine height

	addWindowToDesktop(master->currDesktop, window, x, y, width, height);
	configureWindow(window, x, y, width, height);
	xcb_map_window(dpy, window);

	xcb_flush(dpy);
}	

void cleanup(void)
{
	/* clean up structs we allocated */
	window_t * iter;
	for(unsigned int i = 0; i < NUM_DESKTOPS; ++i)
	{
		iter = master->desktops[i].w_head;
		while(iter != NULL)
		{
			window_t * next = iter->w_next;
			free(iter);
			iter = next;
		}
	}

	// free up master handler
	free(master->desktops);
	free(master);

	/* clean up x */
	xcb_ungrab_key(dpy, XCB_GRAB_ANY, root, XCB_MOD_MASK_ANY);

	xcb_query_tree_reply_t  *query;
    xcb_window_t *c;

    // set up event to send to kill window
    xcb_client_message_event_t ev;
    ev.response_type = XCB_CLIENT_MESSAGE;
    ev.format = 32;
    ev.sequence = 0;
    ev.type = wmatoms[WM_PROTOCOLS];
    ev.data.data32[0] = wmatoms[WM_DELETE_WINDOW];
    ev.data.data32[1] = XCB_CURRENT_TIME;

	query = xcb_query_tree_reply(dpy,xcb_query_tree(dpy,root),0); //query the tree of root
	if(query)
	{
		// get the children from the query and iterate through them
		c = xcb_query_tree_children(query);
        for (unsigned int i = 0; i != query->children_len; ++i) {
        	printf("THERE IS A CHILD %d\n", i);
        	ev.window = c[i]; // set the window in the client message event
        	xcb_send_event(dpy, 0, c[i], XCB_EVENT_MASK_NO_EVENT, (char*)&ev); // send delete window event to the client
        	xcb_flush(dpy);
        }
        free(query);
	}
	xcb_disconnect(dpy);
}

void quit(void)
{
	running = false;
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
	// init new window struct with parameters
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
		// if the list is empty
		if(desktop->w_head == NULL)
		{
			desktop->w_head = newWindow;
			desktop->w_tail = newWindow;
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

/* 
 * Sets the configuration mask and configures the window to the parameters
 */
void configureWindow(xcb_window_t window, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
	uint16_t configWindowMask = XCB_CONFIG_WINDOW_X | 
								XCB_CONFIG_WINDOW_Y | 
								XCB_CONFIG_WINDOW_WIDTH | 
								XCB_CONFIG_WINDOW_HEIGHT;

	const uint32_t configValues[] = { x, y, width, height };
	xcb_configure_window(dpy, 
						 window,
						 configWindowMask,
						 configValues);
}



