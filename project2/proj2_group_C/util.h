#ifndef UTIL_H
#define UTIL_H

#include <unistd.h>

#define FILENAME __FILE__
#define FUNCNAME __func__

#define MSG_SIZE 1024
#define CURR_DIR getenv("PWD")
#define SHELL_PROG "shell"
#define EXIT_CMD "\\exit"
#define MAX_USERS 10
#define XTERM_PATH "/usr/bin/xterm"
#define XTERM "xterm"
#define FILE_NAME 64

/*
 * possible server commands
 */
#define CMD_CHILD_PID "\\child_pid"
#define CMD_LIST_USERS "\\list"
#define CMD_ADD_USER "\\add"
#define CMD_EXIT "\\exit"
#define CMD_P2P "\\p2p"
#define CMD_KICK "\\kick"
#define CMD_SEG  "\\seg"

#define SH_DELIMS " \n\t\r"

typedef enum server_cmd_type {
	CHILD_PID,
	LIST_USERS,
	ADD_USER,
	EXIT,
	BROADCAST,
	P2P,
	KICK,
	SEG
} server_cmd_type;

/*
 * user slot status
 */
typedef enum slot_status {
	SLOT_FULL,
	SLOT_EMPTY
} slot_stat;

/*
 * Structure for the server
 */
typedef struct server_controller_s {
	int ptoc[2];
	int ctop[2];
	pid_t pid;
	pid_t child_pid;
} server_ctrl_t;

/*
 * Structure for user chat boxes
 */
typedef struct user_chat_box_s {
	int ptoc[2];
	int ctop[2];
	char name[MSG_SIZE];
	pid_t pid;
	pid_t child_pid;
	int status;
} user_chat_box_t;

void print_prompt(char *name);
int starts_with(const char *a, const char *b);

#endif
