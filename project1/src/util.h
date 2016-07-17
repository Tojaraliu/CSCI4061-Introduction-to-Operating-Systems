/********************
 * util.h
 *
 * You may put your utility function definitions here
 * also your structs, if you create any
 *********************/

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// the following ifdef/def pair prevents us from having problems if
// we've included util.h in multiple places... it's a handy trick
#ifndef _UTIL_H_
#define _UTIL_H_

#define INELIGIBLE 0
#define READY 1
#define RUNNING 2
#define FINISHED 3

#define MAX_LENGTH 1024
#define MAX_CHILDREN 10
#define MAX_NODES 10

// This stuff is for easy file reading
FILE * file_open(char*);
char * file_getline(char*, FILE*);
int is_file_exist(char *);
int get_file_modification_time(char *);
int compare_modification_time(char *, char *);

typedef enum exception {
    syntaxError,
    targetError
} exception;

typedef struct dependency{
    char DepTarget[64];
    int DepTargetNum;
    struct dependency *next;
}dep_list;

typedef struct targetSimpleNode{
    int targetNum;
    struct targetSimpleNode *next;
} targetSimpleNode;

typedef struct target{
	pid_t pid;
    int targetNum;
	char szTarget[64];
	char szCommand[64];
	int nStatus;
    int nDep;
	dep_list *szDependencies;
}target_t;

int makeargv(const char *s, const char *delimiters, char ***argvp);
void freemakeargv(char **argv);

#endif
