#ifndef CONFIG_H
#define CONFIG_H

static char *const terminal[] = {"/bin/sh", "-c", "st", NULL};
//static char *const menu[] = {"/bin/sh", "-c", "dmenu_run", NULL};

static xcb_keysym_t keys[] = { 
	XK_c, // + WINDOWS: quits out of window manager
	XK_Return, // + WINDOWS: starts a terminal
	XK_r // +WINDOWS: starts a menu
};

#endif
