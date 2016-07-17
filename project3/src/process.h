#include <sys/msg.h>
#include <sys/types.h>

#define DATA 1
#define ACK 2
#define MAX_SIZE 1024
#define PACKET_SIZE 16
#define MAX_NAME_LEN 32
#define MAX_TIMEOUT 5

/* This structure stores the information of a process */
typedef struct {
    int pid;
    char process_name[MAX_NAME_LEN];
    key_t key;
} process_t;

/* This is the structure of the packet. You are free to add any other fields as needed */
typedef struct {
    long mtype;       // packet type: DATA or ACK
    int message_id;   // message id
    int pid;          // sender's pid
    char process_name[MAX_NAME_LEN];  // sender's name
    int num_packets;  // total number of packets for the message
    int packet_num;   // which packet in the message
    int total_size;   // total size of the message
    char data[PACKET_SIZE]; // the actual content
} packet_t;

/* This structure is used to monitor the status of each packet */
typedef struct {
    packet_t packet;
    int is_sent;
    int ACK_received;
} packet_status_t;

/* This structure is used by the sender to monitor the status of the message */
typedef struct {
    process_t receiver_info;
    int mailbox_id;
    int num_packets_received;
    int num_packets;
    int is_sending;
    packet_status_t *packet_status;
} message_status_t;

/* This structure is used by the receiver to store a message */
typedef struct {
    process_t sender;
    int num_packets_received;
    int is_complete;
    int *is_received;
    char *data;
} message_t;

typedef struct {
  int status;
  int msg_idx;
  int packet_idx;
} window_t;

void receive_packet(int sig);
void timeout_handler(int sig);
int init(char *process_name, key_t key, int wsize, int delay, int to, int drop);
int get_process_info(char *process_name, process_t *info);
int get_num_packets(char *data, int packet_size);
int create_packets(char *data, message_status_t *message_stats);
int get_next_packet(int num_packets);
int send_packet(packet_t *packet, int mailbox_id, int pid);
int drop_packet();
int send_message(char *receiver, char* content);
int send_ACK(int mailbox_id, pid_t pid, int packet_num);
void handle_data(packet_t *packet, process_t *sender, int sender_mailbox_id);
void handle_ACK(packet_t *packet);
int receive_message(char *data);
