#include <stdio.h>
#include <sys/types.h> //types for socket.h and netinet/in.h
#include <sys/socket.h>  // structures for sockets
#include <netinet/in.h>  // structures for sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  // signal name kill()

int debug = 0; //when on, prints more detail to stderr msg.
int REQUEST_LEN = 1000;
int RESPONSE_LEN = 1000;

int reportError(char* msg, int errorCode)
{
  fprintf(stderr,"Error: %s\n", msg);
  exit(errorCode);
}

char* createResponse(int fd, int fileType)
//return response 
//response contains header, file content from fd
//fileType: html, htm, jpg, jpeg, gif
{
  char* response = malloc(RESPONSE_LEN);
  char header[12][100]; //magic number
  strcpy(header[0],"HTTP/1.1 200 OK\r\n");
  strcpy(header[1],"Date: Sun, 18 Oct 2009 08:56:53 GMT\r\n");
  strcpy(header[2],"Server: Apache/2.2.14 (Win32)\r\n");
  strcpy(header[3],"Last-Modified: Sat, 20 Nov 2004 07:16:26 GMT\r\n");
  strcpy(header[4],"ETag: \"10000000565a5-2c-3e94b66c2e680\"\r\n");
  strcpy(header[5],"Accept-Ranges: bytes\r\n");
  strcpy(header[6],"Content-Length: 44\r\n");
  strcpy(header[7],"Connection: close\r\n");
  strcpy(header[8],"Content-Type: text/html\r\n");
  strcpy(header[9],"X-Pad: avoid browser bug;\r\n\r\n");
  strcpy(header[10],"<html><body><h1>It works!!</h1></body></html>");
  
  sprintf(response,"%s%s%s%s%s%s%s%s%s%s%s",
	    header[0],header[1],header[2],header[3],header[4],
	    header[5],header[6],header[7],header[8],header[9],
	  header[10]);

  return response;
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

  //buffered read request
  char request[REQUEST_LEN];
  memset(request,0,REQUEST_LEN);
  int z = read(clientFD, request, REQUEST_LEN-1);
  if (debug) fprintf(stderr, "\nREQUEST MSG:\n");
  fprintf(stdout, "%s", request);

  //simple http response (header + body)
  int fd = 4; //assume file descriptor
  int ft = 3; //file type
  char* response = createResponse(fd, ft);
  int n = write(clientFD, response, strlen(response));
  if (debug) fprintf(stderr, "RESPONSE MSG:\n%s\n", response);
  
  //close connection
  close (clientFD);
  close (sockfd);
  // ^^ this should be in loop
}
