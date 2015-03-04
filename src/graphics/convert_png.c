#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <jpeglib.h>
#include <png.h>

extern int remove_temporary(const char *path);
extern int inflate_zlib(int fd, const char *output_path);

unsigned char **read_jpeg_image(const int id, unsigned char **img,
    int *image_width, int *image_height) {
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  FILE *fp;

  int i, width, height;
  char jpeg_name[4096];

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  sprintf(jpeg_name, "%d.jpg", id);
  if((fp = fopen(jpeg_name, "rb")) == NULL) {
    perror(jpeg_name); return NULL;
  }

  jpeg_stdio_src(&cinfo, fp);
  jpeg_read_header(&cinfo, TRUE);
  jpeg_start_decompress(&cinfo);

  width = cinfo.output_width;
  height = cinfo.output_height;

  img = (unsigned char **)calloc(height, sizeof(unsigned char *));
  for(i = 0; i < height; i++) {
    img[i] = (unsigned char *)calloc(width, sizeof(unsigned char) * 3);
  }

  while(cinfo.output_scanline < cinfo.output_height) {
    jpeg_read_scanlines(&cinfo, img + cinfo.output_scanline,
        cinfo.output_height - cinfo.output_scanline);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  fclose(fp);

  *image_width = width;
  *image_height = height;

  remove_temporary(jpeg_name);
  return img;
}

unsigned char **read_alpha_image(const int id, unsigned char **alpha,
    const int jpeg_width, const int jpeg_height) {
  int i, fd;
  char zlib_path[4096], inflate_path[4096];
  sprintf(zlib_path, "%d.zlib", id);
  sprintf(inflate_path, "%d.inf", id);

  fd = open(zlib_path, O_RDONLY);
  if(fd < 0) return NULL;

  inflate_zlib(fd, inflate_path);
  close(fd);

  fd = open(inflate_path, O_RDONLY);
  if(fd < 0) return NULL;

  alpha = (unsigned char **)calloc(jpeg_height, sizeof(unsigned char *));
  for(i = 0; i < jpeg_height; i++) {
    alpha[i] = (unsigned char *)calloc(jpeg_width, sizeof(unsigned char));
    read(fd, alpha[i], jpeg_width);
  }
  close(fd);

  remove_temporary(zlib_path);
  remove_temporary(inflate_path);

  return alpha;
}

int convert_png(const int id) {
  FILE *fp;
  char png_path[4096];
  int image_width = 0, image_height = 0, i, j;
  unsigned char **jpeg_data = NULL, **alpha_data = NULL;
  unsigned char **png_data = NULL;
  png_structp png_ptr;
  png_infop png_info;

  sprintf(png_path, "%d.png", id);
  fp = fopen(png_path, "wb");

  if(fp == NULL) return EOF;

  if((jpeg_data = read_jpeg_image(id, jpeg_data,
          &image_width, &image_height)) == NULL) {
    fclose(fp); return EOF;
  }
  if((alpha_data = read_alpha_image(id, alpha_data,
          image_width, image_height)) == NULL) {
    for(i = 0; i < image_height; i++)
      free(jpeg_data[i]);
    free(jpeg_data);
    fclose(fp); return EOF;
  }

  png_data = (unsigned char **)calloc(image_height, sizeof(unsigned char *));
  for(i = 0; i < image_height; i++) {
    png_data[i] = (unsigned char *)calloc(image_width, sizeof(unsigned char) * 4);
    for(j = 0; j < image_width; j++) {
      png_data[i][j * 4 + 0] = jpeg_data[i][j * 3 + 0];
      png_data[i][j * 4 + 1] = jpeg_data[i][j * 3 + 1];
      png_data[i][j * 4 + 2] = jpeg_data[i][j * 3 + 2];
      png_data[i][j * 4 + 3] = alpha_data[i][j];
    }
  }

  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
      NULL, NULL, NULL);
  png_info = png_create_info_struct(png_ptr);
  png_init_io(png_ptr, fp);

  png_set_IHDR(png_ptr, png_info, image_width, image_height,
      8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
      PNG_FILTER_TYPE_BASE);

  png_write_info(png_ptr, png_info);
  png_write_image(png_ptr, png_data);
  png_write_end(png_ptr, png_info);
  png_destroy_write_struct(&png_ptr, &png_info);

  for(i = 0; i < image_height; i++) {
    free(jpeg_data[i]);
    free(alpha_data[i]);
    free(png_data[i]);
  }
  free(jpeg_data);
  free(alpha_data);
  free(png_data);
  fclose(fp);

  return 1;
}
