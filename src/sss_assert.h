/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef SSS_ASSERT_H
#define SSS_ASSERT_H

#ifdef NDEBUG
#define fassert1(exp)
#define assert2(exp, msg)

#else

#include "log_trace.h"

#define DO_SSS_DEATH \
printf(\
"SSS has crashed. If you don't think this was your fault, please \n" \
"try to reproduce the problem by running with sss_debug.cfg and \n" \
"send an email containing the contents of this window, the file sss.log \n" \
"plus any other info (like what you were doing when this happened) to\n" \
"flight@rowlhouse.freeserve.co.uk. Thank you, and sorry!\n" \
"Press 'c <RET>' to continue (bad idea!) or 'q <RET>' to exit SSS.\n"); \
for (;;) {int c = getc(stdin); if (c == 'c') break; else if (c == 'q') abort();}
//if (getc(stdin) != 'c') abort();

#include <stdio.h>
#include <stdlib.h>
#define assert1(exp) \
if (!(exp)) { \
TRACE("Assert %s at %s:%d\n", #exp, __FILE__, __LINE__); \
DO_SSS_DEATH \
}

#define assert2(exp, msg) \
if (!(exp)) { \
TRACE("Assert %s at %s:%d\n%s\n", #exp, __FILE__, __LINE__, msg); \
DO_SSS_DEATH \
}

#endif

#endif // file included
