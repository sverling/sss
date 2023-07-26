/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef JOYSTICK_H
#define JOYSTICK_H

#include <vector>
using namespace std;

//! Stores the control parameters in terms of values and channels. 
class Joystick
{
public:
  Joystick(unsigned int num_channels);

  //! The value is equivalent to the stick position - clipped to the range {-1,+1}
  void set_value(unsigned int channel, float value);
  void incr_value(unsigned int channel, float increment);
  
  //! The trim (added to the channel value) - clipped to {-1,+1}
  void set_trim(unsigned int channel, float trim);
  void incr_trim(unsigned int channel, float increment);
  
  //! Returns value + trim for the channel
  float get_value(unsigned int channel) const;
  
  unsigned int get_num_channels() const {return m_values.size();}

  void zero_all_channels();

  void show() const;

  // allow getting/setting - used in transmitting over remote cxn.
  void set_vals(const unsigned char joystick[9], const unsigned char trims[9]);
  void get_vals(unsigned char joystick[9],  unsigned char trims[9]) const;

private:
  inline static float clip(float val); // returns the value clipped to [-1,1]

  vector<float> m_values;
  vector<float> m_trims;
};

#endif
