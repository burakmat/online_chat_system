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

int server_socket;

typedef struct s_messages
{
	char message[256];
	char from[32];
	time_t raw_time;
	struct s_messages *next;
}   t_messages;



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

int find_user(t_server *server, char *name, int *user_id)
{
	if (server->users == NULL) //first_user
	{
		++server->user_count;
		server->users = malloc(sizeof(char *) * 2);
		server->users[0] = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(server->users[0], name);
		server->users[1] = NULL;
		server->number_of_messages = malloc(sizeof(int));
		server->number_of_messages[0] = 0;
		server->user_status = malloc(sizeof(int));
		server->user_fd = malloc(sizeof(int));
		server->messages = malloc(sizeof(t_messages *));
		server->messages[0] = NULL;
		*user_id = 0;
		return (0);
	}
	else
	{
		int i = 0;

		while (server->users[i])
		{
			if (strcmp(server->users[i], name) == 0) //already exist user
			{
				*user_id = i;
				return (1);
			}
			++i;
		}
		++server->user_count;
		char **tmp = malloc(sizeof(char *) * (i + 2));
		i = 0;
		while (server->users[i])
		{
			tmp[i] = server->users[i];
			++i;
		}
		tmp[i + 1] = server->users[i];
		tmp[i] = malloc(sizeof(char) * (strlen(name) + 1));
		strcpy(tmp[i], name);
		free(server->users);
		server->users = tmp;

		int *tmpnum = malloc(sizeof(int) * server->user_count);
		for (int i = 0; i < server->user_count - 1; ++i)
			tmpnum[i] = server->number_of_messages[i];
		free(server->number_of_messages);
		tmpnum[server->user_count - 1] = 0;
		server->number_of_messages = tmpnum;

		int n = 0;
		int *tmp_status = malloc(sizeof(int) * server->user_count);
		while (n < server->user_count - 1)
		{
			tmp_status[n] = server->user_status[n];
			++n;
		}
		free(server->user_status);
		server->user_status = tmp_status;

		tmp_status = malloc(sizeof(int) * server->user_count);
		n = 0;
		while (n < server->user_count - 1)
		{
			tmp_status[n] = server->user_fd[n];
			++n;
		}
		free(server->user_fd);
		server->user_fd = tmp_status;

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

int read_protocol(int sock, t_server *server, int *user_id)
{
	char user_input[256];
	int buff = read(sock, user_input, 256);
	if (buff <= 0)
	{
		printf("Client disconnected\n");
		return (1);
	}
	if (strncmp(user_input, "BEGIN_SESSION ", 14) == 0)
	{
		if (*user_id != -1)
		{
			write(sock, "You should end the active session to begin another", 51);
			return (0);
		}
		find_user(server, user_input + 14, user_id);// find user's id
		server->user_status[*user_id] = 1;
		server->user_fd[*user_id] = sock;
		// write(sock, "Session Started\n", 17);
		// usleep(5000);
		for (int i = 0; i < server->user_count; ++i)
		{
			if (strcmp(server->users[i], user_input + 14) == 0)
			{
				if (server->number_of_messages[*user_id] == 0)
				{
					write(sock, "Session Started\nYou do not have any pending messages", 53);
				}
				else
				{
					send_pending_messages(server, *user_id, 1);
				}
				return 0;
			}
		}
	}
	else if (strncmp(user_input, "SEND_MESSAGE ", 13) == 0)
	{
		if (*user_id == -1)
		{
			write(sock, "You need to have an active session to send message", 51);
			return (0);
		}
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
		if (server->messages[send_id] == NULL)
		{
			server->messages[send_id] = malloc(sizeof(t_messages));
			strncpy(server->messages[send_id]->message, user_input + i + 14, 224);
			server->messages[send_id]->next = NULL;
			strcpy(server->messages[send_id]->from, server->users[*user_id]);
			// printf("first message sent by %s\n", server->messages[send_id]->from);
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
			// printf("sent by %s\n", server->messages[send_id]->from);
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
		server->user_status[*user_id] = 0;
		write(sock, "Session ended", 14);
		*user_id = -1;
		return 0;
	}
	else
	{
		write(sock, "Message Not Understood", 23);
		return 0;
	}
	return (0);
}

void *client_service(void *param)
{
	int client_socket = ((t_server *)param)->last_client_socket;
	t_server *server = (t_server *)param;
	int user_id = -1;

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
	int name_len = strlen(name);
	int msg_len = strlen(msg);
	char *message = malloc(sizeof(char) * (name_len + msg_len + 19));
	sprintf(message, "Message sent to %s: %s", name, msg);
	return (message);
}

void send_pending_messages(t_server *server, int user_id, int first)
{
	int i = 0;

	while (i < server->user_count)
	{
		if (server->number_of_messages[i] > 0 && server->user_status[i] == 1)
		{
			t_messages *current = server->messages[i];
			char *send_package;
			while (current != NULL)
			{
				usleep(500);
				if (first)
				{
					send_package = malloc(sizeof(char) * (28 + strlen(current->from) + strlen(current->message)));
					sprintf(send_package, "Session Started\n%02d:%02d:%02d-%s: %s", localtime(&current->raw_time)->tm_hour, localtime(&current->raw_time)->tm_min, localtime(&current->raw_time)->tm_sec, current->from, current->message);
					first = 0;
				}
				else
				{
					send_package = malloc(sizeof(char) * (12 + strlen(current->from) + strlen(current->message)));
					sprintf(send_package, "%02d:%02d:%02d-%s: %s", localtime(&current->raw_time)->tm_hour, localtime(&current->raw_time)->tm_min, localtime(&current->raw_time)->tm_sec, current->from, current->message);
				}

				write(server->user_fd[i], send_package, strlen(send_package) + 1);
				free(send_package);
				server->messages[i] = server->messages[i]->next;
				free(current);
				current = server->messages[i];
			}
			server->number_of_messages[i] = 0;
		}
		++i;
	}
}

void receiver(int signal)
{
	close(server_socket);
	exit(0);
}

int main()
{
	signal(SIGINT, &receiver);
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("Socket Error");
		exit(1);
	}

	struct sockaddr_in server_address;
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(9002);
	server_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("Bind Error");
		exit(1);
	}

	listen(server_socket, 10);

	int client_socket;
	pthread_t tid;
	t_server server;
	server.users = NULL;
	server.number_of_messages = NULL;
	server.user_count = 0;

	while (1)
	{
		printf("Waiting for a client to connect...\n");
		server.last_client_socket = accept(server_socket, NULL, NULL);
		if (server.last_client_socket < 0)
		{
			perror("Accept Error");
			exit(1);
		}
		printf("New client has been connected\n");
		pthread_create(&tid, NULL, &client_service, &server);
	}

	return (0);
}


