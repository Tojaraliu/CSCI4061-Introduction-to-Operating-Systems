/* CSci4061 Spring 2016 Assignment 2
 * Lecture section: 1
 * Lab section: 002, 007, 006
 * date: 03/11/2016
 * name: Yusen Su, Tianhao Liu, Simin Sun
 * id:   5108043,  5131873,     5089150
 */

1. The purpose of this program

    Our program's purpose is to implement a simple "local" multi-party chat application using a multi-process architecture. This allows the application run on single machine instead of multiple machines as a real chat program supposes to do.

2. How to compile the program

    To compile the program, call make.

    user> make

3. How to use the program from the shell (syntax)

    To run it, simply call ./server

	user> ./server

4. What exactly this program does

    After running the program, the shell will run as a server, like what shows below:
    Server>>

    Then as the what required in the Project2, we allow the server do the following functions by typing different commands:

      	1. Server>>\add name1
        //After calling this command, a Xterm window will pop up as a individual window for the person "name1".

        2. Server>>\list
        //This command will list all the current logged users that in the server. "<No users>" will show up if there is no user currently in the chat room.

        3. Server>>\kick name2
        //This command is only available for the sever, all individual user can't kick other user from their own window. When server call "kick name2", this user will exit the process and its xterm will closed automatically.

        4. Server>>\exit
        //This command will closed the entire chat process and all users are forced to quit and all xterm windows will close.

        5. <any-other-text>
        //This function will help us to deal with all other cases. It will broadcast all the input to every users windows and do nothing if no one in the chat room.

    When a user logs in, an Xterm window will pop up for them. There are four functions available for them to execute the file:

        1. user1>>\list
        //This command will list all the current logged users that in the server. "<No users>" will show up if there is no user currently in the chat room.

        2. user1>>\exit
        //This command will log out the user who inputs it, close the Xterm window. Also, the server process will notes it and do the cleanup.

        3. user1>>\seg
        //We did this by creating a char * n = NULL and then set *n = 1, which would generate a segment fault intentionally. It will crash the child process who inputs \seg. Therefore, it will quit after calling \seg. The server process will notes its status and do the cleanup.

        4. <any-other-text>
        //This function will help us to deal with all other cases, it will broadcast all the input to every users windows and do nothing if no one in the chat room.

5. Explicit assumptions

    We make some general assumptions about our program: first, we assume that the server program and the shell program are under the same path. Second,the maximum users are less or equal to ten. The maximum message length is 1024.

    We also have some assumptions about the program structure. The first assumption is we use pipes to connect the server and shell process. By using bi-directional pipes, we can write from server and read from the shell. After the shell process it, it can write the feedback in the shell and the server can get access to it.

    We also want to assume server as parent while each individual shell process as child. Therefore, we can fork and execute the child process. At the same time, for each process, we will close unnecessary pipe ends. When we exit from the server, child processes might become zombie, we need to clean up them. When we exit from one of the shell (by \seg, \exit or the close buttom), then orphans will be adopted by init parent.


6. Strategies for error handling

    We have two related programs for this project, shell and server. Basically, shell focuses on input and output, and server do the major part of error handling.

    The execl might have some errors. If that happens the user the server will print out an error and halt.

    The user might type some commands with syntax error, like add/kick with no name, send p2p message to some user currently doesn't exist, etc. The shell will send the original command to the server, and the server will detect syntax errors via string checking methods. If there is any syntax error, it will stop executing it and perror() the error message.

    For string operations, the server will do the NULL check to avoid a lot of problems. When using pipes to send or receive messages, the server will check the return value of read() and write() to detect if there is any problem about pipes.

    Except \exit command, a member in the chat room might quit other ways, like \seq to crash the child process, close the Xterm window, etc. We use waitpid() with WNOHANG option to check the status of the child process repeatedly. When detecting a member did quit, no matter how it did, the server will print out a message to inform user and also do the cleanup. For normal \quit, the server will do the same things. In addition, closing unnecessary pipes and appropriate cleanup are also important to avoid errors.
