/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#ifndef REMOTE_SSS_QUEUE_H
#define REMOTE_SSS_QUEUE_H

#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
#include <pthread.h>
#else
#include <atlbase.h>
#endif

#include <queue>
#include <vector>

#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
class CComCriticalSection
{
public:
  void Init(){ pthread_mutex_init(&mutex, 0); }
  void Term(){ pthread_mutex_destroy(&mutex); }
  void Lock(){ pthread_mutex_lock(&mutex); }
  void Unlock(){ pthread_mutex_unlock(&mutex); }
private:
  pthread_mutex_t mutex;
};
#endif

//! Allows thread-safe writing to/reading from a FIFO queue
class Remote_sss_queue
{
public:
  Remote_sss_queue();
  ~Remote_sss_queue();
  
  //! Places an element on the back of the queue
  int push(void * val);
  //! Removes and returns all elements from the queue.
  std::vector<void *> pop();
  
  size_t size() const;

private:
  std::queue<void *> m_queue;
  
  mutable CComCriticalSection critical_section;
};


#endif
