/* log_general.c
** GLS logging using UDP packets
** $Header: /home/cjm/cvs/log_udp/c/log_general.c,v 1.1 2009-01-09 14:54:37 cjm Exp $
*/
/**
 * Error handler.
 * @author Chris Mottram
 * @version $Revision: 1.1 $
 */
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_SOURCE 1
/**
 * This hash define is needed before including source files give us POSIX.4/IEEE1003.1b-1993 prototypes.
 */
#define _POSIX_C_SOURCE 199309L

#include <errno.h>   /* Error number definitions */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "log_general.h"

/* external variables */
/**
 * The error number.
 */
int Log_Error_Number = 0;
/**
 * The error string.
 * @see #LOG_GENERAL_ERROR_LENGTH
 */
char Log_Error_String[LOG_GENERAL_ERROR_LENGTH];

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: log_general.c,v 1.1 2009-01-09 14:54:37 cjm Exp $";

/* internal function declarations */
static void Log_General_Get_Current_Time_String(char *time_string,int string_length);

/* ---------------------------------------------------------------
**  External functions 
** --------------------------------------------------------------- */

/**
 * Basic error reporting routine, to stderr.
 * @see #Log_Error_Number
 * @see #Log_Error_String
 * @see #Log_General_Get_Current_Time_String
 */
void Log_General_Error(void)
{
	char time_string[32];

	Log_General_Get_Current_Time_String(time_string,32);
	if(Log_Error_Number == 0)
		sprintf(Log_Error_String,"%s Log_UDP:An unknown error has occured.",time_string);
	fprintf(stderr,"%s Log_UDP:Error(%d) : %s\n",time_string,Log_Error_Number,
		Log_Error_String);
}

/**
 * Basic error reporting routine, to the specified string.
 * @param error_string Pointer to an already allocated area of memory, to store the generated error string. 
 *        This should be at least 256 bytes long.
 * @see #Log_Error_Number
 * @see #Log_Error_String
 * @see #Log_General_Get_Current_Time_String
 */
void Log_General_Error_To_String(char *error_string)
{
	char time_string[32];

	strcpy(error_string,"");
	Log_General_Get_Current_Time_String(time_string,32);
	if(Log_Error_Number != 0)
	{
		sprintf(error_string+strlen(error_string),"%s Log:Error(%d) : %s\n",time_string,
			Log_Error_Number,Log_Error_String);
	}
	if(strlen(error_string) == 0)
	{
		sprintf(error_string,"%s Error:Log:Error not found\n",time_string);
	}
}

/**
 * Routine to return the current value of the error number.
 * @return The value of Log_Error_Number.
 * @see #Log_Error_Number
 */
int Log_General_Get_Error_Number(void)
{
	return Log_Error_Number;
}

/* ---------------------------------------------------------------
** Internal functions
** --------------------------------------------------------------- */
/**
 * Routine to get the current time in a string. The string is returned in the format
 * '01/01/2000 13:59:59', or the string "Unknown time" if the routine failed.
 * The time is in UTC.
 * @param time_string The string to fill with the current time.
 * @param string_length The length of the buffer passed in. It is recommended the length is at least 20 characters.
 */
static void Log_General_Get_Current_Time_String(char *time_string,int string_length)
{
	time_t current_time;
	struct tm *utc_time = NULL;

	if(time(&current_time) > -1)
	{
		utc_time = gmtime(&current_time);
		strftime(time_string,string_length,"%d/%m/%Y %H:%M:%S",utc_time);
	}
	else
		strncpy(time_string,"Unknown time",string_length);
}

/*
** $Log: not supported by cvs2svn $
*/
