//
//  main.c
//  ThreadTest
//
//  Created by Tianhao Liu on 3/29/16.
//  Copyright © 2016 Arthur Liu. All rights reserved.
//

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *tfn(void *arg) {
    printf("x = %d\n",*((int *)arg));
    return NULL;
}

int main(int argc, const char * argv[]) {
    pthread_t t1;
    int x = 1;
    pthread_create(&t1, NULL, tfn, (void *)&x);
    usleep(250);
    x = 2;
    sleep(3);
}
