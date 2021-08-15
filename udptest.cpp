#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT     5551
#define MAXLINE 1024
//----------------------------------------------------------------------------------------------------------------------
#define TEST_MESSAGE "\x55\x6E\x49\x51\x01\x07\x00\x00\x00\x00\x5E\x00\x10\x00\x00\x00\x01\x04\x00\x00\xf0\x41\x02\x04\x00\x00\x70\x42\x03\x04\x00\x00\x96\x44\x04\x04\xcd\xcc\x0c\xc0\x05\x04\xcd\xcc\x8c\x3f\x06\x04\x00\x80\x34\x43\x07\x04\x9a\x99\x35\x43\x08\x04\xcd\x4c\x05\x43\x09\x08\x00\xa0\x73\x92\xe8\x05\xcc\x08\x0a\x04\x56\x14\x45\x50\x81\x04\x97\x10\x10\x00\x0b\x04\xcd\xcc\xc7\x42\x0c\x02\x03\x00\x0d\x04\x01\x00\x00\x00\x0f\x02\x73\x13\x80\x02\x00\x00"
//----------------------------------------------------------------------------------------------------------------------
// Driver code
int main() {
    int sockfd;
    char buffer[MAXLINE];
    char *hello = TEST_MESSAGE;
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family    = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,
              sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int n;
    socklen_t len = sizeof(cliaddr);
    n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                 MSG_WAITALL, ( struct sockaddr *)&cliaddr,
                 &len);
    buffer[n] = '\0';
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cliaddr.sin_addr), str, INET_ADDRSTRLEN);
    printf("Client %s %d: %s\n",str, ntohs(cliaddr.sin_port), buffer);
    int err = sendto(sockfd, (const char *)hello, sizeof(TEST_MESSAGE),
           MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
           sizeof(cliaddr));
    printf("Hello message sent.\n");

    return 0;
}