#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
	char request[256];
	int net_socket = socket(AF_INET, SOCK_STREAM, 0);
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
		while (1)
		{

			read(0, request, 256);
			int i = 0;
			while (request[i] != '\n')
				++i;
			request[i] = '\0';
			write(net_socket, request, 256);
			read(net_socket, request, 256);
			printf("Server: %s\n", request);
		}
	}
	close(net_socket);
	return (0);
}
