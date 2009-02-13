/* log_udp.h
** $Header: /home/cjm/cvs/log_udp/include/log_udp.h,v 1.2 2009-02-13 17:27:04 cjm Exp $
*/
#ifndef LOG_UDP_H
#define LOG_UDP_H

/* stdint.h defines int64_t (Java long) but only exists under Linux */
#ifdef __linux
#include <stdint.h>
#endif
/* solaris definition of int64_t (Java long) ( typedef long long int64_t) */
#ifdef __sun
# if __WORDSIZE == 64
typedef long int                int64_t;
# else
typedef long long int           int64_t;
# endif
#endif

/* hash defines */
/**
 * The length of the Log_Context_Struct Keyword string field in characters/bytes.
 * @see #Log_Context_Struct
 */
#define LOG_CONTEXT_KEYWORD_LENGTH           (32)
/**
 * The length of the Log_Context_Struct Value string field in characters/bytes.
 * @see #Log_Context_Struct
 */
#define LOG_CONTEXT_VALUE_LENGTH             (256)
/**
 * The length of the Log_Record_Struct System string field in characters/bytes.
 * @see #Log_Record_Struct
 */
#define LOG_RECORD_SYSTEM_LENGTH             (12)
/**
 * The length of the Log_Record_Struct Sub-System string field in characters/bytes.
 * @see #Log_Record_Struct
 */
#define LOG_RECORD_SUB_SYSTEM_LENGTH         (12)
/**
 * The length of the Log_Record_Struct Source file string field in characters/bytes.
 * @see #Log_Record_Struct
 */
#define LOG_RECORD_SOURCE_FILE_LENGTH        (128)
/**
 * The length of the Log_Record_Struct Source instance string field in characters/bytes.
 * @see #Log_Record_Struct
 */
#define LOG_RECORD_SOURCE_INSTANCE_LENGTH    (128)
/**
 * The length of the Log_Record_Struct Function string field in characters/bytes.
 * @see #Log_Record_Struct
 */
#define LOG_RECORD_FUNCTION_LENGTH           (64)
/**
 * The length of the Log_Record_Struct Category string field in characters/bytes.
 * @see #Log_Record_Struct
 */
#define LOG_RECORD_CATEGORY_LENGTH           (32)
/**
 * The length of the Log_Record_Struct Message string field in characters/bytes.
 * @see #Log_Record_Struct
 */
#define LOG_RECORD_MESSAGE_LENGTH            (1024)

/* enums */
/**
 * This enum describes a verbosity filtering level of the message. The idea is that the high priority/
 * terse level messages are always displayed, whilst the detail/very verbose messages can be filtered out.
 * <dl>
 * <dt>LOG_VERBOSITY_VERY_TERSE</dt> <dd>High priority/top level message.</dd>
 * <dt>LOG_VERBOSITY_TERSE</dt> <dd> Higher priority message.</dd>
 * <dt>LOG_VERBOSITY_INTERMEDIATE</dt> <dd>Intermediate level message.</dd>
 * <dt>LOG_VERBOSITY_VERBOSE</dt> <dd>Lower priority/more detailed/verbose message.</dd>
 * <dt>LOG_VERBOSITY_VERY_VERBOSE</dt> <dd>Lowest level/most verbose message.</dd>
 * </dl>
 */
enum LOG_VERBOSITY
{
	LOG_VERBOSITY_VERY_TERSE=1,
	LOG_VERBOSITY_TERSE=2,
	LOG_VERBOSITY_INTERMEDIATE=3,
	LOG_VERBOSITY_VERBOSE=4,
	LOG_VERBOSITY_VERY_VERBOSE=5
};

/**
 * Macro to check whether the severity is a legal value.
 * @see #LOG_SEVERITY
 */
#define LOG_UDP_IS_VERBOSITY(verbosity) ((verbosity == LOG_VERBOSITY_VERY_TERSE)|| \
                                         (verbosity == LOG_VERBOSITY_TERSE) || \
                                         (verbosity == LOG_VERBOSITY_INTERMEDIATE) || \
                                         (verbosity == LOG_VERBOSITY_VERBOSE) || \
                                         (verbosity == LOG_VERBOSITY_VERY_VERBOSE))

/**
 * This enum describes whether the log message is an informational (log) message or an error message.
 * <dl>
 * <dt>LOG_SEVERITY_INFO</dt> <dd>Informational message.</dd>
 * <dt>LOG_SEVERITY_ERROR</dt> <dd>Error message.</dd>
 * </dl>
 */
enum LOG_SEVERITY
{
	LOG_SEVERITY_INFO=1,
	LOG_SEVERITY_ERROR=3
};

/**
 * Macro to check whether the severity is a legal value.
 * @see #LOG_SEVERITY
 */
#define LOG_UDP_IS_SEVERITY(severity) ((severity == LOG_SEVERITY_INFO)||(severity == LOG_SEVERITY_ERROR))

/* structures */
/**
 * Structure used to define a context for a log message, a list of these keyword-value pairs
 * are attached to each log message.
 * <dl>
 * <dt>Keyword</dt> <dd>The context keyword, a string of length LOG_CONTEXT_KEYWORD_LENGTH.</dd>
 * <dt>Value</dt> <dd>The value of the keyword, a string of length LOG_CONTEXT_VALUE_LENGTH.</dd>
 * </dl>
 * @see #LOG_CONTEXT_KEYWORD_LENGTH
 * @see #LOG_CONTEXT_VALUE_LENGTH
 */
struct Log_Context_Struct
{
	char Keyword[LOG_CONTEXT_KEYWORD_LENGTH];
	char Value[LOG_CONTEXT_VALUE_LENGTH];
};

/**
 * The structure of a log record. Used to fill out the buffer sent as a UDP packet.
 * <dl>
 * <dt>Timestamp</dt>  <dd>The time the log message was recorded, 
 *     a int64_t (Java long) filled with milliseconds since 1970.</dd>
 * <dt>System</dt>     <dd>Which system is logging the message, a string of length LOG_RECORD_SYSTEM_LENGTH.</dd>
 * <dt>Sub_System</dt> <dd>Which sub-system is logging the message, 
 *     a string of length LOG_RECORD_SUB_SYSTEM_LENGTH.</dd>
 * <dt>Source_File</dt> <dd>The C source filename/script name the log was generated from, 
 *     a string of length LOG_RECORD_SOURCE_FILE_LENGTH. 
 *     This is actually represented in the database as srcCompClass and is effectively anologous to a 
 *     Java class name.</dd>
 * <dt>Source_Instance</dt> <dd>Which instance of something the log was generated from, 
 *     a string of length LOG_RECORD_SOURCE_INSTANCE_LENGTH. This is actually represented in the database 
 *     as srcCompId and is effectively anologous to a Java object instance. 
 *     Difficult to fill in meaningfully in the C layer, you could use the process Id or a pointer to the 
 *     data that originated the message, or just leave it blank.</dd>
 * <dt>Function</dt> <dd>Which function/procedure the log was generated from, 
 *     a string of length LOG_RECORD_FUNCTION_LENGTH. This is actually represented in the database as 
 *     srcBlock and is effectively anologous to a Java method name.</dd>
 * <dt>Severity</dt> <dd>Whether the log message is an informational or error message. 
 *     Fill in with values from LOG_SEVERITY.</dd>
 * <dt>Verbosity</dt> <dd>The level of logging of the message. 
 *     Is it a top level message, or a detail/low level information message? 
 *     Fill in with values from LOG_VERBOSITY.</dd>
 * <dt>Category</dt> <dd>What sort of information is the message. A string of length LOG_RECORD_CATEGORY_LENGTH. 
 *     Designed to be used as a filter.</dd>
 * <dt>Message</dt> <dd>The actual message. A string of length LOG_RECORD_MESSAGE_LENGTH.</dd>
 * </dl>
 * @see #LOG_RECORD_SYSTEM_LENGTH
 * @see #LOG_RECORD_SUB_SYSTEM_LENGTH
 * @see #LOG_RECORD_SOURCE_FILE_LENGTH
 * @see #LOG_RECORD_SOURCE_INSTANCE_LENGTH
 * @see #LOG_RECORD_FUNCTION_LENGTH
 * @see #LOG_RECORD_CATEGORY_LENGTH
 * @see #LOG_RECORD_MESSAGE_LENGTH
 * @see #LOG_SEVERITY
 * @see #LOG_VERBOSITY
 */
struct Log_Record_Struct
{
	int64_t Timestamp;
	char System[LOG_RECORD_SYSTEM_LENGTH];
	char Sub_System[LOG_RECORD_SUB_SYSTEM_LENGTH];
	char Source_File[LOG_RECORD_SOURCE_FILE_LENGTH];
	char Source_Instance[LOG_RECORD_SOURCE_INSTANCE_LENGTH];
	char Function[LOG_RECORD_FUNCTION_LENGTH];
	int Severity;
	int Verbosity;
	char Category[LOG_RECORD_CATEGORY_LENGTH];
	char Message[LOG_RECORD_MESSAGE_LENGTH];
};

extern int Log_UDP_Open(char *hostname,int port_number,int *socket_id);
extern int Log_UDP_Send(int socket_id,struct Log_Record_Struct log_record,
		 int log_context_count,struct Log_Context_Struct *log_context_list);
extern int Log_UDP_Close(int socket_id);

#endif
/*
** $Log: not supported by cvs2svn $
** Revision 1.1  2009/01/09 14:54:41  cjm
** Initial revision
**
*/
