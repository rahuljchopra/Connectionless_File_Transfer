#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>

struct packet {
    unsigned int total_frag;  
    unsigned int frag_no; 
    unsigned int size; 
    char* filename; 
    char filedata[1000]; 
} file_packet;
int tot = 0;

void decode_string(char buf[]) {
    int index = 0;
    int no_of_colons = 0;
    int colons[5];
    while(no_of_colons < 4) {
        if(buf[index] == ':') {
           colons[no_of_colons] = index;
           no_of_colons++;   
        }
        index++;       
    }
    char temp1[20] = {0}, temp2[20] = {0},temp3[20] = {0};
    char * fname = (char*)malloc(30 * sizeof(char));
    for(int i = 0, k = 0; i < colons[0]; i++,k++)
        temp1[k] = buf[i];
    file_packet.total_frag = atoi(temp1);
    for(int i = colons[0]+1, k = 0; i < colons[1]; i++,k++)
        temp2[k] = buf[i];
    file_packet.frag_no = atoi(temp2);
    for(int i = colons[1]+1, k = 0; i < colons[2]; i++,k++)
        temp3[k] = buf[i];
    file_packet.size = atoi(temp3);
    for(int i = colons[2]+1,k = 0; i < colons[3]; i++,k++)
        fname[k] = buf[i];
    file_packet.filename = (char*)malloc(30 * sizeof(char));
    strcpy(file_packet.filename, fname);
    memset(file_packet.filedata,0,sizeof(file_packet.filedata));
    for(int i = colons[3] + 1, k = 0; k < file_packet.size; i++, k++)
        file_packet.filedata[k] = buf[i];
}

int random_number(int min, int max) {
    int num = (rand() % (max - min + 1) + min);
    return num;
}

#define MAXBUFLEN 1500

int main(int argc, char* argv[]) {
    // First we connect to socket then we bind. Then we recv
    // if arguments are less than 2 then error
    int sock_fd, rv, num_bytes;
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_storage their_addr = {0};
    socklen_t addr_len;
    char buf[MAXBUFLEN];


    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if(argc < 2) {
        fprintf(stderr, "server: Too less arguments\n");
        return -1;
    }
    else if(argc > 2) {
        fprintf(stderr, "server: Too many arguments\n");
        return -1;
    }
    rv = getaddrinfo(NULL, argv[1], &hints, &res);
    if (rv != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock_fd == -1) {
        fprintf(stderr, "server: failed to connect to socket\n");
        return 1;
    }
    
    if(bind(sock_fd,res->ai_addr, res->ai_addrlen) < 0) {
        close(sock_fd);
        fprintf(stderr, "server: failed to bind socket\n");
        return 2;
    }
    printf("Server receiving on port %s\n", argv[1]);
    
    num_bytes = recvfrom(sock_fd, buf, MAXBUFLEN - 1, 0, (struct sockaddr*)&their_addr, &addr_len);
    if (num_bytes == -1) {
     fprintf(stderr, "server: error\n");
     exit (1);
    }
    if(strcmp(buf, "ftp") == 0) {
        char* msg = "yes";
        sendto(sock_fd, msg, 4, 0,( struct sockaddr *)&their_addr, addr_len);
    }
    else {
        sendto(sock_fd, "no", 3, 0, (struct sockaddr *)&their_addr, addr_len);
    }
    recvfrom(sock_fd, buf, MAXBUFLEN, 0, (struct sockaddr*)&their_addr, &addr_len);
    sendto(sock_fd, "GotIt", 6, 0, (struct sockaddr *)&their_addr, addr_len);
    decode_string(buf);

    FILE* out;
    char file_name[20];
    strcpy(file_name, file_packet.filename);
    printf("%s:\n",file_name);
    out = fopen(file_name, "wb");
    fwrite(file_packet.filedata, 1, file_packet.size,out);
    
    int frag = 2;
    while(frag <= file_packet.total_frag) {
        recvfrom(sock_fd, buf, MAXBUFLEN, 0, (struct sockaddr*)&their_addr, &addr_len);
        if(random_number(1, 100) > 5) {
            sendto(sock_fd, "GotIt", 6, 0, (struct sockaddr *)&their_addr, addr_len);
            decode_string(buf);
            fwrite(file_packet.filedata, 1, file_packet.size,out);
            frag++;
        }
        else {
            printf("Packet has been dropped\n");
        }
    }
    
    return 0;
}