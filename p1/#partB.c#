#include <stdio.h>
#include <sys/types.h> //types for socket.h and netinet/in.h
#include <sys/socket.h>  // structures for sockets
#include <netinet/in.h>  // structures for sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  // signal name kill()

int debug = 0; //when on, prints more detail to stderr msg.

int reportError(char* msg, int errorCode)
{
  fprintf(stderr,"Error: %s\n", msg);
  exit(errorCode);
}

char* response(int fd, int fileType)
//return response 
//response contains header, file content from fd
//fileType: html, htm, jpg, jpeg, gif
{
  char* res = "abcdef"; //calloc(100,1);
  
  //  response = header;

  return res;
}


int main(int argc, char * argv[])
{
  struct sockaddr_in serverA; //Address structure for server
      
  //store port number
  if (argc < 2) reportError("argument failure: ./server portNumber", 1);
  if (argc > 2) debug = 1;
  if (debug) fprintf(stderr, "Server Port: %d\n", atoi(argv[1]));
    
  //create and bind socket
  int sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) reportError("socket creation failed", 2);

  serverA.sin_family = AF_INET; //communication domain
  serverA.sin_addr.s_addr = INADDR_ANY; // ip (accepts any internet address)
  serverA.sin_port = htons(atoi(argv[1])); //server port

  if (bind(sockfd, (struct sockaddr *) &serverA, sizeof(serverA)) < 0)
    reportError("socket binding failed",2);

  //listen for connection
  if (debug) fprintf(stderr, "Socket defined. now listening for client...\n");
  listen(sockfd, 3); // 3 simultaneous connections to this socket

  //accept client's address
  struct sockaddr_in clientA; int clientA_len; //Address structure for client
  int clientFD = accept(sockfd, (struct sockaddr *) &clientA, (socklen_t *) &clientA_len);
  if (clientFD < 0) reportError("accept failed. didn't find client FD",2);

  if (debug) { //getting client ip & port (fails: always prints 0.0.0.0:0)
    unsigned char *ip = (unsigned char *)&(clientA.sin_addr.s_addr); 
    fprintf(stderr, "Found client %d.%d.%d.%d:%d\n"
	    ,clientA.sin_addr.s_addr, ip[1], ip[2], ip[3]
	    ,ntohs(((struct sockaddr_in)clientA).sin_port));
  }

  //read
  char request[1280];
  memset(request,0,1280);
  int z = read(clientFD, request, 1279);
  fprintf(stdout, "%s", request);



  //response
  char* res;
  int fd = 4; //file descriptor
  int ft = 3; //file type
  int x = response(res, fd, ft);
  int n = write(clientFD, res, x);
  n = write(1, res, x);
  fprintf(stderr, "hwerw%s%d\n",res,x);

  //write message
  n = write(clientFD, "response message", 30);

  
  //close connection
  close (clientFD);
  close (sockfd);
  // ^^ this should be in loop
}
