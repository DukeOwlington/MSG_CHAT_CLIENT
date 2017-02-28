#ifndef CHATROOM_INTERFACE_H_INCLUDED
#define CHATROOM_INTERFACE_H_INCLUDED

#include <ncurses.h>
#include "chatroom_interface.h"

WINDOW *msgwin, *msgframe, *usrlist, *usrframe, *dlgwin, *frame;

int DrawBoxWin(WINDOW *win);
int DrawNonBoxWin(WINDOW *win);
int DeleteWindow(WINDOW *win);
int InitializeColors(void);

#endif
