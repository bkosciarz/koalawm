#ifndef CONFIG_H
#define CONFIG_H

static const char * terminal[] = {"/bin/sh", "-c", "st", NULL};
static const char * menu[] = {"/bin/sh", "-c", "dmenu_run", NULL};

#define MASTER_KEY XCB_MOD_MASK_4
#define NUM_DESKTOPS 4
#define SHOW_PANEL false

static key keys[] = {
    {MASTER_KEY,        XK_q,        quit,       {NULL}},     
    {MASTER_KEY,        XK_Return,   launch,     {.com = terminal}},
    {MASTER_KEY,        XK_r,   launch,     {.com = menu}}
};

#endif
