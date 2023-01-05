#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <signal.h>

// net_socket is a global variable that will store the socket descriptor for the network connection
int net_socket;

// t_client is a struct that stores the display_tty of the client
typedef struct s_client
{
	char *display_tty;
}	t_client;

// new_terminal_macOS creates a new terminal window and returns the tty of the new terminal window
char *new_terminal_macOS()
{
	// Get the list of ttys before opening the terminal window
	// The "ps -o tty,comm | grep zsh" command lists all of the ttys that are running zsh
	FILE* fp = popen("ps -o tty,comm | grep zsh", "r");
	if (fp == NULL)
	{
		perror("Error: popen failed");
		return NULL;
	}
	int num_tasks = 0;
	char ttys[256][256];
	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		sscanf(line, "%s", ttys[num_tasks]);
		++num_tasks;
	}
	pclose(fp);

	// Open a new terminal window
	system("open -n -a Terminal");

	// Wait for the terminal window to open
	sleep(1);

	// Get the tty of the terminal window
	fp = popen("ps -o tty,comm | grep zsh", "r");
	if (fp == NULL)
	{
		perror("Error: popen failed");
		return NULL;
	}
	char tty[256]; // Name of the tty
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		sscanf(line, "%s", tty);

		// Check if the tty is not in the list of ttys before opening the terminal window
		int found = 0;
		for (int i = 0; i < num_tasks; i++)
		{
			if (strcmp(ttys[i], tty) == 0)
			{
				found = 1;
				break;
			}
		}
		if (!found)
		{
			// The tty is the terminal window that was opened by the C program
			break;
		}
	}
	pclose(fp);

	// printf("The tty of the terminal window is: %s\n", tty);
	char *tty_str = malloc(strlen(tty) + 1);
	strcpy(tty_str, tty);

	return tty_str;
}

char *new_terminal_linux()
{
	FILE* fp = popen("ps a -o tty,comm | grep bash", "r");
	if (fp == NULL)
	{
		perror("Error: popen failed");
		return NULL;
	}
	int num_tasks = 0;
	char ttys[256][256];
	char line[256];
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		sscanf(line, "%s", ttys[num_tasks]);
		++num_tasks;
	}
	pclose(fp);

	system("gnome-terminal");

	sleep(1);

	fp = popen("ps a -o tty,comm | grep bash", "r");
	if (fp == NULL)
	{
		perror("Error: popen failed");
		return NULL;
	}
	char tty[256];
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		sscanf(line, "%s", tty);
		int found = 0;
		for (int i = 0; i < num_tasks; i++)
		{
			if (strcmp(ttys[i], tty) == 0)
			{
				found = 1;
				break;
			}
		}
		if (!found)
		{
			// The tty is the terminal window that was opened by the C program
			break;
		}
	}
	pclose(fp);

	char *tty_str = malloc(strlen(tty) + 1);
	strcpy(tty_str, tty);

	return tty_str;
}

// create_tty_path creates a path string for the given tty
char *create_tty_path(char* tty)
{
	// Allocate memory for the tty path string and create the string by concatenating "/dev/" and the tty
	char* tty_string = malloc(strlen("/dev/") + strlen(tty) + 1);
	sprintf(tty_string, "/dev/%s", tty);
	// Free the memory allocated for the tty string
	free(tty);
	return tty_string;
}

// display_incoming_message is a function that is executed by a separate thread
// It will display incoming messages from the server on the terminal window
void *display_incoming_message(void *param);

// receiver is a function that is called when the program receives a SIGINT signal (Ctrl+C)
// It sends an "END_SESSION" message to the server and closes the network socket, tty, and stdout
void receiver(int signal)
{
	write(net_socket, "END_SESSION", 12);
	close(net_socket);
	close(1);
	exit(0);
}

int main()
{
	// request is a buffer to store user input
	char request[256];
	// tid is a thread identifier for the display_incoming_message thread
	pthread_t tid;
	// client is a t_client struct to store the display_tty of the client
	t_client client;

	// Get system info to open suitable terminal
	struct utsname sys_info;
	uname(&sys_info);

	// Create the network socket
	net_socket = socket(AF_INET, SOCK_STREAM, 0);
	// Set the receiver function to be called when the program receives a SIGINT signal
	signal(SIGINT, &receiver);

	// Set up the server address to connect to
	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// Connect to the server
	int status = connect(net_socket, (struct sockaddr *)&server_address, sizeof(server_address));
	if (status == -1)
		printf("Connection error!\n");
	else
	{
		printf("Connected to the server successfully!\n");
		// Create a new terminal window and store the tty in the client struct
		char *tty;
		// Open Terminal app on macOS
		if (strcmp(sys_info.sysname, "Darwin") == 0)
			tty = new_terminal_macOS();
		// Open gnome-terminal on Linux
		else if (strcmp(sys_info.sysname, "Linux") == 0)
			tty = new_terminal_linux();
		if (tty == NULL)
			return (1);
		client.display_tty = create_tty_path(tty);
		pthread_create(&tid, NULL, &display_incoming_message, &client);
		// Read user input and send it to the server
		while (1)
		{
			read(0, request, 256);
			int i = 0;
			while (request[i] != '\n')
				++i;
			request[i] = '\0';
			write(net_socket, request, 256);
		}
	}
	return (0);
}

char* concat_strings(const char* str1, const char* str2, const char* str3) {
	// Calculate the length of the resulting string
	size_t len1 = strlen(str1);
	size_t len2 = strlen(str2);
	size_t len3 = strlen(str3);
	size_t len = len1 + len2 + len3 + 1; // +1 for the null-terminating character

	// Allocate memory for the resulting string
	char* result = malloc(len);
	if (result == NULL) {
		return NULL; // Error: malloc failed
	}

	// Concatenate the strings
	strcpy(result, str1);
	strcat(result, str2);
	strcat(result, str3);

	return result;
}


void *display_incoming_message(void *param)
{
	t_client *client = (t_client *)param;
	char message[512];
	int fd = open(client->display_tty, O_WRONLY);
	int i;

	write(fd, "\n", 1);
	// Read server output and send it to the opened terminal
	while (1)
	{
		i = read(net_socket, message, 256);
		if (i == -1)
			break ;
		else if (i == 0)
		{
			write(1, "Server connection lost\n", 24);
			write(fd, "Server connection lost\n", 24);
			close(net_socket);
			exit(0);
		}
		i = strlen(message);
		message[i] = '\n';
		message[i + 1] = '\0';
		write(fd, message, strlen(message) + 1);
	}
	return (NULL);
}
