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

#define DATALENGTH 64
#define BUFFERSIZE 256

#define SYN 0b001
#define SYNACK 0b001
#define ACK 0b001
#define DATA 0b001
#define RST 0b001
#define FIN 0b001
#define FINACK 0b001

//uint16_t segment_checksum(struct segment *seg);
//void segment_populate(struct segment *seg, uint8_t headtype, uint8_t headseq);

//data structure for header
struct header{
	uint8_t type;
	uint8_t seqnum;
	uint16_t checksum;
};
/*	Segment Header Type Guide
*	001 - SYN
*	010 - SYNACK
*	011 - ACK
*	100 - DATA
*	101 - RST
*	110 - FIN
*	111 - FINACK
*/

//data structure for a segment
struct segment{
	struct header head;
	char *data;
	int segsize;
};

uint16_t segment_checksum(struct segment *seg){
	return 0;
}

//function for populating values of members of the segment
struct segment* segment_populate(struct segment *seg, uint8_t headtype, uint8_t headseq, char *data){
	memset(seg, 0, sizeof(const struct segment));
	seg->head.type = headtype;
	seg->head.seqnum = headseq;

	if(data == NULL){
		seg->segsize = sizeof(*seg);
	}
	else if(data != NULL){
		if((seg->data = (char *) realloc(seg->data, strlen(data))) == NULL){
			perror("Realloc failure");
			exit(EXIT_FAILURE);
		}
		memset(seg->data, 0, strlen(data));
		memcpy(seg->data, data, strlen(data));
		seg->segsize = sizeof(*seg) + strlen(seg->data);
	}

	seg->head.checksum = segment_checksum(seg);
	//printf("crafted packet\n");
	return seg;
}

//function for converting a struct segment to a string that can be passed to sendto
char* segment_to_string(char *buff, const struct segment *seg){
	if((buff = (char *) realloc(buff, seg->segsize)) == NULL){
		perror("Realloc failure");
		exit(EXIT_FAILURE);
	}

	if((seg->segsize) == sizeof(*seg)) memset(buff, 0, sizeof(*seg));
	else memset(buff, 0, sizeof(*seg)+strlen(seg->data));
	memcpy(buff, &(seg->head.type), sizeof(seg->head.type));
	memcpy(buff + sizeof(seg->head.type), &(seg->head.seqnum), sizeof(seg->head.seqnum));
	memcpy(buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum), &(seg->head.checksum), sizeof(seg->head.checksum));
	if((seg->segsize) > sizeof(*seg)) memcpy(buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum) + sizeof(seg->head.checksum), seg->data, strlen(seg->data));

	return buff;
}

//function for converting a string from recvfrom into a struct segment
struct segment* string_to_segment(struct segment *seg, const char *buff, int lenrecv){
	memcpy(&(seg->head.type), buff, sizeof(seg->head.type));
	memcpy(&(seg->head.seqnum), buff + sizeof(seg->head.type), sizeof(seg->head.seqnum));
	memcpy(&(seg->head.checksum), buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum), sizeof(seg->head.checksum));
	//printf("Func: %d, %d\n", seg->head.type, seg->head.seqnum);
	if(lenrecv - sizeof(*seg) > 0){
		if(((seg->data = (char *) realloc(seg->data, lenrecv - sizeof(*seg))) == NULL)){
			perror("Realloc failure");
			exit(EXIT_FAILURE);
		}
		memcpy(seg->data, buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum) + sizeof(seg->head.checksum), lenrecv - sizeof(*seg));
	}

	return seg;
}
