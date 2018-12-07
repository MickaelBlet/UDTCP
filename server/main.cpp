#include <netdb.h> 
#include <netinet/in.h> 
#include <stdlib.h> 
#include <stdio.h>
#include <unistd.h>
#include <string.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <signal.h>

#define MAX 80 
#define PORT 4242 
  
int portUDCount = 3900;
int sockServ    = 0;

void signalStop(int sig)
{
    (void)sig;
    if (sockServ != 0)
        close(sockServ);
    exit(EXIT_SUCCESS);
}

// Function designed for chat between client and server. 
void func(int sockfd) 
{ 
    char buff[MAX]; 
    int n;
    struct sockaddr_in sockAddrUdp;

    int sockUdp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockUdp == -1)
    {
        fprintf(stderr, "socket create: %s", strerror(sockUdp));
        return;
    }
    bzero(&sockAddrUdp, sizeof(sockAddrUdp));
  
    // assign IP, PORT 
    sockAddrUdp.sin_family = AF_INET; 
    sockAddrUdp.sin_addr.s_addr = htonl(INADDR_ANY); 
    sockAddrUdp.sin_port = portUDCount++; 
    if ((bind(sockUdp, (struct sockaddr*)&sockAddrUdp, sizeof(sockAddrUdp))) != 0) { 
        printf("Socket bind failed...\n"); 
        exit(0); 
    } 
    else
    {
        printf("Socket successfully binded..\n");
    }

    write(sockfd, &sockAddrUdp.sin_port, sizeof(int));
    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MAX); 
  
        // read the message from client and copy it in buffer 
        read(sockfd, buff, sizeof(buff)); 
        // print buffer which contains the client contents 
        printf("From client: %s\t To client : ", buff); 
        bzero(buff, MAX); 
        n = 0; 
        // copy server message in the buffer 
        while ((buff[n++] = getchar()) != '\n') 
            ; 
  
        // and send that buffer to client 
        write(sockfd, buff, sizeof(buff)); 
  
        // if msg contains "Exit" then server exit and chat ended. 
        if (strncmp("exit", buff, 4) == 0) { 
            printf("Server Exit...\n"); 
            break; 
        } 
    } 
} 
  
// Driver function 
int main() 
{ 
    int connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockServ = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockServ == -1) { 
        printf("Socket creation failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully created..\n"); 
    bzero(&servaddr, sizeof(servaddr));

    signal(SIGABRT, &signalStop);
    signal(SIGSTOP, &signalStop);
    signal(SIGINT,  &signalStop);
    signal(SIGQUIT, &signalStop);
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(PORT); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockServ, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("Socket bind failed...\n"); 
        exit(0); 
    } 
    else
        printf("Socket successfully binded..\n"); 
  
    // Now server is ready to listen and verification 
    if ((listen(sockServ, 5)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else
        printf("Server listening..\n"); 
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockServ, (struct sockaddr *)&cli, (socklen_t*)&len); 
    if (connfd < 0) { 
        printf("Server acccept failed...\n"); 
        exit(0); 
    } 
    else
    {
        printf("Server acccept the client...\n"); 
    }
  
    // Function for chatting between client and server 
    func(connfd); 
  
    // After chatting close the socket 
    close(sockServ); 
} 