/* messages_to_udp.c
** $Header: /home/cjm/cvs/log_udp/test/messages_to_udp.c,v 1.1 2009-01-21 11:01:07 cjm Exp $
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
 * This program attempts to tail/read a /var/log/messages file, 
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
static char rcsid[] = "$Id: messages_to_udp.c,v 1.1 2009-01-21 11:01:07 cjm Exp $";

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
static char *System = "Messages";
/**
 * Parsed message filename. The filename to watch and extract message from, usually /var/log/messages
 */
static char *Message_Filename = "/var/log/messages";
/**
 * Parsed log severity.
 * @see ../cdocs/log_udp.html#LOG_SEVERITY
 */
static int Severity = LOG_SEVERITY_INFO;
/**
 * Parsed log verbosity.
 * @see ../cdocs/log_udp.html#LOG_VERBOSITY
 */
static int Verbosity = LOG_VERBOSITY_INTERMEDIATE;
/**
 * Start at end of log file rather than start (tail).
 */
static int Start_At_End = FALSE;
/**
 * Terminate when we reach the end of the file, or wait for new input.
 */
static int End_At_End = FALSE;
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
static int Create_Time(char *month_buff,int day_of_month,char *time_buff,struct tm *time_tm);
static int Is_Log_Rolled(FILE *fp);
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
 * @see #System
 * @see #Messages_To_UDP
 */
int main(int argc, char *argv[])
{

#if DEBUG > 1
	fprintf(stdout,"messages_to_udp:Started.\n");
#endif
	/* parse arguments */
	if(!Parse_Arguments(argc,argv))
	{
		fprintf(stderr,"messages_to_udp:Parse Arguments failed.\n");
		return 1;
	}
	if(Hostname == NULL)
	{
		fprintf(stderr,"messages_to_udp:No hostname specified.\n");
		return 2;
	}
	if(Message_Filename == NULL)
	{
		fprintf(stderr,"messages_to_udp:No message filename specified.\n");
		return 2;
	}
	Messages_To_UDP();
#if DEBUG > 1
	fprintf(stdout,"messages_to_udp:Freeing allocated data.\n");
#endif
	if(System != NULL)
		free(System);
	if(Hostname != NULL)
		free(Hostname);
	if(Message_Filename != NULL)
		free(Message_Filename);
#if DEBUG > 1
	fprintf(stdout,"messages_to_udp:Finished.\n");
#endif
	return 0;
}

/**
 * Routine to open a UDP socket, open and tail the mesasge file, and create new log records
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
	if(Start_At_End)
	{
		/* if test round this bit, whether we are injesting a copied log file
		** or an active one still being written to? 
		** You _must_ do this as we close the file to test for log rolling. */
		/* find the end of the file */
		if(fseek(fp,0,SEEK_END) == -1)
		{
			my_errno = errno;
			fprintf(stderr,"Seeking to end of file failed (%d).\n",my_errno);
		}
		/* where are we in the file */
		current_position = ftell(fp);
	}
#if DEBUG > 1
	fprintf(stdout,"messages_to_udp:Messages_To_UDP:Entering loop.\n");
#endif
	done = FALSE;
	while(done == FALSE)
	{
		/* if UDP socket is not open open it */
		if(Socket_Id < 0)
		{
#if DEBUG > 1
			fprintf(stdout,"messages_to_udp:Messages_To_UDP:Opening socket to %s:%d.\n",
				Hostname,Port_Number);
#endif
			if(!Log_UDP_Open(Hostname,Port_Number,&Socket_Id))
				Log_General_Error();
		}
		/* if message filename is not open open it */
		if(fp == NULL)
		{
#if DEBUG > 1
			fprintf(stdout,"messages_to_udp:Messages_To_UDP:Opening Message filename '%s'.\n",
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
#if DEBUG > 1
			fprintf(stdout,"messages_to_udp:Messages_To_UDP:End Position = %ld.\n",end_position);
#endif
			if(current_position < end_position)
			{
#if DEBUG > 1
				fprintf(stdout,"messages_to_udp:Messages_To_UDP:"
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
#if DEBUG > 1
				fprintf(stdout,"messages_to_udp:Messages_To_UDP:Read %d bytes.\n",number_read);
#endif
				/* process Message_Buffer */
				Process_Message_Buffer();
				/* reset current_message_buffer_position */
				current_message_buffer_position = strlen(Message_Buffer);
			}
			else if(current_position == end_position)
			{
				/* if we are just ingesting a log file and then quitting, terminate the loop */
				if(End_At_End)
				{
					done = TRUE;
				}
#if DEBUG > 1
				fprintf(stdout,"messages_to_udp:Messages_To_UDP:Current Position == End Position.\n");
#endif
				if(Is_Log_Rolled(fp))
				{
#if DEBUG > 1
					fprintf(stdout,"messages_to_udp:Messages_To_UDP:"
					"Close and reopen next time (we have been log rolled).\n");
#endif
					/* close log file , and next time round the loop we will reopen */
					fclose(fp);
					fp = NULL;
				}
				/* wait a bit */
				sleep(1);
			}
			else /* current_position > end_position) */
			{
#if DEBUG > 1
				fprintf(stdout,"messages_to_udp:Messages_To_UDP:Current Position > End Position:"
					"Close and reopen next time (we have been log rolled).\n");
#endif
				/* we must have been log rolled */
				/*  reopen file */
				/* close file , and next time round the loop we will reopen */
				fclose(fp);
				fp = NULL;
				/* reset current position to 0 */
				current_position = 0L;
			}
		}
	}/* end while forever */
#if DEBUG > 1
	fprintf(stdout,"messages_to_udp:Closing socket.\n");
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

#if DEBUG > 1
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
#if DEBUG > 1
			fprintf(stdout,"Process_Message_Buffer:No newline found in '%s'.\n",Message_Buffer);
#endif
		}
	}/* end while */
}

/**
 * Process the single /var/log/message line in line_buffer, attempt to parse into log fields.
 * Line should be of the form:
 * <pre>
 * Jan 15 20:17:25 ltobs9 gdm(pam_unix)[5232]: session closed for user cjm
 * </pre>
 * @param line_buffer A single /var/log/message line.
 * @see #System
 * @see #Severity
 * @see #Verbosity
 * @see #Socket_Id
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
	char machine_name_buff[128];
	char sub_system_buff[128];
	int retval,day_of_month,char_count;

#if DEBUG > 1
	fprintf(stdout,"Process_Line_Buffer:Started (%s).\n",line_buffer);
#endif
	/* try to parse */
	retval = sscanf(line_buffer,"%3s %2d %8s %127s %127s %n",month_buff,&day_of_month,time_buff,
			machine_name_buff,sub_system_buff,&char_count);
	if(retval != 5)
	{
		fprintf(stderr,"Process_Line_Buffer:Failed to parse:'%s' (%d,%s,%d,%s,%s,%s,%d).\n",line_buffer,retval,
			month_buff,day_of_month,time_buff,machine_name_buff,sub_system_buff,char_count);
	}
	if(char_count < strlen(line_buffer))
	{
		strcpy(message_buff,line_buffer+char_count);
	}
	else
		message_buff[0] = '\0';
#if DEBUG > 1
	fprintf(stdout,"Process_Line_Buffer:month = %s, day_of_month = %d,time = %s, machine = %s,"
		"sub_system = %s,mesage = %s.\n",
		month_buff,day_of_month,time_buff,machine_name_buff,sub_system_buff,message_buff);
#endif
	if(!Log_Create_Record(System,sub_system_buff,NULL,NULL,NULL,Severity,Verbosity,machine_name_buff,
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
	/* add machine name as context */
	if(!Log_Create_Context_List_Add(&log_context_list,&log_context_count,"Machine Name",machine_name_buff))
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
		fprintf(stderr,"messages_to_udp:Create_Time:time_tm was NULL.\n");
		return FALSE;
	}
	/* set tm to 0 bytes */
	memset(time_tm,0,sizeof(struct tm));
	now_time_s = time(NULL);
	now_time_tm = localtime(&now_time_s);
	retval = sscanf(time_buff,"%d:%d:%d",&hours,&minutes,&seconds);
	if(retval != 3)
	{
		fprintf(stderr,"messages_to_udp:Create_Time:Failed to parse time_buff:%s (%d).\n",time_buff,retval);
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
		fprintf(stderr,"messages_to_udp:Create_Time:Unknown month %s.\n",month_buff);
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
 * Function to determine whether the message file has been log-rolled by syslogd.
 * We use fstat on the open file and stat on the filename, and then compare indes.
 * @param fp The currently opened file pointer.
 * @return The function returns TRUE if the fp is not currently using the Message_Filename (i.e. it has been renamed),
 *         FALSE if Message_Filename and fp point to the same file (have the same inode).
 * @see #Message_Filename
 */
static int Is_Log_Rolled(FILE *fp)
{
	int fn,ferrno;
	struct stat file_stat1,file_stat2;

	fn = fileno(fp);
	if(fstat(fn,&file_stat1) == -1)
	{
		ferrno = errno;
		fprintf(stderr,"messages_to_udp:Is_Log_Rolled:Failed to fstat opened file.\n");
		return FALSE; /* guess not log-rolled */
	}
	
	if(stat(Message_Filename,&file_stat2) == -1)
	{
		ferrno = errno;
		fprintf(stderr,"messages_to_udp:Is_Log_Rolled:Failed to stat file'%s'.\n",Message_Filename);
		return FALSE; /* guess not log-rolled */
	}
	/* if inodes are same, file has not been log rolled.
	** if inodes are different, opened and current filename are different files. */
	return (file_stat1.st_ino != file_stat2.st_ino);
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
 * @see #Severity
 * @see #Verbosity
 * @see #Start_At_End
 * @see #End_At_End
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,j,retval,ivalue;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-end_at_end")==0)||(strcmp(argv[i],"-eae")==0))
		{
			End_At_End = TRUE;
		}
		else if((strcmp(argv[i],"-error")==0)||(strcmp(argv[i],"-e")==0))
		{
			Severity = LOG_SEVERITY_ERROR;
		}
		else if((strcmp(argv[i],"-filename")==0)||(strcmp(argv[i],"-f")==0))
		{
			if((i+1)<argc)
			{
				Message_Filename = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"messages_to_udp:Parse_Arguments:Message_Filename requires an argument.\n");
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
				fprintf(stderr,"messages_to_udp:Parse_Arguments:Hostname requires a name.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-info")==0)||(strcmp(argv[i],"-i")==0))
		{
			Severity = LOG_SEVERITY_INFO;
		}
		else if((strcmp(argv[i],"-port_number")==0)||(strcmp(argv[i],"-p")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Port_Number);
				if(retval != 1)
				{
					fprintf(stderr,"messages_to_udp:Parse_Arguments:Failed to parse port number '%s'.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"messages_to_udp:Parse_Arguments:Port number requires a number.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-start_at_end")==0)||(strcmp(argv[i],"-sae")==0))
		{
			Start_At_End = TRUE;
		}
		else if((strcmp(argv[i],"-system")==0)||(strcmp(argv[i],"-s")==0))
		{
			if((i+1)<argc)
			{
				System = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"messages_to_udp:Parse_Arguments:System requires an argument.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-verbosity")==0)||(strcmp(argv[i],"-v")==0))
		{
			if((i+1)<argc)
			{
				if(strcmp(argv[i+1],"veryterse")==0)
					Verbosity = LOG_VERBOSITY_VERY_TERSE;
				else if(strcmp(argv[i+1],"terse")==0)
					Verbosity = LOG_VERBOSITY_TERSE;
				else if(strcmp(argv[i+1],"intermediate")==0)
					Verbosity = LOG_VERBOSITY_INTERMEDIATE;
				else if(strcmp(argv[i+1],"verbose")==0)
					Verbosity = LOG_VERBOSITY_VERBOSE;
				else if(strcmp(argv[i+1],"veryverbose")==0)
					Verbosity = LOG_VERBOSITY_VERY_VERBOSE;
				else if(strcmp(argv[i+1],"1")==0)
					Verbosity = LOG_VERBOSITY_VERY_TERSE;
				else if(strcmp(argv[i+1],"2")==0)
					Verbosity = LOG_VERBOSITY_TERSE;
				else if(strcmp(argv[i+1],"3")==0)
					Verbosity = LOG_VERBOSITY_INTERMEDIATE;
				else if(strcmp(argv[i+1],"4")==0)
					Verbosity = LOG_VERBOSITY_VERBOSE;
				else if(strcmp(argv[i+1],"5")==0)
					Verbosity = LOG_VERBOSITY_VERY_VERBOSE;
				else
				{
					fprintf(stderr,"messages_to_udp:Parse_Arguments:Failed to parse verbosity '%s'.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"messages_to_udp:Parse_Arguments:Verbosity requires an argument.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"messages_to_udp:Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
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
	fprintf(stdout,"messages_to_udp help.\n");
	fprintf(stdout,"messages_to_udp reads a /var/log/messages file and emits Log_UDP messages from them.\n");
	fprintf(stdout,"messages_to_udp -hostname|-ip <hostname> -p[ort_number] <n> -f[ilename] <message filename>\n");
	fprintf(stdout,"\t<-i[nfo]|-e[rror]> [-help]\n");
	fprintf(stdout,"\t-v[erbosity] <veryterse|terse|intermediate|verbose|veryverbose|1|2|3|4|5>\n");
	fprintf(stdout,"\t[-s[ystem] <system>]\n");
	fprintf(stdout,"\t[-start_at_end][-end_at_end]\n");
}

/*
** $Log: not supported by cvs2svn $
*/
