#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <zlib.h>

#define BUFFER_SIZE (4096)

int inflate_zlib(int fd, const char *output_path) {
  int err = 0, output;
  unsigned char in_buffer[BUFFER_SIZE], out_buffer[BUFFER_SIZE];
  z_stream stream;

  /* Initialize */
  stream.zalloc = Z_NULL;
  stream.zfree  = Z_NULL;
  stream.opaque = Z_NULL;
  stream.next_in  = Z_NULL;
  stream.avail_in = 0;
  memset(in_buffer, '\0', BUFFER_SIZE);
  memset(out_buffer, '\0', BUFFER_SIZE);

  if((output = open(output_path, O_WRONLY | O_CREAT, 0644)) == EOF) {
    perror(output_path); return EOF;
  }

  if((err = inflateInit(&stream)) != Z_OK) {
    fprintf(stderr, "伸長の初期化に失敗.\n");
    return EOF;
  }

  while(1) {
    stream.avail_in = read(fd, in_buffer, BUFFER_SIZE);
    stream.next_in = in_buffer;
    if(stream.avail_in == 0) break;

    while(1) {
      stream.next_out = out_buffer;
      stream.avail_out = BUFFER_SIZE;
      err = inflate(&stream, Z_NO_FLUSH);
      switch(err) {
        case Z_NEED_DICT:
        case Z_STREAM_ERROR:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR: fprintf(stderr, "Error.\n"); return EOF;
      }
      write(output, out_buffer, BUFFER_SIZE - stream.avail_out);
      if(stream.avail_out != 0) break;
    }
    if(err == Z_STREAM_END) break;
  }

  if((err = inflateEnd(&stream)) != Z_OK) {
    fprintf(stderr, "伸長の終了に失敗.\n");
    return EOF;
  }

  close(output);
  return 1;
}
