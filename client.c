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
#include "ece151_mp1.h"

#define MAXLINE 1024



int main(int argc, char* argv[]) {
	//check number of input arguments
	if (argc < 5) {
		perror("Incomplete input arguments");
		exit(EXIT_FAILURE);
	}

	//get port number from input arguments
	int portnum = 0;
	portnum = atoi(argv[2]);

	int socketfd;
	struct sockaddr_in serveraddr;
	char buffer[MAXLINE];
	memset(buffer, 0, MAXLINE);
	memset(&serveraddr, 0, sizeof(serveraddr));

	//populate server address data
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnum);
	serveraddr.sin_addr.s_addr = inet_addr(argv[1]);

	//create socket
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd < 0){
		perror("Socket() system call failure");
		exit(EXIT_FAILURE);
	}

	//seed randomizer
	srand(57);
	
	//initialize segsend segment
	struct segment *segsend;
	segsend = malloc(sizeof(const struct segment));
	segsend->head.seqnum = rand() % 1000;

	//send SYN to server
	segment_populate(segsend, 0b001, (segsend->head.seqnum)++);
	sendto(socketfd, (const struct segment*)segsend, sizeof(const struct segment), MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));

	//initialize segrecv segment
	int lenrecvd = 0, lenserveraddr;
	struct segment *segrecv;
	segrecv = malloc(sizeof(const struct segment));
	memset(segrecv, 0, sizeof(const struct segment));

	//wait for SYNACK to be sent
	while((segrecv->head.type) != (0b010)){
		lenrecvd = recvfrom(socketfd, (const struct segment*)segrecv, sizeof(const struct segment),MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr);
	}

	printf("Type: %d, Seqnum: %d\n", segrecv->head.type, segrecv->head.seqnum);



	//get file to send to server
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
	free(segsend);
	fclose(fp);
	close(socketfd);
	return 0;
}
