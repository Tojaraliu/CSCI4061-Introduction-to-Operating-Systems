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

struct msg1
{
    long mtype;
    int info;
};


void reset(int sig) {
    //i = 0;
    //printf("haha\n");
    //alarm(1);
}

int main(int argc, char const **argv)
{
    int qid = msgget(3, 0666 | IPC_CREAT);
    msgctl(qid,IPC_RMID,0);
    qid = msgget(3, 0666 | IPC_CREAT);

    struct msg1 * m1 = (struct msg1 *)malloc(sizeof(struct msg1));
    m1->mtype = 1;
    m1->info = 0;

    int pid;
    printf("QID:%d\n", qid);
    printf("PID:");
    scanf("%d",&pid);
    int i = 0;

    while (i < 10) {
        usleep(100000);
        m1->info = rand() % 100;
        if (m1->mtype == 1)
            m1->mtype = 2; 
        else
            m1->mtype = 1;
        msgsnd(qid, (void *)m1, sizeof(struct msg1), 0);
        printf("Send %d!\n", m1->info);
        kill(pid, SIGIO);
        i++;
    }

    return 0;
}
