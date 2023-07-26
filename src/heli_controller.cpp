/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file heli_controller.cpp
*/

#include "heli_controller.h"
#include "glider.h"
#include <limits>

void wrap(float &val, float min, float max)
{
  while (val > max)
    val -= (max - min);
  while (val < min)
    val += (max - min);
}

void limit(float &val, float min, float max)
{
  if (val < min)
    val = min;
  else if (val > max)
    val = max;
}

//==============================================================
// PID_controller
//==============================================================
PID_controller::PID_controller(float p, float i, float d) : 
  m_p(p), m_i(i), m_d(d)
{
  m_last_val = std::numeric_limits<float>::max();
  m_integral = 0.0f;
}
//==============================================================
// update
//==============================================================
float PID_controller::update(float dt, float target, float current, float min_output, float max_output)
{
  if (m_last_val == std::numeric_limits<float>::max())
    m_last_val = current;

  float error = target - current;
  float rate = (current - m_last_val) / dt;
  m_last_val = current;

  float output = m_p * error + m_d * rate + m_i * m_integral;

  if (output > max_output)
    return max_output;
  else if (output < min_output)
    return min_output;

  m_integral += dt * error;

  return output;
}


//==============================================================
// Heli_controller
//==============================================================
Heli_controller::Heli_controller(class Glider *glider) 
  : 
  m_glider(glider),
  m_joystick(glider->get_joystick().get_num_channels()),
  m_input_mult(0.5f),
  m_yaw_controller(-0.03f, 0.0f, 0.00f),
  m_power_controller(0.4f, 0.0f, -0.1f)
{
  m_glider->take_control(&m_joystick);
  m_joystick.zero_all_channels();

  // http://www.helis.com/70s/h_h58.php
  m_max_fwd_speed = 60.0f;
  m_max_left_speed = 60.0f;
  m_max_up_speed = 15.0f;
  m_max_yaw_rate = 180.0f;

  m_desired_height = glider->get_pos()[2];
  m_desired_dir = 0.0f;

  m_mr_fwd = NO_FWD;
  m_mr_left = NO_LEFT;
  m_mr_up = NO_UP;
  m_mr_strafe = NO_STRAFE;
}

//==============================================================
// Heli_controller
//==============================================================
Heli_controller::~Heli_controller()
{
  m_glider->set_eye_offset(Vector3(0.0f));
  m_glider->take_control(0);
}

//====================================================================
// mouse_motion
//====================================================================
void Heli_controller::mouse_motion(float x, float y)
{
#if 1
  // use mouse for view control
  float look_y = 1.0f - (y * 2.0f);
  float look_x = 1.0f - (x * 2.0f);

  m_glider->set_eye_offset(Vector3(look_x, look_y, 0.0f));
#else
  // use mouse for control
  m_mr_fwd = 1.0f - (y * 2.0f);
  m_mr_left = 1.0f - (x * 2.0f);
#endif
}

//====================================================================
// mouse
//====================================================================
void Heli_controller::mouse(int button, int state, int x, int y)
{
  if (GLUT_DOWN == state)
    m_joystick.set_value(5, 1);
}


//==============================================================
// key
//==============================================================
bool Heli_controller::key(unsigned char key, int px, int py)
{
  switch (key)
  {
  case 'w':
  case 'W':
    m_mr_fwd = FWD;
    return true;
  case 'a':
  case 'A':
    m_mr_left = LEFT;
    return true;
  case 's':
  case 'S':
    m_mr_fwd = BWD;
    return true;
  case 'd':
  case 'D':
    m_mr_left = RIGHT;
    return true;
  case 'q':
  case 'Q':
    m_mr_strafe = STRAFE_LEFT;
    return true;
  case 'e':
  case 'E':
    m_mr_strafe = STRAFE_RIGHT;
    return true;
  case ' ':
    m_mr_up = UP;
    return true;
  case 'z':
  case 'Z':
    m_mr_up = DOWN;
    return true;
  case 'x':
  case 'X':
    m_input_mult = m_input_mult > 0.7f ? 0.5f : 1.0f;
    return true;
  }
  return false;
}

//==============================================================
// special
//==============================================================
bool Heli_controller::special(int key, int px, int py)
{
  return false;
}

//==============================================================
// key_up
//==============================================================
bool Heli_controller::key_up(unsigned char key, int px, int py)
{
  switch (key)
  {
  case 'w':
  case 's':
  case 'W':
  case 'S':
    m_mr_fwd = NO_FWD;
    return true;
  case 'a':
  case 'd':
  case 'A':
  case 'D':
    m_mr_left = NO_LEFT;
    return true;
  case 'q':
  case 'e':
  case 'Q':
  case 'E':
    m_mr_strafe = NO_STRAFE;
    return true;
  case ' ':
  case 'z':
  case 'Z':
    m_mr_up = NO_UP;
    return true;
  }
  return false;
}

//==============================================================
// special_up
//==============================================================
bool Heli_controller::special_up(int key, int px, int py)
{
  return false;
}

//==============================================================
// update
//==============================================================
void Heli_controller::update(float dt)
{
  const Orientation &orient = m_glider->get_orient();
  const Vector &current_fwd_dir = orient.get_col(0);
  const Vector &current_left_dir = orient.get_col(1);
  const Vector &current_up_dir = orient.get_col(2);
  // in degrees
  // +ve pitch means nose up
  float current_pitch = asin_deg(current_fwd_dir[2]);
  // +ve roll means to the left
  float current_roll = asin_deg(current_left_dir[2]);
  // +ve direction mean rotation anti-clocwise about the z axis - 0 means along x
  float current_dir = atan2_deg(current_fwd_dir[1], current_fwd_dir[0]);

  Vector current_fwd_dir2D = current_fwd_dir;
  current_fwd_dir2D[2] = 0.0f;
  current_fwd_dir2D.normalise();

  Vector current_left_dir2D(-current_fwd_dir2D[1], current_fwd_dir2D[0], 0.0f);

  Vector current_vel2D = m_glider->get_vel();
  current_vel2D[2] = 0.0f;

  float current_height = m_glider->get_pos()[2];
  Vector hor_fwd_dir = orient.get_col(0);
  hor_fwd_dir[2] = 0.0f;
  hor_fwd_dir.normalise();
  float current_fwd_speed = dot(m_glider->get_vel(), hor_fwd_dir);
  Vector hor_left_dir = orient.get_col(1);
  hor_left_dir[2] = 0.0f;
  hor_left_dir.normalise();
  float current_left_speed = dot(m_glider->get_vel(), hor_left_dir);

  // desired things

  static float yaw_decrease_scale = 10.0f; // units of m/s to half the yawing
  float turn_decrease_scale = yaw_decrease_scale / (yaw_decrease_scale + fabs(current_fwd_speed));
  float turn_increase_scale = 1.0f - turn_decrease_scale;

  m_desired_dir += turn_decrease_scale * m_mr_left * m_max_yaw_rate * m_input_mult * dt;
  wrap(m_desired_dir, -180.0f, 180.0f);

  wrap(current_dir, m_desired_dir - 180.0f, m_desired_dir + 180.0f);
  float delta = m_desired_dir - current_dir;
  limit(delta, -35.0f, 35.0f);
  m_desired_dir = current_dir + delta;

  Vector desired_vel2D = 
    current_fwd_dir2D * m_mr_fwd * m_max_fwd_speed * m_input_mult + 
    current_left_dir2D * m_mr_strafe * m_max_left_speed * m_input_mult;

  m_desired_height += m_mr_up * m_max_up_speed * m_input_mult * dt;

  // calculate the inputs
  float roll_input = m_joystick.get_value(1);
  float pitch_input = m_joystick.get_value(2);
  float power_input = m_joystick.get_value(3);
  float yaw_input = m_joystick.get_value(4);

  Vector desired_vel_change2D = desired_vel2D - current_vel2D;

  static float angle_per_vel_difference = 1.5f;
  float desired_tilt_angle = angle_per_vel_difference * desired_vel_change2D.mag();
  static float max_angle = 50.0f;
  limit(desired_tilt_angle, -max_angle, max_angle);

  float desired_pitch = 0.0f;
  float desired_roll = 0.0f;
  if (desired_tilt_angle > 0.0001f)
  {
    const Vector desired_world_tilt_axis = Vector(-desired_vel_change2D[1], desired_vel_change2D[0], 0.0f).normalise();
    const Vector desired_local_tilt_axis = transpose(orient) * desired_world_tilt_axis;
    desired_pitch = -desired_tilt_angle * desired_local_tilt_axis[1];
    desired_roll = desired_tilt_angle * desired_local_tilt_axis[0];
  }
  // tweaks for turning
  float extra_roll_for_turn = -max_angle * turn_increase_scale * m_mr_left;
  desired_roll += extra_roll_for_turn;

  static float scale_pitch = 0.1f;
  pitch_input = scale_pitch * (desired_pitch - current_pitch);
  roll_input = scale_pitch * (desired_roll - current_roll);

  power_input = m_power_controller.update(dt, m_desired_height, current_height, -1.0f, 1.0f);

  float working_desired_dir = m_desired_dir;
  static float yaw_roll_scale = 0.45f;
  working_desired_dir += yaw_roll_scale * current_roll;

  yaw_input = m_yaw_controller.update(dt, working_desired_dir, current_dir, -1.0f, 1.0f);
 
  m_joystick.set_value(1, roll_input);
  m_joystick.set_value(2, pitch_input);
  m_joystick.set_value(3, power_input);
  m_joystick.set_value(4, yaw_input);
}
