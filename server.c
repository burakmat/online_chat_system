#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>

// Global variable for server socket descriptor
int server_socket;

typedef struct s_messages
{
	// Message content
	char message[256];
	// Name of sender
	char from[32];
	// Time message was sent
	time_t raw_time;
	// Pointer to next message in linked list
	struct s_messages *next;
}   t_messages;


// Function for finding a client in the server's list of clients
typedef struct s_server
{
	int last_client_socket;
	int user_count;
	char **users;
	int *number_of_messages;
	int *user_status;
	int *user_fd;
	t_messages **messages;
}   t_server;

void send_pending_messages(t_server *server, int user_id, int first);
char *receive_confirm(char *name, char *msg);

// Function for finding a client in the server's list of clients
int find_user(t_server *server, char *name, int *user_id)
{
	if (server->users == NULL)
	{
		++server->user_count;
		// Allocate memory for the list of user names
		server->users = malloc(sizeof(char *) * 2);
		// Allocate memory for the new user's name
		server->users[0] = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(server->users[0], name);
		// Set the end of the list
		server->users[1] = NULL;
		// Allocate memory for the number of messages for each user
		server->number_of_messages = malloc(sizeof(int));
		// Set the number of messages for the new user to 0
		server->number_of_messages[0] = 0;
		// Allocate memory for the user status (online/offline)
		server->user_status = malloc(sizeof(int));
		// Allocate memory for the file descriptor of each user
		server->user_fd = malloc(sizeof(int));
		// Allocate memory for the list of messages for each user
		server->messages = malloc(sizeof(t_messages *));
		// Set the user ID for the new user
		server->messages[0] = NULL;
		*user_id = 0;
		return (0);
	}
	else
	{
		int i = 0;

		while (server->users[i])
		{
			// Check if the user already exists
			if (strcmp(server->users[i], name) == 0)
			{
				// Set the user ID for the existing user
				*user_id = i;
				return (1);
			}
			++i;
		}
		++server->user_count;
		// Allocate memory for the new list of user names
		char **tmp = malloc(sizeof(char *) * (i + 2));
		i = 0;
		while (server->users[i])
		{
			// Copy the current user names into the new list
			tmp[i] = server->users[i];
			++i;
		}
		tmp[i + 1] = server->users[i];
		// Allocate memory for the new user's name
		tmp[i] = malloc(sizeof(char) * (strlen(name) + 1));
		// Copy the new user's name into the list
		strcpy(tmp[i], name);
		// Free the memory allocated for the current list of user names
		free(server->users);
		// Set the new list of user names
		server->users = tmp;

		// Allocate memory for temporary array of ints to store values of server->number_of_messages
		int *tmpnum = malloc(sizeof(int) * server->user_count);
		// Copy values from server->number_of_messages to tmpnum
		for (int i = 0; i < server->user_count - 1; ++i)
			tmpnum[i] = server->number_of_messages[i];
		free(server->number_of_messages);
		tmpnum[server->user_count - 1] = 0;
		server->number_of_messages = tmpnum;

		int n = 0;
		// Allocate memory for temporary array of ints to store values of server->user_status
		int *tmp_status = malloc(sizeof(int) * server->user_count);
		while (n < server->user_count - 1)
		{
			tmp_status[n] = server->user_status[n];
			++n;
		}
		free(server->user_status);
		server->user_status = tmp_status;

		// Allocate memory for temporary array of ints to store values of server->user_fd
		tmp_status = malloc(sizeof(int) * server->user_count);
		n = 0;
		while (n < server->user_count - 1)
		{
			tmp_status[n] = server->user_fd[n];
			++n;
		}
		free(server->user_fd);
		server->user_fd = tmp_status;

		// Allocate memory for temporary array of pointers to t_messages to store values of server->messages
		t_messages **tmpmsg = malloc(sizeof(t_messages *) * (server->user_count));
		i = 0;
		while (i < server->user_count - 1)
		{
			tmpmsg[i] = server->messages[i];
			++i;
		}
		tmpmsg[i] = NULL;
		free(server->messages);
		server->messages = tmpmsg;
		*user_id = i;
		return (0);
	}
}

// Read the user input from the client socket
int read_protocol(int sock, t_server *server, int *user_id)
{
	char user_input[256];
	int buff = read(sock, user_input, 256);
	// Checking whether client side exit with interrupt signal
	if (buff <= 0)
	{
		if (*user_id != -1)
			server->user_status[*user_id] = 0;
		printf("Client disconnected\n");
		return (1);
	}
	// If the user input starts with "BEGIN_SESSION ", it's a request to start a new session
	if (strncmp(user_input, "BEGIN_SESSION ", 14) == 0)
	{
		// If the user already has an active session, send an error message
		if (*user_id != -1)
		{
			write(sock, "You should end the active session to begin another", 51);
			return (0);
		}
		// Find the user's id and set their status to active
		if (find_user(server, user_input + 14, user_id) && server->user_status[*user_id] == 1)
		{
			// Do not let user to login if the user has already an active session
			write(sock, "This user already have an active session", 41);
			*user_id = -1;
			return (0);
		}
		server->user_status[*user_id] = 1;
		server->user_fd[*user_id] = sock;
		for (int i = 0; i < server->user_count; ++i)
		{
			if (strcmp(server->users[i], user_input + 14) == 0)
			{
				if (server->number_of_messages[*user_id] == 0)
				{
					write(sock, "Session Started\nYou do not have any pending messages", 53);
				}
				// Send any pending messages to the user
				else
				{
					send_pending_messages(server, *user_id, 1);
				}
				return 0;
			}
		}
	}
	// If the user input starts with "SEND_MESSAGE ", it's a request to send a message to another user
	else if (strncmp(user_input, "SEND_MESSAGE ", 13) == 0)
	{
		// If the user doesn't have an active session, send an error message
		if (*user_id == -1)
		{
			write(sock, "You need to have an active session to send message", 51);
			return (0);
		}
		// Extract the recipient's username and find their id
		char to_send[32];
		int send_id;
		int i = 0;
		while (user_input[i + 13] != ' ')
		{
			to_send[i] = user_input[i + 13];
			++i;
		}
		to_send[i] = '\0';
		find_user(server, to_send, &send_id);
		// Add the message to the recipient's message list
		if (server->messages[send_id] == NULL)
		{
			server->messages[send_id] = malloc(sizeof(t_messages));
			strncpy(server->messages[send_id]->message, user_input + i + 14, 224);
			server->messages[send_id]->next = NULL;
			strcpy(server->messages[send_id]->from, server->users[*user_id]);
			time(&server->messages[send_id]->raw_time);
		}
		else
		{
			t_messages *current = server->messages[send_id];
			while (current->next != NULL)
				current = current->next;
			current->next = malloc(sizeof(t_messages));
			strncpy(current->next->message, user_input + i + 14, 224);
			current->next->next = NULL;
			strcpy(current->next->from, server->users[*user_id]);
			time(&current->next->raw_time);
		}
		++server->number_of_messages[send_id];
		char *confirm = receive_confirm(to_send, user_input + i + 14);
		write(sock, confirm, strlen(confirm) + 1);
		send_pending_messages(server, *user_id, 0);
		free(confirm);
		return 0;
	}
	else if (strncmp(user_input, "END_SESSION", 12) == 0)
	{
		if (*user_id == -1)
		{
			write(sock, "There is no active session to end", 34);
			return (0);
		}
		// Set the user's status to offline and close their socket
		server->user_status[*user_id] = 0;
		write(sock, "Session ended", 14);
		return 1;
	}
	else
	{
		// If the command is not recognized, send an error message
		write(sock, "Message Not Understood", 23);
		return 0;
	}
	return (0);
}

void *client_service(void *param)
{
	// Get the client socket and t_server struct from the param parameter
	int client_socket = ((t_server *)param)->last_client_socket;
	t_server *server = (t_server *)param;
	int user_id = -1;

	// Read and process the client's requests in a loop
	while (1)
	{
		if (read_protocol(client_socket, server, &user_id))
			break ;
	}    
	close(client_socket);
	return (NULL);
}

char *receive_confirm(char *name, char *msg)
{
	// Calculate the length of the name and message strings
	int name_len = strlen(name);
	int msg_len = strlen(msg);
	// Allocate memory for the confirmation message
	char *message = malloc(sizeof(char) * (name_len + msg_len + 19));
	// Create the confirmation message
	sprintf(message, "Message sent to %s: %s", name, msg);
	return (message);
}

void send_pending_messages(t_server *server, int user_id, int first)
{
	int i = 0;

	// Loop through all the users in the server
	while (i < server->user_count)
	{
		// If the user has pending messages and is currently online
		if (server->number_of_messages[i] > 0 && server->user_status[i] == 1)
		{
			// Set current to the first message in the user's message list
			t_messages *current = server->messages[i];
			// Declare a buffer to store the message to be sent
			char *send_package;
			// Loop through all the messages in the user's message list
			while (current != NULL)
			{
				usleep(500);
				// If this is the first message to be sent, create a "Session Started" message
				if (first)
				{
					// Allocate memory for the send_package buffer and create the "Session Started" message
					send_package = malloc(sizeof(char) * (28 + strlen(current->from) + strlen(current->message)));
					sprintf(send_package, "Session Started\n%02d:%02d:%02d-%s: %s", localtime(&current->raw_time)->tm_hour, localtime(&current->raw_time)->tm_min, localtime(&current->raw_time)->tm_sec, current->from, current->message);
					// Set first to 0 to indicate that the first message has been sent
					first = 0;
				}
				// If this is not the first message, create a regular message
				else
				{
					send_package = malloc(sizeof(char) * (12 + strlen(current->from) + strlen(current->message)));
					sprintf(send_package, "%02d:%02d:%02d-%s: %s", localtime(&current->raw_time)->tm_hour, localtime(&current->raw_time)->tm_min, localtime(&current->raw_time)->tm_sec, current->from, current->message);
				}
				// Write the message to the user's file descriptor
				write(server->user_fd[i], send_package, strlen(send_package) + 1);
				free(send_package);
	            // Move to the next message in the user's message list
				server->messages[i] = server->messages[i]->next;
				free(current);
				current = server->messages[i];
			}
        	// Set the number of messages for the user to 0
			server->number_of_messages[i] = 0;
		}
		++i;
	}
}

// Signal handler function
void receiver(int signal)
{
	close(server_socket);
	exit(0);
}

int main()
{
	// Set the receiver function to be called when the program receives a SIGINT signal (Ctrl+C)
	signal(SIGINT, &receiver);

	// Create a socket for the server
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket Error");
		exit(1);
	}

	// Set up the server address to bind to
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// Bind the socket to the server address
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Bind Error");
		exit(1);
	}

	// Start listening for incoming connections
	listen(server_socket, 20);

	// Declare variables for the client socket, thread identifier, and t_server struct
	int client_socket;
	pthread_t tid;
	t_server server;

	// Initialize the fields of the t_server struct
	server.users = NULL;
	server.number_of_messages = NULL;
	server.user_count = 0;

	
	// Accept incoming connections in a loop
	while (1)
	{
		printf("Waiting for a client to connect...\n");
		
		// Accept an incoming connection and store the client socket descriptor in the t_server struct
		server.last_client_socket = accept(server_socket, NULL, NULL);
		if (server.last_client_socket < 0)
		{
			perror("Accept Error");
			exit(1);
		}
		printf("New client has been connected\n");
		// Start a new thread to handle the client's requests
		pthread_create(&tid, NULL, &client_service, &server);
	}

	return (0);
}


