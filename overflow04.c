#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void saru()
{
  char buf[128];

  gets(buf);
  puts(buf);

  printf("system = %p\n", system);
  printf("puts = %p\n", puts);
  printf("buf = %p\n", buf);
}

int main(){
  saru();

  return 0;
}

