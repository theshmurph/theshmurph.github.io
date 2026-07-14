// example from beej intro to networks

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

// TODO more signals stuff
void sigchld_handler(int s) {
  (void) s;
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0);
  errno = saved_errno;
}

void* get_in_addr(struct sockaddr* sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in*)sa)->sin_addr);
  }
}

// our main doer
int main(void) {
  int sockfd, new_fd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  struct sigaction sa;
  int yes = 1;
  char s[INET6_ADDRSTRLEN];
  int rv;

  // zero out the hints struct
  memset(&hints, 0, sizeof(hints));
  // setting hints to include ipv4, tcp, and using our ip addr
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // this is the error block, but on success, the function call actually
  // stores potential ip addresses in the servinfo variable
  if ((rv = getaddrinfo(NULL, "8080", &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // cycling through all the potential addresses we can bind to. it's a 
  // linked list
  for (p = servinfo; p != NULL; p = p->ai_next) {
    // tries to get a socket file descriptor and put it in sockfd
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }
    
    // this line is to avoid that "address already in use" error
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    // we have a file descriptor that we can use, and an address option for 
    // localhost, so let's bind our comms to that socket
    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }
    
    break;
  }

  // free that linked list when we're done with it
  freeaddrinfo(servinfo);

  // kinda seems redundantj, but if we get no servers returned, i'm guessing 
  // this would hit
  if (p == NULL) {
    fprintf(stderr, "server: failed to bind");
    exit(1);
  }

  // now we're listening for incoming connections on our socket file descriptor
  // we can have 20 possible connections in the queue
  if (listen(sockfd, 20) == -1) {
    perror("listen");
    exit(1);
  }

  // TODO learn more about signals
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  while (1) {
    // this essentially accepts an incoming connection. their_addr is the 
    // actual struct, and sin_size is the size of it
    sin_size = sizeof(their_addr);
    new_fd = accept(sockfd, (struct sockaddr*) &their_addr, &sin_size);

    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    // read in the incoming ip addr. there's some crazy stuff with inet_ntop
    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), 
      s, sizeof(s));
      printf("server: got connection from %s\n", s);

    // makes new process, sends data, closes socket
    if (!fork()) {
      close(sockfd);
      if (send(new_fd, "Hello, world!", 13, 0) == -1) {
        perror("send");
      }
      close(new_fd);
      exit(0);
    }
    close(new_fd);
  }
  return 0;
}
