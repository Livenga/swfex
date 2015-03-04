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

int swf_definesound(int fd, const int tag_number, const int tag_size) {
  swf_sound sound_info;
  size_t bytes = 0;
  int sound_data_fd = 0;
  char sound_name[4096];
  unsigned char sound_id[2], sound_count[4], sound_type, *sound_data;
  unsigned int read_bytes = 0, sound_data_size, sound_size;

  sound_info.swf_tag_type = tag_number;
  /* SoundID の読み取り */
  if((bytes = read(fd, sound_id, 2)) < 0) return EOF;
  sound_info.swf_character_id =
    (sound_id[1] << 8) + (sound_id[0]);
  read_bytes += bytes;

  /* SoundType の読み取り */
  if((bytes = read(fd, &sound_type, 1)) < 0) return EOF;
  read_bytes += bytes;

  /* SoundSamleCound の読み取り */
  if((bytes = read(fd, sound_count, 4)) < 0) return EOF;
  read_bytes += bytes;
  sound_info.swf_sound_sample_count =
    (sound_count[3] << 24) + (sound_count[2] << 16)
    + (sound_count[1] << 8) + sound_count[0];

  sound_info.swf_sound_format = (sound_type >> 4);
  sound_info.swf_sound_rate = (sound_type >> 2) & 0x03;

  switch(sound_info.swf_sound_rate) {
    case 0: sound_info.swf_sound_rate = 5512; break;
    case 1: sound_info.swf_sound_rate = 11025; break;
    case 2: sound_info.swf_sound_rate = 22050; break;
    case 3: sound_info.swf_sound_rate = 44100; break;
  }

  sound_info.swf_sound_size = (sound_type >> 1) & 0x01;
  sound_info.swf_sound_sample_count = sound_type & 0x01;

  switch(sound_info.swf_sound_format) {
    case 0:
    case 3: printf("SoundData contains raw\n"); break;
    case 1: printf("SoundData contains ADPCM\n"); break;
    case 2: printf("SoundData contains MP3\n");
            sprintf(sound_name, "%d.mp3", sound_info.swf_character_id);
            break;
    case 4:
    case 5:
    case 6: printf("SoundData contains Nellymoser\n"); break;
    case 11: printf("SoundData contains Speex data\n");
  }

  printf("SoundRate: %d\n", sound_info.swf_sound_rate);
  printf("SoundSize: snd%dBit\n", (sound_info.swf_sound_size == 0 ? 8 : 16));
  printf("SoundType: %s\n", (sound_info.swf_sound_type == 0 ? "sndMono" : "sndStereo"));
  printf("Number of Samples: %u\n",
      sound_info.swf_sound_sample_count);

  if((bytes = read(fd, sound_id, 2)) < 0) return EOF;
  read_bytes += bytes;
  sound_size = (sound_id[1] << 8) + sound_id[0];
  printf("Sound ... %d\n", sound_size);

  sound_data_size = (unsigned int)(tag_size - read_bytes);
  sound_data = (unsigned char *)calloc(sound_data_size, sizeof(unsigned char));
  sound_data_fd = open(sound_name, O_WRONLY | O_CREAT, 0644);
  bytes = read(fd, sound_data, sound_data_size);
  if(sound_data_fd > 0)
    write(sound_data_fd, sound_data, bytes);

  free(sound_data);
  close(sound_data_fd);

  read_bytes += bytes;
  sound_data_size = (unsigned int)(tag_size - read_bytes);
  return sound_data_size;
}
