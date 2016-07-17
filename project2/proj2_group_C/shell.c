/* CSci4061 S2016 Assignment 2
 * Lecture section: 1
 * Lab section: 002, 007, 006
 * date: 03/11/2016
 * name: Yusen Su, Tianhao Liu, Simin Sun
 * id:   5108043,  5131873,     5089150
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include "util.h"

/*
 * Check if the line is empty (no data; just spaces or return)
 */

int is_server = 0;

int is_empty(char *line)
{
	while (*line != '\0') {
		if (!isspace(*line))
			return 0;
		line++;
	}
	return 1;
}

/*
 * Implement a segfault when user imput \seg
 */
void suicide(void) {
	// Most useful function ever
	char *c = NULL;
	*c = '1';
	return;
}

/*
 * Implement readline function to handle read by a line
 */
int readline(int fd, char *buf, int nbytes) {
	int numread = 0;
	int returnval;
	while (numread < nbytes - 1) {
		returnval = read(fd, buf + numread, 1);
		if ((returnval == -1) && (errno == EINTR))
      		continue;
    	if ( (returnval == 0) && (numread == 0) )
      		return 0;
    	if (returnval == 0)
      		break;
    	if (returnval == -1)
      		return -1;
    	numread++;
    	if (buf[numread-1] == '\n' || buf[numread-1] == '\r') {
      		buf[numread] = '\0';
      		return numread;
		}
	}
	errno = EINVAL;
	return -1;
}

/*
 * Do stuff with the shell input here.
 */
int sh_handle_input(char *line, int fd_toserver)
{
 	/* Check for \seg command and create segfault */
	if (starts_with(line,CMD_SEG) && is_server == 0) {
		suicide();
		exit(-1);
	}
	/* Write message to server for processing */
	else {
		write(fd_toserver,line,MSG_SIZE);
	}
	return 0;
}

void sh_start(char *name, int fd_toserver)
{
	char msg[MSG_SIZE] = "";
	print_prompt(name);
	while (readline(0, msg, MSG_SIZE) >= 0) {
		/*Check empty line*/
		if (is_empty(msg)){
			print_prompt(name);
			continue;
		}
		sh_handle_input(msg ,fd_toserver);
		memset(msg, 0, MSG_SIZE);
		print_prompt(name);
	}
}

int main(int argc, char **argv)
{
	char name[MSG_SIZE] = "";
	char msg[MSG_SIZE+100] = "", msgtmp[MSG_SIZE + 100] = "";
	strcpy(name, argv[1]);
	/* Extract pipe descriptors and name from argv */
	int readEnd = atoi(argv[2]), writeEnd = atoi(argv[3]);
	is_server = atoi(argv[4]);
	//printf("Name: %s, readend: %d, writeEnd: %d, is_server: %d\n", name,readEnd,writeEnd,is_server);
	/* Fork a child to read from the pipe continuously */
	pid_t pid = fork();
	if (pid > 0) {
		//Send the child's pid to the server for later cleanup
		strcpy(msg,CMD_CHILD_PID);
		sprintf(msgtmp, " %d", pid);
		strcat(msg,msgtmp);
		write(writeEnd,msg,MSG_SIZE);
		//Start the main shell loop
		sh_start(name,writeEnd);
	}
	else {
		// Set non-block for reading
		int flags = fcntl(readEnd, F_GETFL, 0);
		fcntl(readEnd, F_SETFL, flags | O_NONBLOCK);
		// Look for new data from server every 1000 usecs and print it
		while (1) {
			usleep(1000);
			int read_size = 0;
			int checked = 0;
			while((read_size = read(readEnd,msg,MSG_SIZE)) > 0) {
				fprintf(stdout, "\n%s\n", msg);
				fflush(stdout);
				checked = 1;
			}
			if (checked) {
				checked = 0;
				print_prompt(name);
			}
		}
	}
	return 0;
}
