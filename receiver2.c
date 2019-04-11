// Server side implementation of UDP client-server model 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
#include "ece151mp1.h"
  
#define MAXLINE 1024 
//#define PORT 8080
  
int main(int argc, char *argv[]) { 
    
    // Check if usage is correct
    if (argc != 3) {
        fprintf(stderr,"Usage: receiver <src_port> <filename>\n");
        exit(1);
    }
    
    // Creating socket file descriptor 
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }
    memset(&servaddr, 0, sizeof(servaddr)); 
    memset(&cliaddr, 0, sizeof(cliaddr)); 
      
      
    // Filling server information 
    servaddr.sin_family = AF_INET; // IPv4 
    servaddr.sin_addr.s_addr = INADDR_ANY; 
    servaddr.sin_port = htons((uint16_t) atoi(argv[1])); 
      
      
    // Bind the socket with the server address 
    if (bind(sockfd, (const struct sockaddr *)&servaddr,sizeof(servaddr)) < 0){ 
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
     
    
    FILE *fr;
	if((fr = fopen(argv[2], "w")) == NULL){
		perror("File open failure");
		exit(EXIT_FAILURE);
	}
	
	// initiate variables that will be used for sending and receiving packets
    char buffer[MAXLINE]="";
    uint8_t type=0, seqnum=0;
    uint16_t r_checksum=0, checksum=0;
    char packet[14];
    char data='\0';
    
    int len, n;
    while (1){
        // receive a string from client
        n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len); 
        buffer[n] = '\0';
    
        // parse received string
        parse(&type, &seqnum, &r_checksum, &data, buffer);
        printf("%s packet %u received.\n", typestr[type-1], seqnum);
        
        // check if received data is correct using checksum
        checksum = getchecksum(type, seqnum, data);  
        if (checksum != r_checksum){
            type = RST;
        }
        
        // determine the type of ack reply
        if (type == SYN){
            type = SYNACK;
        }else if  (type == FIN){
            type = FINACK;
        }else if  (type == DATA){
            type = ACK;
            fputc(data, fr);
        }
        
        // generate packet to be sent
        data = '\0';
        sprintf(packet, "%u,%u,%u,%c", type, seqnum, checksum, data);
        
        sendto(sockfd, (const char *)packet, strlen(packet), MSG_CONFIRM, (const struct sockaddr *) &cliaddr, len); 
        printf("%s packet %u sent.\n", typestr[type-1], seqnum);
        
        // if a FINACK is sent, end connection
        if (type == FINACK)
            break;
        
    }
        
    close(sockfd);  
      
    return 0; 
} 