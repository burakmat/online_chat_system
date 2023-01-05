# Online Chat System via Network Programming



## Chat System User Manual
This is a guidance document for the online Chat System implemented using Network Programming. The program is capable of sending real-time messages through a client to other users, with orientation of the messages and users handled by the server.

*The program works on both Linux and macOS machines.*

### Running the server and client programs

In order to run the chat system, you will need to compile and run both the server and client programs. The server program should be run first, followed by the client program.

#### **Compiling the programs**
To compile the server program on a Linux machine, use the following command:
```
gcc -o server server.c -pthread
```
To compile the client program on a Linux machine, use the following command:

```
gcc -o client client.c -pthread
```
On a macOS machine, you can omit the `-pthread` flag.

#### **Running the programs**
To run the server program, use the following command:

```
./server
```
To run the client program, use the following command:

```
./client
```


### How to use client side

When you start the client, a terminal will automatically be opened for server responses. You can use the terminal you started the process on to give commands to the server, and you can use the second terminal (opened by the program) to see all the outputs.

There are three commands available for the client:

+ `BEGIN_SESSION`
+ `SEND_MESSAGE`
+ `END_SESSION`

#### Starting a session	

>You can use the `BEGIN_SESSION` command to begin your session. This command takes a username argument as a parameter. You should provide your username after the command with a space character in between, like this:
>```
>BEGIN_SESSION <username>
>```

After you start your session, you will receive any pending messages if there are any. While you are online, all messages sent to you will immediately appear on your terminal. During this time, you can also send messages to different users.

#### Sending a message to another user

>In order to send a message to another user, you should use the `SEND_MESSAGE` command. This command takes two arguments: the username you want to send the message to and the message text itself. Here is an example of its usage:
>```
>SEND_MESSAGE burak Hello there!
>```
>This line will send the text "Hello there!" to username "burak".

#### Ending session

>You can end your session using the `END_SESSION` command. This command does not take any arguments. After using this command, you will not receive any messages until you are online again. However, your messages will still be stored by the server while you are away.

Here are some real examples:

![ObiWan terminal 1](https://github.com/burakmat/online_chat_system/blob/master/images/obiwan_terminal_1.png)

![ObiWan terminal 2](https://github.com/burakmat/online_chat_system/blob/master/images/obiwan_terminal_2.png)

![Grievous terminal 1](https://github.com/burakmat/online_chat_system/blob/master/images/grievous_terminal_1.png)

![Grievous terminal 2](https://github.com/burakmat/online_chat_system/blob/master/images/grievous_terminal_2.png)


## Design Guide

This design guide outlines the architecture and implementation of a client-server based program. The program allows multiple clients to connect to a server and communicate with each other through the server. The server handles the communication between clients by storing and forwarding messages using a linked list data structure. The client establishes a connection to the server, sends and receives messages, and displays them in real-time using a suitable terminal program.

### Thread Usage on Server Side
The server listens for incoming connections from clients in an infinite loop and creates a new thread to serve each new client that connects. This allows each client to be handled on a separate thread, which makes calculations and responses faster. The server maintains a list of connected users and their respective sockets. It is also necessary to use parallel programming at some point due to the nature of the `accept` function. When the program is waiting for connections, it is unable to execute any other tasks. Using parallel programming allows the program to perform other tasks concurrently while still being able to accept incoming connections.

### Server Structure
Messages between clients are stored and forwarded using a linked list data structure, which follows the queue principle. This allows the system to add every message to the next node and send them in order. An array of node pointer points to a new head for users' message list. These nodes contains information about message text, sender of the message, the time server got the message. Information about users and messages is stored in a structure containing multiple arrays. A parallel array structure is used to store multiple pieces of information about each user.

### Server Data Storage
To efficiently store data on the server, the program dynamically expands the server's storage space as needed. This means that the server does not allocate a fixed amount of storage space upfront. Instead, it only allocates additional space when a new user begins a session or when a new message needs to be stored.

In addition to dynamically allocating space for new users and messages, the program also frees memory after sending a message. This helps ensure that the server does not run out of storage space and can continue to function smoothly.

By using this approach, the program avoids allocating unnecessary memory, which can help improve performance and reduce resource usage. This is especially useful when the program is handling a large number of users and messages, as it can help prevent the server from running out of storage space.

### Client Side Logic

The client establishes a connection to the server and sends and receives messages. A suitable terminal program is opened (depending on the operating system) to allow the user to interact with the server and prevent problems with mixed-up output when sending and receiving messages. The client displays messages to the user in real-time. All outputs of the client are forwarded to the second terminal, and the first terminal gets no output to allow the user to give input safely.

### Redirecting the Output to the New Terminal

To obtain the tty path of the new terminal that the client has opened, the program first obtains a list of all the shell terminals by using the `ps` command. It then opens a new terminal and obtains a second list of all the terminals using the `ps` command again. The program compares the two lists and detects the new terminal by identifying the terminal that is present in the second list but not in the first list. The tty path of the new terminal can then be obtained by looking up the terminal in the second list. This tty path is used to direct the output of the client to the new terminal. On Linux machines, it opens **gnome-terminal** and on macOS machines, it opens **Terminal** application.

### Thread Usage on Client Side

The client uses two threads to handle input and output concurrently. One thread waits for user input and sends it to the server, while the other thread waits for input from the server and displays it on the second terminal. Without using threads to handle input and output concurrently, the client would have to process user input and server input sequentially. This means that the client would have to wait for the user to finish inputting a message before it can process any incoming messages from the server. Similarly, the client would have to wait for incoming messages from the server to be processed before it can prompt the user for input.

It also allows the client to handle input and output asynchronously, which makes the program more responsive and interactive.

### Interrupt Signal Handling
On both the server and client sides, interrupt signals (Ctrl+C) are handled for a clean shut-down. Without handling interrupt signals, the program would end without closing the port, which can cause problems for other programs trying to use the port. Additionally, the output buffer may be displayed multiple times before the program ends due to unexpected read values.

To handle interrupt signals, the program uses the signal function to specify a function (also known as a signal handler) to be called when an interrupt signal is received. The signal handler then performs any necessary clean-up tasks before exiting the program.


### Sending Pending Messages

Sending pending messages at the right time for each user was an important consideration during the development of this program. It was necessary to check whether the intended recipient of a message was online, and if they were, the message should be sent immediately. If the recipient was not online, the message needed to be stored until they were able to receive it.

To solve this, the program could have tried to send messages in a loop, but this would have been costly in terms of CPU usage. Instead, the program was designed to send pending messages at specific points in the program's execution. These points are when a user begins a new session, and when a user sends a message. At these points, a function is called that checks whether the intended recipients of any pending messages are online, and if they are, the messages are sent.

This approach allows the program to efficiently handle the sending of pending messages without consuming excessive resources.

