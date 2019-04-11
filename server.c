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

	int lenrecvd = 0, lenclientaddr;

	//initialize segrecv segment
	struct segment *segrecv;
	segrecv = malloc(sizeof(const struct segment));
	memset(segrecv, 0, sizeof(const struct segment));

	//wait for SYN to be sent
	while((segrecv->head.type) != (0b001)){
		lenrecvd = recvfrom(socketfd, (const struct segment*)segrecv, sizeof(const struct segment),MSG_WAITALL, ( struct sockaddr *) &clientaddr, &lenclientaddr);
	}
	
	printf("Type: %d, Seqnum: %d\n", segrecv->head.type, segrecv->head.seqnum);
	printf("Data: %s\n", segrecv->data);

	//initialize segsend segment
	struct segment *segsend;
	segsend = malloc(sizeof(const struct segment));
	segsend->head.seqnum = rand() % 1000;

	//send SYNACK
	segment_populate(segsend, 0b010, (segsend->head.seqnum)++);
	//send SYNACK to client
	sendto(socketfd, (const struct segment*)segsend, sizeof(const struct segment), MSG_CONFIRM, (const struct sockaddr *) &clientaddr, sizeof(clientaddr));

	//initialize file to save to
	FILE *fp;

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

	free(segrecv);

	//free(filename);
	//free(proto);

	return 0;
}
