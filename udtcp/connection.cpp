#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define UDTCP_BUFF_MAX_SIZE 1024

void connect_w_to(void) { 
  int res; 
  struct sockaddr_in addr; 
  long arg; 
  fd_set myset; 
  struct timeval tv; 
  int valopt; 
  socklen_t lon; 

  // Create socket 
  soc = socket(AF_INET, SOCK_STREAM, 0); 
  if (soc < 0) { 
     fprintf(stderr, "Error creating socket (%d %s)\n", errno, strerror(errno)); 
     exit(0); 
  } 

  addr.sin_family = AF_INET; 
  addr.sin_port = htons(2000); 
  addr.sin_addr.s_addr = inet_addr("192.168.0.1"); 

  // Set non-blocking 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg |= O_NONBLOCK; 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // Trying to connect with timeout 
  res = connect(soc, (struct sockaddr *)&addr, sizeof(addr)); 
  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
        do { 
           tv.tv_sec = 15; 
           tv.tv_usec = 0; 
           FD_ZERO(&myset); 
           FD_SET(soc, &myset); 
           res = select(soc+1, NULL, &myset, NULL, &tv); 
           if (res < 0 && errno != EINTR) { 
              fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
              exit(0); 
           } 
           else if (res > 0) { 
              // Socket selected for write 
              lon = sizeof(int); 
              if (getsockopt(soc, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
                 fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
                 exit(0); 
              } 
              // Check the value returned... 
              if (valopt) { 
                 fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) 
); 
                 exit(0); 
              } 
              break; 
           } 
           else { 
              fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
              exit(0); 
           } 
        } while (1); 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        exit(0); 
     } 
  } 
  // Set to blocking mode again... 
  if( (arg = fcntl(soc, F_GETFL, NULL)) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  arg &= (~O_NONBLOCK); 
  if( fcntl(soc, F_SETFL, arg) < 0) { 
     fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
     exit(0); 
  } 
  // I hope that is all 
}

int     udtcp_connection(const char *hostname, int port, int timeout, int *ret_sock_fd)
{
    int                 sock_fd;
    int                 sock_opt;
    struct hostent      *host_entity;
    struct sockaddr_in  serv_addr;

    /* create socket */
    sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd == -1)
    {
        perror("socket() failed");
        return (-1);
    }

    /* get current options of socket */
    sock_opt = fcntl(sock_fd, F_GETFL, NULL);
    if (sock_opt < 0)
    {
        perror("fcntl(F_GETFL) failed");
        return (-1);
    }

    /* add non block */
    sock_opt |= O_NONBLOCK; 
    /* set new options */
    if(fcntl(sock_fd, F_SETFL, sock_opt) < 0)
    { 
        perror("fcntl(F_SETFL) failed");
        return (-1);
    } 

    /* get host list by name */
    host_entity = gethostbyname(hostname);
    if (host_entity == NULL)
    {
        perror("gethostbyname() failed");
        return (-1);
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));

    memcpy(&serv_addr.sin_addr, host_entity->h_addr_list[0], host_entity->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("connect() failed");
        return (-1);
    }

    /* get current options of socket */
    sock_opt = fcntl(sock_fd, F_GETFL, NULL);
    if (sock_opt < 0)
    {
        perror("fcntl(F_GETFL) failed");
        return (-1);
    }

    /* add non block */
    sock_opt &= (~O_NONBLOCK);
    /* set new options */
    if(fcntl(sock_fd, F_SETFL, sock_opt) < 0)
    { 
        perror("fcntl(F_SETFL) failed");
        return (-1);
    }

    *ret_sock_fd = sock_fd;

    return (0);
}