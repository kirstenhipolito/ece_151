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
#include <time.h>
#include "ece151_mp1.h"

//#define MAXLINE 1024
//#define DATALENGTH 64

int main(int argc, char* argv[]) {
	//check number of input arguments
	if (argc < 5) {
		perror("Incomplete input arguments");
		exit(EXIT_FAILURE);
	}

	//get port number from input arguments
	int portnum = 0;
	portnum = atoi(argv[2]);

	//instantiate necessary variables and buffers
	int socketfd;
	char *sendbuffer, *recvbuffer;
	struct sockaddr_in serveraddr;
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

	//initialize segsend segment (allocate memory and initialize to 0)
	struct segment *segsend;
	segsend = malloc(sizeof(struct segment));
	memset(segsend, 0, sizeof(struct segment));
	sendbuffer = malloc(BUFFERSIZE);
	segsend->head.seqnum = rand() % 8;	//set a random sequence number below 8 (capacity of header seqnum)

	//initialize segrecv segment (allocate memory and initialize to 0)
	int lenrecvd = 0;
	unsigned int lenserveraddr = sizeof(struct sockaddr);
	struct segment *segrecv;
	segrecv = malloc(sizeof(struct segment));
	memset(segrecv, 0, sizeof(struct segment));
	recvbuffer = malloc(BUFFERSIZE);

	//send SYN to server
	segsend = segment_populate(segsend, SYN, (segsend->head.seqnum)++, NULL);	//populate segment to be sent as a SYN segment
	//printf("Segment: %d, %d, %d, %s, %d\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data, segsend->segsize);
	sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
	sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
	printf("Sent SYN packet.\n");

	//wait for SYNACK to be sent
	while((segrecv->head.type) != (SYNACK)) {
		if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
			perror("Recvfrom() error");
			exit(EXIT_FAILURE);
		}
		segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
		//printf("Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data, segrecv->segsize);
		if(segment_checksum(segrecv) != segrecv->head.checksum){
			printf("Packet checksum failed.\n");
			memset(segrecv, 0, sizeof(*segrecv));
			continue;
		}
	}
	printf("Got SYNACK packet.\n");

	//get file to send to server
	FILE *fp;
	if((fp = fopen(argv[3], "r")) == NULL){
		perror("File open failure");
		exit(EXIT_FAILURE);
	}

	char filebuff[DATALENGTH];
	memset(filebuff, 0, DATALENGTH);
	
	while (fgets(filebuff, DATALENGTH, (FILE*)fp) != NULL){
		filebuff[strlen(filebuff)] = 0;
		//printf("From file: %s\n", filebuff);
		//send DATA to server
		segsend = segment_populate(segsend, DATA, (segsend->head.seqnum)++, filebuff);	//populate segment to be sent as a SYN segment
		//printf("Segment: %d, %d, %d, %s, %d\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data, segsend->segsize);
		sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
		sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
		//printf("Sent DATA packet.\n");
		memset(filebuff, 0, DATALENGTH);
		
		//wait for ACK to be sent
		while((segrecv->head.type) != (ACK)) {
			if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
				perror("Recvfrom() error");
				exit(EXIT_FAILURE);
			}
			segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
			//printf("Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data, segrecv->segsize);
			if(segment_checksum(segrecv) != segrecv->head.checksum){
				printf("Packet checksum failed.\n");
				memset(segrecv, 0, sizeof(*segrecv));
				continue;
			}
		}
		//printf("Got ACK\n");
		
	}

	//send FIN to server
	segsend = segment_populate(segsend, FIN, (segsend->head.seqnum)++, NULL);	//populate segment to be sent as a SYN segment
	//printf("Segment: %d, %d, %d, %s\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data);
	sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
	sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
	printf("Sent FIN packet.\n");

	//wait for FINACK to be sent
	while((segrecv->head.type) != (FINACK)) {
		if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
			perror("Recvfrom() error");
			exit(EXIT_FAILURE);
		}
		segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
		//printf("Segment: %d, %d, %d, %s\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data);
	}
	printf("Got FINACK packet.\n");

	free(segsend->data);
	free(segsend);
	free(sendbuffer);
	free(segrecv->data);
	free(segrecv);
	free(recvbuffer);
	//fclose(fp);
	close(socketfd);

	return 0;
}
