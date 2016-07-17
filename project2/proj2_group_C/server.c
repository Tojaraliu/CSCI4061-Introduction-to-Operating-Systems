/* CSci4061 S2016 Assignment 2
 * Lecture section: 1
 * Lab section: 002, 007, 006
 * date: 03/11/2016
 * name: Yusen Su, Tianhao Liu, Simin Sun
 * id:   5108043,  5131873,     5089150
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "util.h"

/*
 * Identify the command used at the shell
 */
int parse_command(char *buf)
{
	int cmd;

	if (starts_with(buf, CMD_CHILD_PID))
		cmd = CHILD_PID;
	else if (starts_with(buf, CMD_P2P))
		cmd = P2P;
	else if (starts_with(buf, CMD_LIST_USERS))
		cmd = LIST_USERS;
	else if (starts_with(buf, CMD_ADD_USER))
		cmd = ADD_USER;
	else if (starts_with(buf, CMD_EXIT))
		cmd = EXIT;
	else if (starts_with(buf, CMD_KICK))
		cmd = KICK;
	else
		cmd = BROADCAST;

	return cmd;
}

int MAX(int a, int b) {
	return (a>b?a:b);
}

/*
 * Utility function.
 * Find user index for given user name.
 */
int find_user_index(user_chat_box_t *users, char *name)
{
	int i, user_idx = -1;
	if (name == NULL) {
		fprintf(stderr, "NULL name passed.\n");
		return user_idx;
	}
	for (i = 0; i < MAX_USERS; i++) {
		if (users[i].status == SLOT_EMPTY)
			continue;
		if (strcmp(users[i].name, name) == 0) {
			user_idx = i;
			break;
		}
	}
	return user_idx; // -1 if no such a user exist currently.
}

/*
 * List the existing users on the server shell
 */
int list_users(user_chat_box_t *users, int fd)
{
	/*
	 * Construct a list of user names
	 */
	char names[MAX_USERS * MSG_SIZE] = "";
	int i = 0;
	int j = 0;
	for (i = 0; i < MAX_USERS; ++i) {
		if (users[i].status == SLOT_EMPTY) {
			j++;
			continue;
		}
		strcat(names, users[i].name);
		strcat(names,"\n"); // For format
	}
	if (j == MAX_USERS) {
		// No user currently in the chat room
		write(fd, "<No users>", MSG_SIZE) ;
	}
	else {
		names[strlen(names)-1] = '\0';
		if (write(fd, names, MSG_SIZE) == -1) {
			perror("Cannot write name into Server Shell: ") ;
			exit(1);
		}
	}
	return 0;
}

/*
 * Add a new user
 */
int add_user(user_chat_box_t *users, char *buf, int server_fd)
{
	if (buf == NULL) return -1;
	int i = 0;
	for (i = 0; i < MAX_USERS; ++i) {
		if (users[i].status == SLOT_EMPTY && find_user_index(users, buf) == -1) {
			// First empty slot
			int t1 = pipe(users[i].ptoc);
			int t2 = pipe(users[i].ctop);
			int flags = fcntl(users[i].ctop[0], F_GETFL, 0);
			fcntl(users[i].ctop[0], F_SETFL, flags | O_NONBLOCK); // Set non-block for pipe
			if (t1 == -1 || t2 == -1) {
				// pipe allocate error
				perror("Could not build up pipe between New User Process and Server Shell Process: ");
				exit(-1);
			}
			/* Fork the server shell */
			users[i].pid = fork();
			if (users[i].pid == -1) {
				perror("Forks New User process failure: ");
				exit(-1);
			}
			if (users[i].pid == 0) { //child
				char s1[100] = "";
				char s2[100] = "";
				close(users[i].ptoc[1]);
				close(users[i].ctop[0]);
				sprintf(s1,"%d",users[i].ptoc[0]);
				sprintf(s2,"%d",users[i].ctop[1]); // Unnecessary pipe ends
				execl(XTERM_PATH, XTERM, "+hold", "-e", "./shell", buf, s1, s2, "0", (char *)NULL);
				// "0" is to tell the shell it is a user child
				// this is for \seg

				// If the program can go here, it means some errors occur to execl.
				perror("Execl error. ");
				exit(-1);
			}
			else { //Parent
				close(users[i].ptoc[0]);
				close(users[i].ctop[1]); // Unnecessary pipe ends
				users[i].status = SLOT_FULL;
				strcpy(users[i].name, buf);
			 	char notify[MSG_SIZE] = "";
				sprintf(notify,"Adding user %s...", users[i].name);
				write(server_fd, notify, MSG_SIZE); // Inform the server shell
				break;
			}
		}
		if (find_user_index(users, buf) > -1) {
			// Another user has the same name already
			char msg[MSG_SIZE] = "";
			sprintf(msg, "Could not add user %s: user name conflict...", buf);
		 	write(server_fd, msg, MSG_SIZE);
			break;
		}
		if (i == MAX_USERS - 1) {
			//Check if user limit reached.
			write(server_fd, "Max user limit reached\ncould not add user: Resource temporarily unavailable", MSG_SIZE);
		}
	}
	return 0;
}

/*
 * Broadcast message to all users. Completed for you as a guide to help with other commands :-).
 */
int broadcast_msg(user_chat_box_t *users, char *buf, int fd, char *sender)
{
	int i;
	const char *msg = "Broadcasting...", *s;
	char text[MSG_SIZE] = "";
	memset(text, 0, MSG_SIZE);
	/* Notify on server shell */
	if (write(fd, msg, strlen(msg) + 1) < 0)
		perror("writing to server shell");

	/* Send the message to all user shells */
	s = strtok(buf, "\n");
	sprintf(text, "%s : %s", sender, s);
	for (i = 0; i < MAX_USERS; i++) {
		// Send to every members
		if (users[i].status == SLOT_EMPTY)
			continue;
		if (write(users[i].ptoc[1], text, MSG_SIZE) <= 0)
			perror("write to child shell failed");
	}
	return 0;
}

/*
 * Close all pipes for this user
 */
void close_pipes(int idx, user_chat_box_t *users)
{
	// ctop[1] and ptoc[0] had already been closed
	close(users[idx].ctop[0]);
	close(users[idx].ptoc[1]);
}

/*
 * Cleanup single user: close all pipes, kill user's child process, kill user
 * xterm process, free-up slot.
 * Remember to wait for the appropriate processes here!
 */
void cleanup_user(int idx, user_chat_box_t *users)
{
	int status;
	close_pipes(idx, users);
	strcpy(users[idx].name,"");
	kill(users[idx].pid, SIGKILL);
	kill(users[idx].child_pid, SIGKILL);
	users[idx].status = SLOT_EMPTY;
	waitpid(users[idx].pid, &status,0);
	waitpid(users[idx].child_pid, &status,0);
}

/*
 * Cleanup all users: given to you
 */
void cleanup_users(user_chat_box_t *users)
{
	int i;
	for (i = 0; i < MAX_USERS; i++) {
		if (users[i].status == SLOT_EMPTY)
			continue;
		cleanup_user(i, users);
	}
}

/*
 * Cleanup server process: close all pipes, kill the parent process and its
 * children.
 * Remember to wait for the appropriate processes here!
 */
void cleanup_server(server_ctrl_t server_ctrl)
{
	int status;
	close(server_ctrl.ctop[0]);
	close(server_ctrl.ctop[1]);
	close(server_ctrl.ptoc[0]);
	close(server_ctrl.ptoc[1]);
	kill(server_ctrl.pid, SIGKILL);
	kill(server_ctrl.child_pid, SIGKILL);
	waitpid(server_ctrl.pid, &status,0);
	waitpid(server_ctrl.child_pid, &status,0);
}

/*
 * Utility function.
 * Given a command's input buffer, extract name.
 */
char *extract_name(int cmd, char *buf)
{
	char *s = NULL;
	s = strtok(buf, " ");
	s = strtok(NULL, " ");
	if (cmd == P2P)
		return s;	/* s points to the name as no newline after name in P2P */
	s = strtok(s, "\n");	/* other commands have newline after name, so remove it*/
	return s;
}

/*
 * Send p2p message. Print error on the user shell if user not found.
 */
void send_p2p_msg(int idx, user_chat_box_t *users, char *buf)
{
	if (buf == NULL) return;
	char msg[MSG_SIZE+10] = "";
	char text[MSG_SIZE+10] = "";
	strcpy(msg,buf);
	char name[MSG_SIZE+10] = "";
	char *pch = NULL;
	if ((pch = extract_name(P2P, buf)) != NULL) {
		strcpy(name, pch);
		strcpy(msg,msg+4+1+strlen(name)+1);
		msg[strlen(msg)-1] = '\0';
		int targetid = find_user_index(users, name);
		if (targetid != -1) {
			strcpy(text, users[idx].name);
			strcat(text, " : ");
			strcat(text, msg);
			// Generate the formatted message.
			write(users[targetid].ptoc[1], text, MSG_SIZE);
		}
		else {
			// Can't find specific user
			sprintf(text, "Can not find user %s: send message failed...", name);
			write(users[idx].ptoc[1], text, MSG_SIZE);
		}
	}
}

int main(int argc, char **argv)
{
	/* Initialization */
	server_ctrl_t sc;
	user_chat_box_t *users = (user_chat_box_t *)malloc(MAX_USERS * sizeof(user_chat_box_t)) ;
	int i = 0;
	for (; i < MAX_USERS; ++i) {
		users[i].status = SLOT_EMPTY;
	}
	if (pipe(sc.ptoc) == -1 || pipe(sc.ctop) == -1) {
		perror("Could not pipe between Server Process and Server Shell Process: ") ;
	}

	/* Fork the server shell */
	int child_pid = fork();
	if (child_pid == -1) {
		perror("Forks Server Shell process failure: ") ;
	}
	if (child_pid == 0) {
		/*
		 * Inside the child.
		 * Start server's shell.
		 * exec the SHELL program with the required program arguments.
		 */
		char s1[100] = "";
		char s2[100] = "";
		close(sc.ptoc[1]);
		close(sc.ctop[0]); // Unnecessary pipe ends
		sprintf(s1,"%d",sc.ptoc[0]);
		sprintf(s2,"%d",sc.ctop[1]);
		execl("./shell", "shell", "Server", s1, s2, "1", (char *)NULL);
		// "1" is to tell the shell it is server
		// this is for protecting server shell from seg-kill itself when
		// user types \seg to server shell
		
		// If the program can go here, it means some errors occur to execl.
		perror("Execl error. ");
		exit(-1);
	}
	else {
		/* Start a loop which runs every 1000 usecs.
	 	 * The loop should read messages from the server shell, parse them using the
	 	 * parse_command() function and take the appropriate actions. */
		close(sc.ptoc[0]);
		close(sc.ctop[1]); // Unnecessary pipe ends
		sc.pid = child_pid;
		int validLoop = 1;
		char buf[MSG_SIZE+10] = "";
		int flags = fcntl(sc.ctop[0], F_GETFL, 0);
		fcntl(sc.ctop[0], F_SETFL, flags | O_NONBLOCK);
		while (validLoop) {
			/* Let the CPU breathe */
			usleep(1000);
			strcpy(buf,"");
			int status;
			if (waitpid(sc.pid, &status, WNOHANG) == -1) {
				cleanup_users(users);
				cleanup_server(sc);
				return 0;
			}
			//Read the message from server shell
			int readresult = read(sc.ctop[0], buf, MSG_SIZE);
			if (readresult > 0) {
				//Parse the command
				switch(parse_command(buf)) {
					//Begin switch statement to identify command and take appropriate action
					case CHILD_PID:
					{
						// Store the child pid.
						sc.child_pid = atoi(extract_name(CHILD_PID, buf));
						break;
					}
					case LIST_USERS:
					{
						list_users(users, sc.ptoc[1]);
						break;
					}
					case ADD_USER:
					{
						if (add_user(users, extract_name(ADD_USER, buf), sc.ptoc[1]) == -1) {
							write(sc.ptoc[1], "Can't add user with no name", MSG_SIZE);
						}
						break;
					}
					case KICK:
					{
						char name[MSG_SIZE] = "";
						char *pch = NULL;
						if ((pch = extract_name(KICK, buf)) != NULL) {
							strcpy(name,pch);
							int id = find_user_index(users,name);
							if (id != -1)
								cleanup_user(id, users);
							else {
								char msg[MSG_SIZE] = "";
								sprintf(msg, "Can not kick user %s: name not found...", name);
								write(sc.ptoc[1], msg, MSG_SIZE);
							}
						}
						else {
							write(sc.ptoc[1], "Can't kick user with no name", MSG_SIZE);
						}
						break;
					}
					case EXIT:
					{
						// kill all the users processes and kill server itself
						cleanup_users(users);
						cleanup_server(sc);
						return 0;
						break;
					}
					case BROADCAST:
					{
						broadcast_msg(users, buf, sc.ptoc[1], "Server");
						break;
					}
				}
			}

			/* Check all users' input and their status */
			int i = 0;
			for (i = 0; i < MAX_USERS; ++i) {
				int status;
				// check if the user did quit
				if (users[i].status == SLOT_FULL && waitpid(users[i].pid, &status, WNOHANG) == -1) {
					char msg[MSG_SIZE] = "";
					sprintf(msg, "User %s did quit...", users[i].name);
					write(sc.ptoc[1], msg, MSG_SIZE);
					cleanup_user(i,users);
					continue;
				}
				strcpy(buf,"");
				//Read messages from the user shells
				if (users[i].status == SLOT_EMPTY || read(users[i].ctop[0], buf, MSG_SIZE) <= 0) {
					continue;
				}
				//Parse the command
				switch(parse_command(buf)) {
					//Begin switch statement to identify command and take appropriate action
					case CHILD_PID:
					{
						// Store the child pid.
						users[i].child_pid = atoi(extract_name(CHILD_PID, buf));
						break;
					}
					case LIST_USERS:
					{
						list_users(users, users[i].ptoc[1]);
						break;
					}
					case P2P:
					{
						send_p2p_msg(i, users, buf);
						break;
					}
					case EXIT:
					{
						char msg[MSG_SIZE] = "";
						sprintf(msg, "User %s did exit...", users[i].name) ;
						write(sc.ptoc[1], msg, MSG_SIZE);
						cleanup_user(i,users);
						break;
					}
					case BROADCAST:
					{
						broadcast_msg(users, buf, sc.ptoc[1], users[i].name);
						break;
					}
				}
			}
		 }	/* while loop ends when server shell sees the \exit command */
	}
	return 0;
}
