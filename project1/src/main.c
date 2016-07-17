#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util.h"

typedef target_t* target_array_t;
target_array_t targets;
int numOfTargets;
int highestLevel;
int force_recompile;
int not_run_sign;
int reversedlevel[MAX_NODES + 1]; //reversedlevel[i] means the level of the ith target.
						  //Level for final target is 0. Increase from low to high.
targetSimpleNode * compileOrder[MAX_NODES + 1];

//findTargetNum is for finding the order number of each dependency
int findTargetNum(char *s,target_array_t targets, int index) {
    int i;
    for (i = 1; i<=index; ++i) {
        if (strcmp(s,targets[i].szTarget) == 0)
            return i;
    }
	// All targets in the makefile are numbered from 1 to n.
	// Here we return 0 for "No.0 target"
	// No.0 target is designed for the file that does not show up in the target list but needed by some target.
	// For these dependencies or files we can do nothing but try to find them since we don't know how to make one.
	// .c file is an example for this.
    return 0;
}

//This function will parse makefile input from user or default makeFile.
int parse(char * lpszFileName)
{
	int index;
	targets = (target_array_t)malloc(sizeof(target_t)*MAX_NODES) ;
	int i;
	int nLine=0;
	char szLine[MAX_LENGTH+1];
	char *lpszLine;
	char *token;
	int nDep = 0;
	FILE *fp = file_open(lpszFileName);

	if(fp == NULL)
	{
		return -1;
	}
	index = 0;
	while(file_getline(szLine, fp) != NULL)
	{
		nLine++;
		// this loop will go through the given file, one line at a time
		// each line of the file to be able to deal with it later

		//Remove newline character at end if there is one
		lpszLine = strtok(szLine, "\n");

		if (lpszLine == NULL) {
			continue;
		}
		//continue;
		//Skip if blank or comment.
		if (strstr(lpszLine, "#") != NULL) {
			char *pch = strstr(lpszLine, "#");
			lpszLine[(int)(pch-lpszLine)] = '\0';
			if (strcmp(lpszLine,"") == 0) continue; //comment only
		}
		if (strcmp(lpszLine,"\n") == 0 || strcmp(lpszLine,"\r") == 0 || (strstr(lpszLine, "#") != NULL)) {
			continue;
		}
		int numofSpaces = 0;
		while (lpszLine[0] != '\0' &&
			   lpszLine[0] == ' ' &&
			   lpszLine[0] != '\n' &&
			   lpszLine[0] != '\r') {
			numofSpaces++;
			lpszLine = (char *)(lpszLine + sizeof(char));
		}

		if (numofSpaces != 0) {
			//when the line contains comments and whitespace only
			if (lpszLine[0] == '\0' || lpszLine[0] == '\n' || lpszLine[0] == '\r') {
				continue;
			}
			else {
				//when there are some extra whitespace for commmand or target line,
				//it should be considered as Syntax error
				printf("ERROR: Syntax Error at line %d\n",nLine);
				exit(-1);
			}
		}

		//If lpszLine starts with '\t' it will be command else it will be target.

		// a) If line containing command does not start with a tab, you should report it.

		// b) If line containing target starts with white space or tab, you should report it.

		// c) If line starts with '#', you should ignore it ( as this is comment). In this regard, I would like to point out one test case. Assume, a line in Make File starts with white space, followed by #. In this case, you need to parse and remove white spaces to see if next character after white space is # or not. If you encounter #, ignore it, else throw an error which is similar to what you get in Linux make utility.

		// d) If line contains only white spaces or if line is empty, you must ignore it.

		if (strstr(lpszLine, "\t")) {
			//This is command
			//printf("\"%s\"\n", lpszLine);
			if (strtok(lpszLine, "\t") != NULL)
				//store the command for ith target
				strcpy(targets[index].szCommand,strtok(lpszLine, "\t"));
			else
				strcpy(targets[index].szCommand,"");
		}
		else {
			//This is target
			index++;
			strcpy(targets[index].szTarget,strtok(lpszLine, ": "));
			targets[index].targetNum = index;
			targets[index].szDependencies = NULL;
			targets[index].nStatus = 0;
			//printf("\"Target #%d: %s\" -> ", targets[index].targetNum, targets[index].szTarget);
			nDep = 0;
			dep_list* list = targets[index].szDependencies;
			while ((token = strtok(NULL, " ")) != NULL) {
				list = (dep_list*)malloc(sizeof(dep_list));
				strcpy(list->DepTarget,token);
				list->next = targets[index].szDependencies;
				targets[index].szDependencies = list;
				//printf("\"%s\" ", list->DepTarget);
				list = list->next;
				++nDep;
			}
			targets[index].nDep = nDep;
			//printf("\n");
		}
		//Find the number of each dependency

		//It is possbile that target may not have a command as you can see from the example on project write-up. (target:all)
	}
	numOfTargets = index;
	//Order all the targets for convenience
	for (i = 1;i<=numOfTargets;++i) {
		dep_list *p = targets[i].szDependencies;
		while (p != NULL) {
			p->DepTargetNum = findTargetNum(p->DepTarget,targets,numOfTargets);
			p = p->next;
		}
	}
	// for (i = 1;i<=numOfTargets;++i) {
	// 	printf("Target #%d (%d dep)->",targets[i].targetNum,targets[i].nDep);
	// 	dep_list *p = targets[i].szDependencies;
	// 	while (p != NULL) {
	// 		printf(" %d", p->DepTargetNum);
	// 		p = p->next;
	// 	}
	// 	printf("\n");
	// }
	//Close the makefile.
	fclose(fp);
	return 0;
}

//dfs is for find the level order of all targets that relatives to our final target by using depth-first search.
//Also we recursively check the timestamp for all No.0 files and our targets if target is already exist.
//A target does not need to be compiled again as long as all it's up-to-date for all of its dependencies.
int dfs(int TargetNum,int currentLevel,int usedT) {
	if (usedT & (1<<TargetNum)) {
		printf("ERROR: Dependency cycle detected!\n");
		exit(-1);
	}
	int needRecompile = 0;
	dep_list *p = targets[TargetNum].szDependencies;
	while (p!=NULL) {
		//check all of its targets.
		if (p->DepTargetNum == 0) {
			//found a No.0 dependency
			if (is_file_exist(p->DepTarget) == -1) {
				//for No.0 file if it is not exist we can't help anyway
				//printf("Searching: %s\n", targets[p->DepTargetNum].szTarget);
				printf("make4061: *** No rule to make target %s, needed by %s.  Stop.\n", p->DepTarget, targets[TargetNum].szTarget);
				exit(-1);
			}
			//if it is not up-to-date we should re-compile it.
			int cmt = compare_modification_time(p->DepTarget,
										  	targets[TargetNum].szTarget);
			if (cmt == 1 || cmt == -1) needRecompile += 1;
		} else
			//recursively find other relative targets.
			needRecompile += dfs(p->DepTargetNum, currentLevel + 1, usedT | (1<<TargetNum));
		p = p->next;
	}
	if (needRecompile == 0 && targets[TargetNum].nDep != 0 && force_recompile == 0)
		// if it is up-to-date and there is no force_recompile command we can get rid of it from our tree.
		reversedlevel[TargetNum] = -1;
	else
		reversedlevel[TargetNum] = currentLevel;
	highestLevel = (highestLevel < currentLevel? currentLevel : highestLevel);
	return needRecompile;
}

//dispatchCommand is for analyze the command for executing.
char **dispatchCommand (char * command) {
	char **argv = (char **) malloc (sizeof(char *)*(MAX_NODES + 2)) ;
	int i;
	for (i = 0; i < MAX_NODES; ++i) {
		argv[i] = (char *) malloc (sizeof(char)*64) ;
	}
	char * token;
	i = 0;
	token = strtok(command, " ");
	while (token != NULL) {
		argv[i] = token;
		token = strtok(NULL, " ");
		++i;
	}
	argv[i] = NULL;
	return argv;
}

int getNumOfCompilerOrder (int level) {
	int count = 0 ;
	targetSimpleNode *current = compileOrder[level];
	while (current != NULL) {
		count ++;
		current = current->next;
	}
	return count;
}

void execute() {
	int i;
	int nothingToBeDone = 0;
	//printf("highestLevel is: %d\n", highestLevel) ;
	for (i = 0; i <= highestLevel; ++i) {
		int j;
		int numOfCurrentNodes = getNumOfCompilerOrder(i);
		//printf("current level is %d, numOfCurrentNodes is: %d\n", i, numOfCurrentNodes) ;
		int pid[numOfCurrentNodes];
		int status[numOfCurrentNodes];
		targetSimpleNode* getTarget = compileOrder[i] ;
		for (j = 0; j < numOfCurrentNodes; ++j) {
			status[j] = 0;
			if ((pid[j] = fork()) > 0) {
				nothingToBeDone += 1;
			    //Parent
				getTarget = getTarget->next;
				if (j == numOfCurrentNodes - 1) {
					int z;
					for (z = 0; z < numOfCurrentNodes; ++z) {
						pid[z] = waitpid(pid[z], &status[z], 0) ;
					}
				}
			}
			else if (pid[j] == 0) {
				//Child
				char **argv;
				if (not_run_sign == 1)
		        	printf("%s\n", targets[getTarget->targetNum].szCommand);
				else {
		        	printf("%s\n", targets[getTarget->targetNum].szCommand);
		        	argv = dispatchCommand(targets[getTarget->targetNum].szCommand) ;
		        	if(execvp(argv[0], argv) < 0) {
		            	perror("*** ERROR: exec failed.") ;
		            	exit(1) ;
		        	}
				}
		        exit(status[j]);
			}
			else {
		        perror("Get fork problem") ;
		        exit(-1) ;
		    }
		}
	}
	if (nothingToBeDone == 0) {
		printf("Nothing to be done. Already up-to-date.\n");
	}
}

//Analyze is for creating linked-lists for each level of the targets.
void analyze(int szTargetNum) {
	int i;
	for(i = 1; i<=numOfTargets;++i) reversedlevel[i] = -1;
	reversedlevel[szTargetNum] = 0;
	highestLevel = -1;
	dfs(szTargetNum,0,0);

	// for (i = 1; i <= numOfTargets; i++) {
	// 	printf("(Target #%d, Level %d)\n", i, reversedlevel[i]);
	// }

	for (i = 0; i <= highestLevel; i++) compileOrder[i] = NULL;

	// Check each target's level and build linked-lists
	for (i = 1; i <= numOfTargets; i++){
		if (reversedlevel[i] == -1) continue;
		targetSimpleNode *t = (targetSimpleNode *)malloc(sizeof(targetSimpleNode));
		t->targetNum = i;
		t->next = compileOrder[highestLevel-reversedlevel[i]];
		compileOrder[highestLevel-reversedlevel[i]] = t;
	}
	// for (i = 0; i <= highestLevel; i++) {
	// 	printf("Level: %d:",i);
	// 	targetSimpleNode *t = compileOrder[i];
	// 	while (t != NULL) {
	// 		printf(" %s",targets[t->targetNum].szTarget);
	// 		t = t->next;
	// 	}
	// 	printf("\n");
	// }
	//printf("\n");
}

void show_error_message(char * lpszFileName)
{
	fprintf(stderr, "Usage: %s [options] [target] : only single target is allowed.\n", lpszFileName);
	fprintf(stderr, "-f FILE\t\tRead FILE as a maumfile.\n");
	fprintf(stderr, "-h\t\tPrint this message and exit.\n");
	fprintf(stderr, "-n\t\tDon't actually execute commands, just print them.\n");
	fprintf(stderr, "-B\t\tDon't check files timestamps.\n");
	fprintf(stderr, "-m FILE\t\tRedirect the output to the file specified .\n");
	exit(0);
}

//makedup2 is for re-direct the log information output
void makedup2(char* szLog) {
	int fd;
	fd = open("log.txt", O_CREAT|O_WRONLY, S_IRWXU | S_IRWXO ) ;
	if (fd < 0) {
		perror("Can't open/create log file.") ;
	}
	if (dup2(fd, 1) < 0 || dup2(fd, 2) < 0) {
		perror("Can't duplicate file descriptors.") ;
	}
	close(fd) ;
}

int main(int argc, char **argv)
{
	// Declarations for getopt
	extern int optind;
	extern char * optarg;
	int ch;
	char *format = "f:hnBm:";

	// Default makefile name will be Makefile
	char szMakefile[64] = "Makefile";
	char szTarget[64] = "";// Final target
	int szTargetNum = 0;// Final target order
	char szLog[64] = "";
	not_run_sign = 0;
	force_recompile = 0;

	while((ch = getopt(argc, argv, format)) != -1)
	{
		switch(ch)
		{
			case 'B':
				printf("COMMAND: No timestamps check.\n");
				force_recompile = 1;
				break;
			case 'f':
				printf("COMMAND: Specified file: %s.\n", strdup(optarg));
				strcpy(szMakefile, strdup(optarg));
				break;
			case 'n':
				printf("COMMAND: Display the commands without running.\n");
				not_run_sign = 1;
				break;
			case 'm':
				printf("COMMAND: Log to %s.\n",strdup(optarg));
				strcpy(szLog, strdup(optarg));
				makedup2(szLog);
				break;
			case 'h':
			default:
				show_error_message(argv[0]);
				exit(1);
		}
	}

	argc -= optind;
	argv += optind;

	// at this point, what is left in argv is the targets that were
	// specified on the command line. argc has the number of them.
	// If getopt is still really confusing,
	// try printing out what's in argv right here, then just running
	// with various command-line arguments.

	if(argc > 1)
	{
		show_error_message(argv[0]);
		return EXIT_FAILURE;
	}

	if(argc == 1)
	{
		printf("COMMAND: Specified target: %s.\n", argv[0]);
		strcpy(szTarget,argv[0]);
	}
	else
	{
		strcpy(szTarget,"all");
	}

	/* Parse graph file or die */
	if((parse(szMakefile)) == -1)
	{
		return EXIT_FAILURE;
	}
	//printf("Parse Done!\n\n");
	szTargetNum = findTargetNum(szTarget,targets,numOfTargets);
	if (szTargetNum == 0) {
		printf("Can not find target %s.\n", szTarget) ;
		exit(1);
	}

	analyze(szTargetNum);
	//printf("Analyze Done!\n\n");
	execute();
	//printf("Execute Done!\n\n");
	//after parsing the file, you'll want to check all dependencies (whether they are available targets or files)
	//then execute all of the targets that were specified on the command line, along with their dependencies, etc.
	return EXIT_SUCCESS;
}
