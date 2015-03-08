#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/swfheader.h"
#include "../include/swftags.h"

extern int convert_png(const int id);

int swf_definebitsjpeg(int fd, const int tag_size) {
  int image_fd;
  char imageID[4096];
  size_t bytes  = 0;
  unsigned int read_bytes = 0, image_size = 0;
  unsigned char characterID[2], *image_field;

  if((bytes = read(fd, characterID, 2)) != 2) {
    return EOF;
  }
  read_bytes += bytes;
  image_size = tag_size - read_bytes;

  sprintf(imageID, "%d.jpg",
      (characterID[1] << 8) + (characterID[0] << 0));

  image_fd = open(imageID, O_WRONLY | O_CREAT, 0644);

  image_field = (unsigned char *)calloc(image_size, sizeof(unsigned char));

  if((bytes = read(fd, image_field, image_size)) != image_size) {
    free(image_field); return EOF;
  }

  if(image_fd > 0) {
    write(image_fd, image_field, bytes);
    close(image_fd);
  }
  free(image_field);
  return (characterID[1] << 8) + (characterID[0]);
}

int swf_definebitsjpeg2(int fd, const int tag_size) {
  int image_fd;
  char imageID[4096];
  size_t bytes  = 0;
  unsigned int read_bytes = 0, image_size = 0;
  unsigned char characterID[2], *image_field, void_area[4];

  if((bytes = read(fd, characterID, 2)) != 2) {
    return EOF;
  }
  read_bytes += bytes;

  sprintf(imageID, "%d.jpg",
      (characterID[1] << 8) + (characterID[0] << 0));

  /*
  bytes = read(fd, void_area, 4);
  read_bytes += bytes;
  image_size = tag_size - read_bytes;
  */

  image_fd = open(imageID, O_WRONLY | O_CREAT, 0644);

  image_field = (unsigned char *)calloc(image_size, sizeof(unsigned char));

  if((bytes = read(fd, image_field, image_size)) != image_size) {
    free(image_field); return EOF;
  }

  if(image_fd > 0) {
    write(image_fd, image_field, bytes);
    close(image_fd);
  }
  free(image_field);
  //return 0;
  return (characterID[1] << 8) + (characterID[0]);
}

int swf_definebitsjpeg3(int fd, const int tag_size) {
  int image_fd, alpha_fd, image_id = 0;
  char imageID[4096], alphaID[4096];
  size_t bytes = 0;
  unsigned char characterID[2], offset_size_str[4], *load_space;
  unsigned int offset_size = 0, alpha_size = 0;

  if(read(fd, characterID, 2) != 2) return EOF;
  if(read(fd, offset_size_str, 4) != 4) return EOF;

  image_id = (characterID[1] << 8) + (characterID[0] << 0);
  sprintf(imageID, "%d.jpg", image_id);
  sprintf(alphaID, "%d.zlib", image_id);

  offset_size = 
    (offset_size_str[3] << 24) + (offset_size_str[2] << 16) +
    (offset_size_str[1] << 8) + (offset_size_str[0] << 0);

  alpha_size = tag_size - offset_size - 6;

  image_fd = open(imageID, O_WRONLY | O_CREAT, 0644);
  alpha_fd = open(alphaID, O_WRONLY | O_CREAT, 0644);

  if(image_fd > 0 && alpha_fd > 0) {
    load_space = (unsigned char *)calloc(offset_size, sizeof(unsigned char));
    bytes = read(fd, load_space, offset_size);
    if(bytes <= 0) {
      free(load_space); close(image_fd); close(alpha_fd); return EOF;
    }
    write(image_fd, load_space, bytes);
    close(image_fd); free(load_space);

    load_space = (unsigned char *)calloc(alpha_size, sizeof(unsigned char));
    bytes = read(fd, load_space, alpha_size);
    if(bytes <= 0) {
      free(load_space); close(image_fd); close(alpha_fd); return EOF;
    }
    write(alpha_fd, load_space, alpha_size);
    close(alpha_fd);
    free(load_space);

    /* 画像変換 */
    convert_png(image_id);
  }
  //return 0;
  return (characterID[1] << 8) + (characterID[0]);
}
