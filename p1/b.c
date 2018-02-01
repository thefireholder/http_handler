#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
int main(int argc, char* argv[])
{

  FILE *fp;
  char c[] = "this is tutorialspoint\n";
  char buffer[100];

  // Open file for both reading and writing                                                     
  fp = fopen(argv[1], "rb");
  if(fp == 0) {printf("fread: No such file\n"); exit(1);}
  // Seek to the beginning of the file                                                          
  fseek(fp, 0, SEEK_SET);

  // Read and display data                                                                      
  fread(buffer, strlen(c)+1, 1, fp);
  printf("fread: %s\n", buffer);
  fclose(fp);

  memset(buffer,0, 100);
  int fd = open(argv[1],O_RDONLY);
  if(fd<0) {printf("read: No such file\n"); exit(1);}

  read(fd, buffer,99 );
  printf("read: %s\n",buffer);

  return(0);


}
