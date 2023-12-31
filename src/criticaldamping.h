//==============================================================
// Copyright (C) 2004 Danny Chapman 
//               danny@rowlhouse.freeserve.co.uk
//--------------------------------------------------------------
//               
/// @file criticaldamping.hpp 
//                     
//==============================================================
#ifndef CRITICALDAMPING_HPP
#define CRITICALDAMPING_HPP
//
// SmoothCD for ease-in / ease-out smoothing 
// Based on Game Programming Gems 4 Chapter 1.10
//

template <typename T>
inline void SmoothCD(T &val,          // in/out: value to be smoothed
                      T &valRate,      // in/out: rate of change of the value
                      const float timeDelta, // in: time interval
                      const T &to,     // in: the target value
                      const float smoothTime)// in: timescale for smoothing
{
  if (smoothTime > 0.0f)
  {
    float omega = 2.0f / smoothTime;
    float x = omega * timeDelta;
    float exp = 1.0f / (1.0f + x + 0.48f * x * x + 0.235f * x * x * x);
    T change = val - to;
    T temp = (valRate + omega * change) * timeDelta;
    valRate = (valRate - omega * temp) * exp;
    val = to + (change + temp) * exp;
  }
  else if (timeDelta > 0.0f)
  {
    valRate = (to - val) / timeDelta;
    val = to;
  }
  else
  {
    val = to;
    valRate -= valRate; // zero it...
  }
}

#endif
