#include "chatroom_utils.h"
#include "chatroom_interface.h"

/* global variables to work with threads */
int msqid_in, msqid_out, anon_msqid, stat_msqid;
bool dead_server = false;
int dlgy, dlgx;
pthread_mutex_t buffer_lock;
key_t key_in;
key_t key_out = 5;
key_t stat_key = 9;
key_t anon_key = 10;
MessageBuf message_buf;
Clients client;

/* create message queues*/
int InitializeClient(void) {
  int msgflg = IPC_CREAT | 0666;
  if ((msqid_out = msgget(key_out, msgflg)) < 0) {
    perror("msgget");
    return 1;
  }
  if ((stat_msqid = msgget(stat_key, msgflg)) < 0) {
    perror("msgget");
    return -1;
  }
  if ((anon_msqid = msgget(anon_key, msgflg)) < 0) {
    perror("msgget");
    return -1;
  }

  return 0;
}

/* trim newline character */
void TrimNewlineChar(char *text) {
  int len = strlen(text) - 1;
  if (text[len] == '\n') {
    text[len] = '\0';
  }
}

/* input user name */
int SetUserName(void) {
  char usrmsg[20];
  wprintw(dlgwin, "Set your username: ");
  wrefresh(dlgwin);
  wgetnstr(dlgwin, usrmsg, 20);
  strcpy(message_buf.mtext, usrmsg);
  strcpy(client.username, usrmsg);
  memset(usrmsg, 0, 20);
  return 0;
}

/* check whether the server is alive every 5 seconds */
void *CheckServerStatus(void *args) {
  int status;
  while (!dead_server) {
    sleep(5);
    pthread_mutex_lock(&buffer_lock);
    message_buf.mtype = ALIVE;
    status = msgrcv(stat_msqid, &message_buf, MSGSZ, ALIVE, IPC_NOWAIT);
    if (status < 0) {
      dead_server = true;
    }
    pthread_mutex_unlock(&buffer_lock);
  }
  wprintw(dlgwin, "Cannot reach the server\n");
  wrefresh(dlgwin);
  pthread_exit(NULL);
}

/* send message that client is alive every 5 seconds */
void *SendClientStatus(void *args) {
  size_t buf_len;
  while (!dead_server) {
    pthread_mutex_lock(&buffer_lock);
    message_buf.mtype = ALIVE;
    buf_len = strlen(message_buf.mtext) + 1;
    msgsnd(msqid_in, &message_buf, buf_len, 0);
    pthread_mutex_unlock(&buffer_lock);
    sleep(5);
  }
  pthread_exit(NULL);
}

/* get user list */
int GetUserList(void) {
  int i = 0;
  int line = 1;
  DrawBoxWin(usrlist);
  wmove(usrlist, line, 1);
  while (message_buf.mtext[i] != '\0') {
    waddch(usrlist, message_buf.mtext[i]);
    i++;
    if (message_buf.mtext[i] == '\n') {
      i++;
      line++;
      wmove(usrlist, line, 1);
    }
  }
  wmove(msgwin, 1, 1);
  wrefresh(usrlist);
  wrefresh(msgwin);
  return 0;
}

/* register user */
int ConnectServer() {
  int msgflg = IPC_CREAT | 0666;
  size_t buf_len;
  while (message_buf.mtype != SUCCESS) {
    SetUserName();
    message_buf.mtype = CONNECT;
    buf_len = strlen(message_buf.mtext) + 1;
    wprintw(dlgwin, "Sending username to server...\n");
    wrefresh(dlgwin);
    msgsnd(msqid_out, &message_buf, buf_len, 0);
    wprintw(dlgwin, "Waiting for server response...\n");
    wrefresh(dlgwin);
    msgrcv(anon_msqid, &message_buf, MSGSZ, 0, 0);
    switch (message_buf.mtype) {
      case USERNAME_ERROR:
        wprintw(dlgwin, "This username has been taken, choose another one\n");
        wrefresh(dlgwin);
        break;
      case TOO_FULL:
        wprintw(dlgwin, "Chatroom is too full");
        wrefresh(dlgwin);
        return -1;
      case SUCCESS:
        wprintw(dlgwin, "Success!\n");
        wrefresh(dlgwin);
        key_in = strtol(message_buf.mtext, NULL, 10);
        if ((msqid_in = msgget(key_in, msgflg)) < 0) {
          perror("msgget");
          return 1;
        }
        break;
      default:
        break;
    }
  }
  getyx(dlgwin, dlgy, dlgx);
  return 0;
}

/* handle incoming messages from server */
void *HandleIncomingMsg(void *args) {
  int status;
  wmove(dlgwin, dlgy, dlgx);
  wprintw(dlgwin, "Press /h for help\n");
  wrefresh(dlgwin);
  while (!dead_server) {
    pthread_mutex_lock(&buffer_lock);
    status = msgrcv(msqid_in, &message_buf, MSGSZ, ALIVE, IPC_NOWAIT | MSG_EXCEPT);
    if (status < 0) {
      pthread_mutex_unlock(&buffer_lock);
      continue;
    }
    switch (message_buf.mtype) {
      case PUBLIC_MESSAGE:
        wattron(dlgwin, COLOR_PAIR(2));
        wprintw(dlgwin, "%s\n", message_buf.mtext);
        wattroff(dlgwin, COLOR_PAIR(2));
        wrefresh(dlgwin);
        wrefresh(msgwin);
        break;
      case PRIVATE_MESSAGE:
        wattron(dlgwin, COLOR_PAIR(4));
        wprintw(dlgwin, "%s\n", message_buf.mtext);
        wattroff(dlgwin, COLOR_PAIR(4));
        wrefresh(dlgwin);
        wrefresh(msgwin);
        break;
      case GET_USERS:
        GetUserList();
        break;
      case DISCONNECT:
        dead_server = true;
        break;
      default:
        break;
    }
    pthread_mutex_unlock(&buffer_lock);
  }
  pthread_exit(NULL);
}

/* check  whether the message is private */
bool IsPrivateMsg(int msg_len, char *usrmsg) {
  int i;
  bool is_private = false;

  if (usrmsg[0] != '/')
    return false;
  for (i = 1; i < msg_len; i++) {
    if (usrmsg[i] == '/') {
      is_private = true;
      break;
    }
  }
  return is_private;
}

/* check whether the given string is disconnect option */
bool IsHelpMsg(char *usrmsg) {
  if (strcmp(usrmsg, "/h") == 0) {
    return true;
  } else {
    return false;
  }
}

/* check whether the given string is disconnect option */
bool IsDisconnectMsg(char *usrmsg) {
  if (strcmp(usrmsg, "/q") == 0) {
    return true;
  } else {
    return false;
  }
}

void DisplayHelp(void) {
  waddstr(dlgwin, "/q - quit\n");
  waddstr(dlgwin, "/username/ - private message\n");
  waddstr(dlgwin, "/h - help\n");
  wrefresh(dlgwin);
  wrefresh(msgwin);
}

/* user input thread */
void *HandleUserInput(void *args) {
  int msg_len;
  size_t buf_len;
  char usrmsg[128];
  while (!dead_server) {
    wmove(msgwin, 1, 1);
    wgetnstr(msgwin, usrmsg, 124);
    TrimNewlineChar(usrmsg);
    msg_len = strlen(usrmsg);
    pthread_mutex_lock(&buffer_lock);
    if (IsPrivateMsg(msg_len, usrmsg)) {
      message_buf.mtype = PRIVATE_MESSAGE;
    } else if (IsHelpMsg(usrmsg)) {
      DisplayHelp();
      pthread_mutex_unlock(&buffer_lock);
      memset(usrmsg, 0, 128);
      DrawBoxWin(msgwin);
      continue;
    } else if (IsDisconnectMsg(usrmsg)) {
      message_buf.mtype = DISCONNECT;
      dead_server = true;
    } else {
        message_buf.mtype = PUBLIC_MESSAGE;
    }
    sprintf(message_buf.mtext, "%d", key_in);
    strncpy(message_buf.mtext + 2, usrmsg, strlen(usrmsg) + 1);
    buf_len = strlen(message_buf.mtext) + 1;
    memset(usrmsg, 0, 128);
    msgsnd(msqid_out, &message_buf, buf_len, 0);
    pthread_mutex_unlock(&buffer_lock);
    DrawBoxWin(msgwin);
  }
  pthread_exit(NULL);
}

int main(void) {
  int i;
  pthread_t chat_func[3];
  pthread_attr_t attr;

  initscr();
  InitializeColors();

  /* draw frame */
  frame = newwin(LINES, COLS, 0, 0);
  DrawBoxWin(frame);

  usrlist = newwin(LINES - 4, 25, 0, COLS - 25);
  dlgwin = newwin(LINES - 5, COLS - 26, 1, 1);
  msgwin = newwin(LINES - (LINES - 4), COLS, LINES - 4, 0);
  scrollok(dlgwin, TRUE);

  DrawBoxWin(frame);
  DrawBoxWin(usrlist);
  DrawBoxWin(msgwin);
  wrefresh(dlgwin);

  if (InitializeClient() < 0) {
    return EXIT_FAILURE;
  }

  if (ConnectServer() < 0) {
    return EXIT_FAILURE;
  }

  GetUserList();

  if (pthread_mutex_init(&buffer_lock, NULL) < 0) {
    perror("mutex failed: ");
    return EXIT_FAILURE;
  }

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  /*0: user input thread, 1: server messages thread
   *2: checking whether server is on thread, 3: client alive status thread */
  pthread_create(&chat_func[0], NULL, HandleUserInput, NULL);
  pthread_create(&chat_func[1], NULL, HandleIncomingMsg, NULL);
  pthread_create(&chat_func[2], NULL, CheckServerStatus, NULL);
  pthread_create(&chat_func[3], NULL, SendClientStatus, NULL);

  pthread_attr_destroy(&attr);

  for (i = 0; i < 4; i++) {
    pthread_join(chat_func[i], NULL);
  }

  pthread_mutex_destroy(&buffer_lock);

  DeleteWindow(frame);
  DeleteWindow(usrlist);
  DeleteWindow(dlgwin);
  DeleteWindow(msgwin);

  return EXIT_SUCCESS;
}
