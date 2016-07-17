#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    int pid;
    int status = 0;
    pid = fork ();
    if (pid > 0) {
        printf ("Parent: child has pid=%d\n", pid);
        pid = waitpid(pid, &status, 0);
        printf("Child done.\n");
    } else if (pid == 0) {
        printf ("child here\n");
        //sleep(2000);
        exit(status);
    } else {
        perror ("fork problem\n");
        exit (-1);
    }
    return 0;
}
