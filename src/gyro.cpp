/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#include "gyro.h"
#include "config_file.h"
#include "object.h"
#include "joystick.h"

using namespace std;

Gyro::Gyro(Config_file & gyro_config, const Object & parent)
  :
  m_parent(parent)
{
  TRACE_METHOD_ONLY(1);

  unsigned i;
  
  for (i = 0 ; i < MAX_CHANNEL ; ++i)
  {
    m_zero_per_control[i] = 0.0f;
  }

  gyro_config.get_next_value_assert("name", m_name);
  gyro_config.get_next_value_assert("axis_x", m_axis[0]);
  gyro_config.get_next_value_assert("axis_y", m_axis[1]);
  gyro_config.get_next_value_assert("axis_z", m_axis[2]);
  gyro_config.get_next_value_assert("output_per_rot_rate", m_output_per_rot_rate);
  gyro_config.get_next_value("zero_per_chan_1", m_zero_per_control[0]);
  gyro_config.get_next_value("zero_per_chan_2", m_zero_per_control[1]);
  gyro_config.get_next_value("zero_per_chan_3", m_zero_per_control[2]);
  gyro_config.get_next_value("zero_per_chan_4", m_zero_per_control[3]);
  gyro_config.get_next_value("zero_per_chan_5", m_zero_per_control[4]);
  gyro_config.get_next_value("zero_per_chan_6", m_zero_per_control[5]);
  gyro_config.get_next_value("zero_per_chan_7", m_zero_per_control[6]);
  gyro_config.get_next_value("zero_per_chan_8", m_zero_per_control[7]);

  m_axis.normalise();
}

float Gyro::get_output(const Joystick & joystick) const
{
  float zero = 0.0f;
  for (unsigned i = 0 ; i < MAX_CHANNEL ; ++i)
  {
    zero += m_zero_per_control[i] * joystick.get_value(i+1);
  }

  Vector3 world_axis = m_parent.get_orient() * m_axis;

  float rot_rate = (dot(m_parent.get_rot(), world_axis) / TWO_PI) - zero;

  return rot_rate * m_output_per_rot_rate;
}

