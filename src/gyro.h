/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GYRO_H
#define GYRO_H

#include "matrix_vector3.h"

class Config_file;
class Object;
class Joystick;

class Gyro
{
public:
  Gyro(Config_file & gyro_config, const Object & parent);

  float get_output(const Joystick & joystick) const;

  const std::string get_name() const {return m_name;}

private:
  const Object & m_parent;

  Vector3 m_axis;
  std::string m_name;
  float m_output_per_rot_rate;

  enum {MAX_CHANNEL = 8};
  float m_zero_per_control[MAX_CHANNEL];
};

#endif
