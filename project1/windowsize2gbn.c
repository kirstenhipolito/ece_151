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
#include "ece151_mp1.h"

//#define MAXLINE 1024
//#define DATALENGTH 64
int checkACK, checkACK2, sentSEQ, sentSEQ0, sentSEQ1,sentSEQ2,n;
int ctr;		//flag if data is sent 6 times
int seqnum1=0;
int seqnum2=0;

char packet1buffer[DATALENGTH];//for
char packet2buffer[DATALENGTH];//window
char packet3buffer[DATALENGTH];//size=2

//interrupt handler of alarm when timer runs out
void alarm_handler()
{ 
	alarm(3);
}


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
	char *sendbuffer1;
	char *sendbuffer2;
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
	segsend->head.seqnum = rand() % 256;	//set a random sequence number below 256 (capacity of header seqnum)

	//initialize segsend segment 1(allocate memory and initialize to 0)
	struct segment *segsend1;
	segsend1 = malloc(sizeof(struct segment));
	memset(segsend1, 0, sizeof(struct segment));
	sendbuffer1 = malloc(BUFFERSIZE);
	segsend1->head.seqnum = rand() % 256;	//set a random sequence number below 256 (capacity of header seqnum)

	//initialize segsend segment 2(allocate memory and initialize to 0)
	struct segment *segsend2;
	segsend1 = malloc(sizeof(struct segment));
	memset(segsend2, 0, sizeof(struct segment));
	sendbuffer2 = malloc(BUFFERSIZE);
	segsend2->head.seqnum = rand() % 256;	//set a random sequence number below 256 (capacity of header seqnum)

	//initialize segrecv segment (allocate memory and initialize to 0)
	int lenrecvd = 0;
	unsigned int lenserveraddr = sizeof(struct sockaddr);
	struct segment *segrecv;
	segrecv = malloc(sizeof(struct segment));
	memset(segrecv, 0, sizeof(struct segment));
	recvbuffer = malloc(BUFFERSIZE);
	


	//send SYN to server
	segsend = segment_populate(segsend, SYN, (segsend->head.seqnum)++, NULL);	//populate segment to be sent as a SYN segment
	printf("SYN Segment: %d, %d, %d, %s, %d\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data, segsend->segsize);
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
		printf("Sending SYNACK");
		printf("Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data, segrecv->segsize);
		if(segment_checksum(segrecv) != segrecv->head.checksum){
			printf("Packet checksum failed.\n");
			memset(segrecv, 0, sizeof(*segrecv));
			continue;
		}
	}
	memset(segrecv, 0, sizeof(*segrecv));
	
	printf("Got SYNACK packet.\n");

	//get file to send to server
	FILE *fp;
	if((fp = fopen(argv[3], "r")) == NULL){
		perror("File open failure");
		exit(EXIT_FAILURE);
	}

	char filebuff[DATALENGTH];
	memset(filebuff, 0, DATALENGTH);
	
	uint8_t seqnuminc = rand()%256;
	int i = 0;
	char charbuff = 0;
	n=1;
	ctr=0;
	while (!(feof(fp)))
	{
	
		//send DATA to server
	if (n=1){ //in slow mode
		//get data from file
		memset(filebuff, 0, DATALENGTH);
		for (i = 0; i <DATALENGTH; i++){
			charbuff = fgetc(fp);
			if(feof(fp)){
				filebuff[i]=0;
				break;
			}
			filebuff[i] = charbuff;
		}
		printf("From file: %s\n", filebuff);
		
		while(1)
		{
		
		SLOWMODE:	
		if (strlen(packet1buffer)!=0)
		{
			for (i=0;i<DATALENGTH;i++) {
				filebuff[i]=packet1buffer[i];
			}
			seqnuminc=seqnum1;
		}
		signal(SIGALRM,alarm_handler);
		alarm(3);			//initial alarm
		segsend = segment_populate(segsend, DATA, seqnuminc, filebuff);	//populate segment to be sent as a DATA segment
		printf("Send DATA Segment: %d, %d, %d, %s, %d\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data, segsend->segsize);
		sentSEQ=segsend->head.seqnum;
		printf("Seqnum sent is %d\n",sentSEQ);
		ctr++;		//flag if data is sent 6 times
		if(ctr==6){
			exit(EXIT_FAILURE);
		}
		sendbuffer = segment_to_string(sendbuffer, segsend);	//convert struct segment to string for sending through socket
		sendto(socketfd, (char *) sendbuffer, segsend->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
		printf("Sent DATA packet.\n");
		
		//wait for ACK to be sent
		while((segrecv->head.type) != (ACK)) {
			if(sentSEQ!=checkACK)
			{ //CHECK if seqnum of sent and recv are the same
			if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
				perror("Recvfrom() error");
				exit(EXIT_FAILURE);
			}
			segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
			//int checkACK;
			checkACK=segrecv->head.seqnum-25;
			printf("Recv ACK Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, checkACK, segrecv->head.checksum, segrecv->data, segrecv->segsize);
			if(segment_checksum(segrecv) != segrecv->head.checksum){
				printf("Packet checksum failed.\n");
				memset(segrecv, 0, sizeof(*segrecv));
				continue;
			}
			}
			else{
			break;
			}
		}//check if ACKED
		printf("Got ACK %d\n",checkACK);
		memset(segrecv, 0, sizeof(*segrecv));
		seqnuminc++;
		alarm(0);	//turn of the alarm because ACK is received
		printf("ALARM ENDSS.\n");
		printf("Enter fast mode.\n");
		n=2;//n=2;
		ctr=0; //clear sending flag
		sentSEQ0=sentSEQ;
		memset(filebuff, 0, DATALENGTH);
		break;
		}//while1
	}//n=1
	
	//FASTMODE
	if (n=2)
	{
		//get first packet
		memset(filebuff, 0, DATALENGTH);
		for (i = 0; i <DATALENGTH; i++){
			charbuff = fgetc(fp);
			if(feof(fp)){
				filebuff[i]=0;
				break;
			}
			filebuff[i] = charbuff;
		}
		//make a copy of the first packet 
		for (i = 0; i <DATALENGTH; i++){
			packet1buffer[i]=filebuff[i];
		}
		//get  packet
		memset(filebuff, 0, DATALENGTH);
		for (i = 0; i <DATALENGTH; i++){
			charbuff = fgetc(fp);
			if(feof(fp)){
				filebuff[i]=0;
				break;
			}
			filebuff[i] = charbuff;
		}
		//make a copy of the second packet 
		for (i = 0; i <DATALENGTH; i++){
			packet2buffer[i]=filebuff[i];
		}
		
		//display the data obtained
		printf("From file (Packet 1): %s\n", packet1buffer);
		printf("From file (Packet 2): %s\n", packet2buffer);
		seqnum1=seqnuminc;
		seqnuminc++;
		seqnum2=seqnuminc;
		while(1){
			signal(SIGALRM,alarm_handler);
			alarm(3);			//initial alarm
			
			//if ctr is >0, both packets are lost; will go back to slow mode
			if (ctr>0) {
				goto SLOWMODE;
			}
			
			//sending first packet
			segsend1 = segment_populate(segsend1, DATA, seqnum1, packet1buffer);	//populate segment to be sent as a DATA segment
			printf("Send DATA Segment 1: %d, %d, %d, %s, %d\n", segsend1->head.type, segsend1->head.seqnum, segsend1->head.checksum, segsend1->data, segsend1->segsize);
			sentSEQ1=segsend1->head.seqnum;
			printf("Seqnum sent is %d\n",sentSEQ1);
			
			sendbuffer1 = segment_to_string(sendbuffer1, segsend1);	//convert struct segment to string for sending through socket
			sendto(socketfd, (char *) sendbuffer1, segsend1->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
			printf("Sent first DATA packet.\n");
			
			ctr++;		//flag if data is sent 6 times
			
			//sending second packet
			segsend2 = segment_populate(segsend2, DATA, seqnum2, packet2buffer);	//populate segment to be sent as a DATA segment
			printf("Send DATA Segment 2: %d, %d, %d, %s, %d\n", segsend2->head.type, segsend2->head.seqnum, segsend2->head.checksum, segsend2->data, segsend2->segsize);
			sentSEQ2=segsend2->head.seqnum;
			printf("Seqnum sent is %d\n",sentSEQ2);
			sendbuffer2 = segment_to_string(sendbuffer2, segsend2);	//convert struct segment to string for sending through socket
			sendto(socketfd, (char *) sendbuffer2, segsend2->segsize, MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
			printf("Sent second DATA packet.\n");
		//-------WILL CONTINUE HERE---------//
			//wait for ACK1 and ACK2 to be sent
			while((segrecv->head.type)!=(ACK)) {
				if(sentSEQ1!=checkACK){ //CHECK if seqnum of sent and recv are the same
					if (sentSEQ0==checkACK)
					{
						goto SLOWMODE;
					}
					if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
						perror("Recvfrom() error");
						exit(EXIT_FAILURE);
					}	
					segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
					//int checkACK;
					checkACK=segrecv->head.seqnum-25;
					printf("Recv ACK Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, checkACK, segrecv->head.checksum, segrecv->data, segrecv->segsize);
					if(segment_checksum(segrecv) != segrecv->head.checksum){
						printf("Packet checksum failed.\n");
						memset(segrecv, 0, sizeof(*segrecv));
						continue;
					}
				}
				else{
					sentSEQ0=sentSEQ1;
					for (i=0;i<DATALENGTH;i++) {
						packet1buffer[i]=packet2buffer[i];
					}
					seqnum1=seqnum2;
					if (sentSEQ2!=checkACK2)
					{
						if (sentSEQ0==checkACK2)
						{
							goto SLOWMODE;
						}
						if((lenrecvd = recvfrom(socketfd, (char*)recvbuffer, BUFFERSIZE, MSG_WAITALL, ( struct sockaddr *) &serveraddr, &lenserveraddr)) == -1){
							perror("Recvfrom() error");
							exit(EXIT_FAILURE);
						}
						segrecv = string_to_segment(segrecv, recvbuffer, lenrecvd);
						//int checkACK;
						checkACK2=segrecv->head.seqnum-25;
						printf("Recv ACK Segment: %d, %d, %d, %s, %d\n", segrecv->head.type, checkACK2, segrecv->head.checksum, segrecv->data, segrecv->segsize);
						if(segment_checksum(segrecv) != segrecv->head.checksum){
							printf("Packet checksum failed.\n");
							memset(segrecv, 0, sizeof(*segrecv));
							continue;
						}
					}
					else
					{
						break;
					}
				}//if sentseq1 ==checkack
				}//while not ack
				printf("Got ACK 1 %d and ACK 2 %d\n",checkACK,checkACK2);
				memset(segrecv, 0, sizeof(*segrecv));
				seqnuminc++;
				alarm(0);	//turn of the alarm because ACK is received
				printf("ALARM ENDS.\n");
				printf("Enter fast mode.\n");
				n=2;//n=2;
				ctr=0; //clear sending flag
				sentSEQ0=sentSEQ2;
				memset(filebuff, 0, DATALENGTH);
				memset(packet1buffer, 0, DATALENGTH);
				memset(packet2buffer, 0, DATALENGTH);
				break;
			}//check if ACKED
		
	}
	}//check if eof
	TERMINATE:
	//send FIN to server
	segsend = segment_populate(segsend, FIN, (segsend->head.seqnum)++, NULL);	//populate segment to be sent as a SYN segment
	printf("Segment: %d, %d, %d, %s\n", segsend->head.type, segsend->head.seqnum, segsend->head.checksum, segsend->data);
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
		printf("Segment: %d, %d, %d, %s\n", segrecv->head.type, segrecv->head.seqnum, segrecv->head.checksum, segrecv->data);
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
