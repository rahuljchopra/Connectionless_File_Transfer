#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <math.h>
 

struct packet {
    unsigned int total_frag;  
    unsigned int frag_no; 
    unsigned int size; 
    char* filename; 
    char filedata[1000]; 
} file_packet;


void append_to_string(char src[], char dest[], int *start_index ) {
    int len_dest = strlen(dest);
    int index = *start_index;
    for(int dest_index = 0; dest_index < len_dest; index++,dest_index++) {
        src[index] = dest[dest_index];
    }
    src[index] = ':';
    *start_index = index+1;

}

#define MAXBUFLEN 100

int main(int argc, char* argv[]) {
    int sockfd,numbytes,rv;
    struct addrinfo hints, *serverinfo;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char buf[MAXBUFLEN];
    char existance[100];
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    
    if(argc < 3) {
        printf("client: Too less arguments\n");
        return 1;
    }
    else if(argc > 3) {
        printf("client: Too many arguments\n");
        return 1;
    }
    if((rv = getaddrinfo(argv[1], argv[2], &hints, &serverinfo)) != 0) {
        printf("getaddrinfo error \n");
        return 1;
    }
    
    sockfd = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);
    
    printf("Input a message of the following form\n");
    printf("    ftp <file name>\n");
    scanf("%[^\n]%*c", existance);
    char* filename = strtok(existance, " ");
    filename = strtok(NULL, " "); //filename contains the name of the file
    char* msg = "ftp";
    FILE* send_file = fopen(filename, "rb");
    
    if(send_file == NULL) {
        printf("file %s does not not exist\n", filename);
        return -1;
    }
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    sendto(sockfd, msg, 4, 0, serverinfo->ai_addr, serverinfo->ai_addrlen);
    numbytes = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, NULL, NULL);
    gettimeofday(&end, NULL);
    int RTT = (end.tv_sec * 1000000 + end.tv_usec) - (start.tv_sec * 1000000 + start.tv_usec);
    printf("RTT is : %ld micro seconds\n", RTT);
    
    if(strcmp(buf, "yes") == 0) {
        printf("Start file transfer process\n");
    }
    
    fseek(send_file, 0L, SEEK_END);
    int file_size = ftell(send_file); //calculating the size of the file
    rewind(send_file);
    
    int total_fragments = (file_size + 1000 -1) / 1000; //calculating the fragments needed

    //This is the packet formation portion 
    file_packet.total_frag = total_fragments;
    file_packet.frag_no = 1;
    
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = RTT * 10;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    int n;
    
    int i = 1;
    while(i <= total_fragments) {
        int index;
        int len_packet = 1500;
        char packet_header[1500];
        memset(packet_header, 0, len_packet);
        int sz = fread(file_packet.filedata, 1, 1000, send_file);
        index = sprintf(packet_header, "%d:%d:%d:%s:", file_packet.total_frag, file_packet.frag_no, sz, filename, file_packet.filedata);
        memcpy(&packet_header[index], file_packet.filedata, sz);
        sendto(sockfd, packet_header, len_packet, 0, serverinfo->ai_addr, serverinfo->ai_addrlen);
        n = recvfrom(sockfd, buf, MAXBUFLEN - 1, 0, NULL, NULL);
        if(n >= 0) {
            i++;
            file_packet.frag_no++;
        }
        else if(strcmp(buf, "GotIt") != 0) {
            printf("deliver: acknowledgement error\n");
            return -1;
        }
        else {
            printf("Timeout occurred. Retransmitting packet\n");
            fseek(send_file, -sz, SEEK_CUR);
        }
    }
    
    printf("File transfer complete!\n");
    fclose(send_file);
    close(sockfd);
    
    return 0; 
}