/* CSci4061 Spring 2016 Assignment 3
 * Lecture section: 1
 * Lab section: 002, 007, 006
 * date: 04/13/2016
 * name: Yusen Su, Tianhao Liu, Simin Sun, Zhong Lin
 * id:   5108043,  5131873,     5089150,   4962802
 */

1. The purpose of this program

    Our program's purpose is to implement a simple message passing application using message queue as mailbox and signals. This allows the application run on single machine and choose to play the role as a receiver or a sender. By choosing the right name of the receiver, the sender will be able to send  information.

2. How to compile the program

    To compile the program, call make.

    user> make

3. How to use the program from the shell (syntax)

    To run it, simply call it as follow:

    For sender:
	user> ./application B 2 3 4 5 20

    Role(sender/receiver): sender

    where B stands for the name of sender and it could be any name you like
          2 for the mailbox key, it could be any number you like. It should be UNIQUE among all the senders and receivers
          3 for window size, it should be >= 1.
          4 for Max delay, it could be >= 1.
          5 for timeout, it would be used by sender to check if it "loses" the connection and need to re-send the packet(s)
          20 for 20% as the probability of a packet to be lost. It should >=0 in this case as required.

    You have to open another terminal to run as the receiver:

    user> ./application A 2 3 4 5 20

    Role(sender/receiver): receiver

    where A stands for the name of receiver and it could be any name you like and other arguments have the same meaning and usage as sender

    After set up sender and receive, you have to type the name of the receiver in the sender's window. By doing this, the sender will be able to find the correct receiver.

    The last thing is to type anything you want to send to the receiver.

4. How this program works:

    As a sender, it will separate the message in to several packets (depends on the packet size and the message size). It will send window_size amount of packets once and for each packet it sends, it will send a SIGIO to notify the receiver.
    As a receiver, it will pause until it receive a SIGIO and launch the signal handler. After receiving a signal, it will check its message queue and pick up all packets. For each packet received, it will send back an acknowledgement to the sender. Also, it will send a SIGIO to notify the sender.
    Within the timeout amount, if the sender receive the acknowledgement, it will mark the packet as sent. If not, it will resend all packets that is on the way.
    After all packets have been received, the receiver will print out the packet and wait for the next job allocation.

5. What is the effect of having a very short TIMEOUT period and a very long TIMEOUT period?

    When the TIMEOUT period is very short, we will check the time after a short time interval and may result in sending the message slowly or even failing to send the message we want. As it will reach the TIMEOUT alarm and resend all remaining packets over and over again until reach the limitation. Since the TIMEOUT period is very short, it gives the receiver a really short time to react as well.

    When the TIMEOUT period is very long, it will send the message all in one time except the missing package. The reason is the TIMEOUT period is very long, it has enough time to send all the package before the TIMEOUT alarm been raised.

6. What is the effect of having a very small window size (e.g., n=1) and a very large window size?

   A very small window size may reduce the sending efficiency. The reason is, for example, we have window size n = 1, then the sender can only send 1 package every time. In this case, all message need to be sent one by one, this will increase the chance we miss the packet and lower the efficiency.

   When window size is large, it will send several packets and the receiver could handle one for each time while block the other. However, if the timeout is short, it may result in unnecessary resend and the receiver will be busy. What's more, we have delay limitation for it, therefore, a large window size may not be that helpful as expected.
