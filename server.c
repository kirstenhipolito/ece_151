#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define MAXLINE 1024

struct header{
	uint8_t type;
	uint8_t seqnum;
	uint16_t checksum;
}

struct segment{
	struct header segheader;
	char *segdata;
}

int main(int argc, char* argv[]) {
	if (argc < 4) {
		perror("Incomplete input arguments");
		exit(EXIT_FAILURE);
	}

	int portnum = 0;
	portnum = atoi(argv[1]);

	int socketfd = 0;
	struct sockaddr_in serveraddr, clientaddr;
	char buffer[MAXLINE];
	memset(buffer, 0, MAXLINE);
	char ping[5] = "ping";

	memset(&serveraddr, 0, sizeof(serveraddr));
	memset(&clientaddr, 0, sizeof(clientaddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(portnum);

	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd < 0){
		perror("Socket() system call failure");
		exit(EXIT_FAILURE);
	}

	if (bind(socketfd, (const struct sockaddr *)&serveraddr,
			sizeof(serveraddr)) < 0 ){
		perror("Bind() system call failure");
		exit(EXIT_FAILURE);
	}

	FILE *fp;

	int lenrecvd = 0, lenclientaddr;

	while(1) {
		lenrecvd = recvfrom(socketfd, (char *)buffer, MAXLINE,
					MSG_WAITALL, ( struct sockaddr *) &clientaddr,
					&lenclientaddr);
		buffer[lenrecvd] = '\0';
		printf("Client : %s\n", buffer);

		if((fp = fopen(argv[2], "a")) == NULL){
			perror("File open failure");
			exit(EXIT_FAILURE);
		}
		fputs(buffer, fp);
		fclose(fp);
	}



	//free(filename);
	//free(proto);

	return 0;
}
