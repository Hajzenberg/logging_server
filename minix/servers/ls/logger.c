#include "inc.h"

//u store.c se nalaza neke funkcije kojima se dobija process name iz struktura koje
//on ima u sebi, moze nam biti korisno kad mi budemo radili nas logger

//1:08:35 bane prica kako dohvatiti proces na osnovu endpointa

#define MAX_COUNT 128
#define MAX_NAME 16

int currentcount = 0;

struct mycount {
	char name [MAX_NAME];
	int value;
} mycount [MAX_COUNT];

int do_make_counter(message *m_ptr){

	char *newname = m_ptr -> m_m3.m3ca1;	//pogledaj u ipc.h
	strcpy(mycount[currentcount].name, newname);
	mycount[currentcount].value = 0;	//tehnicki ne moramo ovo da radimo jer je sve setovano na 0 u sef_cb_init_fresh
	currentcount++;

	return(OK);
}
int do_increment_counter(message *m_ptr){

	char* toincrement = m_ptr -> m_m3.m3ca1;

	int i = 0;

	for (i=0; i<currentcount; i++){
		if (strcmp(toincrement, mycount[i].name) == 0){	//vraca 0 kad su isti
			mycount[i].value++;
		}
	}

	return(OK);
}
int do_get_counter(message *m_ptr){

	char* toincrement = m_ptr -> m_m3.m3ca1;

	int i = 0;

		for (i=0; i<currentcount; i++){
			if (strcmp(toincrement, mycount[i].name) == 0){	//vraca 0 kad su isti
				return mycount[i].value;
			}
		}

	return (OK);
}

/*===========================================================================*
 *		            sef_cb_init_fresh                                *
 *===========================================================================*/
int sef_cb_init_fresh(int UNUSED(type), sef_init_info_t *info)
{
	memset(mycount, 0, sizeof(mycount));
	return(OK);
}
