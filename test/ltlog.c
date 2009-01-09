/* ltlog.c
** $Header: /home/cjm/cvs/log_udp/test/ltlog.c,v 1.1 2009-01-09 14:54:44 cjm Exp $
*/
#include <stdio.h>
#include <string.h>
#include "log_general.h"
#include "log_udp.h"
#include "log_create.h"

/**
 * This program creates and sends log messages to the GlobalLoggingSystem.
 * @author $Author: cjm $
 * @version $Revision: 1.1 $
 */

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: ltlog.c,v 1.1 2009-01-09 14:54:44 cjm Exp $";

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
static char *System = NULL;
/**
 * Parsed sub-system.
 */
static char *Sub_System = NULL;
/**
 * Parsed source file.
 */
static char *Source_File = NULL;
/**
 * Parsed source instance.
 */
static char *Source_Instance = NULL;
/**
 * Parsed function.
 */
static char *Function = NULL;
/**
 * Parsed category.
 */
static char *Category = NULL;
/**
 * Parsed message.
 */
static char Message[LOG_RECORD_MESSAGE_LENGTH];
/**
 * Parsed log severity.
 * @see ../cdocs/log_udp.html#LOG_SEVERITY
 */
static int Severity = LOG_SEVERITY_INFO;
/**
 * Parsed log verbosity.
 * @see ../cdocs/log_udp.html#LOG_VERBOSITY
 */
static int Verbosity = LOG_VERBOSITY_VERY_VERBOSE;
/**
 * List of log contexts.
 * @see #Log_Context_Count
 * @see ../cdocs/log_udp.html#Log_Context_Struct
 */
static struct Log_Context_Struct *Log_Context_List = NULL;
/**
 * Number of log contexts in list.
 * @see #Log_Context_List
 */
static int Log_Context_Count = 0;

/* internal routines */
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive boolean if it fails.
 * @see #Hostname
 * @see #Port_Number
 * @see #System
 * @see #Sub_System
 * @see #Source_File
 * @see #Source_Instance
 * @see #Function
 * @see #Category
 * @see #Message
 * @see #Severity
 * @see #Verbosity
 * @see #Log_Context_List
 * @see #Log_Context_Count
 */
int main(int argc, char *argv[])
{
	struct Log_Record_Struct log_record;
	int socket_id;

#if DEBUG > 1
	fprintf(stdout,"ltlog:Started.\n");
#endif
	/* parse arguments */
	if(!Parse_Arguments(argc,argv))
	{
		fprintf(stderr,"ltlog:Parse Arguments failed.\n");
		return 1;
	}
	if(Hostname == NULL)
	{
		fprintf(stderr,"ltlog:No hostname specified.\n");
		return 2;
	}
	if(strlen(Message) == 0)
	{
		fprintf(stderr,"ltlog:No message specified.\n");
		return 2;
	}
#if DEBUG > 1
	fprintf(stdout,"ltlog:Creating record.\n");
#endif
	if(!Log_Create_Record(System,Sub_System,Source_File,Source_Instance,Function,Severity,Verbosity,
			      Category,Message,&log_record))
	{
		Log_General_Error();
		return 3;
	}
#if DEBUG > 1
	fprintf(stdout,"ltlog:Opening socket.\n");
#endif
	if(!Log_UDP_Open(Hostname,Port_Number,&socket_id))
	{
		Log_General_Error();
		return 4;
	}
#if DEBUG > 1
	fprintf(stdout,"ltlog:Sending record.\n");
#endif
	if(!Log_UDP_Send(socket_id,log_record,Log_Context_Count,Log_Context_List))
	{
		Log_General_Error();
		Log_UDP_Close(socket_id);
		return 5;
	}
#if DEBUG > 1
	fprintf(stdout,"ltlog:Closing socket.\n");
#endif
	if(!Log_UDP_Close(socket_id))
	{
		Log_General_Error();
		return 6;
	}
#if DEBUG > 1
	fprintf(stdout,"ltlog:Freeing allocated data.\n");
#endif
	if(System != NULL)
		free(System);
	if(Sub_System != NULL)
		free(Sub_System);
	if(Source_File != NULL)
		free(Source_File);
	if(Source_Instance != NULL)
		free(Source_Instance);
	if(Function != NULL)
		free(Function);
	if(Category != NULL)
		free(Category);
#if DEBUG > 1
	fprintf(stdout,"ltlog:Finished.\n");
#endif
	return 0;
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
 * @see #Sub_System
 * @see #Source_File
 * @see #Source_Instance
 * @see #Function
 * @see #Category
 * @see #Message
 * @see #Severity
 * @see #Verbosity
 * @see #Log_Context_List
 * @see #Log_Context_Count
 * @see ../cdocs/log_udp.html#LOG_RECORD_MESSAGE_LENGTH
 */
static int Parse_Arguments(int argc, char *argv[])
{
	struct Log_Context_Struct log_context;
	int i,j,retval,ivalue;

	for(i=1;i<argc;i++)
	{
		if((strcmp(argv[i],"-category")==0)||(strcmp(argv[i],"-c")==0))
		{
			if((i+1)<argc)
			{
				Category = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Category requires an argument.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-context")==0)||(strcmp(argv[i],"-co")==0))
		{
			if((i+2)<argc)
			{
				if(!Log_Create_Context_List_Add(&Log_Context_List,&Log_Context_Count,
								argv[i+1],argv[i+2]))
				{
					Log_General_Error();
					return FALSE;
				}
				i+= 2;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:context requires two arguments: keyword and value.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-error")==0)||(strcmp(argv[i],"-e")==0))
		{
			Severity = LOG_SEVERITY_ERROR;
		}
		else if((strcmp(argv[i],"-function")==0)||(strcmp(argv[i],"-f")==0))
		{
			if((i+1)<argc)
			{
				Function = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Function requires an argument.\n");
				return FALSE;
			}
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
				fprintf(stderr,"ltlog:Parse_Arguments:Hostname requires a name.\n");
				return FALSE;
			}
		}
		else if(strcmp(argv[i],"-help")==0)
		{
			Help();
			exit(0);
		}
		else if((strcmp(argv[i],"-info")==0)||(strcmp(argv[i],"-i")==0))
		{
			Severity = LOG_SEVERITY_INFO;
		}
		else if((strcmp(argv[i],"-message")==0)||(strcmp(argv[i],"-m")==0))
		{
			if((i+1)<argc)
			{
				Message[0] = '\0';
				for(j = i+1; j < argc; j++)
				{
					if((strlen(Message)+strlen(argv[j])+2) < LOG_RECORD_MESSAGE_LENGTH)
					{
						strcat(Message,argv[j]);
						strcat(Message," ");
					}
					else
					{
						fprintf(stderr,"ltlog:Parse_Arguments:"
							"No more room in message buffer for:%s.\n",argv[j]);
						/* don't exit - assume truncated message is more useful them bailout */
					}
					i++;
				}/* end for */
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Message requires an argument.\n");
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
					fprintf(stderr,"ltlog:Parse_Arguments:Failed to parse port number '%s'.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Port number requires a number.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-source_file")==0)||(strcmp(argv[i],"-sf")==0))
		{
			if((i+1)<argc)
			{
				Source_File = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Source_File requires an argument.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-source_instance")==0)||(strcmp(argv[i],"-si")==0))
		{
			if((i+1)<argc)
			{
				Source_Instance = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Source_Instance requires an argument.\n");
				return FALSE;
			}
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
				fprintf(stderr,"ltlog:Parse_Arguments:System requires an argument.\n");
				return FALSE;
			}
		}
		else if((strcmp(argv[i],"-sub_system")==0)||(strcmp(argv[i],"-ss")==0))
		{
			if((i+1)<argc)
			{
				Sub_System = strdup(argv[i+1]);
				i++;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Sub System requires an argument.\n");
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
					fprintf(stderr,"ltlog:Parse_Arguments:Failed to parse verbosity '%s'.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"ltlog:Parse_Arguments:Verbosity requires an argument.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"ltlog:Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
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
	fprintf(stdout,"ltlog help.\n");
	fprintf(stdout,"ltlog creates,formats and sends a udp log packet to the GlobalLoggingSystem.\n");
	fprintf(stdout,"ltlog -hostname|-ip <hostname> -p[ort_number] <n>\n");
	fprintf(stdout,"\t<-i[nfo]|-e[rror]> \n");
	fprintf(stdout,"\t-v[erbosity] <veryterse|terse|intermediate|verbose|veryverbose|1|2|3|4|5>\n");
	fprintf(stdout,"\t[-s[ystem] <system>][-sub_system <subsystem>][-source_file <source_file>]\n");
	fprintf(stdout,"\t[-source_instance <instance>][-f[unction] <function>]\n");
	fprintf(stdout,"\t[-c[ategory] <category>][-help]\n");
	fprintf(stdout,"\t[-co[ntext] <keyword> <value>]\n");
	fprintf(stdout,"\t-m[essage] <string> <string> ...\n");
}

/*
** $Log: not supported by cvs2svn $
*/
