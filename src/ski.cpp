/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#include "ski.h"
#include "joystick.h"
#include "config_file.h"

using namespace std;

Ski::Ski(Config_file & ski_config)
{
  TRACE_METHOD_ONLY(1);
 
  m_hard_points.resize(3);
  m_dir = 30;
  
  m_control_per_chan_1 = 0;
  m_control_per_chan_2 = 0;
  m_control_per_chan_3 = 0;
  m_control_per_chan_4 = 0;
  m_control_per_chan_5 = 0;
  m_control_per_chan_6 = 0;
  m_control_per_chan_7 = 0;
  m_control_per_chan_8 = 0;

  ski_config.get_next_value_assert("name", m_name);
  ski_config.get_next_value_assert("position_x", m_position[0]);
  ski_config.get_next_value_assert("position_y", m_position[1]);
  ski_config.get_next_value_assert("position_z", m_position[2]);
  ski_config.get_next_value_assert("total_length", m_total_length);
  ski_config.get_next_value_assert("mu_min", m_mu_min);
  ski_config.get_next_value_assert("mu_max", m_mu_max);
  ski_config.get_next_value_assert("hardness", m_hardness);
  ski_config.get_next_value("control_per_chan_1", m_control_per_chan_1);
  ski_config.get_next_value("control_per_chan_2", m_control_per_chan_2);
  ski_config.get_next_value("control_per_chan_3", m_control_per_chan_3);
  ski_config.get_next_value("control_per_chan_4", m_control_per_chan_4);
  ski_config.get_next_value("control_per_chan_5", m_control_per_chan_5);
  ski_config.get_next_value("control_per_chan_6", m_control_per_chan_6);
  ski_config.get_next_value("control_per_chan_7", m_control_per_chan_7);
  ski_config.get_next_value("control_per_chan_8", m_control_per_chan_8);
  
  calculate_hard_points();
}

void Ski::calculate_hard_points()
{
  Vector dir(cos_deg(m_dir), sin_deg(m_dir), 0);
  
  m_hard_points[0].pos = m_position + 0.5f * m_total_length * dir;
  m_hard_points[1].pos = m_position;
  m_hard_points[2].pos = m_position -0.5f * m_total_length * dir;
  
  for (unsigned i = 0 ; i < 3 ; ++i)
  {
    m_hard_points[i].mu_min = m_mu_min;
    m_hard_points[i].mu_max = m_mu_max;
    m_hard_points[i].hardness = m_hardness;
    m_hard_points[i].min_friction_dir = dir;
  }
}

void Ski::update(const Joystick & joystick)
{
  m_dir = 
    m_control_per_chan_1 * joystick.get_value(1) + 
    m_control_per_chan_2 * joystick.get_value(2) + 
    m_control_per_chan_3 * joystick.get_value(3) + 
    m_control_per_chan_4 * joystick.get_value(4) + 
    m_control_per_chan_5 * joystick.get_value(5) + 
    m_control_per_chan_6 * joystick.get_value(6) + 
    m_control_per_chan_7 * joystick.get_value(7) + 
    m_control_per_chan_8 * joystick.get_value(8);
  calculate_hard_points();
}

