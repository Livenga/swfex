#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/swfheader.h"

//#define SWF_PATH "vptgugtdxqec.swf"
#define SWF_PATH "wujywbyjntbp.swf"
extern int swfdump(const char *path);

int main(int argc, char *argv[])
{
  int i;
  if(argc < 2) {
    fprintf(stderr, "%s: Operand Error.\n", argv[0]);
    return EOF;
  }

  for(i = 1; i < argc; i++) {
    swfdump(argv[i]);
  }
  return 0;
}
