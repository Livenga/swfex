typedef struct swf_definebitslossless2 {
  int swf_tag_type;
  int swf_character_id;
  unsigned int swf_bitmap_format; // unsigned int 8
  unsigned int swf_bitmap_width; // unsigned int 16
  unsigned int swf_bitmap_height; // unsigned int 16
  /* 色テーブルに使用する色の数 - 1
   * BitmapFormat = 3 の場合使用 */
  unsigned int swf_bitmap_colortable_size; // unsigned 8
  /* zlib bitmap data */
  unsigned int swf_bitmap_size;
  unsigned char *swf_bitmap_data;
} swf_lossless2;

typedef struct swf_definesound {
  int swf_tag_type;
  int swf_character_id; // unsigned int 16
  unsigned int swf_sound_format; // unsigned byte 4
  unsigned int swf_sound_rate; // unsigned byte 2
  unsigned int swf_sound_size; // unsigned byte 1
  unsigned int swf_sound_type; // unsigned byte 1
  unsigned int swf_sound_sample_count; // unsigned int 32
} swf_sound;
