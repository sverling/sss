/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef SKI_H
#define SKI_H

#include "types.h"

#include <vector>
#include <string>

class Config_file;
class Joystick;

/// This class represents a "ski" with graphical, aerodynamic and
/// structural properties.
///
/// It assumes that it is flat in the x-y plane, with forward in the
/// +ve x direction (except that it allows for rotations around the z
/// axis).
///
/// It differs from an aerofoil in that the structural properties can
/// get modified - in particular the direction of minimal friction.
///
/// Internally it gets represented by 3 points in a line (that can
/// rotate about the z axis).
class Ski
{
public:
  Ski(Config_file & ski_config);

  /// Returns the list of hard points
  const std::vector<Hard_point> & get_hard_points() const 
    {return m_hard_points;}

  /// Updates the hard points
  void update(const Joystick & joystick);

private:

  void calculate_hard_points();

  /// General position
  Position m_position;

  float m_total_length;
  /// friction in the dir of the ski
  float m_mu_min;
  /// friction in the dir not that of the ski
  float m_mu_max;
  /// hardness of the points
  float m_hardness;

  float m_control_per_chan_1;     //control per joystick channel
  float m_control_per_chan_2;     //control per joystick channel
  float m_control_per_chan_3;     //control per joystick channel
  float m_control_per_chan_4;     //control per joystick channel
  float m_control_per_chan_5;     //control per joystick channel
  float m_control_per_chan_6;     //control per joystick channel
  float m_control_per_chan_7;     //control per joystick channel
  float m_control_per_chan_8;     //control per joystick channel
  
  std::vector<Hard_point> m_hard_points;

  std::string m_name;

  /// this varies - it is our direction (rotation around the z axis in
  /// deg)
  float m_dir;
};


#endif
