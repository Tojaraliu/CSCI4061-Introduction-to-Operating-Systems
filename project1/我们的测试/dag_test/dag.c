#include <stdio.h>
#include <stdlib.h>
#include "dag.h"

struct target
{
    char *TargetName;
    char *Cmd;
    int nDep;
    char **Dep;
    char *Params;
};

struct target * build(char *name, char *Cmd, char *params, int nDep, char **dep)
{
    static int i;
    int lenName = strlen(name);
    int lenParams = strlen(params);
    int lenCmd = strlen(Cmd);
    struct target * newNode = (struct target *)malloc(sizeof(struct target));

    newNode->TargetName = (char *)malloc((lenName + 1) * sizeof(char));
    newNode->Dep = (char **)malloc(nDep * sizeof(char *));
    newNode->Params = (char *)malloc((lenParams + 1) * sizeof(char));
    newNode->Cmd = (char *)malloc((lenCmd + 1) * sizeof(char));

    newNode->nDep = nDep;
    strcpy(newNode->TargetName, name);
    strcpy(newNode->Params, params);
    strcpy(newNode->Cmd, Cmd);
    for (i = 0; i < nDep; ++i) {
        newNode->Dep[i] = (char *)malloc(strlen(dep[i]) * sizeof(char));
        strcpy(newNode->Dep[i], dep[i]);
    }

    return newNode;
}

void outputNode(struct target *T)
{
    static int i;
    printf("TargetName: %s\n", T->TargetName);
    printf("Cmd: %s\n", T->Cmd);
    printf("Params: %s\n", T->Params);
    for (i = 0; i < T->nDep; ++i) {
        printf("%s\n", T->Dep[i]);
    }
}

char * commandBuilder(struct target *T)
{
    int i;
    int totalLen = strlen(T->Cmd) + strlen(T->Params) + 2;
    for (i = 0; i < T->nDep; ++i)
        totalLen += strlen(T->Dep[i]) + 1;
    char *command = (char *)malloc(totalLen * sizeof(char));
    strcpy(command,"");
    strcat(command,T->Cmd);strcat(command," ");
    strcat(command,T->Params);strcat(command," ");
    if (!(strcmp(T->Params, "-o") != 0)) {
        strcat(command,T->TargetName);strcat(command," ");
    }
    for (i = 0; i < T->nDep; ++i)
    {
        strcat(command,T->Dep[i]);strcat(command," ");
    }
    return command;
}
