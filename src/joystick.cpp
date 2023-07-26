/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file joystick.cpp
*/

#include "joystick.h"
#include "misc.h"
#include "log_trace.h"

#include <iomanip>

Joystick::Joystick(unsigned int num_channels)
  :
  m_values(vector<float>(num_channels, -1)),
  m_trims(vector<float>(num_channels, 0))
{
  TRACE_METHOD_ONLY(1);
}

inline float Joystick::clip(float val)
{
  return (val > 1.0f ? 1.0f : (val < -1.0f ? -1.0f : val));
}

void Joystick::set_value(unsigned int channel, float value)
{
  if (channel >= m_values.size())
  {
    TRACE("Channel %d too large - max is %d\n",
          channel,
          m_values.size());
    return;
  }
  value = clip(value);
  m_values[channel] = value;
}

void Joystick::set_trim(unsigned int channel, float trim)
{
  if (channel >= m_trims.size())
  {
    TRACE("Channel %d too large - max is %d\n",
          channel,
          m_values.size());
    return;
  }
  trim = clip(trim);
  m_trims[channel] = trim;
}

void Joystick::incr_value(unsigned int channel, float increment)
{
  if (channel >= m_values.size())
  {
    TRACE("Channel %d too large - max is %d\n",
          channel,
          m_values.size());
    return;
  }
  m_values[channel] = clip(m_values[channel] + increment);
}

void Joystick::incr_trim(unsigned int channel, float increment)
{
  if (channel >= m_trims.size())
  {
    TRACE("Channel %d too large - max is %d\n",
          channel,
          m_values.size());
    return;
  }
  m_trims[channel] = clip(m_trims[channel] + increment);
}

float Joystick::get_value(unsigned int channel) const
{
  if (channel >= m_values.size())
  {
    TRACE("Channel %d too large - max is %d\n",
          channel,
          m_values.size());
    return 0;
  }
  return clip(m_values[channel] + m_trims[channel]);
}

void Joystick::show() const
{
  TRACE("Joystick: channel, value + trim = actual_value\n");
  
  for (unsigned int i = 0 ; i < m_values.size() ; ++i)
  {
    TRACE("%5d %5d + %5d = %5d\n", i, 
          m_values[i], 
          m_trims[i],
          clip(m_values[i] + m_trims[i]) );;
  }
}

void Joystick::set_vals(const unsigned char joystick[9], 
                        const unsigned char trims[9])
{
  size_t num = sss_min(m_values.size(), (size_t) 9);
  for (size_t i = 0 ; i < num ; ++i)
  {
    m_values[i] = (((float) joystick[i])-127)/127.0f;
    m_trims[i] =  (((float) trims[i]   )-127)/127.0f;
  }
}

void Joystick::get_vals(unsigned char joystick[9], 
                        unsigned char trims[9]) const
{
  size_t num = sss_min(m_values.size(), (size_t) 9);
  for (size_t i = 0 ; i < num ; ++i)
  {
    joystick[i] = (unsigned char) (127.0f + (127.0f * m_values[i]));
    trims[i] =    (unsigned char) (127.0f + (127.0f * m_trims[i] ));
  }
}

void Joystick::zero_all_channels()
{
  unsigned i;
  for (i = 0 ; i < m_values.size() ; ++i)
    set_value(i, 0.0f);
}

