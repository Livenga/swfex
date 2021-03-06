#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../../include/swfheader.h"
#include "../../include/swftags.h"
#include "../../include/swfex.h"
#include "../../include/swfutil.h"

extern int getLZMA_data(int fd, lzma_header *head, const char *lzma_path);
extern int decompressLZMA(const char *lzma_path, const unsigned long int  uncompress_length);

int swf_get_header(int fd, swf_header *head) {
  unsigned char tsize[4];
  if(read(fd, head->f_head, 3) != 3) {
    fprintf(stderr, "ヘッダの読み込みに失敗.\n"); return EOF;
  }
  if(read(fd, (unsigned char *)&head->f_version, 1) != 1) {
    fprintf(stderr, "バージョンの読み込みに失敗.\n"); return EOF;
  }
  if(read(fd, tsize, 4) != 4) {
    fprintf(stderr, "フラッシュサイズの取得に失敗.\n"); return EOF;
  }
  head->f_size =
    (tsize[3] << 24) + (tsize[2] << 16) +
    (tsize[1] << 8) + (tsize[0] << 0);
  return 1;
}

int swf_get_property(int fd, swf_property *property) {
  unsigned char rect, upRate, downRate;
  int skip_seek = 0;
  char frame_rate[32];

  if(read(fd, &rect, 1) != 1) {
    fprintf(stderr, "レクトサイズの取得に失敗.\n"); return EOF;
  }
  property->f_bit_length = rect >> 3;
  //skip_seek = (((property->f_bit_length * 4) - 3) / 8) + 1;
  skip_seek = round_up((double)(property->f_bit_length * 4 - 3) / 8.0);
  lseek(fd, skip_seek, SEEK_CUR);
  read(fd, &downRate, 1); read(fd, &upRate, 1);
  sprintf(frame_rate, "%d.%d", upRate, downRate);
  property->f_frame_rate = atof(frame_rate);
  read(fd, &downRate, 1); read(fd, &upRate, 1);
  property->f_number_frame = (upRate << 8) + (downRate << 0);

  return 1;
}

long int swf_get_tag(int fd) {
  int err = 0;
  unsigned int tag_header = 0, tag_number, tag_length, tag_extend;
  unsigned char tag[2];
  unsigned char extend_tag[4];

  if((err = read(fd, tag, 2)) != 2) {
    if(err == 0) return SWF_OK;
    fprintf(stderr, "タグの読み込みに失敗.\n"); return 0;
  }
  tag_header = (tag[1] << 8) + (tag[0] << 0);
  tag_number = tag_header >> 6;
  tag_length = tag_header & 0x3F;

  if(tag_length == 0x3F) {
    if(read(fd, extend_tag, 4) != 4) {
      fprintf(stderr, "タグの読み込みに失敗.\n"); return EOF;
    }
    tag_extend = (extend_tag[3] << 24) + (extend_tag[2] << 16)
      + (extend_tag[1] << 8) + (extend_tag[0] << 0);
    tag_length = tag_extend;
  }

  printf("[%04x] %-28s %u ", tag_number, swf_tag_function(tag_number), tag_length);
  if(tag_number == END) return SWF_OK;

  switch(tag_number) {
    case DEFINEBITS:
      err = swf_definebitsjpeg(fd, tag_length);
      printf("(%d)", err);
      tag_length = 0;
      break;
    case DEFINEBITSJPEG2:
      err = swf_definebitsjpeg2(fd, tag_length);
      printf("(%d)", err);
      tag_length = 0;
      break;
    case DEFINEBITSJPEG3:
      err = swf_definebitsjpeg3(fd, tag_length);
      printf("(%d)", err);
      tag_length = 0;
      break;
    case DEFINEBITSLOSSLESS2:
      tag_length = swf_definebitslossless2(fd, tag_number, tag_length);
      break;
    case DEFINESOUND:
      tag_length = swf_definesound(fd, tag_number, tag_length);
      break;
  }
  putchar('\n');

  return tag_length;
}

int swf_load(int fd, const unsigned long int swf_size) {
  unsigned int seek_bytes = 1;
  static unsigned long int loaded_bytes = 0;
  while(seek_bytes != SWF_OK) {
    seek_bytes = swf_get_tag(fd);
    loaded_bytes += seek_bytes;
    if(loaded_bytes == swf_size) break;
    lseek(fd, seek_bytes, SEEK_CUR);
  }
  return 1;
}

int swfdump(const char *path) {
  int fd;
  char *p, output_path[4096];
  unsigned long int swf_size = 0;
  swf_header s_header;
  swf_property s_prop;
  lzma_header l_header;

  if((fd = open(path, O_RDONLY)) == EOF) {
    perror(path); return EOF;
  }

  /* SWF ヘッダの取得 */
  if(swf_get_header(fd, &s_header) == EOF) {
    close(fd);
    return EOF;
  }

  printf("[SWF] Header Type : %s\n", s_header.f_head);
  printf("[SWF] Adobe Flash Version : %u\n", s_header.f_version);
  printf("[SWF] Uncompless Size : %lu\n", s_header.f_size);

  strcpy(output_path, path);
  p = strchr(output_path, '.');
  *p = '\0';
  mkdir(output_path, 0777);
  chdir(output_path);

  switch(s_header.f_head[0]) {
    case 'F': // 非圧縮
      break;
    case 'C': // Zlib 圧縮
      sprintf(output_path, "%s.zlib", output_path);
      inflate_zlib(fd, output_path);
      close(fd);
      if((fd = open(output_path, O_RDONLY)) == EOF) {
        perror(output_path); return EOF;
      }
      break;
    case 'Z': // LZMA 圧縮
      sprintf(output_path, "%s.lzma", output_path);
      l_header.l_uncompress_size = s_header.f_size - 8;
      /* LZMA 圧縮データの取得 */
      if(getLZMA_data(fd, &l_header, output_path) == EOF) {
        close(fd); return EOF;
      }
      printf("[LZMA] Compress Length: %lu\n", l_header.l_compress_size);
      printf("[LZMA] Uncompress Length: %lu\n", l_header.l_uncompress_size);
      close(fd);
      decompressLZMA(output_path, l_header.l_uncompress_size + 8);
      p = strchr(output_path, '.'); *p = '\0';
      sprintf(output_path, "%s.temp", output_path);
      if((fd = open(output_path, O_RDONLY)) == EOF) {
        perror(output_path); return EOF;
      }

      break;
  }

  swf_size = s_header.f_size;
  /* RECT, FrameRate, etc... 読み込み */
  swf_get_property(fd, &s_prop);
  swf_size -= lseek(fd, 0L, SEEK_CUR);

  printf("[SWF] Bit Length: %u\n", s_prop.f_bit_length);
  printf("[SWF] Frame Rate : %f\n", s_prop.f_frame_rate);
  printf("[SWF] Number Of Frame : %f\n", s_prop.f_number_frame);

  /* SWF Chunk読み込み開始 */
  if(s_header.f_head[0] == 'C' || s_header.f_head[0] == 'F')
    swf_load(fd, swf_size);
  close(fd);

  switch(s_header.f_head[0]) {
    case 'C':
      //remove_temporary(output_path); // 一時ファイルの削除
      break;
    case 'Z': break;
  }

  chdir("..");
  return 1;
}
