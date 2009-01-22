/* tcs_to_udp.c
** $Header: /home/cjm/cvs/log_udp/test/tcs_to_udp.c,v 1.1 2009-01-22 11:11:49 cjm Exp $
*/
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "log_general.h"
#include "log_udp.h"
#include "log_create.h"

/**
 * This program attempts to read a TCS log file, 
 * and then creates and sends log messages to the GlobalLoggingSystem.
 * @author $Author: cjm $
 * @version $Revision: 1.1 $
 */

/* hash defines */
#ifndef FALSE
/**
 * FALSE.
 */
#define FALSE                            (0)
#endif
#ifndef TRUE
/**
 * TRUE.
 */
#define TRUE                             (1)
#endif
/**
 * Length of the intermediate message buffer.
 */
#define MESSAGE_BUFFER_LENGTH            (1024)
/**
 * MIN macro.
 */
#define MIN(A,B) ((A)<(B)?(A):(B))

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: tcs_to_udp.c,v 1.1 2009-01-22 11:11:49 cjm Exp $";

/**
 * The hostname of the PLC.
 */
static char *Hostname = NULL;
/**
 * The port number.
 */
static int Port_Number = 0;
/**
 * Parsed system.
 */
static char System[LOG_RECORD_SYSTEM_LENGTH] = "TCS";
/**
 * Parsed message filename. The filename to watch and extract message from, usually /var/log/messages
 */
static char *Message_Filename = NULL;
/**
 * Intermediate buffer for storing messages read from the file, but not yet transmitted as 
 * UDP messages (as we havn't found \n yet).
 */
static char Message_Buffer[MESSAGE_BUFFER_LENGTH];
/**
 * Socket used for sending the message via UDP.
 */
static int Socket_Id = -1;

/* internal routines */
static void Messages_To_UDP(void);
static void Process_Message_Buffer(void);
static void Process_Line_Buffer(char *line_buffer);
static int Verbosity_Buff_To_Severity_Verbosity(char *verbosity_buff,int *severity,int *verbosity);
static int Parse_Parameter_Lists(char *message_buff,struct Log_Context_Struct **log_context_list,
				 int *log_context_count);
static int Create_Time(char *month_buff,int day_of_month,char *time_buff,struct tm *time_tm);
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive boolean if it fails.
 * @see #Hostname
 * @see #Port_Number
 * @see #Message_Filename
 * @see #Messages_To_UDP
 */
int main(int argc, char *argv[])
{

#if DEBUG > 1
	fprintf(stdout,"tcs_to_udp:Started.\n");
#endif
	/* parse arguments */
	if(!Parse_Arguments(argc,argv))
	{
		fprintf(stderr,"tcs_to_udp:Parse Arguments failed.\n");
		return 1;
	}
	if(Hostname == NULL)
	{
		fprintf(stderr,"tcs_to_udp:No hostname specified.\n");
		return 2;
	}
	if(Message_Filename == NULL)
	{
		fprintf(stderr,"tcs_to_udp:No message filename specified.\n");
		return 2;
	}
	Messages_To_UDP();
#if DEBUG > 1
	fprintf(stdout,"tcs_to_udp:Freeing allocated data.\n");
#endif
	if(Hostname != NULL)
		free(Hostname);
	if(Message_Filename != NULL)
		free(Message_Filename);
#if DEBUG > 1
	fprintf(stdout,"tcs_to_udp:Finished.\n");
#endif
	return 0;
}

/**
 * Routine to open a UDP socket, open and read the mesasge file, and create new log records
 * as each new message (line) is added to the file.
 * @see #Socket_Id
 * @see #Hostname
 * @see #Port_Number
 * @see #Message_Buffer
 * @see #MESSAGE_BUFFER_LENGTH
 */
static void Messages_To_UDP(void)
{
	int my_errno;
	int current_message_buffer_position;
	int done;
	FILE *fp = NULL;
	long current_position,end_position;
	size_t number_read;

	/* initialise socket id */
	Socket_Id = -1;
	/* initialise message buffer */
	Message_Buffer[0] = '\0';
	current_message_buffer_position = 0;
	/* initialisw file pointer. First open may be different from other opens */
	fp = fopen(Message_Filename,"r");
	if(fp == NULL)
	{
		my_errno = errno;
		fprintf(stderr,"Opening message file '%s' failed (%d).\n",Message_Filename,
			my_errno);
	}
	current_position = 0L;
#if DEBUG > 1
	fprintf(stdout,"tcs_to_udp:Messages_To_UDP:Entering loop.\n");
#endif
	done = FALSE;
	while(done == FALSE)
	{
		/* if UDP socket is not open open it */
		if(Socket_Id < 0)
		{
#if DEBUG > 1
			fprintf(stdout,"tcs_to_udp:Messages_To_UDP:Opening socket to %s:%d.\n",
				Hostname,Port_Number);
#endif
			if(!Log_UDP_Open(Hostname,Port_Number,&Socket_Id))
				Log_General_Error();
		}
		/* if message filename is not open open it */
		if(fp == NULL)
		{
#if DEBUG > 1
			fprintf(stdout,"tcs_to_udp:Messages_To_UDP:Opening Message filename '%s'.\n",
				Message_Filename);
#endif
			fp = fopen(Message_Filename,"r");
			if(fp == NULL)
			{
				my_errno = errno;
				fprintf(stderr,"Opening message file '%s' failed (%d).\n",Message_Filename,
					my_errno);
			}
		}
		/* if the file has succcessfully opened */
		if(fp != NULL)
		{
			/* find the end of the file */
			if(fseek(fp,0,SEEK_END) == -1)
			{
				my_errno = errno;
				fprintf(stderr,"Seeking to end of file failed (%d).\n",my_errno);
			}
			/* where is the end of the file */
			end_position = ftell(fp);
#if DEBUG > 2
			fprintf(stdout,"tcs_to_udp:Messages_To_UDP:End Position = %ld.\n",end_position);
#endif
			if(current_position < end_position)
			{
#if DEBUG > 2
				fprintf(stdout,"tcs_to_udp:Messages_To_UDP:"
					"Current Position %ld less than end position %ld:Read new message.\n",
					current_position,end_position);
#endif
				/* new message has been added to the file */
				/* rewind to where we were */
				if(fseek(fp,current_position,SEEK_SET) == -1)
				{
					my_errno = errno;
					fprintf(stderr,"Setting position to '%d' failed (%d).\n",
						current_position,my_errno);
				}
				/* read the new data into the buffer */
				number_read = fread(Message_Buffer+current_message_buffer_position,sizeof(char),
						    MESSAGE_BUFFER_LENGTH-current_message_buffer_position-1,fp);
				Message_Buffer[current_message_buffer_position+number_read] = '\0';
				current_message_buffer_position += number_read;
				current_position += number_read;
				if(number_read == 0)
				{
					my_errno = errno;
					fprintf(stderr,"fread returned zero:(%d).\n",my_errno);
					sleep(1);
				}
#if DEBUG > 2
				fprintf(stdout,"tcs_to_udp:Messages_To_UDP:Read %d bytes.\n",number_read);
#endif
				/* process Message_Buffer */
				Process_Message_Buffer();
				/* reset current_message_buffer_position */
				current_message_buffer_position = strlen(Message_Buffer);
			}
			else if(current_position == end_position)
			{
				/* if we are just ingesting a log file and then quitting, terminate the loop */
				done = TRUE;
#if DEBUG > 2
				fprintf(stdout,"tcs_to_udp:Messages_To_UDP:Current Position == End Position.\n");
#endif
			}
			else /* current_position > end_position) */
			{
				/* this should be impossible but we will cover the case just in case. */
#if DEBUG > 2
				fprintf(stdout,"tcs_to_udp:Messages_To_UDP:Current Position > End Position.\n");
#endif
				done = TRUE;
			}
		}
	}/* end while not done */
#if DEBUG > 1
	fprintf(stdout,"tcs_to_udp:Closing socket.\n");
#endif
	if(!Log_UDP_Close(Socket_Id))
	{
		Log_General_Error();
	}
	if(fp != NULL)
		fclose(fp);
}

/**
 * Check to see if the message buffer has a '\n' in it, if so extract the message,
 *  parse, and create and emit a UDP log record.
 * @see #Message_Buffer
 * @see #MESSAGE_BUFFER_LENGTH
 * @see #Process_Line_Buffer
 */
static void Process_Message_Buffer(void)
{
	char line_buffer[MESSAGE_BUFFER_LENGTH];
	char *ch = NULL;
	int num_chars,i,j,done;

#if DEBUG > 2
	fprintf(stdout,"Process_Message_Buffer:Started.\n");
	fprintf(stdout,"Process_Message_Buffer:Buffer = '%s'.\n",Message_Buffer);
#endif
	done = FALSE;
	while (done == FALSE)
	{
		ch = strchr(Message_Buffer,'\n');
		if(ch != NULL)
		{
			num_chars = MIN((ch-Message_Buffer),(MESSAGE_BUFFER_LENGTH-1));
			strncpy(line_buffer,Message_Buffer,num_chars);
			line_buffer[num_chars] = '\0';
			/* copy any chars read after \n to begginning of Message_Buffer */
			j = 0;
			for(i = num_chars+1; i < strlen(Message_Buffer); i++)
			{
				Message_Buffer[j] = Message_Buffer[i];
				j++;
			}
			Message_Buffer[j] = '\0';
			/* Process line_buffer */
			Process_Line_Buffer(line_buffer);
		}
		else /* no more \n found */
		{
			done = TRUE;
#if DEBUG > 2
			fprintf(stdout,"Process_Message_Buffer:No newline found in '%s'.\n",Message_Buffer);
#endif
		}
	}/* end while */
}

/**
 * Process the single /var/log/message line in line_buffer, attempt to parse into log fields.
 * Line should be of the form:
 * <pre>
 * Jan 20 00:00:33 <INFO>    USER  90000 Network command: FOCUS 27.575
 * </pre>
 * @param line_buffer A single TCS log line.
 * @see #System
 * @see #Socket_Id
 * @see #Verbosity_Buff_To_Severity_Verbosity
 * @see #Parse_Parameter_Lists
 */
static void Process_Line_Buffer(char *line_buffer)
{
	struct Log_Record_Struct log_record;
	struct Log_Context_Struct *log_context_list = NULL;
	struct tm time_tm;
	int log_context_count = 0;
	char message_buff[1024];
	char month_buff[4];
	char time_buff[9];
	char tcs_status_buff[8];
	char verbosity_buff[128];
	char sub_system_buff[128];
	int retval,day_of_month,tcs_status_number,char_count;
	int severity = LOG_SEVERITY_INFO;
	int verbosity = LOG_VERBOSITY_INTERMEDIATE;

#if DEBUG > 2
	fprintf(stdout,"Process_Line_Buffer:Started (%s).\n",line_buffer);
#endif
	/* try to parse */
	retval = sscanf(line_buffer,"%3s %2d %8s %127s %127s %x %n",month_buff,&day_of_month,time_buff,
			verbosity_buff,sub_system_buff,&tcs_status_number,&char_count);
	if(retval != 6)
	{
		fprintf(stderr,"Process_Line_Buffer:Failed to parse:'%s' (%d)(%s,%d,%s,%s,%s,%#x,%d).\n",line_buffer,
			retval,month_buff,day_of_month,time_buff,verbosity_buff,sub_system_buff,tcs_status_number,
			char_count);
	}
	if(char_count < strlen(line_buffer))
	{
		strcpy(message_buff,line_buffer+char_count);
	}
	else
		message_buff[0] = '\0';
#if DEBUG > 1
	fprintf(stdout,"Process_Line_Buffer:month = %s, day_of_month = %d,time = %s, verbosity = %s,"
		"sub_system = %s,tcs status number = %#x,mesage = %s.\n",
		month_buff,day_of_month,time_buff,verbosity_buff,sub_system_buff,tcs_status_number,message_buff);
#endif
	/* convert TCS verbosity */
	Verbosity_Buff_To_Severity_Verbosity(verbosity_buff,&severity,&verbosity);
	if(!Log_Create_Record(System,sub_system_buff,NULL,NULL,NULL,severity,verbosity,NULL,
			      message_buff,&log_record))
	{
		Log_General_Error();
		return;
	}
	/* reset timestamp to parsed time stamp rather than current time
	** especially for old log ingesting */
	Create_Time(month_buff,day_of_month,time_buff,&time_tm);
	if(!Log_Create_Record_Timestamp_Set(time_tm,&log_record))
	{
		Log_General_Error();
		/* attempt to continue with wrong timestamp */
	}
	/* parse any <<01>> <<02>>.. into context records */
	Parse_Parameter_Lists(message_buff,&log_context_list,&log_context_count);
	/* TCS status number */
	sprintf(tcs_status_buff,"%#x",tcs_status_number);
	if(!Log_Create_Context_List_Add(&log_context_list,&log_context_count,"TCS Status Code",tcs_status_buff))
	{
		Log_General_Error();
		/* attempt to continue */
	}
	/* send log record */
	if(!Log_UDP_Send(Socket_Id,log_record,log_context_count,log_context_list))
	{
		Log_General_Error();
		Log_UDP_Close(Socket_Id);
		Socket_Id = 0;
		/* free context */
		if(log_context_list != NULL)
			free(log_context_list);
		return;
	}
	/* free context */
	if(log_context_list != NULL)
		free(log_context_list);
}

/**
 * Convert a verbosity string into  a log record Severity/Verbosity.
 * @param verbosity_buff The string to conver, currently supports: <NOTICE>|<INFO>|<WARNING>|<CRIT>
 * @param severity The log record severity, from LOG_SEVERITY.
 * @param verbosity The log record verbosity, from LOG_VERBOSITY.
 * @return The routine returns TRUE if it suceeded and FALSE if it fails.
 * @see ../cdocs/log_udp.html#LOG_SEVERITY
 * @see ../cdocs/log_udp.html#LOG_VERBOSITY
 */
static int Verbosity_Buff_To_Severity_Verbosity(char *verbosity_buff,int *severity,int *verbosity)
{
	if(verbosity_buff == NULL)
	{
		fprintf(stderr,"Verbosity_Buff_To_Severity_Verbosity:verbosity_buff was NULL.\n");
		return FALSE;
	}
	if(severity == NULL)
	{
		fprintf(stderr,"Verbosity_Buff_To_Severity_Verbosity:severity was NULL.\n");
		return FALSE;
	}
	if(verbosity == NULL)
	{
		fprintf(stderr,"Verbosity_Buff_To_Severity_Verbosity:verbosity was NULL.\n");
		return FALSE;
	}
	(*severity) = LOG_SEVERITY_INFO;
	(*verbosity) = LOG_VERBOSITY_INTERMEDIATE;
	if(strcmp(verbosity_buff,"<CRIT>") == 0)
	{
		(*severity) = LOG_SEVERITY_ERROR;
		(*verbosity) = LOG_VERBOSITY_VERY_TERSE;
	}
	else if(strcmp(verbosity_buff,"<WARNING>") == 0)
	{
		(*severity) = LOG_SEVERITY_ERROR;
		(*verbosity) = LOG_VERBOSITY_TERSE;
	}
	else if(strcmp(verbosity_buff,"<NOTICE>") == 0)
	{
		(*severity) = LOG_SEVERITY_INFO;
		(*verbosity) = LOG_VERBOSITY_VERBOSE;
	}
	else if(strcmp(verbosity_buff,"<INFO>") == 0)
	{
		(*severity) = LOG_SEVERITY_INFO;
		(*verbosity) = LOG_VERBOSITY_VERY_VERBOSE;
	}
	else
	{
		fprintf(stderr,"Verbosity_Buff_To_Severity_Verbosity:Unknown verbosity '%s'.\n",verbosity_buff);
		return FALSE;
	}
	return TRUE;
}

/**
 * Parse TCS log message lines of the form:
 * <pre>
 * <<01>>ENABLED<<02>>DISABLED<<03>>OKAY<<04>>OKAY<<05>>FALSE<<06>>FALSE<<X>>
 * </pre>
 * Into a list of contexts (01 = ENABLED). Modify the message buffer.
 * @param message_buff The message buffer part of the log message containing the parameter list.
 *        Any parsed parameters put into the context list will be removed from the message_buff.
 * @param log_context_list The address of a pointer to a reallocatable list of Log_Context_Structs.
 * @param log_context_count The address of an integer containing the number of log contecxts in log_context_list.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see ../cdocs/log_udp.html#Log_Context_Struct
 * @see ../cdocs/log_udp.html#LOG_CONTEXT_KEYWORD_LENGTH
 * @see ../cdocs/log_udp.html#LOG_CONTEXT_VALUE_LENGTH
 */
static int Parse_Parameter_Lists(char *message_buff,struct Log_Context_Struct **log_context_list,
				 int *log_context_count)
{
	char keyword_string[LOG_CONTEXT_KEYWORD_LENGTH];
	char value_string[LOG_CONTEXT_VALUE_LENGTH];
	int done,message_buff_start_pos,string_length,my_context_count,i;
	char *ch1 = NULL;
	char *ch2 = NULL;
	char *ch3 = NULL;

	if(message_buff == NULL)
	{
		fprintf(stderr,"Parse_Parameter_Lists:Message buff was NULL.\n");
		return FALSE;
	}
	if(log_context_list == NULL)
	{
		fprintf(stderr,"Parse_Parameter_Lists:log_context_list was NULL.\n");
		return FALSE;
	}
	if(log_context_count == NULL)
	{
		fprintf(stderr,"Parse_Parameter_Lists:log_context_count was NULL.\n");
		return FALSE;
	}
#if DEBUG > 5
	fprintf(stdout,"Parse_Parameter_Lists:Start Message = '%s'.\n",message_buff);
#endif
	done  = FALSE;
	message_buff_start_pos = 0;
	my_context_count = 0;
	while(done == FALSE)
	{
		/* ch1 = find start delimater */
		ch1 = strstr(message_buff+message_buff_start_pos,"<<");
		if(ch1 != NULL)
		{
			/* ch2 = find end delimiter */
			ch2 = strstr(ch1+2,">>");
			if(ch2 != NULL)
			{
				/* ch3 = find next start delimiter */
				ch3 = strstr(ch2+2,"<<");
				if(ch3 != NULL)
				{
					/* copy keyword and value into temp buffers */
					string_length = MIN((ch2-ch1)-2,LOG_CONTEXT_KEYWORD_LENGTH);
					strncpy(keyword_string,ch1+2,string_length);
					keyword_string[string_length] = '\0';
					string_length = MIN((ch3-ch2)-2,LOG_CONTEXT_VALUE_LENGTH);
					strncpy(value_string,ch2+2,string_length);
					value_string[string_length] = '\0';
#if DEBUG > 6
					fprintf(stdout,"Parse_Parameter_Lists:"
						"Extracted keyword = '%s', Value = '%s'.\n",
						keyword_string,value_string);
#endif
					/* add to context list */
					if(!Log_Create_Context_List_Add(log_context_list,log_context_count,
									keyword_string,value_string))
					{
						Log_General_Error();
						/* attempt to continue */
					}
					my_context_count++; /* number of keyword/values parsed rather than 
							** log_context_count which may contain other context as well */
					/* update start pos */
					message_buff_start_pos = (ch1-message_buff);
					/* remove keyword/value from message buff */
					string_length = strlen(ch3);
					for(i = 0; i < string_length; i++)
					{
						ch1[i] = ch3[i];
					}
					ch1[i] = '\0';
#if DEBUG > 6
					fprintf(stdout,"Parse_Parameter_Lists:Message Buff now '%s'.\n",message_buff);
#endif
				}
				else
					done = TRUE;

			}
			else
				done = TRUE;
		}
		else
			done = TRUE;
		if(my_context_count >= 100)
		{
			fprintf(stderr,"Parse_Parameter_Lists:Too many parameters, something has gone wrong.\n");
			exit(1);
		}
	}/* end while */
	/* add parameter count */
#if DEBUG > 1
	fprintf(stdout,"Parse_Parameter_Lists:Extracted %d parameters.\n",my_context_count);
#endif
	sprintf(value_string,"%d",my_context_count);
	if(!Log_Create_Context_List_Add(log_context_list,log_context_count,"Parameter Count",value_string))
	{
		Log_General_Error();
		/* attempt to continue */
	}
#if DEBUG > 1
	fprintf(stdout,"Parse_Parameter_Lists:Message Buff now '%s'.\n",message_buff);
#endif
	return TRUE;
}

/**
 * Convert parsed time elements of a message line into fields in a struct tm.
 * @param month_buff Three character month, 'Jan' to 'Dec'.
 * @param day_of_month Integer describing day of month, from 1 to 31.
 * @param time_buff String describing time, in format HH:MM:SS.
 * @param time_tm Pointer to a struct tm to fill in with the parsed time.
 */
static int Create_Time(char *month_buff,int day_of_month,char *time_buff,struct tm *time_tm)
{
	struct tm *now_time_tm = NULL;
	int hours,minutes,seconds,retval;
	time_t now_time_s;

	if(time_tm == NULL)
	{
		fprintf(stderr,"tcs_to_udp:Create_Time:time_tm was NULL.\n");
		return FALSE;
	}
	/* set tm to 0 bytes */
	memset(time_tm,0,sizeof(struct tm));
	now_time_s = time(NULL);
	now_time_tm = localtime(&now_time_s);
	retval = sscanf(time_buff,"%d:%d:%d",&hours,&minutes,&seconds);
	if(retval != 3)
	{
		fprintf(stderr,"tcs_to_udp:Create_Time:Failed to parse time_buff:%s (%d).\n",time_buff,retval);
		return FALSE;
	}
	time_tm->tm_sec = seconds;/* 0..59 (also 60,61) */
	time_tm->tm_min = minutes;/* 0..59 */
	time_tm->tm_hour = hours;/* 0..23*/
	time_tm->tm_mday = day_of_month;/* 1..31 */
	/* tm_mon takes the values 0..11 (!) */
	if(strcmp(month_buff,"Jan") == 0)
		time_tm->tm_mon = 0;
	else if(strcmp(month_buff,"Feb") == 0)
		time_tm->tm_mon = 1;
	else if(strcmp(month_buff,"Mar") == 0)
		time_tm->tm_mon = 2;
	else if(strcmp(month_buff,"Apr") == 0)
		time_tm->tm_mon = 3;
	else if(strcmp(month_buff,"May") == 0)
		time_tm->tm_mon = 4;
	else if(strcmp(month_buff,"Jun") == 0)
		time_tm->tm_mon = 5;
	else if(strcmp(month_buff,"Jul") == 0)
		time_tm->tm_mon = 6;
	else if(strcmp(month_buff,"Aug") == 0)
		time_tm->tm_mon = 7;
	else if(strcmp(month_buff,"Sep") == 0)
		time_tm->tm_mon = 8;
	else if(strcmp(month_buff,"Oct") == 0)
		time_tm->tm_mon = 9;
	else if(strcmp(month_buff,"Nov") == 0)
		time_tm->tm_mon = 10;
	else if(strcmp(month_buff,"Dec") == 0)
		time_tm->tm_mon = 11;
	else
	{
		fprintf(stderr,"tcs_to_udp:Create_Time:Unknown month %s.\n",month_buff);
		return FALSE;
	}
	/* this won't work for ingesting logs over the new year. */
	time_tm->tm_year = now_time_tm->tm_year;/* number of years since 1900! */
	/* mktime ignores tm_wday: number of days since Sunday (0..6) */
	/* mktime ignores tm_yday: number of days since Jan 1 (0.365) */
	time_tm->tm_isdst = now_time_tm->tm_isdst;/* +ve if daylight saving, 0 if not, -ve if unknown */
	return TRUE;
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @see #Help
 * @see #Hostname
 * @see #Hostname
 * @see #Port_Number
 * @see #System
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,j,retval,ivalue;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-filename")==0)||(strcmp(argv[i],"-f")==0))
		{
			if((i+1)<argc)
			{
				Message_Filename = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"tcs_to_udp:Parse_Arguments:Message_Filename requires an argument.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-help")==0)
		{
			Help();
			exit(0);
		}
		else if((strcmp(argv[i],"-hostname")==0)||(strcmp(argv[i],"-ip")==0))
		{
			if((i+1)<argc)
			{
				Hostname = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"tcs_to_udp:Parse_Arguments:Hostname requires a name.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-port_number")==0)||(strcmp(argv[i],"-p")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Port_Number);
				if(retval != 1)
				{
					fprintf(stderr,"tcs_to_udp:Parse_Arguments:Failed to parse port number '%s'.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"tcs_to_udp:Parse_Arguments:Port number requires a number.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-system")==0)||(strcmp(argv[i],"-s")==0))
		{
			if((i+1)<argc)
			{
				strncpy(System,argv[i+1],LOG_RECORD_SYSTEM_LENGTH-1);
				System[LOG_RECORD_SYSTEM_LENGTH-1] = '\0';
				i++;
			}
			else
			{
				fprintf(stderr,"tcs_to_udp:Parse_Arguments:System requires an argument.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"tcs_to_udp:Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
			return FALSE;
		}			
	}
	return TRUE;
}

/**
 * Help routine.
 */
static void Help(void)
{
	fprintf(stdout,"tcs_to_udp help.\n");
	fprintf(stdout,"tcs_to_udp reads a TCS log file and emits Log_UDP messages from them.\n");
	fprintf(stdout,"tcs_to_udp -hostname|-ip <hostname> -p[ort_number] <n> -f[ilename] <message filename>\n");
	fprintf(stdout,"\t[-help][-s[ystem] <system>]\n");
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.1  2009/01/21 11:01:07  cjm
** Initial revision
**
*/
