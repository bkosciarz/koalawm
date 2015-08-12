#ifndef CONFIG_H
#define CONFIG_H

//static char *const terminal[] = {"/bin/sh", "-c", "st", NULL};
static const char * terminal[] = {"/bin/sh", "-c", "st", NULL};
//static char *const menu[] = {"/bin/sh", "-c", "dmenu_run", NULL};
#define MASTER_KEY XCB_MOD_MASK_4
/*
static xcb_keysym_t keys[] = { 
	XK_c, // + WINDOWS: quits out of window manager
	XK_Return, // + WINDOWS: starts a terminal
	XK_r // +WINDOWS: starts a menu
};
*/

static key keys[] = {
    {MASTER_KEY,        XK_q,        quit,        {NULL}},     
    {MASTER_KEY,        XK_Return,   launch,      terminal}
};

#endif
