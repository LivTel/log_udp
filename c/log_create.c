/* log_create.c
** GLS logging using UDP packets
** $Header: /home/cjm/cvs/log_udp/c/log_create.c,v 1.1 2009-01-09 14:54:37 cjm Exp $
*/
/**
 * Routines for filling in the Log_Record_Struct and Log_Context_Struct.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for nanosleep.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes
 * for nanosleep.
 */
#define _POSIX_C_SOURCE 199309L
#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _POSIX_TIMERS
#include <sys/time.h>
#endif
#include "log_general.h"
#include "log_udp.h"
#include "log_create.h"

/* hash defines */
/**
 * The number of nanoseconds in one microsecond.
 */
#define ONE_MICROSECOND_NS		(1000)
/**
 * The number of nanoseconds in one millisecond.
 */
#define ONE_MILLISECOND_NS              (1000000)

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: log_create.c,v 1.1 2009-01-09 14:54:37 cjm Exp $";

/* internal functions */
static int Log_Create_Timestamp(struct Log_Record_Struct *log_record);

/* ---------------------------------------------------------------
**  External functions 
** --------------------------------------------------------------- */
/**
 * Fill in a log record based upon the passed in parameters.
 * @param system The System, a string of length LOG_RECORD_SYSTEM_LENGTH. Can be NULL.
 * @param sub_system The Sub_System, a string of length LOG_RECORD_SUB_SYSTEM_LENGTH. Can be NULL.
 * @param source_file The source filename, a string of length LOG_RECORD_SOURCE_FILE_LENGTH. Can be NULL.
 * @param source_instance The instance of the source filename, a string of length LOG_RECORD_SOURCE_INSTANCE_LENGTH. 
 *        Can be NULL.
 * @param function The function calling the log, a string of length LOG_RECORD_FUNCTION_LENGTH. Can be NULL.
 * @param severity Whether the log is INFO or ERROR, a valid member of the LOG_SEVERITY enum.
 * @param verbosity At what level is the log message (TERSE/high level or VERBOSE/low level), 
 *         a valid member of LOG_VERBOSITY.
 * @param category What sort of information is the message. A string of length LOG_RECORD_CATEGORY_LENGTH. 
 *     Designed to be used as a filter. Can be NULL.
 * @param message The actual message. A string of length LOG_RECORD_MESSAGE_LENGTH.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #Log_Create_Timestamp
 * @see log_udp.html#LOG_RECORD_SYSTEM_LENGTH
 * @see log_udp.html#LOG_RECORD_SUB_SYSTEM_LENGTH
 * @see log_udp.html#LOG_RECORD_SOURCE_FILE_LENGTH
 * @see log_udp.html#LOG_RECORD_SOURCE_INSTANCE_LENGTH
 * @see log_udp.html#LOG_RECORD_FUNCTION_LENGTH
 * @see log_udp.html#LOG_RECORD_CATEGORY_LENGTH
 * @see log_udp.html#LOG_RECORD_MESSAGE_LENGTH
 * @see log_udp.html#LOG_SEVERITY
 * @see log_udp.html#LOG_VERBOSITY
 * @see log_udp.html#Log_Record_Struct
 */
int Log_Create_Record(char *system,char *sub_system,char *source_file,char *source_instance,char *function,
			     int severity,int verbosity,char *category,char *message,
			     struct Log_Record_Struct *log_record)
{
	if(message == NULL)
	{
		Log_Error_Number = 100;
		sprintf(Log_Error_String,"Log_Create_Record:message was NULL.");
		return FALSE;
	}
	if(log_record == NULL)
	{
		Log_Error_Number = 101;
		sprintf(Log_Error_String,"Log_Create_Record:log_record was NULL.");
		return FALSE;
	}
	if(!LOG_UDP_IS_SEVERITY(severity))
	{
		Log_Error_Number = 102;
		sprintf(Log_Error_String,"Log_Create_Record:severity is not a legal value(%d).",severity);
		return FALSE;
	}
	if(!LOG_UDP_IS_VERBOSITY(verbosity))
	{
		Log_Error_Number = 103;
		sprintf(Log_Error_String,"Log_Create_Record:verbosity is not a legal value(%d).",verbosity);
		return FALSE;
	}
	/* set the whole record to zero byte */
	memset(log_record,0,sizeof(struct Log_Record_Struct));
	/* timestamp */
	Log_Create_Timestamp(log_record);
	/* system */
	if(system != NULL)
	{
		strncpy(log_record->System,system,LOG_RECORD_SYSTEM_LENGTH);
		if(strlen(system) >= LOG_RECORD_SYSTEM_LENGTH)
			log_record->System[LOG_RECORD_SYSTEM_LENGTH-1] = '\0';
	}
	/* sub_system */
	if(sub_system != NULL)
	{
		strncpy(log_record->Sub_System,sub_system,LOG_RECORD_SUB_SYSTEM_LENGTH);
		if(strlen(sub_system) >= LOG_RECORD_SUB_SYSTEM_LENGTH)
			log_record->Sub_System[LOG_RECORD_SUB_SYSTEM_LENGTH-1] = '\0';
	}
	/* source_file */
	if(source_file != NULL)
	{
		strncpy(log_record->Source_File,source_file,LOG_RECORD_SOURCE_FILE_LENGTH);
		if(strlen(source_file) >= LOG_RECORD_SOURCE_FILE_LENGTH)
			log_record->Source_File[LOG_RECORD_SOURCE_FILE_LENGTH-1] = '\0';
	}
	/* source_instance */
	if(source_instance != NULL)
	{
		strncpy(log_record->Source_Instance,source_instance,LOG_RECORD_SOURCE_INSTANCE_LENGTH);
		if(strlen(source_instance) >= LOG_RECORD_SOURCE_INSTANCE_LENGTH)
			log_record->Source_Instance[LOG_RECORD_SOURCE_INSTANCE_LENGTH-1] = '\0';
	}
	/* function */
	if(function != NULL)
	{
		strncpy(log_record->Function,function,LOG_RECORD_FUNCTION_LENGTH);
		if(strlen(function) >= LOG_RECORD_FUNCTION_LENGTH)
			log_record->Function[LOG_RECORD_FUNCTION_LENGTH-1] = '\0';
	}
	/* severity */
	log_record->Severity = severity;
	/* verbosity */
	log_record->Verbosity = verbosity;
	/* category */
	if(category != NULL)
	{
		strncpy(log_record->Category,category,LOG_RECORD_CATEGORY_LENGTH);
		if(strlen(category) >= LOG_RECORD_CATEGORY_LENGTH)
			log_record->Category[LOG_RECORD_CATEGORY_LENGTH-1] = '\0';
	}
	/* message */
	strncpy(log_record->Message,message,LOG_RECORD_MESSAGE_LENGTH);
	if(strlen(message) >= LOG_RECORD_MESSAGE_LENGTH)
		log_record->Message[LOG_RECORD_MESSAGE_LENGTH-1] = '\0';
	return TRUE;
}

/**
 * Routine to add a context (keyword/value pair) to a list of context structures.
 * @param log_context_list The address of a pointer to a list of log contexts to add this context to.
 * @param log_context_count The number of log contexts in the list
 * @param keyword A string containing the keyword of the context to add, 
 *        should have length less than LOG_CONTEXT_KEYWORD_LENGTH.
 * @param value A string containing the value of the context to add, 
 *        should have length less than LOG_CONTEXT_VALUE_LENGTH.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see log_udp.html#LOG_CONTEXT_KEYWORD_LENGTH
 * @see log_udp.html#LOG_CONTEXT_VALUE_LENGTH
 * @see log_udp.html#Log_Context_Struct
 */
int Log_Create_Context_List_Add(struct Log_Context_Struct **log_context_list,int *log_context_count,
				       char *keyword,char *value)
{
	struct Log_Context_Struct new_log_context;

	if(log_context_list == NULL)
	{
		Log_Error_Number = 104;
		sprintf(Log_Error_String,"Log_Create_Context_List_Add:log_context_list was NULL.");
		return FALSE;
	}
	if(log_context_count == NULL)
	{
		Log_Error_Number = 105;
		sprintf(Log_Error_String,"Log_Create_Context_List_Add:log_context_count was NULL.");
		return FALSE;
	}
	if(keyword == NULL)
	{
		Log_Error_Number = 106;
		sprintf(Log_Error_String,"Log_Create_Context_List_Add:keyword was NULL.");
		return FALSE;
	}
	if(value == NULL)
	{
		Log_Error_Number = 107;
		sprintf(Log_Error_String,"Log_Create_Context_List_Add:value was NULL.");
		return FALSE;
	}
	/* setup new_log_context */
	memset(&new_log_context,0,sizeof(struct Log_Context_Struct));
	/* keyword */
	strncpy(new_log_context.Keyword,keyword,LOG_CONTEXT_KEYWORD_LENGTH);
	if(strlen(keyword) >= LOG_CONTEXT_KEYWORD_LENGTH)
		new_log_context.Keyword[LOG_CONTEXT_KEYWORD_LENGTH-1] = '\0';
	/* value */
	strncpy(new_log_context.Value,value,LOG_CONTEXT_VALUE_LENGTH);
	if(strlen(value) >= LOG_CONTEXT_VALUE_LENGTH)
		new_log_context.Value[LOG_CONTEXT_VALUE_LENGTH-1] = '\0';
	/* reallocate list */
	if((*log_context_list) == NULL)
		(*log_context_list) = (struct Log_Context_Struct *)malloc(sizeof(struct Log_Context_Struct));
	else
		(*log_context_list) = (struct Log_Context_Struct *)realloc((*log_context_list),
				      ((*log_context_count)+1)*sizeof(struct Log_Context_Struct));
	if((*log_context_list) == NULL)
	{
		Log_Error_Number = 108;
		sprintf(Log_Error_String,"Log_Create_Context_List_Add:Failed to (re-)allocate context list (%d).",
			(*log_context_count));
		return FALSE;
	}
	/* add new context to list */
	(*log_context_list)[(*log_context_count)++] = new_log_context;
	return TRUE;
}

/* ---------------------------------------------------------------
**  Internal functions 
** --------------------------------------------------------------- */
/**
 * Fill in the timestamp field of the log record with the current time.
 * @param log_record A pointer to the log record instance.
 * @return The routine returns TRUE on success and FALSE on failure.
 */
static int Log_Create_Timestamp(struct Log_Record_Struct *log_record)
{
	struct timespec current_time;
#ifndef _POSIX_TIMERS
	struct timeval gtod_current_time;
#endif
	int64_t long_current_time;

	if(log_record == NULL)
	{
		Log_Error_Number = 109;
		sprintf(Log_Error_String,"Log_Create_Timestamp:log_record was NULL.");
		return FALSE;
	}
#ifdef _POSIX_TIMERS
	clock_gettime(CLOCK_REALTIME,&current_time);
#else
	gettimeofday(&gtod_current_time,NULL);
	current_time.tv_sec = gtod_current_time.tv_sec;
	current_time.tv_nsec = gtod_current_time.tv_usec*ONE_MICROSECOND_NS;
#endif
	long_current_time = (((int64_t)current_time.tv_sec)*1000)+
		(((int64_t)current_time.tv_nsec)/ONE_MILLISECOND_NS);
	log_record->Timestamp = long_current_time;
	return TRUE;
}
/*
** $Log: not supported by cvs2svn $
*/
