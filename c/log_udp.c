/* log_udp.c
** GLS logging using UDP packets
** $Header: /home/cjm/cvs/log_udp/c/log_udp.c,v 1.5 2011-09-26 15:53:58 cjm Exp $
*/
/**
 * UDP packet creation and transmission routines.
 * @author Chris Mottram
 * @version $Revision: 1.5 $
 */
/**
 * Define BSD Source to get BSD prototypes, including gethostbyname_r.
 */
#define _BSD_SOURCE (1)
#ifdef __linux
#include <endian.h>  /* Used to determine whether to byte swap to get network byte order */ 
#include <byteswap.h> /* Get machine dependent optimized versions of byte swapping functions.  */
#endif
#include <errno.h>   /* Error number definitions */
#include <fcntl.h>   /* File control definitions */
#include <netdb.h>
#include <pthread.h>
#ifdef __linux
#include <stdint.h>  /* defines int64_t (Java long) */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h> /* htons etc */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "log_general.h"
#include "log_udp.h"

/* hash defines */
/**
 * Magic word (4 bytes) to distinguish Java and C packets.
 */
#define UDP_PACKET_MAGIC_WORD                  (0xC0C0)

/* internal variables */
/**
 * Revision Control System identifier.
 */
static char rcsid[] = "$Id: log_udp.c,v 1.5 2011-09-26 15:53:58 cjm Exp $";

/* internal function declarations */
static int UDP_Raw_Send(int socket_id,void *message_buff,size_t message_buff_len);
static int UDP_Raw_Recv(int socket_id,char *message_buff,size_t message_buff_len);
static int64_t hton64bitl(int64_t n);
static int Get_Host_By_Name(const char *name,char **host_addr_zero);

/* ---------------------------------------------------------------
**  External functions 
** --------------------------------------------------------------- */
/**
 * Routine to open a UDP socket and connect the default endpoint to a specified host/port.
 * @param hostname The hostname the socket will talk to, either numeric or via /etc/hosts.
 * @param port_number The port number to send to in host (normal) byte order.
 * @param socket_id The address of an integer to store the created socket file descriptor.
 * @return The routine returns TRUE on success and FALSE on failure. If the routine failed,
 *     Log_Error_Number and Log_Error_String are set.
 * @see #Get_Host_By_Name
 * @see log_general.html#Log_Error_Number
 * @see log_general.html#Log_Error_String
 */
int Log_UDP_Open(char *hostname,int port_number,int *socket_id)
{
	char host_ip[256];
	char *host_address_ptr = NULL;
	int socket_errno,retval;
	unsigned short int network_port_number;
	in_addr_t saddr;
	struct sockaddr_in remote_addr;

	if(hostname == NULL)
	{
		Log_Error_Number = 6;
		sprintf(Log_Error_String,"Log_UDP_Send:Hostname was NULL.");
		return FALSE;
	}
#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Open(%s,%d):started.\n",hostname,port_number);
#endif
	if(socket_id == NULL)
	{
		Log_Error_Number = 1;
		sprintf(Log_Error_String,"Log_UDP_Open:socket_id was NULL.");
		return FALSE;
	}
	/* open datagram socket */
	(*socket_id) = socket(AF_INET,SOCK_DGRAM,0);
	if((*socket_id) < 0)
	{
		socket_errno = errno;
		Log_Error_Number = 2;
		sprintf(Log_Error_String,"Log_UDP_Open:Failed to create socket (%d:%s).",socket_errno,
			strerror(socket_errno));
		return FALSE;
	}
	/* convert port number to network short */
	network_port_number = htons((short)port_number);
	/* try to convert hostname to address in network byte order */
	/* try numeric address conversion first */
	saddr = inet_addr(hostname);
	if(saddr == INADDR_NONE)
	{
#if DEBUG > 5
		fprintf(stdout,"Log_UDP_Open:inet_addr didn't work:trying gethostbyname(%s).\n",hostname);
#endif
		/* try getting by hostname instead */
		if(!Get_Host_By_Name(hostname,&host_address_ptr))
		{
			shutdown((*socket_id),SHUT_RDWR);
			(*socket_id) = 0;
			return FALSE;
		}
		strcpy(host_ip,inet_ntoa(*(struct in_addr *)(host_address_ptr)));
		if(host_address_ptr != NULL)
			free(host_address_ptr);
		saddr = inet_addr(host_ip);
	}
	/* set up socket so sends go to remote Hostname/Port_Number */
	memset((char *) &remote_addr,0,sizeof(remote_addr));
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_addr.s_addr = saddr;
	remote_addr.sin_port = network_port_number;
	retval = connect((*socket_id),(struct sockaddr *)&remote_addr,sizeof(remote_addr));
	if(retval < 0)
	{
		socket_errno = errno;
		shutdown((*socket_id),SHUT_RDWR);
		/*close((*socket_id));*/
		(*socket_id) = 0;
       		Log_Error_Number = 5;
		sprintf(Log_Error_String,"Log_UDP_Open:Failed to connect (%d:%s).",socket_errno,strerror(socket_errno));
		return FALSE;
	}
#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Open(%s,%d):returned socket %d:finished.\n",hostname,port_number,(*socket_id));
#endif
	return TRUE;
}

/**
 * Send the log message as a UDP packet.
 * @param int socket_id The previously opened socket to send the message over.
 * @param log_record The log record.
 * @param log_context_count The number of context records in log_context_list.
 * @param log_context_list An allocated list of log contexts.
 * @return The routine returns TRUE on success and FALSE on failure.
 * @see #UDP_PACKET_MAGIC_WORD
 * @see #Log_Record_Struct
 * @see #Log_Context_Struct
 * @see #UDP_Raw_Send
 */
int Log_UDP_Send(int socket_id,struct Log_Record_Struct log_record,
		 int log_context_count,struct Log_Context_Struct *log_context_list)
{
	char *message_buffer = NULL;
	size_t message_buffer_length = 0;
	int message_buffer_position,i,network_int;
	int64_t network_java_long;

#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Send(%d):started.\n",log_context_count);
#endif
	if(log_context_count < 0)
	{
		Log_Error_Number = 7;
		sprintf(Log_Error_String,"Log_UDP_Send:Log context count should be positive/zero(%d).",
			log_context_count);
		return FALSE;
	}
	if((log_context_count > 0)&&(log_context_list == NULL))
	{
		Log_Error_Number = 8;
		sprintf(Log_Error_String,"Log_UDP_Send:Log context list was NULL when log context count was %d.",
			log_context_count);
		return FALSE;
	}
	/* determine length of buffer 
	** Size of log record + all log contexts + 4 bytes for log context count + 4 bytes for magic word */
	message_buffer_length = sizeof(struct Log_Record_Struct) + sizeof(int) + sizeof(int) + (log_context_count * 
								    sizeof(struct Log_Context_Struct));
	message_buffer = (char*)malloc(message_buffer_length*sizeof(char));
	if(message_buffer == NULL)
	{
		Log_Error_Number = 9;
		sprintf(Log_Error_String,"Log_UDP_Send:Failed to allocate message buffer(%d).",
			message_buffer_length);
		return FALSE;
	}
	/* copy structure into buffer
	** Can't just copy whole structure as this may be padded / word aligned. */
	/* integers should be in network byte order */
	message_buffer_position = 0;
	/* magic word - used to differentiate between C and Java packets */
	network_int = htonl(UDP_PACKET_MAGIC_WORD);
	memcpy(message_buffer+message_buffer_position,&network_int,sizeof(int));
	message_buffer_position += sizeof(int);
	/* Timestamp */
	network_java_long = hton64bitl(log_record.Timestamp);
	memcpy(message_buffer+message_buffer_position,&network_java_long,sizeof(int64_t));
	message_buffer_position += sizeof(int64_t);
	/* System */
	strcpy(message_buffer+message_buffer_position,log_record.System);
	message_buffer_position += strlen(log_record.System);
	message_buffer[message_buffer_position++] = '\0';
	/* Sub_System */
	strcpy(message_buffer+message_buffer_position,log_record.Sub_System);
	message_buffer_position += strlen(log_record.Sub_System);
	message_buffer[message_buffer_position++] = '\0';
	/* Source_File */
	strcpy(message_buffer+message_buffer_position,log_record.Source_File);
	message_buffer_position += strlen(log_record.Source_File);
	message_buffer[message_buffer_position++] = '\0';
	/* Source_Instance */
	strcpy(message_buffer+message_buffer_position,log_record.Source_Instance);
	message_buffer_position += strlen(log_record.Source_Instance);
	message_buffer[message_buffer_position++] = '\0';
	/* Function */
	strcpy(message_buffer+message_buffer_position,log_record.Function);
	message_buffer_position += strlen(log_record.Function);
	message_buffer[message_buffer_position++] = '\0';
	/* Severity */
	network_int = htonl(log_record.Severity);
	memcpy(message_buffer+message_buffer_position,&network_int,sizeof(int));
	message_buffer_position += sizeof(int);
	/* Verbosity */
	network_int = htonl(log_record.Verbosity);
	memcpy(message_buffer+message_buffer_position,&network_int,sizeof(int));
	message_buffer_position += sizeof(int);
	/* Category */
	strcpy(message_buffer+message_buffer_position,log_record.Category);
	message_buffer_position += strlen(log_record.Category);
	message_buffer[message_buffer_position++] = '\0';
	/* Message */
#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Send():message='%s'.\n",log_record.Message);
#endif
	strcpy(message_buffer+message_buffer_position,log_record.Message);
	message_buffer_position += strlen(log_record.Message);
	message_buffer[message_buffer_position++] = '\0';
	/* Context_Count */
	network_int = htonl(log_context_count);
	memcpy(message_buffer+message_buffer_position,&network_int,sizeof(int));
	message_buffer_position += sizeof(int);
	/* add context list */
	for(i = 0; i < log_context_count; i++)
	{
		/* Keyword */
		strcpy(message_buffer+message_buffer_position,log_context_list[i].Keyword);
		message_buffer_position += strlen(log_context_list[i].Keyword);
		message_buffer[message_buffer_position++] = '\0';
		/* Value */
		strcpy(message_buffer+message_buffer_position,log_context_list[i].Value);
		message_buffer_position += strlen(log_context_list[i].Value);
		message_buffer[message_buffer_position++] = '\0';
	}
#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Send():message length:allocated=%d,actual=%d.\n",
		message_buffer_length,message_buffer_position);
#endif
	if(message_buffer_position > message_buffer_length)
	{
		Log_Error_Number = 10;
		sprintf(Log_Error_String,"Log_UDP_Send:Message Buffer overun(position %d > length %d).",
			message_buffer_position,message_buffer_length);
		return FALSE;
	}
	/* send buffer */
	/* can use message_buffer_length as length (allocated), or message_buffer_position (actual end position).
	** message_buffer_position should be better, as message_buffer_length could include struct padding bytes */
	if(!UDP_Raw_Send(socket_id,message_buffer,message_buffer_position))
	{
		if(message_buffer != NULL)
			free(message_buffer);
		return FALSE;
	}
	/* free buffer */
	if(message_buffer != NULL)
		free(message_buffer);
#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Send():finished.\n");
#endif
	return TRUE;
}

/**
 * Close a previously opened UDP socket.
 * @param socket_id The socket descriptor.
 * @return The routine returns TRUE on success, and FALSE on failure. 
 *          If the routine failed, a message is printed to stderr.
 */
int Log_UDP_Close(int socket_id)
{
	int retval,socket_errno;

#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Close(%d):started.\n",socket_id);
#endif
	retval = shutdown(socket_id,SHUT_RDWR);
	if(retval < 0)
	{
		socket_errno = errno;
       		Log_Error_Number = 17;
		sprintf(Log_Error_String,"Log_UDP_Close:Close failed (%d,%d:%s).",retval,socket_errno,
			strerror(socket_errno));
		return FALSE;
	}
#if DEBUG > 1
	fprintf(stdout,"Log_UDP_Close(%d):finished.\n",socket_id);
#endif
	return TRUE;
}

/* ---------------------------------------------------------------
**  Internal functions 
** --------------------------------------------------------------- */

/**
 * Send the specified data over the specified socket.
 * @param socket_id A previously opened and connected socket to send the buffer over.
 * @param message_buf A pointer to an area of memory containing the message to send.
 * @param message_buff_len The size of the message to send, in bytes.
 * @return The routine returns TRUE on success, and FALSE on failure. 
 *         If the routine failed, a message is printed to stderr.
 */
static int UDP_Raw_Send(int socket_id,void *message_buff,size_t message_buff_len)
{
	int retval,send_errno;

#if DEBUG > 1
	fprintf(stdout,"UDP_Raw_Send(socket=%d,length=%d):started.\n",socket_id,message_buff_len);
#endif
	if(message_buff == NULL)
	{
       		Log_Error_Number = 11;
		sprintf(Log_Error_String,"UDP_Raw_Send:message_buff was NULL.");
		return FALSE;
	}
	retval = send(socket_id,message_buff,message_buff_len,0);
	if(retval < 0)
	{
		send_errno = errno;
       		Log_Error_Number = 12;
		sprintf(Log_Error_String,"UDP_Raw_Send:Send failed %d (%s).",send_errno,strerror(send_errno));
		return FALSE;
	}
	if(retval != message_buff_len)
	{
       		Log_Error_Number = 13;
		sprintf(Log_Error_String,"UDP_Raw_Send:Send returned %d vs %d.",retval,message_buff_len);
		return FALSE;
	}
#if DEBUG > 1
	fprintf(stdout,"UDP_Raw_Send(%d):finished.\n",socket_id);
#endif
	return TRUE;
}

/**
 * Get some data from the specified socket. This routine blocks until a message arrives, 
 * if the socket is <b>not</b> set to nonblocking.
 * @param socket_id A previously opened and connected socket to get the buffer from.
 * @param message_buf A pointer to an area of memory of size message_buff_len bytes, to put the received message.
 * @param message_buff_len The size of the message to receive, in bytes.
 * @return The routine returns TRUE on success, and FALSE on failure. 
 *         If the routine failed, a message is printed to stderr.
 */
static int UDP_Raw_Recv(int socket_id,char *message_buff,size_t message_buff_len)
{
	int retval,send_errno;

#if DEBUG > 1
	fprintf(stdout,"UDP_Raw_Recv(%d):started.\n",socket_id);
#endif
	if(message_buff == NULL)
	{
       		Log_Error_Number = 14;
		sprintf(Log_Error_String,"UDP_Raw_Recv:message_buff was NULL.");
		return FALSE;
	}
	retval = recv(socket_id,message_buff,message_buff_len,0);
	if(retval < 0)
	{
		send_errno = errno;
       		Log_Error_Number = 15;
		sprintf(Log_Error_String,"UDP_Raw_Recv:Recv failed %d (%s).",send_errno,strerror(send_errno));
		return FALSE;
	}
	if(retval == 0)
	{
       		Log_Error_Number = 16;
		sprintf(Log_Error_String,"UDP_Raw_Recv:Recv returned %d vs %d.",retval,message_buff_len);
		return FALSE;
	}
	/* terminate reply message */
	message_buff[retval] = '\0';
#if DEBUG > 1
	fprintf(stdout,"UDP_Raw_Recv(%d):finished and received %d bytes.\n",socket_id,retval);
#endif
	return TRUE;
}

/**
 * If the endianness of the machine is different from network byte order, swap over the 64-bit int into
 * network byte order.
 * @param n A 64 bit int.
 * @return The 64 bit int in network byte order.
 */
static int64_t hton64bitl(int64_t n)
{
	/* we could use: int64_t __builtin_bswap64 (int64_t x) for GCC/Linux */
	/* See /usr/include/netinet/in.h for the example I copied from */
# if __BYTE_ORDER == __BIG_ENDIAN
	return n;
# else
#  if __BYTE_ORDER == __LITTLE_ENDIAN
	return __bswap_64(n);
#  endif
# endif
}

/**
 * Internal routine to get a host address from it's name. This is traditionally handled by a call
 * to gethostbyname. Unfortunately that routine is not re-entrant because the pointer it returns
 * is to a block of reusable memory in glibc, so a second call to gethostbyname from another thread
 * in the process can lead to the pointers returned from the first call being freed leading to SIGSEGV.
 * This routine wraps gethostbyname_r, the re-entrant version of that routine.
 * @param name The hostname to translate. This should be allocated, zero-terminated and non-null.
 * @param host_addr_zero The address of a pointer  to an array of chars. This routine will allocate
 *       some memory and fill it with a null-terminated, network byte ordered copy of the first hostent host address
 *       list entry returned by gethostbyname_r. NULL can be returned on failure.
 * @return The routine returns TRUE on success and FALSE on failure.
 */
static int Get_Host_By_Name(const char *name,char **host_addr_zero)
{
	struct hostent hostbuf,*hp = NULL;
	size_t hstbuflen;
	char *tmphstbuf = NULL;
	int retval;
	int herr;

	Log_Error_Number = 0;
	if(name == NULL)
	{
		Log_Error_Number = 3;
		sprintf(Log_Error_String,"Get_Host_By_Name:name was NULL.");
		return FALSE;
	}
	if(host_addr_zero == NULL)
	{
		Log_Error_Number = 4;
		sprintf(Log_Error_String,"Get_Host_By_Name:host_addr_zero was NULL.");
		return FALSE;
	}
#if DEBUG > 5
	fprintf(stdout,"Get_Host_By_Name(%s) Started.",name);
#endif /* DEBUG */
	hstbuflen = 1024;
	/* Allocate buffer, remember to free it to avoid memory leakage.  */
	tmphstbuf = malloc(hstbuflen);
	if(tmphstbuf == NULL)
	{
		Log_Error_Number = 18;
		sprintf(Log_Error_String,"Get_Host_By_Name:memory allocation of tmphstbuf failed(%d).",hstbuflen);
		return FALSE;

	}
	while((retval = gethostbyname_r(name,&hostbuf,tmphstbuf,hstbuflen,&hp,&herr)) == ERANGE)
	{
		/* Enlarge the buffer.  */
		hstbuflen *= 2;
		tmphstbuf = realloc(tmphstbuf, hstbuflen);
		/* check realloc succeeds */
		if(tmphstbuf == NULL)
		{
			Log_Error_Number = 19;
			sprintf(Log_Error_String,"Get_Host_By_Name:memory reallocation of tmphstbuf failed(%d).",
				hstbuflen);
			return FALSE;
		}
#if DEBUG > 5
	fprintf(stdout,"Get_Host_By_Name:gethostbyname_r returned ERANGE:Increasing buffer size to %d.",hstbuflen);
#endif /* DEBUG */
	}/* while */
#if DEBUG > 5
	fprintf(stdout,"Get_Host_By_Name:gethostbyname_r loop exited with retval %d and hp %p.",retval,hp);
#endif /* DEBUG */
	if(retval != 0)
	{
		if(tmphstbuf != NULL)
			free(tmphstbuf);
		Log_Error_Number = 20;
		sprintf(Log_Error_String,"Get_Host_By_Name:gethostbyname_r failed to find host %s (%d).",name,herr);
		return FALSE;
	}
	if(hp == NULL)
	{
		if(tmphstbuf != NULL)
			free(tmphstbuf);
		Log_Error_Number = 21;
		sprintf(Log_Error_String,
			"Get_Host_By_Name:gethostbyname_r returned NULL return pointer for hostname %s (%d).",
			name,herr);
		return FALSE;
	}
	/* copy result */
	(*host_addr_zero) = strdup(hp->h_addr_list[0]);
	if((*host_addr_zero) == NULL)
	{
		if(tmphstbuf != NULL)
			free(tmphstbuf);
		Log_Error_Number = 22;
		sprintf(Log_Error_String,"Get_Host_By_Name:"
			"Failed to copy gethostbyname_r result string (%s).",hp->h_addr_list[0]);
		return FALSE;
	}
	/* free buffer*/
	if(tmphstbuf != NULL)
		free(tmphstbuf);
#if DEBUG > 5
	fprintf(stdout,"Get_Host_By_Name(%s) Finished.",name);
#endif /* DEBUG */
	return TRUE;
}

/*
** $Log: not supported by cvs2svn $
** Revision 1.4  2009/02/13 17:29:41  cjm
** Added __linux inclusion around Linux specific headers.
** Not figured out Solaris equivalents yet.
**
** Revision 1.3  2009/01/14 14:51:31  cjm
** Added magic word to UDP buffer to differentiate between C and Java packets.
**
** Revision 1.2  2009/01/09 18:03:39  cjm
** Fixed buffer packing - use strcpy rather than strcat.
**
** Revision 1.1  2009/01/09 14:54:37  cjm
** Initial revision
**
*/
