#include "inc.h"
#include <stdio.h>
#include "../pm/mproc.h"
#include <minix/safecopies.h>

//u store.c se nalaza neke funkcije kojima se dobija process name iz struktura koje
//on ima u sebi, moze nam biti korisno kad mi budemo radili nas logger

//1:08:35 bane prica kako dohvatiti proces na osnovu endpointa

#define MAX_COUNT 50
#define MAX_NAME 50
#define MAX_PUTANJA 50
#define MAX_FORMAT 50
#define OK 0
#define ALREADY_STARTED -1
#define NOT_FOUND -2
#define SYS_ERROR -3	//za greske internih funkcija  LAZY I PARSE ISPRAVITI I ONDA SVAKI LAZY HVATATI U STATUS I ISPRAVITI
#define NOT_STARTED -4
#define NOT_OWNER -5

int parse(void);
int lazy_init(void);
int find_index(char* logger);
char *typeString(int level);
char *levelString(int level);
int level(char tip);
int type(char tip);
char *make_message(char *loger, char *logmessage, message *m_ptr);

struct logger {
	char name[MAX_NAME];
	int type; //f o e file,output,error
	char path[MAX_PUTANJA];
	int level; //t d e trace,debug,error
	char format[MAX_FORMAT];
	int append; // 1 0
	int running; //1 0	//ne parsira se
	int proc_nr;        //ne parsira se
	endpoint_t current_proc;
} loggers[MAX_COUNT];

int LoggersCount = 0;
int init = 0;

int FILE_TYPE = 0;
int OUT_TYPE = 1;
int ERROR_TYPE = 2;
int TRACE_LEVEL = 0;
int DEBUG_LEVEL = 1;
int ERROR_LEVEL = 2;

int do_start_log(message *m_ptr) {

	//handle sistemske greske
	int status = lazy_init();

	if (status == SYS_ERROR) {
		return SYS_ERROR;
	}

	char name[44];
	strcpy(name, m_ptr->m_log.name);

	int index = find_index(name);

	if (index == NOT_FOUND) {
		printf("Pokrenut je nepostojeci loger\n");
		return NOT_FOUND;
	}

	if (loggers[index].running) {
		printf("Logovanje je vec zapoceto\n");
		return ALREADY_STARTED;
	}

	// podesavamo da je running
	loggers[index].running = 1;
	loggers[index].current_proc = m_ptr->m_source;

//	int i;

//	for (i = 0; i < LoggersCount; i++) {
//		printf("LOGGER\n");
//		printf("%s\n", loggers[i].name);
//		printf("%d\n", loggers[i].type);
//		printf("%s\n", loggers[i].path);
//		printf("%d\n", loggers[i].level);
//		printf("%s\n", loggers[i].format);
//		printf("%d\n", loggers[i].append);
//		printf("%d\n", loggers[i].running);
//		printf("%d\n", loggers[i].current_proc);
//	}

	return OK;
}

int do_set_logger_level(message *m_ptr) {

	int status = lazy_init();

	if (status == SYS_ERROR) {
		return SYS_ERROR;
	}

	char name[44];
	strcpy(name, m_ptr->m_log.name);
	int level = m_ptr->m_log.level;

	int index = find_index(name);

	if (index == NOT_FOUND) {
		printf(
				"Podesavanje nivoa logera neuspesno, jer nije pronadjen loger\n");
		return NOT_FOUND;
	}

	if (loggers[index].running != 1) {
		//ako nije pokrenut
		loggers[index].level = level;
	} else {
		return ALREADY_STARTED;
		printf("Loger je vec pokrenut\n");
	}

	return OK;
}

int do_write_log(message *m_ptr) {

	int status = lazy_init();

	if (status == SYS_ERROR) {
		return SYS_ERROR;
	}

	char name[44];
	strcpy(name, m_ptr->m_log.name);
	int request_level = m_ptr->m_log.level;

	/* MAGIJA */

	cp_grant_id_t gid = m_ptr->m_log.gid;

	int length = m_ptr->m_log.length;

	char *message_to_write = malloc(length + 1);

	//Kompiramo memoriju od source procesa od poruke na 0 lokaciju. tj
	int result;
	result = sys_safecopyfrom(m_ptr->m_source, gid, 0,
			(vir_bytes) message_to_write, length);

	if (result != OK) {
		printf("RES NIJE OK\n");
		return SYS_ERROR;
	} else {
		printf("Dobijena poruka je %s\n", message_to_write);
	}

	/* KRAJ MAGIJE */

	int index = find_index(name);

	FILE *fp;

	if (index != NOT_FOUND && loggers[index].level <= request_level
			&& loggers[index].running == 1) {

		switch (loggers[index].type) {
		{
			case 0:
			//upisujemo u fajl

			if (loggers[index].append == 1) {
				fp = fopen(loggers[index].path, "a+");
			} else {
				fp = fopen(loggers[index].path, "w");
			}

			fp = fopen(loggers[index].path, "a");
			char *write = make_message(m_ptr->m_log.name, message_to_write,
					m_ptr);
			fprintf(fp, "%s\n", write);
			fclose(fp);
			break;
		}
		{
			case 1:
			//na stdout pisemo
			printf("%s\n",
					make_message(m_ptr->m_log.name, message_to_write, m_ptr));
			break;
		}
		{
			case 2:
			//na pokvareni stderr pisemo
			printf("[STDERR] %s\n",
					make_message(m_ptr->m_log.name, message_to_write, m_ptr));
			break;
		}

		}

	} else {
		printf("Eror u pisanju loga:\n");
		if (index == NOT_FOUND) {
			printf("\tNije pronadjen loger\n");
			return NOT_FOUND;
		}
		if (loggers[index].level > request_level) {
			printf("\tNivo logera je manji od trazenog nivoa\n");
			return OK;
		}
		if (loggers[index].running != 1) {
			printf("\tLoger nije pokrenut\n");
			return NOT_STARTED;
		}
	}

	return OK;
}

int do_close_log(message *m_ptr) {

	int status = lazy_init();

	if (status == SYS_ERROR) {
		return SYS_ERROR;
	}

	char name[44];
	strcpy(name, m_ptr->m_log.name);
	int index = find_index(name);

	if (index == NOT_FOUND) {
		//nije pronadjen loger
		printf("Zatvaranje neuspesno, nije pronadjen loger %s\n", name);
		return NOT_FOUND;
	}

	if (loggers[index].running == 0) {
		//pokusava da se zatvroi vec zatvoren loger
		printf("Loger je vec zatvoren\n");
		return NOT_STARTED;
	}

	if (loggers[index].current_proc != m_ptr->m_source) {
		//neko ko nije zapoceo ovaj loger pokusava da ga ugasi
		printf("Neko ko nije otvorio loger pokusava da ga zatvori\n");
		return NOT_OWNER;
	}

	loggers[index].running = 0;

	return OK;
}

int do_clear_logs(message *m_ptr) {

	int error_running = 0;
	int error_does_not_exist = 0;
	int error_system = 0;

	int status = lazy_init();

	if (status == SYS_ERROR) {
		return SYS_ERROR;
	}

	if (strcmp(m_ptr->m_log.name, "ALL") == 0) {

		int i;

		//prolazimo kroz sve logere
		for (i = 0; i < LoggersCount; i++) {

			if (loggers[i].type != FILE_TYPE) {
				continue;
			}

			if (loggers[i].running == 1) {
				printf("logger is running \n");
				error_running = 1;
				continue;
			}

			int index = find_index(loggers[i].name);

			//da li postoji?
			if (index == NOT_FOUND) {
				error_does_not_exist = 1;
				printf("ne postoji \n");
				continue;
			}

			//sada smo sigurni da mozemo ocistiti log fajl

			int succes = remove(loggers[i].path);

			//remove vraca 0 kad je uspeo
			if (succes != 0) {
				error_system = 1;
			}
		}

		//IF KOJI RETURNUJE PRAVI ERROR NA KRAJU!!!!!!! NA OSNOVU FLAGOVA

	} else {

		char *loggers_csv = (char *) malloc(strlen(m_ptr->m_log.name));
		strcpy(loggers_csv, m_ptr->m_log.name);

		char *trenutni = (char *) malloc(sizeof(char) * MAX_NAME);
		int j = 0;

		int i, index;	//
		for (i = 0; i <= strlen(loggers_csv); ++i) {
			if (loggers_csv[i] != ',' && loggers_csv[i] != '\0') {
				trenutni[j++] = loggers_csv[i];
			} else {
				trenutni[j] = '\0';
				j = 0;

				index = find_index(trenutni);
				if (index == NOT_FOUND) {
					printf("Error, nepostojeci loger %s\n", trenutni);
					error_does_not_exist = 1;
					continue;
				}

				if (loggers[index].running == 1) {
					printf("Error, loger je pokrenut %s\n", trenutni);
					error_running = 1;
					continue;
				}

				if (loggers[index].type == FILE_TYPE) {
					printf("Brisem loger[%s] na putanji %s\n",
							loggers[index].name, loggers[index].path);

					int success = remove(loggers[index].path);

					//remove vraca 0 kad je uspeo
					if (success != 0) {
						error_system = 1;
					}
				}

				free(trenutni);
				trenutni = (char *) malloc(sizeof(char) * MAX_NAME);
			}
		}
		free(trenutni);

	}

	if (error_does_not_exist) {
		return NOT_FOUND;
	} else if (error_running) {
		return ALREADY_STARTED;
	} else if (error_system) {
		return SYS_ERROR;
	} else {
		return OK;
	}
}

/* Metoda koja na osnovu imena nalazi indeks logera */

int find_index(char* logger) {
	//printf("find index: trazim %s\n",logger);
	int i;

	for (i = 0; i < LoggersCount; i++) {
		if (strcmp(logger, loggers[i].name) == 0) {		//vraca 0 kad su isti
			return i;
		}
	}

	return NOT_FOUND;
}

char *levelString(int level) {
	char *pok;
	switch (level) {
	case 0:
		pok = "TRACE";
		break;
	case 1:
		pok = "DEBUG";
		break;
	case 2:
		pok = "ERROR";
		break;
	default:
		pok = "pogresan index";
		break;
	}
	return pok;
}

char *typeString(int level) {
	char *pok;
	switch (level) {
	case 0:
		pok = "FILE";
		break;
	case 1:
		pok = "OUT";
		break;
	case 2:
		pok = "ERROR";
		break;
	default:
		pok = "pogresan index";
		break;
	}
	return pok;
}

int type(char tip) {

	switch (tip) {
	case 'f':
		return FILE_TYPE;
	case 'o':
		return OUT_TYPE;
	case 'e':
		return ERROR_TYPE;
	}

	return -1;

}

int level(char tip) {
	switch (tip) {
	case 'e':
		return ERROR_LEVEL;
	case 'd':
		return DEBUG_LEVEL;
	case 't':
		return TRACE_LEVEL;
	}

	return -1;
}

void parseLogers(char *text) {

	int len = sizeof(text);
	int niz[5];	//fiksan broj parametara

	int i, j;
	i = 0;
	j = 0;

	//pamtim gde su ;
	while (text[i] != '\0') {
		if (text[i] == ';') {
			niz[j++] = i;
		}
		i++;
	}
	j = 0;
	int k;	//seta od pocetka do kraja trenutnog tokena
	for (i = 0; i < 6; i++) {

		switch (i) {
		case 0:
			//printf("Ime: ");
			for (k = 0; k < niz[i]; k++) {
				loggers[LoggersCount].name[k] = text[j + k];
				//printf("%c", text[j + k]);
			}
			//printf("\n");
			break;
		case 1:
			//printf("Type:%c", text[j]);
			loggers[LoggersCount].type = type(text[j]);
			//printf("\n");
			break;

		case 2:
			//printf("path: ");
			//oduzmem prosli od trenutnog - ;
			for (k = 0; k < niz[i] - niz[i - 1] - 1; k++) {
				loggers[LoggersCount].path[k] = text[j + k];
				//printf("%c", text[j + k]);
			}
			//printf("\n");
			break;

		case 3:
			//printf("level: %c", text[j]);
			loggers[LoggersCount].level = level(text[j]);
			//printf(" %d",loggers[LoggersCount].level);
			//printf("\n");
			break;
		case 4:
			//printf("format: ");
			for (k = 0; k < niz[i] - niz[i - 1] - 1; k++) {
				loggers[LoggersCount].format[k] = text[j + k];
				//printf("%c", text[j + k]);
			}
			//printf("\n");
			break;
		case 5:
			//printf("Append: %c", text[j]);
			loggers[LoggersCount].append = text[j] - '0'; //pretvorim odmah char u integer
			//printf("\n");
			break;
		}
		//pomeram pokazivac na prvo slovo sledeceg tokena
		j = niz[i] + 1;
	}

}

int parse() {
	char *source = NULL;
	FILE *fp = fopen("/root/log.conf", "r");
	if (fp != NULL) {
		/* Go to the end of the file. */
		if (fseek(fp, 0L, SEEK_END) == 0) {
			/* Get the size of the file. */
			long bufsize = ftell(fp);
			if (bufsize == -1) { /* Error */
				return SYS_ERROR;
			}

			/* Allocate our buffer to that size. */
			source = malloc(sizeof(char) * (bufsize + 1));

			/* Go back to the start of the file. */
			if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */
				return SYS_ERROR;
			}

			/* Read the entire file into memory. */
			size_t newLen = fread(source, sizeof(char), bufsize, fp);
			if (ferror(fp) != 0) {
				fputs("Error reading file", stderr);
			} else {
				source[newLen++] = '\0'; /* Just to be safe. */
			}
		} else {
			return SYS_ERROR;
		}

		fclose(fp);

	} else {
		return SYS_ERROR;
	}

	int od = 0;	//pokazuje na pocetak trenutno tokena, reda u fajlu
	int len = strlen(source);	//duzina celog fajla
	int lne;	//line length
	char *pok = NULL;	//pokazuje na trenutni red, salje se u ParseLoggers()
	int k;
	int space = 0;	//magija
	int i;

	//rasturim fajl na redove
	for (i = 0; i < len; i++) {
		if (source[i] == '\n') {

			if (pok != NULL) {
				free(pok);
			}

			//odredim duzinu linije
			lne = sizeof(char) * (i - od + 1 + 1);
			pok = (char *) malloc(lne);
			//kopiramo na novo mesto
			for (k = 0; k < lne; k++) {
				//prepisujem red u pok
				pok[k] = source[od + k];
				if (pok[k] == ' ') {
					space++; //K ima vrednost vecu za onoliko koliko ima spejsova
							 // pa mora da oduzmemo te spejsove jer because fuck you that's why
				}
			}
			pok[k - space] = '\0';	//terminiram ga
			space = 0;
			parseLogers(pok);	//parsiram red
			LoggersCount++;
			//printf("--------------------\n");
			od = i + 1;
		}
	}

	//printf("IME LOGERA je: %s\n", loggers[0].name);

	free(pok);
	free(source); /* Don't forget to call free() later! */

	return OK;
}

int lazy_init() {

	if (!init) {
		int success = parse();

		if (success == OK) {
			init = 1;
			return OK;
		} else {
			return SYS_ERROR;
		}
	}

	return OK;
}

char *get_endpoint_name(message *m_ptr) {

	char *name;
	struct mproc t[256];
	getsysinfo(PM_PROC_NR, SI_PROC_TAB, &t, sizeof(t));
	name = (char *) malloc(
			sizeof(char) * strlen(t[_ENDPOINT_P(m_ptr->m_source)].mp_name));
	strcpy(name, t[_ENDPOINT_P(m_ptr->m_source)].mp_name);
	return name;

}

char *make_message(char *loger, char *logmessage, message *m_ptr) {

	int index = find_index(loger);

	//printf ("make message: nasao sam logger na indeksu %d", index);

	if (index == -1) {
		printf("Nepostojeci loger");
		return "";
	}

	/* RACUNAMO VFREME */

	struct tm *now;
	time_t nowtime;
	time(&nowtime);
	now = gmtime(&nowtime);

	char sat[3];
	sat[2] = '\0';
	char minut[3];
	minut[2] = '\0';
	char sekund[3];
	sekund[2] = '\0';

	sprintf(sat, "%02d", now->tm_hour);
	sprintf(minut, "%02d", now->tm_min);
	sprintf(sekund, "%02d", now->tm_sec);

	//printf("%s\n", sat);
	//printf("%s\n", minut);
	//printf("%s\n", sekund);

	char vreme[9];

	vreme[0] = sat[0];
	vreme[1] = sat[1];
	vreme[2] = ':';
	vreme[3] = minut[0];
	vreme[4] = minut[1];
	vreme[5] = ':';
	vreme[6] = sekund[0];
	vreme[7] = sekund[1];
	vreme[8] = '\0';

	/* SANITY OVER */

	char *time = vreme;

	//printf("get time je izracunao sledece vreme: %s\n", time);

	// printf("sumnjivo getovanje endpointa\n");
	char *endpoint_name = get_endpoint_name(m_ptr);
	//printf("sumnjivo getovanje endpointa\n");

	//racunas duzinu elemenata poruke
	int NIVO_DUZINA = strlen(levelString(loggers[index].level));
	int PROIZVOLJNA_DUZINA = strlen(logmessage);
	int VREME_DUZINA = strlen(time);    	    //napravi funkciju
	int NAZIV_DUZINA = strlen(endpoint_name);    	    //napravljena

	char *format = (char *) malloc(strlen(loggers[index].format));
	int j;
	strcpy(format, loggers[index].format);
	int i, len;
	len = strlen(format);
	int format_len = 0; //duzina zadatog formata minus %
	int format_cout = 0;//broj %
	int bonus_len = NIVO_DUZINA + PROIZVOLJNA_DUZINA + VREME_DUZINA
			+ NAZIV_DUZINA; //duzina onoga sto ide umesto %X
	for (i = 0; i < len; i++) {
		if (format[i] != '%') {
			format_len++;
		} else {
			format_cout++; //posle oduzmemo koliko % ima
		}
	}

	// printf("sumnjivi malloc\n");
	char *poruka = (char *) malloc(
			sizeof(char) * format_len + bonus_len - format_cout * 2 + 1); // +1 za \0, format_cout*2 oduzimamo %X sto je 2 karaktera
	// printf("sumnjivi malloc\n");
	int ofset = 0;	//prati dokle sam stvarno stigao u poruci, da bih znao gde da kopiram value koji insertujem

	int pom_len;

	char *pom_char;
	//idem po formatu i u poruku insertujem vrednosti koje odgovaraju simbolickim karakterima m, t, n...
	for (i = 0; i < len; i++) {
		if (format[i] == '%') {
			//ovde gledam koji je sledeci karakter posle % da bih znao sta da insertujem
			char oznaka = format[i + 1];
			switch (oznaka) {
			case 'n':
				pom_len = strlen(endpoint_name);
				for (j = 0; j < pom_len; j++) {
					poruka[ofset++] = endpoint_name[j];
				}
				break;
			case 't':
				pom_len = strlen(time);
				for (j = 0; j < pom_len; j++) {
					poruka[ofset++] = time[j];
				}
				break;
			case 'l':
				pom_char = levelString(loggers[index].level);
				pom_len = strlen(pom_char);
				for (j = 0; j < pom_len; j++) {
					poruka[ofset++] = pom_char[j];
				}
				break;
			case 'm':
				pom_len = strlen(logmessage);
				for (j = 0; j < pom_len; j++) {
					poruka[ofset++] = logmessage[j];
				}
				break;
			default:
				printf("make_message default\n");
			}
			i++; //preskacemo sledeci karakter posle % jer nam ne treba
		} else {
			poruka[ofset++] = format[i];
		}
	}

	poruka[ofset] = '\0';
	//printf("CHECK 1\n");
	free(format);
	//printf("CHECK 2\n");

	//printf("pre slanja poruke iz funkcije\n");
	return poruka;

}

/*===========================================================================*
 *		            sef_cb_init_fresh                                *
 *===========================================================================*/
int sef_cb_init_fresh(int UNUSED( type), sef_init_info_t *info) {
	memset(loggers, 0, sizeof(loggers));
	return (OK);
}
