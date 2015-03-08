#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <lzma.h>
#include "../../include/swfheader.h"
#include "../../include/swftags.h"
#include "../../include/swfex.h"
#include "../../include/swfutil.h"

#define BUFFER_SIZE (4096)
//#define BUFFER_SIZE (256)

int getLZMA_data(int fd, lzma_header *head, const char *lzma_path) {
  int lzma_fd;
  unsigned char l_length[4];
  char buffer[BUFFER_SIZE];
  size_t bytes = 0;

  if((lzma_fd = open(lzma_path, O_WRONLY | O_CREAT, 0644)) == EOF) {
    fprintf(stderr, "LZMA file descripter のオープンに失敗.\n"); return EOF;
  }

  /* LZMA 情報 */
  if(read(fd, l_length, 4) != 4) {
    close(lzma_fd); return EOF;
  }

  head->l_compress_size =
    (l_length[3] << 24) + (l_length[2] << 16) +
    (l_length[1] << 8) + (l_length[0] << 0);

  if(read(fd, head->l_prop, 5) != 5) {
    close(lzma_fd); return EOF;
  }

  if(write(lzma_fd, head->l_prop, 5) != 5) {
    close(lzma_fd); return EOF;
  }

  // write the uncompress length
  l_length[0] = head->l_uncompress_size & 0xFF;
  l_length[1] = (head->l_uncompress_size >> 8) & 0xFF;
  l_length[2] = (head->l_uncompress_size >> 16) & 0xFF;
  l_length[3] = (head->l_uncompress_size >> 24) & 0xFF;

  if(write(lzma_fd, l_length, 4) != 4) {
    close(lzma_fd); return EOF;
  }

  memset(l_length, 0x00, sizeof(l_length));

  if(write(lzma_fd, l_length, 4) != 4) {
    close(lzma_fd); return EOF;
  }

  lseek(fd, 17L, SEEK_SET);
  do {
    if((bytes = read(fd, buffer, BUFFER_SIZE)) < 0) {
      close(lzma_fd); return EOF;
    }
    if(write(lzma_fd, buffer, bytes) < 0) {
      close(lzma_fd); return EOF;
    }
    memset(buffer, '\0', BUFFER_SIZE);
  } while(bytes != 0);

  close(lzma_fd);
  return 0;
}

int init_decoder(lzma_stream *strm) {
  const char *msg;
  lzma_ret ret = lzma_alone_decoder(strm, (128 << 20));

  if(ret == LZMA_OK) return 0;
  switch(ret) {
    case LZMA_MEM_ERROR:
      msg = "Memory allocation falied."; break;
    case LZMA_OPTIONS_ERROR:
      msg = "Unsupported decompressor flags."; break;
    default:
      msg = "Unknown error, possibly a bug."; break;
  }

  fprintf(stderr, "Error initializing the decoder: %s(error code: %u)\n",
      msg, ret);
  return EOF;
}

#ifdef __SYSCALL__
int decompressLZMA(const char *lzma_path) {
  int output_fd, input_fd;
  size_t write_size = 0;
  char path[4096], uncompress_path[4096], *p;
  uint8_t input_buffer[BUFFER_SIZE], output_buffer[BUFFER_SIZE];
  /* initialize lzma stream */
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret ret = LZMA_OK;
  lzma_action action = LZMA_RUN;

  if(init_decoder(&strm) == EOF)
    return EOF;

  strcpy(path, lzma_path);
  p = strchr(path, '.');
  *p = '\0';
  sprintf(uncompress_path, "%s.temp", path);

  /* file open */
  if((input_fd = open(lzma_path, O_RDONLY)) == EOF) return EOF;
  if((output_fd = open(uncompress_path, O_WRONLY | O_CREAT, 0644)) == EOF) {
    close(input_fd);
    return EOF;
  }

  do {
    strm.next_in = input_buffer;
    strm.avail_in = read(input_fd, input_buffer, sizeof(uint8_t) * BUFFER_SIZE);
    if(strm.avail_in == 0)
      action = LZMA_FINISH;

    do {
      strm.next_out = output_buffer;
      strm.avail_out = sizeof(output_buffer);

      ret = lzma_code(&strm, action);
      //assert((ret == LZMA_OK) || (ret == LZMA_STREAM_END) || (action == LZMA_FINISH));

      write_size = sizeof(output_buffer) - strm.avail_out;
      write(output_fd, output_buffer, sizeof(uint8_t) * write_size);

    } while(strm.avail_out == 0 && ret == LZMA_STREAM_END);
  } while(action != LZMA_FINISH);

  /* file close */
  close(input_fd); close(output_fd);
  lzma_end(&strm);
  return 0;
}
#else
int decompressLZMA(const char *lzma_path, const unsigned long int uncompress_length) {
  FILE *output_fp, *input_fp;
  size_t write_size = 0;
  uint8_t input_buffer[BUFFER_SIZE], output_buffer[BUFFER_SIZE];
  char path[4096], uncompress_path[4096], *p;
  /* initialize lzma stream */
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_ret ret = LZMA_OK;
  lzma_action action = LZMA_RUN;

  //if(init_decoder(&strm) == EOF)
    //return EOF;
  //ret = lzma_alone_decoder(&strm, (256 * 1024 * 1024));
  ret = lzma_auto_decoder(&strm, (256 * 1024 * 1024), 0);

  strcpy(path, lzma_path);
  p = strchr(path, '.');
  *p = '\0';
  sprintf(uncompress_path, "%s.temp", path);

  /* file open */
  if((input_fp = fopen(lzma_path, "rb")) == NULL) return EOF;
  if((output_fp = fopen(uncompress_path, "wb")) == NULL) {
    fclose(input_fp);
    return EOF;
  }

  unsigned char swf_head[4];
  swf_head[0] = 'F'; swf_head[1] = 'W'; swf_head[2] = 'S';
  swf_head[3] = 26;
  fwrite(swf_head, sizeof(unsigned char), 4, output_fp);
  swf_head[0] = uncompress_length & 0xFF;
  swf_head[1] = (uncompress_length >> 8) & 0xFF;
  swf_head[2] = (uncompress_length >> 16) & 0xFF;
  swf_head[3] = (uncompress_length >> 24) & 0xFF;
  fwrite(swf_head, sizeof(unsigned char), 4, output_fp);

  do {
    strm.next_in = input_buffer;
    strm.avail_in = fread(input_buffer, sizeof(uint8_t), BUFFER_SIZE, input_fp);
    if(strm.avail_in == 0)
      action = LZMA_FINISH;

    do {
      strm.next_out = output_buffer;
      strm.avail_out = sizeof(output_buffer);

      ret = lzma_code(&strm, action);
      if(!(ret == LZMA_OK) || (ret == LZMA_STREAM_END)) {
        switch(ret) {
          case LZMA_NO_CHECK: printf("LZMA_NO_CHECK\n"); break;
          case LZMA_UNSUPPORTED_CHECK: printf("LZMA_UNSUPPORTED_CHECK\n"); break;
          case LZMA_MEM_ERROR: printf("LZMA_MEM_ERROR\n"); break;
          case LZMA_MEMLIMIT_ERROR: printf("LZMA_MEMLIMIT_ERROR\n"); break;
          case LZMA_FORMAT_ERROR: printf("LZMA_FORMAT_ERROR\n"); break;
          case LZMA_OPTIONS_ERROR: printf("LZMA_OPTIONS_ERROR\n"); break;
          case LZMA_DATA_ERROR: printf("LZMA_DATA_ERROR\n"); break;
          case LZMA_BUF_ERROR: printf("LZMA_BUF_ERROR\n"); break;
          case LZMA_PROG_ERROR: printf("LZMA_PROG_ERROR\n"); break;
          default: printf("UNKNOWN ERROR\n"); break;
        }
      }
      write_size = sizeof(output_buffer) - strm.avail_out;
      fwrite(output_buffer, sizeof(uint8_t), write_size, output_fp);
    } while(strm.avail_out == 0 && ret != LZMA_STREAM_END);
  } while(action != LZMA_FINISH);

  /* file close */
  fclose(input_fp); fclose(output_fp);
  lzma_end(&strm);
  return 0;
}
#endif
