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

int main(int argc, char* argv[]) {
	//check number of input arguments
    if (argc != 4) {
        fprintf(stderr,"Usage: receiver <src_port> <filename> <proto>\n");
        exit(1);
    }

	//get port number from input arguments
	int portnum = 0;
	portnum = atoi(argv[1]);

	//instantiate necessary variables and buffers
	int socketfd;
	char *sendbuffer, *recvbuffer;
	struct sockaddr_in serveraddr, clientaddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	memset(&clientaddr, 0, sizeof(clientaddr));

	//populate server address data
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(portnum);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	//create socket
	socketfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (socketfd < 0){
		perror("Socket() system call failure");
		exit(EXIT_FAILURE);
	}

	//bind socket
	if (bind(socketfd, (const struct sockaddr *)&serveraddr,
			sizeof(serveraddr)) < 0 ){
		perror("Bind() system call failure");
		exit(EXIT_FAILURE);
	}

	//initialize segsend segment (allocate memory and initialize to 0)
	struct segment *segsend;
	segsend = malloc(sizeof(struct segment));
	memset(segsend, 0, sizeof(struct segment));
	sendbuffer = malloc(BUFFERSIZE);
	segsend->head.seqnum = rand() % 256;	//set a random sequence number below 256 (capacity of header seqnum)

	//initialize segrecv segment (allocate memory and initialize to 0)
	int lenrecvd = 0;
	unsigned int lenclientaddr = sizeof(struct sockaddr);
	struct segment *segrecv;
	segrecv = malloc(sizeof(struct segment));
	memset(segrecv, 0, sizeof(struct segment));
	recvbuffer = malloc(BUFFERSIZE);
	
	//wait for SYN to be sent
	while((segrecv->head.type) != (SYN)){
		if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, (struct sockaddr *) &clientaddr, &lenclientaddr)) == -1){
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
	printf("Got SYN packet.\n");

	//send SYNACK to client
	segsend = segment_populate(segsend, SYNACK, segrecv->head.seqnum, NULL);
	sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
	//printf("Segment: %d, %d, %d, %s, %d\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data, segsend->segsize);
	if((perhaps_sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &clientaddr, lenclientaddr)) == -1){
		perror("Sendto() error");
		exit(EXIT_FAILURE);
	}
	printf("Sent SYNACK packet.\n");

	//initialize file to save to
	FILE *fp;
	if((fp = fopen(argv[2], "w")) == NULL){
		perror("File open failure");
		exit(EXIT_FAILURE);
	}
	
	//uint8_t seqnuminc = rand()%256;
	
	//receive file, continue receiving while FIN has not been sent
	while((segrecv->head.type) != FIN) {
		if((segrecv->segsize) - sizeof(*segrecv))	free(segrecv->data);
		memset(segrecv, 0, sizeof(*segrecv));
		if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, (struct sockaddr *) &clientaddr, &lenclientaddr)) == -1){
			perror("Recvfrom() error");
			exit(EXIT_FAILURE);
		}
		segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
			
		if ((segrecv->head.type) != FIN)
			printf("Got DATA packet %d.\n", segrecv->head.seqnum);
			
		//printf("RECV Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data, segrecv->segsize);
		if(segment_checksum(segrecv) != segrecv->head.checksum){
			printf("Packet checksum failed.\n");
			memset(segrecv, 0, sizeof(*segrecv));
			continue;
		}
		//printf("Got DATA packet\n");
	
		//if packet data is not NULL, append to file and send an ACK
		if(segrecv->segsize > sizeof(*segrecv)){
			fputs(segrecv->data, fp);
			
			//send ACK to client
			segsend = segment_populate(segsend, ACK,  segrecv->head.seqnum, NULL);
			sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
			//printf("Sent ACK Segment: %d, %d, %d, %s\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data);
			if((perhaps_sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &clientaddr, lenclientaddr)) == -1){
				perror("Sendto() error");
				exit(EXIT_FAILURE);
			}
			printf("Sent ACK packet %d.\n", segsend->head.seqnum);
			//printf("Sent ACK\n");
			//seqnuminc++;
		}
		
		
	}
	
	fclose(fp);

	//wait for FIN to be sent
	while((segrecv->head.type) != (FIN)){
		if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, (struct sockaddr *) &clientaddr, &lenclientaddr)) == -1){
			perror("Recvfrom() error");
			exit(EXIT_FAILURE);
		}
		segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
		//printf("Segment: %d, %d, %d, %s\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data);
	}
	printf("Got FIN packet.\n");

	//send FINACK to client
	segsend = segment_populate(segsend, FINACK, (segsend->head.seqnum)++, NULL);
	sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
	//printf("Segment: %d, %d, %d, %s\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data);
	if((sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &clientaddr, lenclientaddr)) == -1){
		perror("Sendto() error");
		exit(EXIT_FAILURE);
	}
	printf("Sent FINACK packet.\n");


	free(segsend->data);
	free(segsend);
	free(sendbuffer);
	free(segrecv->data);
	free(segrecv);
	free(recvbuffer);
	close(socketfd);

	return 0;
}