/* log_udp.h
** $Header: /home/cjm/cvs/log_udp/include/log_general.h,v 1.1 2009-01-09 14:54:41 cjm Exp $
*/
#ifndef LOG_GENERAL_H
#define LOG_GENERAL_H

/* hash defines */
/**
 * TRUE is the value usually returned from routines to indicate success.
 */
#ifndef TRUE
#define TRUE 1
#endif
/**
 * FALSE is the value usually returned from routines to indicate failure.
 */
#ifndef FALSE
#define FALSE 0
#endif

/**
 * How long the error string is.
 */
#define LOG_GENERAL_ERROR_LENGTH (1024)

/* external functions */
extern void Log_General_Error(void);
extern void Log_General_Error_To_String(char *error_string);
extern int Log_General_Get_Error_Number(void);

/* external variables */
extern int Log_Error_Number;
extern char Log_Error_String[];

/*
** $Log: not supported by cvs2svn $
*/
#endif
