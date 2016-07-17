/* CSci4061 Spring 2016 Assignment 4
 * Lecture section: 1
 * Lab section: 002, 007, 006, 005
 * date: 04/30/2016
 * name: Yusen Su, Tianhao Liu, Simin Sun, Zhong Lin
 * id:   5108043,  5131873,     5089150,   4952802
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "util.h"

#define MAX_THREADS 100
#define MAX_QUEUE_SIZE 100
#define MAX_REQUEST_LENGTH 64
#define MAX_MSG_LENGTH 128

int num_dispatchers = 0;
int num_workers = 0;
int qlength = 0;
int count = 0;
int in = 0;
int out = 0;
int logfd = 0;
pthread_mutex_t request_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t request_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t request_full = PTHREAD_COND_INITIALIZER;

//Structure for queue.
typedef struct request_queue
{
    int     m_socket;
    char    m_szRequest[MAX_REQUEST_LENGTH];
} request_queue_t;

request_queue_t *queue;

char file_types[4][15] = {"text/html", "image/jpeg", "image/gif", "text/plain"};

// Detect file type is by the file extension
char * detect_file_type(char * s) {
    if (strlen(s) < 4) 
        return file_types[3]; // Basically not possible but just in case
    char * type = s + strlen(s) - 1;
    while (*type != '.' && (type > s)) 
        --type;
    if (strcmp(type, ".html") == 0 || strcmp(type, ".htm") == 0)
        return file_types[0];
    else if (strcmp(type, ".jpg") == 0)
        return file_types[1];
    else if (strcmp(type, ".gif") == 0)
        return file_types[2];
    else 
        return file_types[3];
}

// log_request
// To log all request that has been token by workers
void log_request(int thid, int req_count, int fd, char * request, char * msg) {
    pthread_mutex_lock(&log_lock);
    char * log_msg;

    if ((log_msg = (char *)malloc(sizeof(char) * MAX_MSG_LENGTH)) == NULL) {
        perror("Malloc error");
        goto END;
    }
    sprintf(log_msg, "[%d][%d][%d][%s][%s]\n", thid, req_count, fd, request, msg);
    printf("%s", log_msg);

    if (write(logfd, log_msg, strlen(log_msg)) < 0) {
        perror("Can't write to log file");
    }

    END:
    pthread_mutex_unlock(&log_lock);
}

// Dispatcher thread function
// It will contineously wait for connection and put the request in to the queue.
void * dispatch(void * arg)
{
    while (1) {
        int fd;
        if ((fd = accept_connection()) < 0) {
            goto END2;
        }
        //printf("FD: %d\n", fd);

        pthread_mutex_lock(&request_lock);
        while(qlength == count) {
            pthread_cond_wait(&request_empty, &request_lock);
        }

        if (get_request(fd, queue[in].m_szRequest) == 0) {
            printf("Request: %s\n", queue[in].m_szRequest);
            queue[in].m_socket = fd;
            in = (in + 1) % qlength;
            count ++;
            pthread_cond_signal(&request_full);
        }
        else {
            printf("Failure: cannot get the request.\n");
        }

        END2:
        pthread_mutex_unlock(&request_lock);
    }
    return NULL;
}

// Worker thread function
// Worker will contineously check if there is a request on the queue.
// When it gets a signal from any dispatcher, it will start working on load the file, send back the requested file and log the request.
void * worker(void * arg)
{
    char *content;
    int worker_id = *(int *)arg;
    int req_count = 0;
    int read_bytes = 0;
    int return_fd;
    int input_fd;
    int fail = 0;
    char error_msg[MAX_MSG_LENGTH];
    char log_msg[MAX_MSG_LENGTH];
    char * file_type;
    char request[MAX_REQUEST_LENGTH];
    struct stat f_stat;

    while(1) {
        pthread_mutex_lock(&request_lock);
        while(count == 0) {
            pthread_cond_wait(&request_full, &request_lock);
        }

        return_fd = queue[out].m_socket;
        strcpy(request, queue[out].m_szRequest+1);
        ++req_count;
        //printf("worker fd: %d, request file name: %s\n", return_fd, request);

        fail = 0;
        if (stat(request, &f_stat) == -1) {
            sprintf(error_msg, "Server can't check file: %s", strerror(errno));
            fail = 1;
            goto LOOP_END;
        }

        if ((input_fd = open(request, O_RDONLY)) == -1) {
            sprintf(error_msg, "Server can't open file: %s", strerror(errno));
            fail = 1;
            goto LOOP_END;
        }

        if ((content = (char *)malloc(f_stat.st_size * sizeof(char))) == NULL) {
            sprintf(error_msg, "Malloc error: %s", strerror(errno));
            fail = 1;
            goto LOOP_END;
        }

        if ((read_bytes = read(input_fd, content, f_stat.st_size)) == -1) {
            sprintf(error_msg, "Server can't read file: %s", strerror(errno));
            fail = 1;
            goto LOOP_END;
        }

        file_type = detect_file_type(request);
        if (return_result(return_fd, file_type, content, f_stat.st_size) != 0) {
            sprintf(error_msg, "Server can't return back: %s", strerror(errno));
            fail = 1;
            printf("Fail to return request\n");
            sprintf(log_msg, "Fail to return request");
        }
        else {
            // Every procedure successes.
            sprintf(log_msg, "%d", read_bytes);
            log_request(worker_id, req_count, return_fd, request, log_msg);
            // Clean up
            queue[out].m_socket = 0;
            strcpy(queue[out].m_szRequest,"");
            strcpy(log_msg,"");
            free(content);
        }

        LOOP_END:
        if (fail) {
            if (return_error(return_fd, error_msg) != 0) {
                printf("Fail to return error\n");
                sprintf(error_msg, "Server cannot return back");
            }
            log_request(worker_id, req_count, return_fd, request, error_msg);
        }
        
        out = (out + 1) % qlength;
        --count;
        close(input_fd);
        pthread_cond_signal(&request_empty);
        pthread_mutex_unlock(&request_lock);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    //Error check first.
    if(argc != 6) {
        printf("usage: %s port path num_dispatcher num_workers queue_length\n", argv[0]);
        return -1;
    }

    char pwd[1024];
    if (chdir(argv[2]) != 0) {
        printf("Can't use web root directory %s: %s\n", argv[2], strerror(errno));
        return -1;
    }
    getcwd(pwd, sizeof(pwd));

    num_dispatchers = atoi(argv[3]);
    num_workers = atoi(argv[4]);

    qlength = atoi(argv[5]);
    queue = (request_queue_t *)malloc(sizeof(request_queue_t)*qlength);

    pthread_t dispatcher_tid[num_dispatchers];
    pthread_t worker_tid[num_workers];

    init(atoi(argv[1]));

    // use O_TRUNC to overwrite previous logs.
    if ((logfd = open("web_server_log", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXO)) < 0) {
        perror("Can't open log file");
        return -1;
    }

    int i;
    int *id;
    // Create n dispatchers
    for (i = 0; i < num_dispatchers; ++i) {
        pthread_create(&dispatcher_tid[i], NULL, dispatch, NULL);
    }
    // Create n workers
    for (i = 0; i < num_workers; ++i) {
        id = (int *)malloc(sizeof(int)); // race condition if using &i directly
        *id = i;
        pthread_create(&worker_tid[i], NULL, worker, (void *)id);
    }

    for (i = 0; i < num_dispatchers; ++i) {
        pthread_join(dispatcher_tid[i], NULL);
    }
    
    for (i = 0; i < num_workers; ++i) {
        pthread_join(worker_tid[i], NULL);
    }

    close(logfd);

    return 0;
}