    
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

#define SYN 1
#define SYNACK 2
#define ACK 3
#define RST 4
#define DATA 5
#define FIN 6
#define FINACK 7

const char typestr[7][7] = {"SYN", "SYNACK", "ACK", "RST", "DATA", "FIN", "FINACK"};

uint16_t getchecksum(uint8_t type, uint8_t seqnum, char data){
    uint16_t checksum;
    int sum;
    sum = type + seqnum + (int)data;
    while (sum > 65535){
        sum = sum - 65536 + 1;
    }
    checksum = -sum;
    return checksum;
}  

void parse(uint8_t *type, uint8_t *seqnum, uint16_t *checksum, char *data, char *buffer){
    char *token;
    
    token = strtok(buffer,",");
    *type = atoi(token);
    
    token = strtok(NULL,",");
    *seqnum = atoi(token);
    
    token = strtok(NULL,",");
    *checksum = atoi(token);
    
    token = strtok(NULL,"");
    if (token != NULL)
        *data = token[0];
    else
        *data = '\0';
    
    return;
}

char* generate_packet(uint8_t type, uint8_t seqnum, char data){
    char *packet = malloc(14);
    uint16_t checksum;
    checksum = getchecksum(type,seqnum,data);
    sprintf(packet, "%u,%u,%u,%c", type, seqnum, checksum, data);
    return packet;
}