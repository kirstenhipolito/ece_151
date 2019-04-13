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

#define SYN 0b00000001
#define SYNACK 0b00000011
#define ACK 0b00000010
#define DATA 0b0001000
#define RST 0b00100000
#define FIN 0b01000000
#define FINACK 0b01000010

//uint16_t segment_checksum(struct segment *seg);
//void segment_populate(struct segment *seg, uint8_t headtype, uint8_t headseq);

//data structure for header
struct header{
	uint8_t type;
	uint8_t seqnum;
	uint16_t checksum;
};

//data structure for a segment
struct segment{
	struct header head;
	uint8_t *data;
	uint16_t segsize;
};

uint16_t segment_checksum(struct segment *seg){
	uint16_t checksum;
    int sum, i = 0;
    
    sum = seg->head.type + seg->head.seqnum;
    
    if (((seg->segsize) - sizeof(*seg)) != 0){
    	//printf("Not null.\n");
		for(i=0; i < ((seg->segsize) - sizeof(*seg)); i++){
			sum = sum + (int) (seg->data)[i];
		}
	}
	
    while (sum > 65535){
        sum = sum - 65536 + 1;
    }
    checksum = -sum;
    return checksum;
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
		if((seg->data = (uint8_t *) realloc(seg->data, DATALENGTH)) == NULL){
			perror("Realloc failure");
			exit(EXIT_FAILURE);
		}
		memset(seg->data, 0, DATALENGTH);
		memcpy(seg->data, data, DATALENGTH);
		seg->segsize = sizeof(*seg) + DATALENGTH;
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
	else memset(buff, 0, seg->segsize);
	memcpy(buff, &(seg->head.type), sizeof(seg->head.type));
	memcpy(buff + sizeof(seg->head.type), &(seg->head.seqnum), sizeof(seg->head.seqnum));
	memcpy(buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum), &(seg->head.checksum), sizeof(seg->head.checksum));
	memcpy(buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum) + sizeof(seg->head.checksum), &(seg->segsize), sizeof(seg->segsize));
	if((seg->segsize) > sizeof(*seg)) memcpy(buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum) + sizeof(seg->head.checksum) + sizeof(seg->segsize), seg->data, (seg->segsize) - sizeof(*seg));

	return buff;
}

//function for converting a string from recvfrom into a struct segment
struct segment* string_to_segment(struct segment *seg, const char *buff, int lenrecv){
	memcpy(&(seg->head.type), buff, sizeof(seg->head.type));
	memcpy(&(seg->head.seqnum), buff + sizeof(seg->head.type), sizeof(seg->head.seqnum));
	memcpy(&(seg->head.checksum), buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum), sizeof(seg->head.checksum));
	memcpy(&(seg->segsize), buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum) + sizeof(seg->head.checksum), sizeof(seg->segsize));
	
	//printf("Func: %d, %d\n", seg->head.type, seg->head.seqnum);
	if(lenrecv - sizeof(*seg) > 0){
		if(((seg->data = (uint8_t *) realloc(seg->data, lenrecv - sizeof(*seg))) == NULL)){
			perror("Realloc failure");
			exit(EXIT_FAILURE);
		}
		memset(seg->data, 0, lenrecv - sizeof(*seg));
		memcpy(seg->data, buff + sizeof(seg->head.type) + sizeof(seg->head.seqnum) + sizeof(seg->head.checksum) + sizeof(seg->segsize), lenrecv - sizeof(*seg));
	}
	else {
		seg->data = NULL;
	}

	return seg;
}
