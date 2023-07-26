/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file log_trace.cpp
*/

#include "log_trace.h"
#include <stdio.h>

/// Overall trace enabled
bool trace_enabled = false;

/// The overall trace level - only trace with a level equal to or less
/// than this comes out.
int trace_level = 0;

/// The strings for which trace is enabled. Normally these will be
/// file names, though they don't have to be. 
std::set<std::string> trace_strings;

/// If this flag is set, all trace strings are enabled
bool trace_all_strings = false;

void trace_printf(const char *fmt, ...)
{
  va_list ap;

  // prepare log file
  static bool init = false;
  static FILE * log_file = 0;
  
  if (init == false)
  {
    init = true;
    log_file = fopen("sss.log", "w");
    
    if (log_file == NULL)
    {
      fprintf(stderr, "Unable to open sss.log\n");
      // We have a backup plan!! Assume that non-win32 is unix based.
#ifdef WIN32
      char log_file_name[] = "C:\\sss.log";
#else
      char log_file_name[] = "/tmp/sss.log";
#endif
      log_file = fopen(log_file_name, "w");
      if (log_file == NULL)
      {
        fprintf(stderr, "Unable to open backup %s\n", log_file_name);
      }
      else
      {
        printf("Opened log file: %s\n", log_file_name);
      }
    }
    else
    {
      printf("Opened log file: sss.log\n");
    }
  }

  // first to stdout
  va_start(ap, fmt);
  vprintf(fmt,ap);
  va_end(ap);

  // now to file
  if (log_file)
  {
    va_start(ap, fmt);
    vfprintf(log_file, fmt, ap);
    // flush it line-by-line so we don't miss any
    fflush(log_file);
    va_end(ap);
  }
  
}
