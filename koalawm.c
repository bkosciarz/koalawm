#include <stdio.h>
#include <xcb/xcb.h>

#define True 1
#define False 0
#define bool short

static bool running = True;
static xcb_keysym_t keys[] = { XK_c };

void initKeys(void);
void handleKeyPress(xcb_keysym_t keysym);

int main() {
	xcb_connection_t *dpy;
	xcb_screen_t *screen;
//	xcb_drawable_t win;
//	xcb_drawable_t root;
    xcb_window_t win
    /* Open a connection to the X server that uses the value 
	 * of the DISPLAY ev and sets the screen to 0
	 */
	dpy = xcb_connect(NULL, NULL);
    if(xcb_connection_has_error(dpy))
		return 1; //there was an error with the connection

	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
//	root = screen->root;

	initKeys();

	xcb_flush(dpy);

	xcb_generic_event_t *event;
	while(running) {
		event = xcb_wait_for_event(dpy);
        switch(event->response_type & ~0x80) {
			case XCB_KEY_PRESS:		
			{
				handleKeyPress(xcb_get_keysym(event->detail));
				break;
			}
		    default:
		        break;	   
		}
	}

	return 0;
}

void initKeys(void)
{
	for(i = 0; i < LENGTH(keys); ++i)
	{
		//get keycodes
		xcb_key_symbols_t 	*keysms;
		xcb_keycode_t 		*keycode;

		if (!(keysyms = xcb_key_symbols_alloc(dis)))
        	exit(1);

        keycode = xcb_key_symbols_get_keycode(keysyms, keys[i]);
        xcb_key_symbols_free(keysyms);

        for(j = 0; keycode[j] != XCB_NO_SYMBOL; ++j)
        {
        	xcb_grab_key(dpy, 1, root, XCB_MOD_MASK_4, 
        				keycode[j], XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        }

        free(keycode);
	}
}

void handleKeyPress(xcb_keysym_t keysym)
{
	switch(keysym) {
		case XK_c:
		{
			running = False;
			break;
		}
	}
}



