#include <lib.h>
#include <string.h>
#include <minix/safecopies.h>

int start_log(char *logger) {
	message m;
	memset(&m, 0, sizeof(m));

	strcpy(m.m_log.name, logger);

	return _syscall(LS_PROC_NR, LS_START_LOG, &m);
}
int set_logger_level(char *logger, int level) {
	message m;
	memset(&m, 0, sizeof(m));

	strcpy(m.m_log.name, logger);
	m.m_log.level = level;


	return _syscall(LS_PROC_NR, LS_SET_LOGGER_LEVEL, &m);
}

int write_log(char *logger, char *mess, int level) {

	message m;
	memset(&m, 0, sizeof(m));
	strcpy(m.m_log.name, logger);


	int length = strlen(mess) + 1;

	cp_grant_id_t gid = cpf_grant_direct(LS_PROC_NR, (vir_bytes) mess, length, CPF_READ);

	m.m_log.gid = gid;
	m.m_log.length = length;
	m.m_log.level = level;

	return _syscall(LS_PROC_NR, LS_WRITE_LOG, &m);
}
int close_log(char *logger) {
	message m;
	memset(&m, 0, sizeof(m));
	strcpy(m.m_log.name, logger);

	return _syscall(LS_PROC_NR, LS_CLOSE_LOG, &m);
}
int clear_logs(char *logger) {
	message m;
	memset(&m, 0, sizeof(m));

	/* Ako nije prosledjen null setuj ime logera
	 * a ako je null, ime ce biti ALL
	 */
	if (logger != NULL){
		strcpy(m.m_log.name, logger);
	} else {
		strcpy(m.m_log.name, "ALL");
	}

	return _syscall(LS_PROC_NR, LS_CLEAR_LOGS, &m);
}
