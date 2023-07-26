/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file remote_sss_queue.cpp
*/

#include "remote_sss_queue.h"
#include "log_trace.h"

#include <string.h>
#include <errno.h>

using namespace std;

Remote_sss_queue::Remote_sss_queue()
{
  TRACE_METHOD_ONLY(1);
  critical_section.Init();
}

Remote_sss_queue::~Remote_sss_queue()
{
  TRACE_METHOD_ONLY(1);
  critical_section.Term();
}

int Remote_sss_queue::push(void * val)
{
  critical_section.Lock();
//  cout << "this = " << this << endl;

  m_queue.push(val);

  critical_section.Unlock();
  return 0;
}

vector<void *> Remote_sss_queue::pop()
{
  vector<void *> result;
  critical_section.Lock();

  while (!m_queue.empty())
  {
    result.push_back(m_queue.front());
    m_queue.pop();
  }
  
  critical_section.Unlock();
  return result;
}

size_t Remote_sss_queue::size() const
{
  size_t rv = 0;
  
  critical_section.Lock();

  rv = m_queue.size();

  critical_section.Unlock();

  return rv;
}

