#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "process.h"

// messaging config
int WINDOW_SIZE;
int MAX_DELAY;
int TIMEOUT;
int DROP_RATE;

// process information
process_t myinfo;
int mailbox_id;
// a message id is used by the receiver to distinguish a message from other messages
// you can simply increment the message id once the message is completed
int message_id = 0;
// the message status is used by the sender to monitor the status of a message
message_status_t message_stats;
// the message is used by the receiver to store the actual content of a message
message_t *message;

int num_available_packets; // number of packets that can be sent (0 <= n <= WINDOW_SIZE)
int is_receiving = 0; // a helper variable may be used to handle multiple senders

struct sigaction act_gio; // Structure for signal SIGIO
struct sigaction act_alarm; // Structure for signal SIGALRM

int firstAttempt = 1;
int attempt_connection = 0; // a helper variable be used to record the times of sender attempt to connecting receiver

window_t* windows; //Sturcture for sliding window
/**
 * Fucntion init
 * 1. Save the process information to a file and a process structure for future use.
 * 2. Setup a message queue with a given key.
 * 3. Setup the signal handlers (SIGIO for handling packet, SIGALRM for timeout).
 * Return 0 if success, -1 otherwise.
 */
int init(char *process_name, key_t key, int wsize, int delay, int to, int drop) {
    myinfo.pid = getpid();
    strcpy(myinfo.process_name, process_name);
    myinfo.key = key;

    // open the file
    FILE* fp = fopen(myinfo.process_name, "wb");
    if (fp == NULL) {
        printf("Failed opening file: %s\n", myinfo.process_name);
        return -1;
    }
    // write the process_name and its message keys to the file
    if (fprintf(fp, "pid:%d\nprocess_name:%s\nkey:%d\n", myinfo.pid, myinfo.process_name, myinfo.key) < 0) {
        printf("Failed writing to the file\n");
        return -1;
    }
    fclose(fp);

    WINDOW_SIZE = wsize;
    MAX_DELAY = delay;
    TIMEOUT = to;
    DROP_RATE = drop;

    printf("[%s] pid: %d, key: %d\n", myinfo.process_name, myinfo.pid, myinfo.key);
    printf("window_size: %d, max delay: %d, timeout: %d, drop rate: %d%%\n", WINDOW_SIZE, MAX_DELAY, TIMEOUT, DROP_RATE);

    // setup a message queue and save the id to the mailbox_id
    if ((mailbox_id = msgget(myinfo.key, IPC_CREAT | 0666)) == -1) {
      perror("Failed to create new private message queue");
      return -1;
    }
    // set the signal handler for receiving packets
    act_gio.sa_handler = receive_packet;
    act_gio.sa_flags = 0;
    if ((sigemptyset(&act_gio.sa_mask) == -1) || sigaddset(&act_gio.sa_mask, SIGIO) == -1 || (sigaction(SIGIO, &act_gio, NULL) == -1)) {
      perror("Failed to set SIGIO handler");
      return -1;
    }
    act_alarm.sa_handler = timeout_handler;
    act_alarm.sa_flags = 0;
    if ((sigemptyset(&act_alarm.sa_mask) == -1) || sigaddset(&act_gio.sa_mask, SIGALRM) == -1 || (sigaction(SIGALRM, &act_alarm, NULL) == -1)) {
      perror("Failed to set SIGALRM handler");
      return -1;
    }
    windows = (window_t*) malloc (WINDOW_SIZE * sizeof(window_t));
    int i;
    for (i = 0; i < WINDOW_SIZE; ++i) {
      windows[i].status = 0;
      windows[i].packet_idx = -1;
      windows[i].msg_idx = -1;
    }
    return 0;
}

/**
 * Get a process' information and save it to the process_t struct.
 * Return 0 if success, -1 otherwise.
 */
int get_process_info(char *process_name, process_t *info) {
    char buffer[MAX_SIZE];
    char *token;

    // open the file for reading
    FILE* fp = fopen(process_name, "r");
    if (fp == NULL) {
        return -1;
    }
    // parse the information and save it to a process_info struct
    while (fgets(buffer, MAX_SIZE, fp) != NULL) {
        token = strtok(buffer, ":");
        if (strcmp(token, "pid") == 0) {
            token = strtok(NULL, ":");
            info->pid = atoi(token);
        } else if (strcmp(token, "process_name") == 0) {
            token = strtok(NULL, ":");
            strcpy(info->process_name, token);
        } else if (strcmp(token, "key") == 0) {
            token = strtok(NULL, ":");
            info->key = atoi(token);
        }
    }
    fclose(fp);
    return 0;
}

/**
 * Send a packet to a mailbox identified by the mailbox_id, and send a SIGIO to the pid.
 * Return 0 if success, -1 otherwise.
 */
int send_packet(packet_t *packet, int mailbox_id, int pid) {
    if (msgsnd(mailbox_id, packet, sizeof(packet_t), 0) == -1) {
        perror("Can not send packet");
        return -1;
    }
    if (kill(pid, SIGIO) == -1) {
        perror("Failed to send the SIGIO signal");
        return -1;
    }
    return 0;
}

/**
 * Get the number of packets needed to send a data, given a packet size.
 * Return the number of packets if success, -1 otherwise.
 */
int get_num_packets(char *data, int packet_size) {
    if (data == NULL) {
        return -1;
    }
    if (strlen(data) % packet_size == 0) {
        return strlen(data) / packet_size;
    } else {
        return (strlen(data) / packet_size) + 1;
    }
}

/**
 * Create packets for the corresponding data and save it to the message_stats.
 * Return 0 if success, -1 otherwise.
 */
int create_packets(char *data, message_status_t *message_stats) {
    if (data == NULL || message_stats == NULL) {
        return -1;
    }
    int i, len;
    for (i = 0; i < message_stats->num_packets; i++) {
        if (i == message_stats->num_packets - 1) {
            len = strlen(data)-(i*PACKET_SIZE);
        } else {
            len = PACKET_SIZE;
        }
        message_stats->packet_status[i].is_sent = 0;
        message_stats->packet_status[i].ACK_received = 0;
        message_stats->packet_status[i].packet.message_id = -1;
        message_stats->packet_status[i].packet.mtype = DATA;
        message_stats->packet_status[i].packet.pid = myinfo.pid;
        strcpy(message_stats->packet_status[i].packet.process_name, myinfo.process_name);
        message_stats->packet_status[i].packet.num_packets = message_stats->num_packets;
        message_stats->packet_status[i].packet.packet_num = i;
        message_stats->packet_status[i].packet.total_size = strlen(data);
        memcpy(message_stats->packet_status[i].packet.data, data+(i*PACKET_SIZE), len);
        message_stats->packet_status[i].packet.data[len] = '\0';
    }
    return 0;
}

/**
 * Get the index of the next packet to be sent.
 * Return the index of the packet if success, -1 otherwise.
 */
int get_next_packet(int num_packets) {
    int packet_idx = rand() % num_packets;
    int i = 0;
    i = 0;
    while (i < num_packets) {
        if (message_stats.packet_status[packet_idx].is_sent == 0) {
            // found a packet that has not been sent
            return packet_idx;
        } else if (packet_idx == num_packets-1) {
            packet_idx = 0;
        } else {
            packet_idx++;
        }
        i++;
    }
    // all packets have been sent
    return -1;
}

/**
 * Use probability to simulate packet loss.
 * Return 1 if the packet should be dropped, 0 otherwise.
 */
int drop_packet() {
    if (rand() % 100 > DROP_RATE) {
        return 0;
    }
    return 1;
}

/**
 * Lock or unlock sliding window to allow sender only send one packet.
 */
void lock_unlock_window (int key) {
  int i;
  for (i = 0; i < WINDOW_SIZE; ++i) {
    if (windows[i].status == key) {
      continue;
    }
    else {
      windows[i].status = key;
    }
  }
}

/**
 * Clean up for each process
 */
void clean_up() {
  //clean up message status structure
  if (message_stats.packet_status != NULL) {
    free(message_stats.packet_status);
    message_stats.packet_status = NULL;
  }
  message_stats.num_packets_received = 0;
  message_stats.num_packets = 0;
  message_stats.is_sending = 0;
  // clean up window
  int i;
  for (i = 0; i < WINDOW_SIZE; ++i) {
    windows[i].status = 0;
    windows[i].packet_idx = -1;
    windows[i].msg_idx = -1;
  }
  //clean up message structure
  if (message != NULL) {
    if (message->data != NULL) {
      free(message->data);
      message->data = NULL;
    }
    if (message->is_received != NULL) {
      free(message->is_received);
      message->is_received = NULL;
    }
    message->is_complete = 0;
    message->num_packets_received = 0;
    free(message);
    message = NULL;
  }
  //clean_up other global variables
  attempt_connection = 0;
  is_receiving = 0;
  firstAttempt = 1;
}

/**
 * Send a message (broken down into multiple packets) to another process.
 * We first need to get the receiver's information and construct the status of
 * each of the packet.
 * Return 0 if success, -1 otherwise.
 */
int send_message(char *receiver, char* content) {
    if (receiver == NULL || content == NULL) {
        printf("Receiver or content is NULL\n");
        return -1;
    }
    if (strcmp(receiver, myinfo.process_name) == 0) {
      printf("Sender sends packets to itself.\n");
      return -1;
    }
    // get the receiver's information
    if (get_process_info(receiver, &message_stats.receiver_info) < 0) {
        printf("Failed getting %s's information.\n", receiver);
        return -1;
    }
    // get the receiver's mailbox id
    message_stats.mailbox_id = msgget(message_stats.receiver_info.key, 0666);
    if (message_stats.mailbox_id == -1) {
        printf("Failed getting the receiver's mailbox.\n");
        return -1;
    }
    // get the number of packets
    int num_packets = get_num_packets(content, PACKET_SIZE);
    if (num_packets < 0) {
        printf("Failed getting the number of packets.\n");
        return -1;
    }
    // set the number of available packets
    if (num_packets > WINDOW_SIZE) {
        num_available_packets = WINDOW_SIZE;
    } else {
        num_available_packets = num_packets;
    }
    // setup the information of the message
    message_stats.is_sending = 1;
    message_stats.num_packets_received = 0;
    message_stats.num_packets = num_packets;
    message_stats.packet_status = malloc(num_packets * sizeof(packet_status_t));
    if (message_stats.packet_status == NULL) {
        return -1;
    }
    // parition the message into packets
    if (create_packets(content, &message_stats) < 0) {
        printf("Failed paritioning data into packets.\n");
        message_stats.is_sending = 0;
        free(message_stats.packet_status);
        return -1;
    }

    /* send packets to the receiver
    ** the number of packets sent at a time depends on the WINDOW_SIZE.
    ** you need to change the message_id of each packet (initialized to -1)
    ** with the message_id included in the ACK packet sent by the receiver
    */
    while(message_stats.num_packets > message_stats.num_packets_received) {
        int i;
        for (i = 0; i < WINDOW_SIZE; ++i) {
          // handle send packets
          int idx;
          if ((windows[i].status == 0) && (idx = get_next_packet(message_stats.num_packets)) != -1) {

            windows[i].packet_idx = idx;
            windows[i].msg_idx = message_id;
            windows[i].status = 1;

            int mid = msgget(message_stats.receiver_info.key, 0666);
            if (send_packet(&message_stats.packet_status[idx].packet, mid, message_stats.receiver_info.pid) == -1) {
                return -1;
            }
            printf("Send a packet [%d] to pid: %d\n", idx, message_stats.receiver_info.pid);
            message_stats.packet_status[idx].is_sent = 1;

            // Attempt to send one packet
            if (message_stats.num_packets_received == 0) {
              i = WINDOW_SIZE;
              lock_unlock_window(1);
            }
          }
        }
        alarm(TIMEOUT);
        if (message_stats.num_packets_received < message_stats.num_packets) {
            pause();
        }
        if (message_stats.num_packets_received == 1 && firstAttempt) {
          firstAttempt = 0;
          lock_unlock_window(0);
          attempt_connection = 0;
        }
        if (attempt_connection == MAX_TIMEOUT) {
          printf("Maximum Timeout reached, quit...\n");
          alarm(0);
          clean_up();
          return 0;
        }
        printf("received: %d, total: %d\n", message_stats.num_packets_received, message_stats.num_packets);
    }
    printf("All packets sent.\n");
    alarm(0);
    clean_up();
    return 0;
}

/**
 * Handle TIMEOUT. Resend previously sent packets whose ACKs have not been
 * received yet. Reset the TIMEOUT.
 */
void timeout_handler(int sig) {

    printf("TIMEOUT! Resending message...\n");
    attempt_connection ++;
    int i;
    for (i = 0; i < WINDOW_SIZE; ++i) {
        if (windows[i].status == 1 && windows[i].packet_idx != -1 && message_stats.packet_status[windows[i].packet_idx].ACK_received == 0) {
          int mid = msgget(message_stats.receiver_info.key, 0666);
          if (send_packet(&message_stats.packet_status[windows[i].packet_idx].packet, mid, message_stats.receiver_info.pid) == -1) {
              // TODO: need to handle this error
              continue;
          }
          printf("Send a packet [%d] to pid: %d[RESEND]\n", windows[i].packet_idx, message_stats.receiver_info.pid);
          message_stats.packet_status[windows[i].packet_idx].is_sent = 1;
          // Attempt to send one packet
          if (message_stats.num_packets_received == 0) {
            i = WINDOW_SIZE;
            //attempt_connection ++;
          }
        }
    }
    alarm(TIMEOUT);
}

/**
 * Send an ACK to the sender's mailbox.
 * The message id is determined by the receiver and has to be included in the ACK packet.
 * Return 0 if success, -1 otherwise.
 */
int send_ACK(int mailbox_id, pid_t pid, int packet_num) {
    // construct an ACK packet
    packet_t *package = (packet_t *) malloc (sizeof(packet_t));
    package->mtype = ACK;
    package->message_id = message_id;
    package->pid = myinfo.pid;
    strcpy(package->process_name, myinfo.process_name);
    package->num_packets = 1;
    package->packet_num = packet_num;
    sprintf(package->data, "%d", packet_num);
    package->total_size = strlen(package->data);
    int delay = rand() % MAX_DELAY;
    sleep(delay);
    // send an ACK for the packet it received
    if (send_packet(package, mailbox_id, pid) == -1) {
      return -1;
    }
    return 0;
}

/**
 * Handle DATA packet. Save the packet's data and send an ACK to the sender.
 * You should handle unexpected cases such as duplicate packet, packet for a different message,
 * packet from a different sender, etc.
 */
void handle_data(packet_t *packet, process_t *sender, int sender_mailbox_id) {
    // Handle data stored
    if (message->num_packets_received == 0) {
      message->data = (char *)malloc(packet->total_size*sizeof(char) + 1);
      message->is_received = (int *)malloc(packet->num_packets * sizeof(int));
      memset(message->data, 0, packet->total_size + 1);
      memset(message->is_received, 0, packet->num_packets * sizeof(int));
      packet->message_id = message_id;
    }
    if (packet->message_id < message_id) {
      return;
    }
    if (message->is_received[packet->packet_num] == 1) {
      if (send_ACK(sender_mailbox_id, sender->pid, packet->packet_num) == -1) {
        perror("Failed to send ACK to sender");
        return;
      }
      printf("Send an ACK for packet [%d] to pid: %d[DUPLICATE]\n", packet->packet_num, sender->pid);
      return;
    }
    int len;
    if (packet->num_packets == packet->packet_num) {
      len = packet->total_size - (packet->packet_num * PACKET_SIZE);
    }
    else {
      len = PACKET_SIZE;
    }
    memcpy(message->data + (packet->packet_num * PACKET_SIZE), packet->data, len);
    message->is_received[packet->packet_num] = 1;
    message->num_packets_received++;
    if (message->num_packets_received == packet->num_packets) {
      message->data[packet->total_size] = '\0';
      message->is_complete = 1;
    }
    if (send_ACK(sender_mailbox_id, sender->pid, packet->packet_num) == -1) {
      perror("Failed to send ACK to sender");
      return;
    }
    printf("Send an ACK for packet [%d] to pid: %d\n", packet->packet_num, sender->pid);

}

/**
 * Reset message id for all packets.
 */
void reset_message_id(int id) {
  int i;
  for (i = 0; i < message_stats.num_packets; ++i) {
    message_stats.packet_status[i].packet.message_id = id;
  }
}

/**
 * Handle ACK packet. Update the status of the packet to indicate that the packet
 * has been successfully received and reset the TIMEOUT.
 * You should handle unexpected cases such as duplicate ACKs, ACK for completed message, etc.
 */
void handle_ACK(packet_t *packet) {
    printf("Received an ACK for [%d]\n", packet->packet_num);
    alarm(TIMEOUT);
    int i;
    if (message_stats.num_packets_received == 0) {
      reset_message_id(packet->message_id);
    }
    for (i = 0; i<WINDOW_SIZE ;++i){
        if (windows[i].packet_idx == packet->packet_num) {
            // found
            if (message_stats.packet_status[windows[i].packet_idx].ACK_received == 1) {
                // duplicate
                return;
            }
            else {
                message_stats.packet_status[windows[i].packet_idx].ACK_received = 1;
                message_stats.num_packets_received ++;
                windows[i].status = 0;
            }
            break;
        }
    }
}

/**
 * Get the next packet (if any) from a mailbox.
 * Return 0 (false) if there is no packet in the mailbox
 */
int get_packet_from_mailbox(int mailbox_id) {
    struct msqid_ds buf;
    return (msgctl(mailbox_id, IPC_STAT, &buf) == 0) && (buf.msg_qnum > 0);
}

/**
 * Receive a packet.
 * If the packet is DATA, send an ACK packet and SIGIO to the sender.
 * If the packet is ACK, update the status of the packet.
 */
void receive_packet(int sig) {
    // you have to call drop_packet function to drop a packet with some probability
    if (drop_packet()) {
        return;
        // RIP ACK
    }
    else {
      packet_t *packet = (packet_t *)malloc(sizeof(packet_t));
      while(get_packet_from_mailbox(mailbox_id)) {
        if (msgrcv(mailbox_id, packet, sizeof(packet_t), DATA, IPC_NOWAIT) == -1) {
            if (msgrcv(mailbox_id, packet, sizeof(packet_t), ACK, IPC_NOWAIT) == -1) {
                perror("Can not receive packet");
                return;
            }
            else {
                if (message_stats.num_packets_received == message_stats.num_packets) {
                  continue;
                }
                handle_ACK(packet);
            }
        }
        else {
            if (message->num_packets_received == packet->num_packets) {
              continue;
            }
            get_process_info(packet->process_name, &message->sender);
            int mailbox = msgget(message->sender.key, 0666);
            handle_data(packet, &message->sender, mailbox);
        }
    }
    free(packet);
  }
}

/**
 * Initialize the message structure and wait for a message from another process.
 * Save the message content to the data and return 0 if success, -1 otherwise
 */
int receive_message(char *data) {
    message = (message_t *) malloc(sizeof(message_t));
    message->is_complete = 0;
    message->num_packets_received =0;
    while (!message->is_complete) {
        pause();
    }
    printf("All packets received.\n");
    memcpy(data, message->data, MAX_SIZE);
    clean_up();
    return 0;
}
