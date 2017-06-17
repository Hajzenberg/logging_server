#include "inc.h"


int do_start_alarm(message *m_ptr){
	return 111;
}
int do_stop_alarm(message *m_ptr){
	return 222;
}
int do_snooze(message *m_ptr){
	return 333;
}


/*===========================================================================*
 *		            sef_cb_init_fresh                                *
 *===========================================================================*/
int sef_cb_init_fresh(int UNUSED(type), sef_init_info_t *info)
{
	return (OK);
}
