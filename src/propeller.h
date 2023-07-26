/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef PROPELLER_H
#define PROPELLER_H

#include "types.h"
#include "object.h"
#include "environment.h"

#include <vector>
#include <string>

class Config_file;
class Joystick;
class Glider_aero_component;

/// This class represents a propeller with graphical, aerodynamic and
/// structural properties.
class Propeller : public Wind_modifier
{
public:
  Propeller(Config_file & prop_config);
  
  // From Wind_modifier
  void get_wind(const Position & pos, Vector & wind) const;

  void disable_main_wash() {m_wash = false;}
  void enable_main_wash() {m_wash = true;}

  void get_forces(const Velocity & wind_rel, // in
                  const Position & glider_pos,
                  const Orientation & glider_orient,
                  const Vector & local_rot,
                  const Joystick & joystick,
                  const Glider_aero_component & glider, // to get the gyros
                  Vector3 & force,  // force out
                  Vector3 & moment, // moment out
                  Position & force_position, // location of force out
                  float density); // air density
  
  /// Return the angular momentum  in the object reference frame
  void get_ang_mom(const Joystick & joystick,
                   Vector3 & ang_mom) const;
  
  const Position & get_pos() const {return m_position;}
  
  // draw (very basic)
  enum Draw_type { NORMAL, SHADOW };
  void draw(Draw_type draw_type);
  
  /// Returns the list of hard points
  const std::vector<Hard_point> & get_hard_points() const 
    {return m_hard_points;}

private:
  void calculate_hard_points();
  
  std::string m_name;
  
  Position m_position;
  
  // axis is in the x/y plane (fwd being x), and then rotated about x, y, then z
  float m_rot_x;
  float m_rot_y;
  float m_rot_z;
  
  // calculate and store the axis
  Vector m_axis;
  
  float m_moment_of_inertia;
  
  float m_radius;
  float m_blade_chord;
  
  // Internally the forces will be determined by calculating at four
  // points at m_radius * m_radius_frac: nominally
  // fwd/left/back/right, though these will be offset through a
  // m_offset_angle about m_axis. If m_axis = 0, then for a prop
  // pointing in the +ve z direction, fwd is fwd and left is left.
  float m_offset_angle;
  float m_radius_frac;
  
  // aerodynamic parameters
  float m_CL_0; ///< CL when prop is at alpha = 0
  float m_CL_per_inc; /// in units per deg
  
  /// the inclination for neutral input
  float m_inc_0;
  /// the speed for neutral input
  float m_speed_0;
  
  // control-surface params. Each array value - e.g. value[i] corresponds to
  // joystick channel = i + 1
  enum {MAX_CHANNEL = 8};
  float m_rotation_speed_per_control[MAX_CHANNEL];
  
  // alpha_back is just -alpha_fwd
  // cyclic:
  float m_inc_fwd_per_control[MAX_CHANNEL];
  float m_inc_left_per_control[MAX_CHANNEL];
  // collective
  float m_inc_per_control[MAX_CHANNEL];
  
  std::string m_gyro_name;

  std::vector<Hard_point> m_hard_points;

  // store these for calculating the wind.etc
  float m_last_speed;
  float m_last_inc;
  float m_last_force_z;
  Velocity m_last_wind_rel;
  Position m_last_pos;
  Orientation m_last_orient;

  // calculate the main wash for the wind modifier?
  bool m_wash;
};



#endif
