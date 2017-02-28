#include "chatroom_interface.h"

int DrawNonBoxWin(WINDOW *win) {
  werase(dlgwin);
  wrefresh(dlgwin);

  return 0;
}

int DrawBoxWin(WINDOW *win) {
  wclear(win);
  werase(win);
  box(win, 0, 0);
  wrefresh(win);

  return 0;
}

int DeleteWindow(WINDOW *win) {
  wclear(win);
  werase(win);
  delwin(win);
  return 0;
}

int InitializeColors(void) {
  if (has_colors() == FALSE)
    return -1;

  start_color();
  init_pair(1,COLOR_WHITE,COLOR_BLACK);
  init_pair(2,COLOR_GREEN, COLOR_BLACK);
  init_pair(3,COLOR_BLUE, COLOR_BLACK);
  init_pair(4,COLOR_RED, COLOR_BLACK);

  return 0;
}
