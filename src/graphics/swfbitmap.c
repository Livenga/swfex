#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <png.h>
#include "../../include/swfheader.h"
#include "../../include/swftags.h"
#include "../../include/swftag_struct.h"

extern int inflate_zlib(int fd, const char *output_path);
extern int remove_temporary(const char *path);

unsigned char **read_png_ARGB(unsigned char **img, const char *bitmap_data_inf, const int image_width, const int image_height) {
  int i, j, fd;

  if((fd = open(bitmap_data_inf, O_RDONLY)) == EOF) return NULL;
  for(i = 0; i < image_height; i++) {
    for(j = 0; j < image_width; j++) {
      read(fd, &img[i][j * 4 + 3], 1); // A
      read(fd, &img[i][j * 4 + 0], 1); // R
      read(fd, &img[i][j * 4 + 1], 1); // G
      read(fd, &img[i][j * 4 + 2], 1); // B
    }
  }

  close(fd);
  return img;
}

int make_bitmap(const swf_lossless2 bitmap_info, const char *bitmap_data_inf) {
  png_structp png_ptr;
  png_infop png_info;
  FILE *bitmap_fp;
  int i;
  char bitmap_path[4096];
  unsigned char **img = NULL;

  sprintf(bitmap_path, "%d.png", bitmap_info.swf_character_id);
  if((bitmap_fp = fopen(bitmap_path, "wb")) == NULL) {
    perror(bitmap_path); return EOF;
  }

  /* 画像格納メモリの確保 */
  img = (unsigned char **)calloc(bitmap_info.swf_bitmap_height, sizeof(unsigned char *));
  for(i = 0; i < bitmap_info.swf_bitmap_height; i++) {
    img[i] = (unsigned char *)calloc(bitmap_info.swf_bitmap_width, sizeof(unsigned char) * 4);
  }

  if((img = read_png_ARGB(img, bitmap_data_inf,
          bitmap_info.swf_bitmap_width, bitmap_info.swf_bitmap_height)) == NULL) {
    for(i = 0; i < bitmap_info.swf_bitmap_height; i++)
      free(img[i]);
    free(img);
    fclose(bitmap_fp);
    remove_temporary(bitmap_path);
    return EOF;
  }

  /* 最小限の情報のPNGデータの作製 */
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  png_info = png_create_info_struct(png_ptr);

  png_init_io(png_ptr, bitmap_fp);
  png_set_IHDR(png_ptr, png_info,
      bitmap_info.swf_bitmap_width, bitmap_info.swf_bitmap_height,
      8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_write_info(png_ptr, png_info);
  png_write_image(png_ptr, img);
  png_write_end(png_ptr, png_info);
  png_destroy_write_struct(&png_ptr, &png_info);

  /* 画像格納メモリの開放 */
  for(i = 0; i < bitmap_info.swf_bitmap_height; i++)
    free(img[i]);
  free(img);
  fclose(bitmap_fp);
  return 1;
}

int swf_definebitslossless2(int fd, const int tag_number, const int tag_size) {
  swf_lossless2 bitmap_info;
  unsigned char two_bytes[2];
  unsigned int read_bytes = 0;
  size_t bytes = 0;
  int bitmap_data_def;
  char bitmap_data_name[4096], bitmap_data_inf[4096];

  /* 初期化 */
  memset(&bitmap_info, 0, sizeof(swf_lossless2));
  bitmap_info.swf_tag_type = tag_number;
  /* CharacterID 格納 */
  if((bytes = read(fd, two_bytes, 2)) < 0) return EOF;
  read_bytes += bytes;
  bitmap_info.swf_character_id = (two_bytes[1] << 8) + (two_bytes[0] << 0);
  sprintf(bitmap_data_name, "%d.def", bitmap_info.swf_character_id);
  
  /* BitmapFormat 格納 */
  if((bytes = read(fd, (unsigned char *)&bitmap_info.swf_bitmap_format, 1)) < 0)
    return EOF;
  read_bytes += bytes;

  /* BitmapWidth */
  if((bytes = read(fd, two_bytes, 2)) < 0) return EOF;
  bitmap_info.swf_bitmap_width= (two_bytes[1] << 8) + (two_bytes[0] << 0);
  read_bytes += bytes;

  /* BitmapHeight */
  if((bytes = read(fd, two_bytes, 2)) < 0) return EOF;
  bitmap_info.swf_bitmap_height = (two_bytes[1] << 8) + (two_bytes[0] << 0);
  read_bytes += bytes;

  /* Bitmap Format が 3 の場合 */
  if(bitmap_info.swf_bitmap_format == 3) {
    if((bytes = read(fd, (unsigned char *)&bitmap_info.swf_bitmap_colortable_size, 1)) < 0) return EOF;
    read_bytes  += bytes;
  }

  /* Bitmap データサイズの取得とメモリ領域の拡張 */
  bitmap_info.swf_bitmap_size = (unsigned int)(tag_size - read_bytes);
  bitmap_info.swf_bitmap_data = (unsigned char *)calloc(bitmap_info.swf_bitmap_size, sizeof(unsigned char));

  /* zlib Bitmap データの取得 */
  bitmap_data_def = open(bitmap_data_name, O_WRONLY | O_CREAT, 0644);
  bytes = read(fd, bitmap_info.swf_bitmap_data, bitmap_info.swf_bitmap_size);
  write(bitmap_data_def, bitmap_info.swf_bitmap_data, bitmap_info.swf_bitmap_size);
  close(bitmap_data_def);
  free(bitmap_info.swf_bitmap_data);

  /* zlib Bitmap データの伸張 */
  bitmap_data_def = open(bitmap_data_name, O_RDONLY);
  sprintf(bitmap_data_inf, "%d.inf", bitmap_info.swf_character_id);
  inflate_zlib(bitmap_data_def, bitmap_data_inf);
  close(bitmap_data_def);
  remove_temporary(bitmap_data_name);

  /* PNGデータの作製 */
  make_bitmap(bitmap_info, bitmap_data_inf);
  /* 一時データの削除 */
  remove_temporary(bitmap_data_inf);
  return 0;
}
