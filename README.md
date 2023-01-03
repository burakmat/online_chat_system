# Chat System User Guide
This is a guidance document for the online Chat System implemented using Network Programming. The program is capable of sending real-time messages through a client to other users, with orientation of the messages and users handled by the server.

The program works on both Linux and macOS machines.

## How to use client side

When you start the client, a terminal will automatically be opened for server responses. You can use the terminal you started the process on to give commands to the server, and you can use the second terminal (opened by the program) to see all the outputs.

There are three commands available for the client:

+ `BEGIN_SESSION`
+ `SEND_MESSAGE`
+ `END_SESSION`

### Starting a session	

>You can use the `BEGIN_SESSION` command to begin your session. This command takes a username argument as a parameter. You should provide your username after the command with a space character in between, like this:
>```
>BEGIN_SESSION <username>
>```

After you start your session, you will receive any pending messages if there are any. While you are online, all messages sent to you will immediately appear on your terminal. During this time, you can also send messages to different users.

### Sending a message to another user

>In order to send a message to another user, you should use the `SEND_MESSAGE` command. This command takes two arguments: the username you want to send the message to and the message text itself. Here is an example of its usage:
>```
>SEND_MESSAGE burak Hello there!
>```
>This line will send the text "Hello there!" to username "burak".


