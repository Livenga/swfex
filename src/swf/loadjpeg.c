#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/swfheader.h"

void load_jpeg3(int fd, unsigned int extend) {
  int jpg_fd;
  unsigned int character_id = 0, offset_length = 0, alpha_length = 0;
  unsigned char str_character_id[2], str_offset_length[4];
  unsigned char *image_area;
  char jpeg_name[32];

  read(fd, str_character_id, 2);
  character_id = (str_character_id[1] << 8) + str_character_id[0];
  read(fd, str_offset_length, 4);
  offset_length
    = (str_offset_length[3] << 24) + (str_offset_length[2] << 16)
    + (str_offset_length[1] << 8) + (str_offset_length[0]);

  alpha_length = extend - (offset_length + 6);

  sprintf(jpeg_name, "%u.jpg", character_id);

  jpg_fd = open(jpeg_name, O_WRONLY | O_CREAT, 0644);
  if(jpg_fd != EOF) {
    image_area = (unsigned char *)calloc(offset_length, sizeof(unsigned char));
    size_t bytes = read(fd, image_area, offset_length * sizeof(unsigned char));
    write(jpg_fd, image_area,  bytes);
    free(image_area);
    close(jpg_fd);
    lseek(fd, alpha_length, SEEK_CUR);
  }
  else {
    lseek(fd, extend - 6, SEEK_CUR);
  }
}
