#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <fcntl.h>

int net_socket;

typedef struct s_client
{
	char *display_tty;
}	t_client;


char *new_terminal()
{
    // Get the list of ttys before opening the terminal window
    char command[256];
    sprintf(command, "ps -o tty,comm | grep zsh");
    FILE* fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error: popen failed");
        return NULL;
    }
    int num_tasks = 0;
    char ttys[256][256];
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        char tty[256];
        sscanf(line, "%s zsh", tty);
        strcpy(ttys[num_tasks], tty);
        num_tasks++;
    }
    pclose(fp);

    // Open a new terminal window
    system("open -n -a Terminal");

    // Wait for the terminal window to open
    sleep(1);

    // Get the tty of the terminal window
    sprintf(command, "ps -o tty,comm | grep zsh");
    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error: popen failed");
        return NULL;
    }
    char tty[256]; // name of the tty
    while (fgets(line, sizeof(line), fp) != NULL) {
        int n = sscanf(line, "%s zsh", tty);
        if (n != 1) {
            continue;
        }
        // Check if the tty is not in the list of ttys before opening the terminal window
        int found = 0;
        for (int i = 0; i < num_tasks; i++) {
            if (strcmp(ttys[i], tty) == 0) {
                found = 1;
                break;
            }
        }
        if (!found) {
            // The tty is the terminal window that was opened by the C program
            break;
        }
    }
    pclose(fp);

    // printf("The tty of the terminal window is: %s\n", tty);
    char *tty_str = malloc(strlen(tty) + 1);
    strcpy(tty_str, tty);

	printf("tty_str: %s\n", tty_str);

    // Return the tty string
    return tty_str;
}

char *create_tty_path(char* tty)
{
    char* tty_string = malloc(strlen("/dev/") + strlen(tty) + 1);
    sprintf(tty_string, "/dev/%s", tty);
    free(tty);
    return tty_string;
}


void *display_incoming_message(void *param);

void receiver(int signal)
{
	write(net_socket, "END_SESSION", 12);
	close(net_socket);
	exit(0);
}

int main()
{
	char request[256];
	pthread_t tid;
	t_client client;
	net_socket = socket(AF_INET, SOCK_STREAM, 0);
	signal(SIGINT, &receiver);

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	int status = connect(net_socket, (struct sockaddr *)&server_address, sizeof(server_address));
	if (status == -1)
		printf("Connection error!\n");
	else
	{
		printf("Connected to the server successfully!\n");
		char *tty = new_terminal();
		if (tty == NULL)
			return (1);
		client.display_tty = create_tty_path(tty);
		pthread_create(&tid, NULL, &display_incoming_message, &client);// add argument
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

	while (1)
	{
		i = read(net_socket, message, 256);
		if (i == 0)
		{
			write(1, "Server connection lost\n", 24);
			write(fd, "Server connection lost\n", 24);
			close(net_socket);
			exit(0);
		}
		i = strlen(message);
		message[i] = '\n';
		message[i + 1] = '\0';
		printf("what i sent: %s\n", message);
		write(fd, message, strlen(message) + 1);
	}
}
