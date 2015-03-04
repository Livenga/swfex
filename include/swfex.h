/* src/graphics/swfjpg.c */
extern int swf_definebitsjpeg(int fd, const int tag_size);
extern int swf_definebitsjpeg2(int fd, const int tag_size);
extern int swf_definebitsjpeg3(int fd, const int tag_size);
extern int swf_definebitslossless2(int fd, const int tag_number, const int tag_size);
/* src/sound/swfsound.c */
extern int swf_definesound(int fd, const int tag_number, const int tag_size);

/* src/swf/swftag.c */
extern char *swf_tag_function(const int number);
