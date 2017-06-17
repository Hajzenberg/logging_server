#ifndef _DS_PROTO_H
#define _DS_PROTO_H

/* Function prototypes. */

/* main.c */
int main(int argc, char **argv);

/* logger.c */
int do_start_log(message *m_ptr);
int do_set_logger_level(message *m_ptr);
int do_write_log(message *m_ptr);
int do_close_log(message *m_ptr);
int do_clear_logs(message *m_ptr);
int sef_cb_init_fresh(int type, sef_init_info_t *info);

#endif
