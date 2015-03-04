#include <stdio.h>
#include <unistd.h>

/* ファイルの削除 */
int remove_temporary(const char *path) {
  if(unlink(path) < 0) {
    perror(path); return EOF;
  }
  return 0;
}

/* 四捨五入 */
int round_up(const double val) {
  const int integer_val = (int)val;
  double temp = val;

  if(val < 1.0) {
    if(val >= 0.5) return 1;
    else return 0;
  }
  else {
    temp = temp - (double)integer_val;
    if(val >= 0.5) return integer_val + 1;
    else return integer_val;
  }

  return 0;
}
