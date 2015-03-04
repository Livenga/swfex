#define SWF_OK (-2)

/* SWF ファイルヘッダ */
typedef struct swf_header {
  char f_head[3];
  unsigned int f_version;
  unsigned long int f_size;
} swf_header;

/* SWF 情報 */
typedef struct swf_property {
  unsigned int f_bit_length;
  double f_frame_rate;
  double f_number_frame;
} swf_property;

/* src/util.c */
extern int swf_get_header(int fd, swf_header *head);
