#include <stdio.h>
#include <xcb/xcb.h>

#define True 1
#define False 0
#define bool short

static bool running = True;

int main() {
	xcb_connection_t *dpy;
	xcb_screen_t *screen;
//	xcb_drawable_t win;
//	xcb_drawable_t root;
    xcb_window_t win;

    /* Open a connection to the X server that uses the value 
	 * of the DISPLAY ev and sets the screen to 0
	 */
	dpy = xcb_connect(NULL, NULL);
    if(xcb_connection_has_error(dpy))
		return 1; //there was an error with the connection

	screen = xcb_setup_roots_iterator(xcb_get_setup(dpy)).data;
//	root = screen->root;

    win = xcb_generate_id(dpy);
	uint32_t mask =  XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[2];
	values[0] = screen->white_pixel;
	values[1] = XCB_EVENT_MASK_EXPOSURE;
	xcb_create_window(dpy,
			          XCB_COPY_FROM_PARENT,
					  win,
					  screen->root,
					  0, 0,
					  150, 150,
					  10,
					  XCB_WINDOW_CLASS_INPUT_OUTPUT,
					  screen->root_visual,
					  mask, values);
	
	xcb_map_window(dpy, win);
	xcb_flush(dpy);

	xcb_generic_event_t *event;
	while(running) {
		event = xcb_wait_for_event(dpy)
        switch(event->response_type & ~0x80) {
			case XCB_KEY_PRESS:		    
		}
	}

	return 0;
}


