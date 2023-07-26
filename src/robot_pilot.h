/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef ROBOT_PILOT_H
#define ROBOT_PILOT_H

#include "pilot.h"
#include "types.h"
#include "text_overlay.h"

//  #include <vector>
//  using namespace std;

class Config_file;
class Glider;
class Joystick;
class Object;

/// Controls robot gliders
class Robot_pilot : public Pilot
{
public:
  Robot_pilot(Config_file & robot_config,
              Position start_pos,
              float offset, // in dir per to wind
              bool pause_on_startup = true);
  ~Robot_pilot();
  Glider * get_glider() {return m_glider;}
  
  void reset(const Object * obj);

  // inherited
  void update(float dt);
private:
  void choose_target_point();
  
  /// send a missile if there's a chance it might hit. If so, reset
  /// the missile timer.
  void try_missile(const Object * target);
  
  Glider * m_glider;
  Joystick * m_joystick;
  
  // here is the stuff that is used for the path planning
  
  //! Robot will try to stick around this point
  Position m_focal_point;
  
  //! Randomly chosen target points will lie along this dir par to hill 
  Vector m_focal_par_dir;

  //! Randomly chosen target points will lie along this derp par to hill 
  Vector m_focal_perp_dir;

  /// distance of the main focal point
  float m_focal_dist;
  
  //! Minimum air-speed for this glider - below this speed the robot
  //! will take drastic action.
  float m_speed_min;
  
  //! Normal flying speed
  float m_speed_ideal;
  
  //! Range to stray from the focal point parallel to the hill
  float m_focal_point_par_range;
  
  //! Range to stray from the focal point perpendicular to the hill
  float m_focal_point_perp_range;
  
  //! Range to stray from the focal point in height
  float m_focal_point_height_range;
  
  //! Point that we're currently aiming for
  Position m_target;
  
  //! when we get this far from the current target, choose another
  float m_target_range;
  
  //! maximum bank angle for turns (deg)
  float m_max_bank_angle;
  
  //! maximum pitch angle
  float m_max_pitch_angle;
  
  //! maximum amount we'll aim to put the pitch up/down
  float m_max_pitch_amount;
  
  //! when we are 1m below the target, aim for this pitch
  float m_pitch_per_height_offset;
  
  //! when aileron control = 1, elev control = this
  float m_elev_aileron_frac;
  
  //! bank angle for maximum elevator
  float m_bank_angle_for_max_elev;
  
  //! What tactic are we using
  enum Tactic {CHASE, CRUISE, RACE};
  Tactic m_tactic;
  
  //! time in seconds to spend chasing the human
  float m_chase_time;
  
  //! time to spend just cruising around
  float m_cruise_time;
  
  //! elapsed time since the last change of tactic
  float m_elapsed_time;
  
  //! reset if we're on the ground for longer than a certain time
  float m_reset_timer;
  
  /// When we send a missile we set m_missile_timer =
  /// m_missile_recharge_time.
  float m_missile_recharge_time;
  /// We decrement this timer and only (possibly) fire a missile
  /// when it reaches 0
  float m_missile_timer;
  
  /// bigger = a more trigger-happy robot
  float m_missile_trigger;
  
  //! for debugging
  Text_overlay m_text_overlay;
  
  /// Used to get the robots to take off at slightly different times
  bool m_doing_initial_wait;
  float m_initial_pause_time;

  // starting position offset
  float m_start_offset;
};

#endif
