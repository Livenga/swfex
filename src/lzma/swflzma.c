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

#define BUFFER_SIZE (65536)
//#define BUFFER_SIZE (256)

int getLZMA_data(int fd, lzma_header *head, const char *lzma_path) {
  int lzma_fd;
  unsigned char l_length[4];
  char buffer[BUFFER_SIZE];
  size_t bytes = 0;

  if((lzma_fd = open(lzma_path, O_WRONLY | O_CREAT, 0644)) == EOF) {
    fprintf(stderr, "LZMA fd の作製に失敗.\n"); return EOF;
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
  //lzma_ret ret = lzma_stream_decoder(strm, UINT64_MAX, LZMA_CONCATENATED);
  lzma_ret ret = lzma_alone_decoder(strm, UINT64_MAX);

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

int decompressLZMA(const char *lzma_path) {
  int output_fd, input_fd;
  size_t write_size = 0;
  char path[4096], uncompress_path[4096], *p;
  const char *msg;
  unsigned char input_buffer[BUFFER_SIZE], output_buffer[BUFFER_SIZE];
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

  strm.next_in = NULL;
  strm.avail_in = 0;
  strm.next_out = output_buffer;
  strm.avail_out = sizeof(output_buffer);

  while(1) {
    strm.next_in = input_buffer;
    strm.avail_in = read(input_fd, input_buffer, BUFFER_SIZE);
    if(strm.avail_in == 0) action = LZMA_FINISH;

    while(1) {
      strm.next_out = output_buffer;
      strm.avail_out = sizeof(output_buffer);

      ret = lzma_code(&strm, action);
      if(strm.avail_out == 0 || ret == LZMA_STREAM_END) {
        write_size = sizeof(output_buffer) - strm.avail_out;
        if(write(output_fd, output_buffer, write_size) != write_size) {
          lzma_end(&strm);
          close(input_fd); close(output_fd); return EOF;
        }
      }

      if(ret != LZMA_OK) {
        if(ret == LZMA_STREAM_END)
          break;
        switch(ret) {
          case LZMA_MEM_ERROR:
            msg = "Memory allocation failed";
            break;
          case LZMA_FORMAT_ERROR:
            msg = "The input is not in the .xz format";
            break;
          case LZMA_OPTIONS_ERROR:
            msg = "Unsupported compression options";
            break;
          case LZMA_DATA_ERROR:
            msg = "Compressed file is corrupt";
            break;
          case LZMA_BUF_ERROR:
            msg = "Compressed file is truncated or otherwise corrupt";
            break;
          default:
            msg = "Unknown error, possibly a bug";
            break;
        }
        fprintf(stderr, "Decoder Error: %s (error code: %u)\n", msg, ret);
        fprintf(stderr, "FileDescriptor Seek %u\n", (unsigned)lseek(input_fd, 0L, SEEK_CUR));
        close(input_fd); close(output_fd);
        lzma_end(&strm);
        return EOF;
      }
    }
    if(action == LZMA_FINISH) break;
  }

  /* file close */
  close(input_fd); close(output_fd);
  lzma_end(&strm);
  return 0;
}

int overrunning(const char *lzma_path, const lzma_header head) {
  int output_fd, input_fd;
  size_t write_size = 0;
  char path[4096], uncompress_path[4096], *p;
  unsigned char *input_buffer, *output_buffer;
  unsigned long int lzma_length = 0;
  /* initialize lzma stream */
  lzma_stream strm = LZMA_STREAM_INIT;
  lzma_action action = LZMA_RUN;
  lzma_ret ret = LZMA_OK;
  uint64_t decompress_memory_limit = 40 * (1 << 20);
  uint64_t decompress_buffer_size;
  size_t input_position = 0, output_position = 0;

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

  decompress_buffer_size = lseek(input_fd, 0L, SEEK_END);
  lseek(input_fd, 0L, SEEK_SET);

  input_buffer = (unsigned char *)calloc(decompress_buffer_size,
      sizeof(unsigned char));
  output_buffer = (unsigned char *)calloc(head.l_uncompress_size, sizeof(unsigned char));

  read(input_fd, input_buffer, decompress_buffer_size);
  ret = lzma_stream_buffer_decode(
      &decompress_memory_limit,
      LZMA_TELL_NO_CHECK,
      NULL,
      input_buffer,
      &input_position,
      decompress_buffer_size,
      output_buffer,
      &output_position,
      head.l_uncompress_size);

  const char *msg;
  switch(ret) {
    case LZMA_MEM_ERROR:
      msg = "Memory allocation failed";
      break;
    case LZMA_FORMAT_ERROR:
      msg = "The input is not in the .xz format";
      break;
    case LZMA_OPTIONS_ERROR:
      msg = "Unsupported compression options";
      break;
    case LZMA_DATA_ERROR:
      msg = "Compressed file is corrupt";
      break;
    case LZMA_BUF_ERROR:
      msg = "Compressed file is truncated or otherwise corrupt";
      break;
    default:
      msg = "Unknown error, possibly a bug";
      break;
  }
  fprintf(stderr, "Decoder Error: %s (error code: %u)\n", msg, ret);
  write(output_fd, output_buffer, write_size);

  /* file close */
  free(input_buffer); free(output_buffer);
  close(input_fd); close(output_fd);
  lzma_end(&strm);
  return 0;
}
