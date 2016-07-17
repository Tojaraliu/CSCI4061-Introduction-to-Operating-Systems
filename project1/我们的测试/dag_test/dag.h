#include <stdio.h>
#include <stdlib.h>

struct target;

struct target * build(char *name, char *Cmd, char *params, int nDep, char **dep);

void outputNode(struct target *T);

char * commandBuilder(struct target *T);