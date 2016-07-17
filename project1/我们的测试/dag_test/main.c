#include "dag.h"

int main(int argc, char const *argv[])
{
    int i;

    char *name = (char *)malloc(10 * sizeof(char));
    strcpy(name,"make4061");
    char *cmd = (char *)malloc(10 * sizeof(char));
    strcpy(cmd,"gcc");
    int nDep = 3;
    char *params = (char *)malloc(10 * sizeof(char));
    strcpy(params,"-c");
    char **deps = (char **)malloc(3 * sizeof(char *));
    for (i = 0; i < 3; ++i){
        deps[i] = (char *)malloc(10 * sizeof(char));
        if (i == 0)
            strcpy(deps[i], "test1.o");
        if (i == 1)
            strcpy(deps[i], "test2.o");
        if (i == 2)
            strcpy(deps[i], "test3.o");
    }

    struct target *T = build(name,cmd,params,3,deps);
    outputNode(T);
    printf("%s\n", commandBuilder(T));
    return 0;
}