#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/msg.h>
#include <signal.h>

int i = 0;
int qid;

struct msg1
{
    long mtype;
    int info;
};

struct msg1 *m1;
sigset_t s1;

int nob;

void reset(int sig) {
    free(m1);
    m1 = (struct msg1 *)malloc(sizeof(struct msg1));
    nob = msgrcv(qid, (void *)m1, sizeof(struct msg1), 1, IPC_NOWAIT);
    usleep(100);
    printf("Get:%d ##%d##\n", m1->info, nob);
}

int main(int argc, char const **argv)
{
    qid = msgget(3, 0666);

    printf("My PID:%d\n",getpid());
    printf("QID:%d\n", qid);

    struct sigaction act;
    act.sa_handler = reset;
    sigfillset(&act.sa_mask);
    if (sigaction(SIGIO, &act, NULL) == -1) {
        perror("Sigaction error");
    }
    
    while (1) {
        
    }

    printf("Get it!\n");
    return 0;
}
