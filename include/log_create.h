/* log_create.h
** $Header: /home/cjm/cvs/log_udp/include/log_create.h,v 1.2 2009-01-19 15:38:23 cjm Exp $
*/
#ifndef LOG_CREATE_H
#define LOG_CREATE_H
#include <time.h>
#include "log_udp.h"

extern int Log_Create_Record(char *system,char *sub_system,char *source_file,char *source_instance,char *function,
			     int severity,int verbosity,char *category,char *message,
			     struct Log_Record_Struct *log_record);
extern int Log_Create_Context_List_Add(struct Log_Context_Struct **log_context_list,int *log_context_count,
				       char *keyword,char *value);
extern int Log_Create_Record_Timestamp_Set(struct tm time_tm,struct Log_Record_Struct *log_record);
#endif
/*
** $Log: not supported by cvs2svn $
** Revision 1.1  2009/01/09 14:54:41  cjm
** Initial revision
**
*/
