#ifndef _DS_PROTO_H
#define _DS_PROTO_H

/* Function prototypes. */

/* main.c */
int main(int argc, char **argv);

/* logger.c */
int do_make_counter(message *m_ptr);
int do_increment_counter(message *m_ptr);
int do_get_counter(message *m_ptr);
int sef_cb_init_fresh(int type, sef_init_info_t *info);

#endif
