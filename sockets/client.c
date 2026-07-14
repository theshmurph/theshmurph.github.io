#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// function returning ipv4 or 6 addr
void* get_in_addr(struct sockaddr* sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char* argv[]) {
  int sockfd, numbytes;
  char buf[100];
  struct addrinfo hints, *servinfo, *p;
  int rv;
  char s[INET6_ADDRSTRLEN];

  // setting up the usage. only one arg to be passed
  if (argc != 2) {
    fprintf(stderr, "usage: client hostname");
    exit(1);
  }

  // zero out the hints struct
  memset(&hints, 0, sizeof(hints));
  // set hints to generic TCP stream
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  // put all possible ip structs in servinfo
  if ((rv = getaddrinfo(argv[1], "8080", &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    // this gives us a usable socket file descriptor based on our results
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("client: socket");
      continue;
    }

    // similar to the server implementation, this will store the ipaddr in s
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
    printf("client: attempting connection to %s\n", s);

    // this connects us with the server
    if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      perror("client: connect");
      close(sockfd);
      continue;
    }

    break;
  }

  if (p == NULL) {
    fprintf(stderr, "client: failed to connect\n");
    return 2;
  }

  // turn the good ip address into readable things we can use
  inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof(s));
  printf("client: connected to %s\n", s);

  // no longer need the servinfo struct
  freeaddrinfo(servinfo);

  // receive info from the server into our buffer of 100 bytes
  if ((numbytes = recv(sockfd, buf, 99, 0)) == -1) {
    perror("recv");
    exit(1);
  }

  // null-terminate the string
  buf[numbytes] = '\0';
  
  // print message
  printf("client: received '%s'\n", buf);

  // close socket
  close(sockfd);

  return 0;

}
