#include <stdio.h>
#include <unistd.h>

int remove_temporary(const char *path) {
  if(unlink(path) < 0) {
    perror(path); return EOF;
  }
  return 0;
}
