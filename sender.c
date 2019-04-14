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
#include <signal.h>
#include <errno.h>
#include "ece151_mp1.h"

//#define MAXLINE 1024
//#define DATALENGTH 64

int main(int argc, char* argv[]) {
	//check number of input arguments
    if (argc != 5) {
        fprintf(stderr,"Usage: sender <dest_IP> <dest_port> <filename> <proto>\n");
        exit(1);
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
		perror("Socket() system call failure\n");
		exit(EXIT_FAILURE);
	}

	//seed randomizer
	srand(57);

	//initialize segsend segment (allocate memory and initialize to 0)
	struct segment *segsend;
	segsend = malloc(sizeof(struct segment));
	memset(segsend, 0, sizeof(struct segment));
	sendbuffer = malloc(BUFFERSIZE);
	segsend->head.seqnum = rand() % 256;	//set a random sequence number below 256 (capacity of header seqnum)

	//initialize segrecv segment (allocate memory and initialize to 0)
	int lenrecvd = 0;
	unsigned int lenserveraddr = sizeof(struct sockaddr);
	struct segment *segrecv;
	segrecv = malloc(sizeof(struct segment));
	memset(segrecv, 0, sizeof(struct segment));
	recvbuffer = malloc(BUFFERSIZE);

	//initialize timeout alarm
	signal(SIGALRM, sig_alrm);
	siginterrupt(SIGALRM, 1);

	//create SYN packet			
	segsend = segment_populate(segsend, SYN, (segsend->head.seqnum)++, NULL);	//populate segment to be sent as a SYN segment
	//printf("SYN Segment: %d, %d, %d, %s, %d\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data, segsend->segsize);
	sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
	
	while((segrecv->head.type) != (SYNACK)) {
		perhaps_sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
		printf("Sent SYN packet.\n");
		
		//wait for SYNACK to be sent		
		alarm(TIMEOUT);
		if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
			if (errno != EINTR){
				perror("Recvfrom() error\n");
				exit(EXIT_FAILURE);
			}else
				printf("Timeout occured. Retransmitting Packet...\n");
		}else{
			segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
			//printf("Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data, segrecv->segsize);
			if(segment_checksum(segrecv) != segrecv->head.checksum){
				printf("Packet checksum failed. Retransmitting Packet...\n");
				memset(segrecv, 0, sizeof(*segrecv));
				continue;
			}else{
				if ((segrecv->head.type) == RST){
					perror("Connection Rejected\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
	memset(segrecv, 0, sizeof(*segrecv));
	
	printf("Got SYNACK packet.\n");

	//get file to send to server
	FILE *fp;
	if((fp = fopen(argv[3], "r")) == NULL){
		perror("File open failure\n");
		exit(EXIT_FAILURE);
	}

	char filebuff[DATALENGTH];
	memset(filebuff, 0, DATALENGTH);
	
	uint8_t seqnuminc = rand()%256;
	int i = 0;
	char charbuff = 0;
	
	while (!(feof(fp))){
		memset(filebuff, 0, DATALENGTH);
		for (i = 0; i <DATALENGTH; i++){
			charbuff = fgetc(fp);
			if(feof(fp)){
				filebuff[i]=0;
				break;
			}
			filebuff[i] = charbuff;
		}
		//printf("From file: %s\n", filebuff);
		
		//create DATA packet
		segsend = segment_populate(segsend, DATA, seqnuminc, filebuff);	//populate segment to be sent as a DATA segment
		//printf("Send DATA Segment: %d, %d, %d, %s, %d\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data, segsend->segsize);
		sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
		
		
		while((segrecv->head.type) != (ACK)) {
			//send DATA to server
			perhaps_sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
			printf("Sent DATA packet %d.\n", segsend->head.seqnum);
			memset(filebuff, 0, DATALENGTH);
			
			alarm(TIMEOUT);
			//wait for ACK to be sent
			if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
				if (errno != EINTR){
					perror("Recvfrom() error\n");
					exit(EXIT_FAILURE);
				}else
					printf("Timeout occured. Retransmitting Packet...\n");
			}else{
				segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
				//printf("Recv ACK Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data, segrecv->segsize);
				if(segment_checksum(segrecv) != segrecv->head.checksum){
					printf("Packet checksum failed.\n");
					memset(segrecv, 0, sizeof(*segrecv));
					continue;
				}
			}
		}
		printf("Got ACK packet %d.\n", segrecv->head.seqnum);
		memset(segrecv, 0, sizeof(*segrecv));
		//printf("Got ACK\n");
		seqnuminc++;
		
	}

	//create FIN packet
	segsend = segment_populate(segsend, FIN, (segsend->head.seqnum)++, NULL);	//populate segment to be sent as a SYN segment
	//printf("Segment: %d, %d, %d, %s\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data);
	sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
	
	while((segrecv->head.type) != (FINACK)) {
		//send FIN to server
		sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
		printf("Sent FIN packet.\n");
	
		alarm(TIMEOUT);
		
		//wait for FINACK to be sent
		if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
			if (errno != EINTR){
				perror("Recvfrom() error\n");
				exit(EXIT_FAILURE);
			}else
				printf("Timeout occured. Retransmitting Packet...\n");
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
	fclose(fp);
	close(socketfd);

	return 0;
}