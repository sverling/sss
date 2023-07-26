/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#ifndef SSS_RANDOM_H
#define SSS_RANDOM_H
#include <limits.h>

class Sss_random
{
public:
  Sss_random(long seed) : 
    m_seed(seed), 
    IA(16807),
    IM(2147483647),
    IQ(127773),
    IR(2836),
    MASK(123459876),
    AM(1.0/IM)
 {}

  void set_seed(long seed) {m_seed = seed;}

  float rand() 
    {
      long k;
      float ans;
      
      m_seed ^= MASK;
      k = m_seed/IQ;
      m_seed = IA * (m_seed - k * IQ) - IR * k;
      if (m_seed < 0) m_seed += IM;
      ans = AM * (m_seed);
      m_seed ^= MASK;
      return ans;
    }

  float rand(double v1, double v2) 
    {
      return v1 + (v2-v1) * ( (float)rand() );
    }
  
private:
  long m_seed;
  const long IA, IM, IQ, IR, MASK;
  const double AM;
};



#endif
