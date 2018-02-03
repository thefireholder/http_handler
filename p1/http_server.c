#include <stdio.h>
#include <sys/types.h> //types for socket.h and netinet/in.h
#include <sys/socket.h>  // structures for sockets
#include <netinet/in.h>  // structures for sockaddr_in
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>  // signal name kill()
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

#define BINARY 0
#define HTML 1
#define JPEG 2
#define GIF 3

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

ssize_t write_with_retry(int fd, const char* buf, size_t size)
{
    ssize_t ret;
    while (size > 0) {
        do
        {
          ret = write(fd, buf, size);
        } while ((ret < 0) && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
        if (ret < 0)
            return ret;
        size -= ret;
        buf += ret;
    }
    return 0;
}

void print_header(int clientFD, int status, int type, int f_size) {
  char dest[1000];
  dest[0]='\0';

  char line[200];
  char* phrase = "OK";
  if(status == 404)
    phrase = "Bad Request";
  int spr = sprintf(line, "HTTP/1.1 %d %s\r\n", status, phrase);
  if(spr<0)
    reportError("sprintf failed", 2);
  strcat(dest, line);

  strcat(dest, "Connection: close\r\n");
  //skipping date
  strcat(dest, "Server: bab server\r\n");
  //skipping last-modified
  spr = sprintf(line, "Content-Length: %d\r\n", f_size);
  strcat(dest, line);

  char* contenttype = "";
  switch(type) {
    case BINARY: contenttype = "application/octet-stream"; break;
    case HTML: contenttype = "text/html"; break;
    case JPEG: contenttype = "image/jpeg"; break;
    case GIF: contenttype = "image/gif"; break;
  }
  spr = sprintf(line, "Content-Type: %s\r\n", contenttype);
  strcat(dest, line);

  strcat(dest, "\r\n");
  // print header
  //int n = write(clientFD, dest, strlen(dest));
  int n = write_with_retry(clientFD, dest, strlen(dest));
}

int file_size(char* path) {
  struct stat buffer;
  int status = stat(path, &buffer);
  if (status==0){
    return buffer.st_size;
  }
  return -1;
}

char* lower_extension(char* full_path) {
  char* lower = (char*) calloc(strlen(full_path) + 1, sizeof(char));
  int i;
  for(i=0; i<strlen(full_path); i++) {
    lower[i] = full_path[i];
  }
  char* dot = strchr(lower, '.');
  if(dot!=NULL) {
    for(char* x = dot + 1; *x != '\0'; x++)
      *x = tolower(*x);
  }
  return lower;
}

int main(int argc, char * argv[])
{
  struct sockaddr_in serverA; //Address structure for server
      
  //store port number
  if (argc < 2) reportError("argument failure: ./http_server portNumber", 1);
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
        break;
      }
      print_request(buffer, &print_state, n);
      if (state < 4) {
        full_path = parse_path(&state, full_path, buffer2, n);
      }
    }
    while(print_state!=4);

    full_path = replace_spaces(full_path);
    // fflush(stdout);
    //write message
    //n = write(clientFD, "response message", 30);

    //open file
    int type = BINARY;
    char* dot = strchr(full_path, '.');
    if (dot!=NULL) {
      for(char* x = dot + 1; *x != '\0'; x++)
        *x = tolower(*x);
      if(strcmp(dot+1, "html")==0||strcmp(dot+1, "htm")==0)
        type = HTML;
      else if(strcmp(dot+1, "jpg")==0||strcmp(dot+1, "jpeg")==0)
        type = JPEG;
      else if(strcmp(dot+1, "gif")==0)
        type = GIF;
      else
        type = BINARY;
    }

    // int fd = open(full_path+1,O_RDONLY);
    //FILE* fd = fopen(full_path + 1, "rb");

    int fd = -1;
    if(strchr(full_path, '.') == NULL) {
      fd = open(full_path+1, O_RDONLY);
    } else {
      DIR* dir;
      struct dirent* ent;
      if ((dir = opendir(".")) != NULL) {
        while((ent = readdir(dir)) != NULL) {
          char* name = lower_extension(ent->d_name);
          if(strcmp(name, full_path+1) == 0){
            int ind;
            for(ind = 0; ind < strlen(ent->d_name); ind++)
              full_path[ind]=(ent->d_name)[ind];
            fd = open(ent->d_name, O_RDONLY);
            free(name);
            break;
          } else {
            free(name);
          }
        }
      }
    }
    

    int status = 200;
    if (fd==-1) {
      //reportError("Read file failed", 2);
      status = 404;
      print_header(clientFD, status, HTML, 39); // , 39, );
      //body
      //int n = write(clientFD, "<html><body><h1>404!</h1></body></html>", 39);
      int n = write_with_retry(clientFD, "<html><body><h1>404!</h1></body></html>", 39);
    } else {
      // file size
      int f_size = file_size(full_path + 1);
      if(f_size == -1) {
        reportError("negative file size", 2);
      }

      //header
      print_header(clientFD, status, type, f_size);
      //body
      char* whole_file = (char*) malloc(f_size + 1);
      // TODO: do more retries
      int num_read = read(fd, whole_file, f_size);
      close(fd);

      //int n = write(clientFD, whole_file, num_read);
      int n = write_with_retry(clientFD, whole_file, num_read);
      free(whole_file);
    }
    
    free(full_path);
    //close connection
    close (clientFD);
  }
  close (sockfd);
  // ^^ this should be in loop
}
