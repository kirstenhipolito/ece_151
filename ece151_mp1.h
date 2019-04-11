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
};

uint16_t segment_checksum(struct segment *seg){
	return 0;
}

void segment_populate(struct segment *seg, uint8_t headtype, uint8_t headseq){
	memset(seg, 0, sizeof(const struct segment));
	seg->head.type = headtype;
	seg->head.seqnum = headseq;
	//seg->head.checksum = segment_checksum();
}
