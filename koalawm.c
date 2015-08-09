#include <stdio.h>
#include <stdlib.h>

#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/keysym.h>

#define LENGTH(x) (sizeof(x)/sizeof(*x)) //only works on staticly allocated memory
#define True 1
#define False 0
#define bool short

/*variables*/
static bool running = True;
static xcb_keysym_t keys[] = { XK_c };
static xcb_connection_t *dpy;
static xcb_screen_t *screen;

/*functions*/
void initKeys(void);
void handleKeyPress(xcb_keysym_t keysym);
int init(void);
void run(void);
void quit(void);

int main(void)
{
    init();
	run();
	return 0;
}

void initKeys(void)
{
	int i, j;
	for(i = 0; i < LENGTH(keys); ++i)
	{
		//get keycodes
		xcb_key_symbols_t 	*keysyms;
		xcb_keycode_t 		*keycode;

		if (!(keysyms = xcb_key_symbols_alloc(dpy)))
        	exit(1);

        keycode = xcb_key_symbols_get_keycode(keysyms, keys[i]);
        xcb_key_symbols_free(keysyms);

        for(j = 0; keycode[j] != XCB_NO_SYMBOL; ++j)
        {
        	xcb_grab_key(dpy, 1, screen->root, XCB_MOD_MASK_4, 
        				keycode[j], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        free(keycode);
	}
}

// void handleKeyPress(xcb_keysym_t keysym)
// {
// 	switch(keysym) {
// 		case XK_c:
// 		{
// 		    quit();
// 			break;
// 		}
// 	}
// }


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
//	root = screen->root;

	initKeys();
	return 0; 
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
				// handleKeyPress(xcb_get_keysym(event->detail));
				break;
			}
		    default:
		        break;	   
		}
	}
}

void quit(void)
{
	running = False;
}
