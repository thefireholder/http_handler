#include <ctype.h>
#include <stdio.h>

int main()
{
  char str[100]="abcderfABCDEF";
  for(int i = 0; str[i]; i++)
    str[i] = tolower(str[i]);
  printf("%s",str);
}
