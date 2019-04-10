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
};

struct segment{
	struct header segheader;
	char *segdata;
};

int main(int argc, char* argv[]) {
	if (argc < 5) {
		perror("Incomplete input arguments");
		exit(EXIT_FAILURE);
	}

	int portnum = 0;
	portnum = atoi(argv[2]);

	int socketfd;
	struct sockaddr_in serveraddr;
	char buffer[MAXLINE];
	memset(buffer, 0, MAXLINE);

	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnum);
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd < 0){
		perror("Socket() system call failure");
		exit(EXIT_FAILURE);
	}

	FILE *fp;
	if((fp = fopen(argv[3], "r")) == NULL){
		perror("File open failure");
		exit(EXIT_FAILURE);
	}

	char filebuff[64];

	memset(filebuff, 0, 64);

	while (fgets(filebuff, 64, (FILE*)fp) != NULL){
		filebuff[strlen(filebuff)] = 0;
		printf("%s\n", filebuff);
		sendto(socketfd, (const char *)filebuff, strlen(filebuff),
			MSG_CONFIRM, (const struct sockaddr *) &serveraddr,
				sizeof(serveraddr));
		printf("Message sent.\n");

	}

	// n = recvfrom(socketfd, (char *)buffer, MAXLINE,
	// 			MSG_WAITALL, (struct sockaddr *) &serveraddr,
	// 			&len);
	// buffer[n] = '\0';
	// printf("Server : %s\n", buffer);

	//free(filebuff);
	fclose(fp);
	close(socketfd);
	return 0;
}
