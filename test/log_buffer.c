/* log_buffer.c
** Take a stream of log messages from stdin (supplied from a pipe), and hold them in a rolling buffer in memeory.
** Provide a telnet socket to save, list, delete the rolling log 
*/
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log_udp.h"
#include "command_server.h"

/**
 * This program takes a stream of log messages from stdin (supplied from a pipe), 
 * and holds them in a rolling buffer in memeory.
 * It provides a telnet socket interface to save, list, and delete the rolling log.
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
 * Default length of the log buffer.
 */
#define DEFAULT_LOG_BUFFER_LENGTH          (1024*1024)
/**
 * Default number of lines in the log buffer line list.
 */
#define DEFAULT_LOG_BUFFER_LINE_LIST_COUNT (1024)

/* structures */
struct Log_Buffer_Line_Struct
{
	int Start_Pos;
	int End_Pos;
	struct timespec Timestamp;
};

/* internal variables */
/**
 * Revision control system identifier.
 */
static char rcsid[] = "$Id: messages_to_udp.c,v 1.1 2009-01-21 11:01:07 cjm Exp $";
/**
 * The port number to host the command server socket on.
 */
static int Port_Number = 0;
/**
 * The log buffer 
 */
static char *Log_Buffer = NULL;
/**
 * The length of the log buffer to allocate.
 * @see #DEFAULT_LOG_BUFFER_LENGTH
 */
static int Log_Buffer_Length = DEFAULT_LOG_BUFFER_LENGTH;
/**
 * Index in Log_Buffer of the start of the wrapped buffer.
 */
static int Log_Buffer_Start_Index = 0;
/**
 * Index in Log_Buffer of the end of the wrapped buffer.
 */
static int Log_Buffer_End_Index = 0;
/**
 * A list of Log_Buffer_Line_Struct structures containing the start and end positions of each line, with a timestamp
 * of when the line was received.
 * @see #Log_Buffer_Line_Struct
 */
static struct Log_Buffer_Line_Struct *Log_Buffer_Line_List = NULL;
/**
 * The number of items in the Log_Buffer_Line_List.
 * @see #DEFAULT_LOG_BUFFER_LINE_LIST_COUNT
 */
static int Log_Buffer_Line_List_Count = DEFAULT_LOG_BUFFER_LINE_LIST_COUNT;
/**
 * Index in Log_Buffer_Line_List of the first line index record of the wrapped buffer.
 */
static int Log_Buffer_Line_Start_Index = 0;
/**
 * Index in Log_Buffer_Line_List of the end line index record of the wrapped buffer.
 */
static int Log_Buffer_Line_End_Index = 0;
/**
 * The server context to use for this server.
 */
static Command_Server_Server_Context_T Server_Context = NULL;

/* internal functions */
static int Allocate_Buffers(void);
static int Start_Input_Thread(void);
static void *Input_Thread(void *user_arg);
static int Log_Buffer_Add_Line(char *input_buffer);
static int Setup_Command_Server(void);
static void Server_Connection_Callback(Command_Server_Handle_T connection_handle);
static int Line_List_Line_Count_Get(void);
static int Line_List_Line_Index_Get(int index);
static int Add_Line_To_String(int line_index,char **string_ptr);
static void Send_Reply(Command_Server_Handle_T connection_handle,char *reply_message);
static int Parse_Arguments(int argc, char *argv[]);
static void Help(void);

/**
 * Main program.
 * @param argc The number of arguments to the program.
 * @param argv An array of argument strings.
 * @return This function returns 0 if the program succeeds, and a positive boolean if it fails.
 * @see #Port_Number
 * @see #Parse_Arguments
 * @see #Log_Buffer_Start_Index
 * @see #Log_Buffer_End_Index
 * @see #Log_Buffer_Line_Start_Index
 * @see #Log_Buffer_Line_End_Index
 * @see #Allocate_Buffers
 * @see #Setup_Command_Server
 * @see #Start_Input_Thread
 * @see #Server_Connection_Callback
 * @see #Server_Context
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Start_Server
 * @see ../../commandserver/cdocs/command_server.html#Command_Server_Error
 */
int main(int argc, char *argv[])
{
	int retval;
	
	/* parse arguments */
	if(!Parse_Arguments(argc,argv))
	{
		fprintf(stderr,"log_buffer:Parse Arguments failed.\n");
		return 1;
	}
	/* initialise buffer and line start/end indexes */
	Log_Buffer_Start_Index = 0;
	Log_Buffer_End_Index = 0;
	Log_Buffer_Line_Start_Index = 0;
	Log_Buffer_Line_End_Index = 0;
	/* allocate buffer/line lists */
	if(!Allocate_Buffers())
	{
		fprintf(stderr,"log_buffer:Failed to allocate buffers.\n");
		return 2;
	}
	if(!Setup_Command_Server())
	{
		fprintf(stderr,"log_buffer:Failed to setup command server.\n");
		return 2;
	}
	/* start a thread to read lines from stdin, and put them in the log buffer */
	if(!Start_Input_Thread())
	{
		fprintf(stderr,"log_buffer:Failed to start input thread.\n");
		return 2;
	}
	/* start command server */
	retval = Command_Server_Start_Server((unsigned short int *)&Port_Number,Server_Connection_Callback,&Server_Context);
	if(retval == FALSE)
	{
		Command_Server_Error();
		return 4;
	}		
	return 0;
}

/**
 * Allocate log buffer and log buffer line list.
 * @return The routine returns TRUE on success, and FALSE if it fails. 
 * @see #Log_Buffer
 * @see #Log_Buffer_Length
 * @see #Log_Buffer_Line_List
 * @see #Log_Buffer_Line_List_Count
 */
static int Allocate_Buffers(void)
{
	if(Log_Buffer == NULL)
		Log_Buffer = (char*)malloc(Log_Buffer_Length*sizeof(char));
	else
		Log_Buffer = (char*)realloc(Log_Buffer,Log_Buffer_Length*sizeof(char));
	if(Log_Buffer == NULL)
	{
		fprintf(stderr,"Failed to allocate Log_Buffer to length %d.\n",Log_Buffer_Length);
		return FALSE;
	}
	if(Log_Buffer_Line_List == NULL)
	{
		Log_Buffer_Line_List = (struct Log_Buffer_Line_Struct*)malloc(Log_Buffer_Line_List_Count*sizeof(struct Log_Buffer_Line_Struct));
	}
	else
	{
		Log_Buffer_Line_List = (struct Log_Buffer_Line_Struct*)realloc(Log_Buffer_Line_List,
						      Log_Buffer_Line_List_Count*sizeof(struct Log_Buffer_Line_Struct));
	}
	if(Log_Buffer_Line_List == NULL)
	{
		fprintf(stderr,"Failed to allocate Log_Buffer_Line_List to count %d.\n",Log_Buffer_Line_List_Count);
		return FALSE;
	}
	return TRUE;
}

/**
 * Start a thread to handle log message input from stdin. This is added to the rolling log buffer.
 * @return The routine returns TRUE on success, and FALSE if it fails. 
 * @see Input_Thread
 */
static int Start_Input_Thread(void)
{
	pthread_t input_thread;
	pthread_attr_t attr;
	int perr;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
	if((perr = pthread_create(&input_thread,&attr,&Input_Thread,NULL)) != 0)
	{
		fprintf(stderr,"log_buffer: Failed to start input thread: %s.\n",strerror(perr));
		return FALSE;
	}
	return TRUE;
}

/**
 * Top level input thread, reading strings of data from stdin, and adding them to the rolling log buffer.
 * @see #Log_Buffer_Add_Line
 */
static void *Input_Thread(void *user_arg)
{
	char *ch_ptr = NULL;
	char input_buffer[1024];
	
	while(stdin != NULL)
	{
		ch_ptr = fgets(input_buffer,1024,stdin);
		if(ch_ptr != NULL)
		{
			Log_Buffer_Add_Line(input_buffer);
		}
	}
	return NULL;
}

/**
 * Add a new line to the log buffer, and setup the log buffer line list.
 * @param input_buffer The null terminated line to add. This will probably also contain a newline.b
 * @see #Log_Buffer
 * @see #Log_Buffer_Length
 * @see #Log_Buffer_Line_List
 * @see #Log_Buffer_Line_List_Count
 * @see #Log_Buffer_Start_Index
 * @see #Log_Buffer_End_Index
 * @see #Log_Buffer_Line_Start_Index
 * @see #Log_Buffer_Line_End_Index
 */
static int Log_Buffer_Add_Line(char *input_buffer)
{
	int input_buffer_length,input_line_index,line_index,done;
	int input_line_index_start_pos, input_line_index_end_pos;
	int overlap1,overlap2,overlap3,overlap4,overlap;
	
	/* find line index in Log_Buffer_Line_List to use for new line */
	if((Log_Buffer_Line_End_Index+1) < Log_Buffer_Line_List_Count)
	{
		input_line_index = (Log_Buffer_Line_End_Index+1);
		Log_Buffer_Line_End_Index++;
	}
	else
	{
		input_line_index = 0;
		Log_Buffer_Line_End_Index = 0;
	}
	/* if we are wrapping round line indexes start moving the start index */
	if(Log_Buffer_Line_Start_Index == Log_Buffer_Line_End_Index)
	{
		Log_Buffer_Line_Start_Index++;
		if(Log_Buffer_Line_Start_Index == Log_Buffer_Line_List_Count)
			Log_Buffer_Line_Start_Index = 0;
	}
	/* add input_buffer text to log buffer */
	input_buffer_length = strlen(input_buffer);
	if((Log_Buffer_End_Index+input_buffer_length) < Log_Buffer_Length)
	{
		/* if the new line will fit into the end of the buffer, add it there */
		strcat(Log_Buffer+Log_Buffer_End_Index,input_buffer);
		Log_Buffer_Line_List[input_line_index].Start_Pos = Log_Buffer_End_Index;
		Log_Buffer_End_Index += input_buffer_length;
		Log_Buffer_Line_List[input_line_index].End_Pos = Log_Buffer_End_Index;
		clock_gettime(CLOCK_REALTIME,&(Log_Buffer_Line_List[input_line_index].Timestamp));
	}
	else
	{
		/* this line will go at the start of the buffer */
		strcat(Log_Buffer,input_buffer);
		Log_Buffer_Line_List[input_line_index].Start_Pos = 0;
		Log_Buffer_End_Index += input_buffer_length;
		Log_Buffer_Line_List[input_line_index].End_Pos = Log_Buffer_End_Index;
		clock_gettime(CLOCK_REALTIME,&(Log_Buffer_Line_List[input_line_index].Timestamp));
	}
	/* Note we might have overwritten lines */
	line_index = Log_Buffer_Line_Start_Index;
	done = FALSE;
	input_line_index_start_pos = Log_Buffer_Line_List[input_line_index].Start_Pos;
	input_line_index_end_pos = Log_Buffer_Line_List[input_line_index].End_Pos;
	while(done == FALSE)
	{
		/* overlap1
		** input line     |----------|
		** line index   |---|
		*/  
		overlap1 = (Log_Buffer_Line_List[line_index].Start_Pos < input_line_index_start_pos)&&(Log_Buffer_Line_List[line_index].End_Pos > input_line_index_start_pos);
		/* overlap2
		** input line     |----------|
		** line index         |---|
		*/  
		overlap2 = (Log_Buffer_Line_List[line_index].Start_Pos > input_line_index_start_pos)&&(Log_Buffer_Line_List[line_index].End_Pos < input_line_index_end_pos);
		/* overlap3
		** input line     |----------|
		** line index              |---|
		*/  
		overlap3 = (Log_Buffer_Line_List[line_index].Start_Pos < input_line_index_end_pos)&&(Log_Buffer_Line_List[line_index].End_Pos > input_line_index_end_pos);
		/* overlap3
		** input line     |----------|
		** line index   |--------------|
		*/  
		overlap4 = (Log_Buffer_Line_List[line_index].Start_Pos < input_line_index_start_pos)&&(Log_Buffer_Line_List[line_index].End_Pos> input_line_index_end_pos);
		overlap = overlap1|overlap2|overlap3|overlap4;
		if(overlap)
		{
			line_index++;
			Log_Buffer_Line_Start_Index++;
			if(Log_Buffer_Line_Start_Index == Log_Buffer_Line_List_Count)
				Log_Buffer_Line_Start_Index = 0;
		}
		else
			done = TRUE;
	}
	return TRUE;
}
		    
/**
 * Setup the command server.
 * @return The routine returns TRUE on success, and FALSE if it fails. 
 */
static int Setup_Command_Server(void)
{
	Command_Server_Set_Log_Handler_Function(Command_Server_Log_Handler_Stdout);
	Command_Server_Set_Log_Filter_Function(Command_Server_Log_Filter_Level_Absolute);
	Command_Server_Set_Log_Filter_Level(LOG_VERBOSITY_VERY_VERBOSE);
	return TRUE;
}

/**
 * Function invoked when a connection is made to the command server.
 * @param connection_handle Globus_io connection handle for this thread.
 * @see #Send_Reply
 * @see #Log_Buffer_Start_Index
 * @see #Log_Buffer_End_Index
 * @see #Line_List_Line_Count_Get
 * @see #Line_List_Line_Index_Get
 * @see #Add_Line_To_String
 */
static void Server_Connection_Callback(Command_Server_Handle_T connection_handle)
{
	char reply_string[256];
	char *reply_string_ptr = NULL;
	char *client_message = NULL;
	int retval;
	int seconds,i,n,line_count,line_list_line_count,line_index;

	/* get message from client */
	retval = Command_Server_Read_Message(connection_handle, &client_message);
	if(retval == FALSE)
	{
		Command_Server_Error();
		return;
	}
	printf("log_buffer: received '%s'\n", client_message);
	/* do something with message */
	if(strcmp(client_message, "buffer positions") == 0)
	{
		sprintf(reply_string,"Buffer Positions: Start: %d, End: %d",Log_Buffer_Start_Index,Log_Buffer_End_Index);
		Send_Reply(connection_handle, reply_string);
	}
	else if(strcmp(client_message, "help") == 0)
	{
		printf("log_buffer server: help detected.\n");
		Send_Reply(connection_handle, "help:\n"
			   "\tbuffer positions\n"
			   "\tlast <n>\n"
			   "\tline count|indexes\n"
			   "\tsince <YYYY-MM-DDThh:mm:ss>\n"
			   "\tshutdown\n");
	}
	else if(strncmp(client_message, "last",4) == 0)
	{
		retval = sscanf(client_message,"last %d",&line_count);
		if(retval != 1)
		{
			sprintf(reply_string,"Could not parse last line count(%d): %s.",retval,client_message);
			Send_Reply(connection_handle, reply_string);
			return;
		}
		reply_string_ptr = NULL;
		line_list_line_count = Line_List_Line_Count_Get();
		if(line_count > line_list_line_count)
			line_count = line_list_line_count;
		for(int i = 0; i < line_count;i++)
		{
			line_index = Line_List_Line_Index_Get(i);
			if((line_index > 0)&&(line_index < Log_Buffer_Line_List_Count))
			{
				if(!Add_Line_To_String(line_index,&reply_string_ptr))
				{
					fprintf(stderr,"last: Add_Line_To_String (line_index %d/%d of %d) failed.\n",i,
						line_index,line_count);
				}
			}
			else
				fprintf(stderr,"last: Out of range line index '%d' computed (s=%d,e=%d,c=%c).",line_index,
					Log_Buffer_Line_Start_Index,Log_Buffer_Line_End_Index,Log_Buffer_Line_List_Count);
		}
		Send_Reply(connection_handle, reply_string_ptr);
	}
	else if(strncmp(client_message, "line count",10) == 0)
	{
		sprintf(reply_string,"Line Count: %d",Line_List_Line_Count_Get());
		Send_Reply(connection_handle, reply_string);
	}
	else if(strncmp(client_message, "line indexes",12) == 0)
	{
		sprintf(reply_string,"Line Indexes: Start:  %d End: %d",Log_Buffer_Line_Start_Index,Log_Buffer_Line_End_Index);
		Send_Reply(connection_handle, reply_string);
	}
	else if(strcmp(client_message, "shutdown") == 0)
	{
		printf("log_buffer server: shutdown detected:about to stop.\n");
		Send_Reply(connection_handle, "ok");
		retval = Command_Server_Close_Server(&Server_Context);
		if(retval == FALSE)
			Command_Server_Error();
	}
	else
	{
		printf("log_buffer: message unknown: '%s'\n", client_message);
		Send_Reply(connection_handle, "failed message unknown");
	}
	/* free message */
	free(client_message);
}

/**
 * Get how many lines are currently in the line list.
 * @return The numbner of active lines in the line list.
 * @see #Log_Buffer_Line_Start_Index
 * @see #Log_Buffer_Line_End_Index
 * @see #Log_Buffer_Line_List_Count
 */
static int Line_List_Line_Count_Get(void)
{
	int line_count;

	if(Log_Buffer_Line_End_Index > Log_Buffer_Line_Start_Index)
		line_count = Log_Buffer_Line_End_Index-Log_Buffer_Line_Start_Index;
	else if(Log_Buffer_Line_End_Index == Log_Buffer_Line_Start_Index)
		return 0; /* or Log_Buffer_Line_List_Count ? */
	else /* Log_Buffer_Line_Start_Index > Log_Buffer_Line_End_Index, therefore line indexes have wrapped */
	{
		line_count = (Log_Buffer_Line_List_Count-Log_Buffer_Line_Start_Index)+Log_Buffer_Line_End_Index;
	}
	return line_count;
}

/**
 * Get the index in the line list (which goes from Log_Buffer_Line_Start_Index to Log_Buffer_Line_End_Index, potentially
 * wrapping).
 * @param index The unwrapped index to get the wrapped index for, from 0 to Line_List_Line_Count_Get.
 * @return The index in the line list which corresponds to wrapped index index. Or -1 if thats not possible.
 * @see #Log_Buffer_Line_Start_Index
 * @see #Log_Buffer_Line_End_Index
 * @see #Log_Buffer_Line_List_Count
 */
static int Line_List_Line_Index_Get(int index)
{
	if(Log_Buffer_Line_End_Index > Log_Buffer_Line_Start_Index)
	{
		if((Log_Buffer_Line_Start_Index+index) < Log_Buffer_Line_List_Count)
			return (Log_Buffer_Line_Start_Index+index);
		else
			return -1;
	}
	else /* Log_Buffer_Line_Start_Index > Log_Buffer_Line_End_Index, therefore line indexes have wrapped */
	{
		if(index < (Log_Buffer_Line_List_Count-Log_Buffer_Line_Start_Index))
			return (Log_Buffer_Line_Start_Index+index);
		else
			return index-(Log_Buffer_Line_List_Count-Log_Buffer_Line_Start_Index);
	}
}

/**
 * Add the log buffer line stored in Log_Buffer_Line_List[line_index] to the end of the re-allocatable string string_ptr.
 * @param line_index A valid index into Log_Buffer_Line_List.
 * @param string_ptr The address of a re-allocatable string. This should be (re)-allocated, and the string specified in 
 *        Log_Buffer_Line_List[line_index] added to the buffer.
 */
static int Add_Line_To_String(int line_index,char **string_ptr)
{
	int line_length;
	
	if(string_ptr  == NULL)
	{
		fprintf(stderr,"Add_Line_To_String: string_ptr was NULL.\n");
		return FALSE;
	}
	if((line_index < 0) || (line_index >= Log_Buffer_Line_List_Count))
	{
		fprintf(stderr,"Add_Line_To_String: line_index %d is out of range (%d).\n",line_index,Log_Buffer_Line_List_Count);
		return FALSE;
	}
	line_length = (Log_Buffer_Line_List[line_index].End_Pos - Log_Buffer_Line_List[line_index].Start_Pos);
	if((*string_ptr) == NULL)
	{
		(*string_ptr) = (char*)malloc((line_length+1)*sizeof(char));
	}
	else
	{
		(*string_ptr) = (char*)realloc((*string_ptr),(strlen((*string_ptr))+line_length+1)*sizeof(char));
	}
	if((*string_ptr) == NULL)
	{
		fprintf(stderr,"Add_Line_To_String:Failed to malloc string (%lu,%d).\n",strlen((*string_ptr)),line_length+1);
		return FALSE;
	}
	strncat((*string_ptr),Log_Buffer+Log_Buffer_Line_List[line_index].Start_Pos,line_length);
	return TRUE;
}

/**
 * Send a message back to the client.
 * @param connection_handle Connection handle for this thread.
 * @param reply_message The message to send.
 */
static void Send_Reply(Command_Server_Handle_T connection_handle,char *reply_message)
{
	int retval;

	/* send something back to the client */
	printf("log_buffer server: about to send '%s'\n", reply_message);
	retval = Command_Server_Write_Message(connection_handle, reply_message);
	if(retval == FALSE)
	{
		Command_Server_Error();
		return;
	}
	printf("log_buffer server: sent '%s'\n", reply_message);
}

/**
 * Routine to parse command line arguments.
 * @param argc The number of arguments sent to the program.
 * @param argv An array of argument strings.
 * @return The routine returns TRUE on success, and FALSE if it fails. 
 * @see #Help
 * @see #Port_Number
 */
static int Parse_Arguments(int argc, char *argv[])
{
	int i,j,retval,ivalue;

	for(i=1;i<argc;i++)
	{
		if(strcmp(argv[i],"-help")==0)
		{
			Help();
			exit(0);
		}
		else if((strcmp(argv[i],"-port_number")==0)||(strcmp(argv[i],"-p")==0))
		{
			if((i+1)<argc)
			{
				retval = sscanf(argv[i+1],"%d",&Port_Number);
				if(retval != 1)
				{
					fprintf(stderr,"log_buffer:Parse_Arguments:Failed to parse port number '%s'.\n",
						argv[i+1]);
					return FALSE;
				}
				i++;
			}
			else
			{
				fprintf(stderr,"log_buffer:Parse_Arguments:Port number requires a number.\n");
				return FALSE;
			}
		}
		else
		{
			fprintf(stderr,"log_buffer:Parse_Arguments:argument '%s' not recognized.\n",argv[i]);
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
	fprintf(stdout,"log_buffer help.\n");
	fprintf(stdout,"log_buffer -p[ort_number] <n> [-help]\n");
}
