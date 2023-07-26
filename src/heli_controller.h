/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef HELI_CONTROLLER_H
#define HELI_CONTROLLER_H

#include "joystick.h"

//==============================================================
// PID
//==============================================================
class PID_controller
{
public:
  PID_controller(float p, float i, float d);
  float update(float dt, float target, float current, 
               float min_output, float max_output);
private:
  float m_p, m_i, m_d;
  float m_last_val, m_integral;
};

//==============================================================
// Heli_controller
//==============================================================
class Heli_controller
{
public:
  Heli_controller(class Glider *glider);
  ~Heli_controller();

  bool key(unsigned char key, int px, int py);
  bool key_up(unsigned char key, int px, int py);
  bool special(int key, int px, int py);
  bool special_up(int key, int px, int py);

  void mouse_motion(float x, float y);
  void mouse(int button, int state, int x, int y);

  void update(float dt);
private:
  enum Movement_request_fwd
  {
    FWD = 1, BWD = -1, NO_FWD = 0
  };
  enum Movement_request_left
  {
    LEFT = 1, RIGHT = -1, NO_LEFT = 0
  };
  enum Movement_request_up
  {
    UP = 1, DOWN = -1, NO_UP = 0
  };
  enum Movement_request_strafe
  {
    STRAFE_LEFT = 1, STRAFE_RIGHT = -1, NO_STRAFE = 0
  };

  class Glider *m_glider;
  Joystick m_joystick;

  float m_max_fwd_speed; ///< m/s
  float m_max_left_speed; ///< m/s
  float m_max_up_speed; ///< m/s
  float m_max_yaw_rate; ///< degrees per second

  float m_input_mult;

  float m_mr_fwd;
  float m_mr_left;
  float m_mr_up;
  float m_mr_strafe;

  float m_desired_height;
  float m_desired_dir; // in deg anti-clockwise - 0 means along x

  PID_controller m_yaw_controller;
  PID_controller m_power_controller;
};

#endif
