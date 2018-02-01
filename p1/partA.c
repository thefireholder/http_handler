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

// n = size of buffer to look at
char* parse_path(int* state, char* path_so_far, char* buffer, int n) {
  int i;
  int num = 0;
  char* adds = NULL;
  for (i = 0; i < n; i++) {
    if (*state == 0 && buffer[i] != ' ') {
      *state = 1;
    }
    else if (*state == 1 && buffer[i] == ' ') {
      *state = 2;
    }
    else if (*state == 2 && buffer[i] != ' ') {
      *state = 3;
      // for ease
      adds = (char*) calloc(n-i, sizeof(char));
      adds[num] = buffer[i];
      num++;
    }
    else if (*state == 3 && buffer[i] != ' ') {
      if (adds == NULL) {
        adds = (char*) calloc(n-i, sizeof(char));
      }
      adds[num] = buffer[i];
      num++;
    }
    else if (*state == 3 && buffer[i] == ' ') {
      *state = 4;
      break;
    }
  }
  if (adds != NULL) {
    char* fin = (char*) malloc((strlen(path_so_far) + num + 1) * sizeof(char));
    fin[0] = '\0';
    fin = strcat(fin, path_so_far);
    fin = strcat(fin, adds);
    free(path_so_far);
    return fin;
  }
  return path_so_far;
}

char* replace_spaces(char* full_path) {
  int path_len = strlen(full_path);
  char* fin = (char*)calloc(path_len + 1, sizeof(char));

  int fin_ind = 0;
  int i;
  for(i = 0; i < path_len; i++) {
    if (i + 2 < path_len && full_path[i]=='%' && full_path[i+1]=='2' && full_path[i+2]=='0'){
      fin[fin_ind] = ' ';
      fin_ind++;
      i+=2;
    } else {
      fin[fin_ind] = full_path[i];
      fin_ind++;
    }
  }
  free(full_path);
  return fin;
}

void print_request(char* buffer, int* end_state, int n) {
  int i;
  int end_index = -1;
  for(i = 0; i < n; i++) {
    // \r\n\r\n
    if(*end_state==0 || *end_state==2) {
      if(buffer[i]=='\r')
        (*end_state)++;
      else
        *end_state = 0;
    }
    else if (*end_state==1 || *end_state==3){
      if(buffer[i]=='\n')
        (*end_state)++;
      else
        end_state = 0;
    }
    if(*end_state==4){
      end_index=i;
      break;
    }
  }
  if (end_index==-1){
    char c = buffer[n-1];
    buffer[n-1] = '\0';
    fprintf(stdout, "%s", buffer);
    fprintf(stdout, "%c", c);
  } else {
    buffer[end_index]='\0';
    fprintf(stdout, "%s", buffer);
    //remaining \n
    fprintf(stdout, "\n");
  }
  // fflush(stdout);
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
  listen(sockfd, 5); // 5 simultaneous connections to this socket

  while(1) {
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

    //read message
    if (debug) fprintf(stderr, "Message from client:\n");
    char buffer[10]; int n;
    char buffer2[10];
    int state = 0;
    
    char* full_path = (char*)malloc(sizeof(char) * 1);
    full_path[0] = '\0';

    int print_state = 0;
    do
    {
      n = read(clientFD, buffer, 10);
      
      int x;
      for(x=0;x<n;x++){
        buffer2[x]=buffer[x];
      }

      if (n < 0) {
        reportError("Read failed", 2);
      }
      if (n == 0) {
        // fprintf(stdout, "%s\n", "Read finished");
        break;
      }
      print_request(buffer, &print_state, n);
      if (state < 4) {
        full_path = parse_path(&state, full_path, buffer2, n);
      }
    }
    while(print_state!=4);

    full_path = replace_spaces(full_path);
    fprintf(stdout, "This is the path (not part of request): %s\n\n\n\n", full_path);
    // fflush(stdout);
    //write message
    //n = write(clientFD, "response message", 30);
    
    free(full_path);
    //close connection
    close (clientFD);
  }
  close (sockfd);
  // ^^ this should be in loop
}
