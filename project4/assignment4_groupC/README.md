/* CSci4061 Spring 2016 Assignment 4
 * Lecture section: 1
 * Lab section: 002, 007, 006, 005
 * date: 04/30/2016
 * name: Yusen Su, Tianhao Liu, Simin Sun, Zhong Lin
 * id:   5108043,  5131873,     5089150,   4952802
 */

1. The purpose of this program

    Our program's purpose is to create a multi-thread web server using POSIX threads. Our web server can handle any type of file: HTML, GIF, JPEG, TXT, etc. And it can also handle a limited portion of the HTTP web protocol.

2. How to compile the program

    To compile the program, call make.

    user> make

3. How to use the program from the shell (syntax)

    To run it, simply call

    user> ./web_server <port_number> <path_to_testing>/testing <num_dispatch> <num_worker> <queue_len>

    For example, to run the web server at port 9000, with root directory "/home/student/joe/testing" with 100 dispatch and worker threads,
    queue length 100, run the following command

    ./web_server 9000 /home/student/joe/testing 100 100 100

    Open another terminal and issue the following command,

    wget -i <path-to-urls>/urls -O results

    In this case we should run
    wget -i /home/student/joe/testing/urls -O myres

    The above command will ask wget to fetch all the URLs listed
    on the file named "urls" that you downloaded from the assignment page.

4. How our programs works

    At the beginning we create n dispatcher threads and n workers threads. As a dispatcher thread, it will wait for a http connection comes. Once it gets a connection, it will put it into the request queue, and then notify a worker thread to serve that request.
    As a worker thread, it will suspend until being notified by a dispatcher. After being notified, it will pop a request from the request queue, and then it will check the requested file, read it into memory and return it back.

5. What we get:

    We would log each request to the file "web_server_log" in following format

    [ThreadID#][Request#][fd][Request file][bytes/error message]

    for example:

    [8][1][5][/image/jpg/30.jpg][17772]


    if there is any error occurs, it will log like:
    [10][1][10][text/html/images/hotel_pics.jpg][Server can't check file: No such file or directory]

    where [8] is Thread ID number
          [1] is Request number
          [5] us file descriptor given by accept_connection() for this request
          [/image/jpg/30.jpg] is the filename buffer filled in by the get request function
          [17772] is either the number of bytes returned by a successful request, or the error string returned by return error if an error occurred.
