// Client side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "ece151mp1.h"
  
#define MAXLINE 14
//#define PORT 8080


int main(int argc, char *argv[]) {    
    
    // check if usage is correct
    if (argc != 5) {
        fprintf(stderr,"Usage: sender <dest_IP> <dest_port> <filename> <proto>\n");
        exit(1);
    }
    
    
    // open filename
    FILE *fs;
	if((fs = fopen(argv[3], "r")) == NULL){
		perror("File open failure");
		exit(EXIT_FAILURE);
	}
    
    
    // Creating socket file descriptor 
    int socketfd;
    struct sockaddr_in serveraddr;
    if ((socketfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    memset(&serveraddr, 0, sizeof(serveraddr)); 
    
      
    // Filling server information 
    serveraddr.sin_family = AF_INET; 
    serveraddr.sin_port = htons((uint16_t) atoi(argv[2])); 
    serveraddr.sin_addr.s_addr = INADDR_ANY; 
      
    
    // Set up sending and receiving variables 
    uint8_t r_type=0, seqnum=0, r_seq;
    uint16_t r_checksum=0;
    char data='\0';
    
    // NOTE: PACKET STRING FORMAT - "x,yyy,zzzzz,c" 
    // x - type (1-7)
    // yyy - seqnum (0-255)
    // zzzzz - checksum (0-65535)
    // c - data (char)
    char *packet = malloc(MAXLINE);     // for sending
    char buffer[MAXLINE] = "";          // for receiving


    //send SYN packet
    seqnum = 0;
    packet = generate_packet(SYN,++seqnum,'\0');
    sendto(socketfd, (const char *)packet, strlen(packet), MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
    printf("%s packet %u sent.\n", typestr[SYN-1], seqnum); 
    
    //receive SYNACK packet
    int lenrecvd, lenserveraddr; 
    
    lenrecvd = recvfrom(socketfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serveraddr, &lenserveraddr);
    buffer[lenrecvd] = '\0';
    parse(&r_type, &r_seq, &r_checksum, &data, buffer);
    printf("%s packet %u received.\n", typestr[r_type-1], r_seq);

    //send file contents    
    while (1){
        
        // send DATA packets
        packet = generate_packet(DATA,++seqnum,fgetc(fs));
        if (feof(fs))   // at end of file, end loop
            break;
        sendto(socketfd, (const char *)packet, strlen(packet), MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
        printf("%s packet %u sent.\n", typestr[DATA-1], seqnum); 
        
        
        // receive ACK packets    
        lenrecvd = recvfrom(socketfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serveraddr, &lenserveraddr);
        buffer[lenrecvd] = '\0';
        parse(&r_type, &r_seq, &r_checksum, &data, buffer);
        printf("%s packet %u received.\n", typestr[r_type-1], r_seq);
    }
    
    // send FIN packet
    packet = generate_packet(FIN,seqnum,'\0');
    sendto(socketfd, (const char *)packet, strlen(packet), MSG_CONFIRM, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
    printf("%s packet %u sent.\n", typestr[FIN-1], seqnum); 
    
    
    // receive FINACK packet    
    lenrecvd = recvfrom(socketfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &serveraddr, &lenserveraddr);
    buffer[lenrecvd] = '\0'; 
    parse(&r_type, &r_seq, &r_checksum, &data, buffer);
    printf("%s packet %u received.\n", typestr[r_type-1], r_seq);
    
    
    fclose(fs);
    close(socketfd); 
    return 0; 
} 
