

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <string.h>
int main()
{
  int fd = open("a.txt",O_RDONLY);
  char buffer[3]; int n;

  do {
  memset(buffer, 0, 3);
  n = read(fd, buffer,3);
  printf("%d:%s\n",n,buffer);
  }
  while(n==3);

}
