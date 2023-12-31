/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  Macros/functions to support logging and tracing. We must be able to:

  1. compile it all out
  
  2. have it compiled in, but disable it at the cost of only one test
  per trace line
  
  3. Enable it on a per file basis

  4. Enable it/disable at different levels - guidelines are:
  
  0 for infrequent but significant trace (given that the overall trace
  and file trace is on) - e.g. object creation/deletion

  1 for infrequent but interesting trace
  
  2 for infrequent trace that is very verbose - e.g. dumping out lots of data

  3 for significant frequent trace - e.g. key functions that get
  called once per frame.

  4 for frequent trace that is more verbose.

  Enabling levels 0-3 shouldn't have a big impact on
  performance. Higher levels will have a significant hit on the
  application.

  The tracing macro looks like this:

  TRACE_IF(int level, char * trace_string)

  or

  TRACE_FILE_IF(int level)

  which calls the first macro with __FILE__ as the trace_string
  argument.

  This resolve into a simple (or complex) "if", which is followed by a
  print-style macro to do the actual tracing. Multiple lines of
  tracing can be done with a single test by using braces - e.g.

  TRACE_FILE_IF(3) TRACE("value = %d", m_value);

  or

  TRACE_IF(3, "file.cpp") 
  {
    int a = calc_value(m_value);
    TRACE("val1 = %d, val2 = %d", m_value, a);
  }
  
  TRACE may at some point write the output to file etc. 
*/
#ifndef LOG_TRACE_H
#define LOG_TRACE_H

#ifdef WIN32
#pragma warning (disable : 4786 4800 4161)    //to avoid stupid warnings from mfc 'cause stl
#endif

#include <vector>
#include <set>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <stdarg.h>

/////////////////////////////////////////////////////////////////
// These are the interface
/////////////////////////////////////////////////////////////////

/// enable/disable overall trace
inline void enable_trace(bool enable);

/// set the overall trace level
inline void set_trace_level(int level);

/// trace all strings?
inline void enable_trace_all_strings(bool enable);

/// add a string to the list of traced strings
inline void add_trace_strings(const std::vector<std::string> & trace_strings); 
inline void add_trace_string(const std::string & trace_string); 
inline void add_trace_string(const char * trace_string);

#define TRACE_IF(level, trace_string) \
if ( (trace_enabled) && \
     ((level) <= trace_level) && \
     ( (trace_all_strings == true) || \
       (check_trace_string(trace_string)) ) )

#define TRACE_FILE_IF(level) \
TRACE_IF(level, __FILE__)

/// for now just printf (in the future may make a copy to file)
// go away MS
#ifdef TRACE
#undef TRACE
#endif
#define TRACE trace_printf

#ifdef USE_FUNCTION
#define TRACE_FUNCTION() \
TRACE("%s():%d: ", __FUNCTION__, __LINE__)
#else
#define TRACE_FUNCTION() \
TRACE("%s:%d: ", __FILE__, __LINE__)
#endif

#ifdef USE_FUNCTION
#define TRACE_METHOD() \
TRACE("%s:%s():%d [%p]: ", __FILE__, __FUNCTION__, __LINE__, this)
#else
#define TRACE_METHOD() \
TRACE("%s:%d: [%p]: ", __FILE__, __LINE__, this)
#endif

#ifdef USE_FUNCTION
#define TRACE_METHOD_STATIC() \
TRACE("%s:%s():%d [static]: ", __FILE__, __FUNCTION__, __LINE__)
#else
#define TRACE_METHOD_STATIC() \
TRACE("%s:%d: [static]: ", __FILE__, __LINE__)
#endif

// just display the function
#define TRACE_FUNCTION_ONLY(level) \
TRACE_FILE_IF(level) {TRACE_FUNCTION(); TRACE("\n");}

#define TRACE_METHOD_ONLY(level) \
TRACE_FILE_IF(level) {TRACE_METHOD(); TRACE("\n");}

#define TRACE_METHOD_STATIC_ONLY(level) \
TRACE_FILE_IF(level) {TRACE_METHOD_STATIC(); TRACE("\n");}

/////////////////////////////////////////////////////////////////
// don't look below here
/////////////////////////////////////////////////////////////////

/// Overall trace enabled
extern bool trace_enabled;

/// The overall trace level - only trace with a level equal to or less
/// than this comes out.
extern int trace_level;

/// The strings for which trace is enabled. Normally these will be
/// file names, though they don't have to be. 
extern std::set<std::string> trace_strings;

/// If this flag is set, all trace strings are enabled
extern bool trace_all_strings;

/// do the output
void trace_printf(const char *format, ...);

/// enable/disable overall trace
inline void enable_trace(bool enable) {trace_enabled = enable;}

inline void enable_trace_all_strings(bool enable) {
  trace_all_strings = enable;}

inline void set_trace_level(int level) {trace_level = level;}

inline void add_trace_string(const std::string & trace_string)
{
  trace_strings.insert(trace_string); 
}

inline void add_trace_string(const char * trace_string)
{
  add_trace_string(std::string(trace_string));
}

inline void add_trace_strings(const std::vector<std::string> & new_trace_strings)
{
  for (unsigned i = 0 ; i < new_trace_strings.size() ; ++i)
    trace_strings.insert(new_trace_strings[i]); 
}

inline bool check_trace_string(const char * trace_string)
{
	std::set<std::string>::const_iterator it = trace_strings.find(std::string(trace_string));
	return (it != trace_strings.end());
}

#endif


