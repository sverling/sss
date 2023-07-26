/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file glider_engine.cpp
*/

#include "glider_engine.h"
#include "config_file.h"
#include "config.h"
#include "joystick.h"
#include "sss.h"
#include "glider.h"
#include "matrix_vector3.h"
#include "particle_engine.h"
#include "environment.h"
#include "log_trace.h"

using namespace std;

Glider_engine::Glider_engine(Config_file & engine_config, Glider & glider)
  :
  m_glider(&glider)
{
  TRACE_METHOD_ONLY(2);
  for (unsigned i = 0 ; 
       i < sizeof(m_control_per_chan)/sizeof(m_control_per_chan[0]) ;
       ++i)
  {
    m_control_per_chan[i] = 0;
  }

  m_min_force = 0;
  m_max_force = m_glider->get_mass() * 9.81f;
  m_force = 0.0f;
  m_max_airspeed = 999999;
  m_name = "none";
  m_bidirectional = false;
  m_absolute_direction = false;
  m_position.set_to(0);
  m_direction[0] = 1;
  m_direction[1] = 0;
  m_direction[2] = 0;
  m_do_smoke = true;
  m_smoke_rate_scale = 1.0f;
  m_smoke_init_size = 0.5f;
  m_smoke_max_size = 10.0f;
  m_smoke_lifetime = 1.0f;
  m_smoke_alpha_scale = 1.0f;
  m_smoke_extra_vel.set_to(0.0f);

  engine_config.get_next_value_assert("name", m_name);
  engine_config.get_next_value_assert("min_force", m_min_force);
  engine_config.get_next_value_assert("max_force", m_max_force);
  engine_config.get_next_value_assert("max_airspeed", m_max_airspeed);
  engine_config.get_next_value("position_x", m_position[0]);
  engine_config.get_next_value("position_y", m_position[1]);
  engine_config.get_next_value("position_z", m_position[2]);
  engine_config.get_next_value("direction_x", m_direction[0]);
  engine_config.get_next_value("direction_y", m_direction[1]);
  engine_config.get_next_value("direction_z", m_direction[2]);
  engine_config.get_next_value("bidirectional", m_bidirectional);
  engine_config.get_next_value("absolute_direction", m_absolute_direction);
  engine_config.get_next_value("control_per_chan_1", m_control_per_chan[1]);
  engine_config.get_next_value("control_per_chan_2", m_control_per_chan[2]);
  engine_config.get_next_value("control_per_chan_3", m_control_per_chan[3]);
  engine_config.get_next_value("control_per_chan_4", m_control_per_chan[4]);
  engine_config.get_next_value("control_per_chan_5", m_control_per_chan[5]);
  engine_config.get_next_value("control_per_chan_6", m_control_per_chan[6]);
  engine_config.get_next_value("control_per_chan_7", m_control_per_chan[7]);
  engine_config.get_next_value("control_per_chan_8", m_control_per_chan[8]);
  engine_config.get_next_value("do_smoke", m_do_smoke);
  engine_config.get_next_value("smoke_rate_scale", m_smoke_rate_scale);
  engine_config.get_next_value("smoke_init_size", m_smoke_init_size);
  engine_config.get_next_value("smoke_max_size", m_smoke_max_size);
  engine_config.get_next_value("smoke_lifetime", m_smoke_lifetime);
  engine_config.get_next_value("smoke_alpha_scale", m_smoke_alpha_scale);
  engine_config.get_next_value("smoke_extra_vel_x", m_smoke_extra_vel[0]);
  engine_config.get_next_value("smoke_extra_vel_y", m_smoke_extra_vel[1]);
  engine_config.get_next_value("smoke_extra_vel_z", m_smoke_extra_vel[2]);
  
  if (m_do_smoke)
  {
    m_particle_id = Particle_engine::instance()->register_source(
      PARTICLE_SMOKE_MEDIUM,
      1000, // initial number
      glider.get_pos() + glider.get_orient() * m_position,
      glider.get_vel() + glider.get_orient() * m_smoke_extra_vel,
      m_smoke_alpha_scale,
      m_smoke_init_size,
      m_smoke_max_size,
      m_smoke_lifetime,
      0   // initial rate - gets updated all the time...
      );
  }
}

Glider_engine::Glider_engine(const Glider_engine & orig)
{
  *this = orig;
  if (m_do_smoke)
  {
    m_particle_id = Particle_engine::instance()->register_source(
      PARTICLE_SMOKE_MEDIUM,
      1000, // initial number
      m_glider->get_pos(),
      m_glider->get_vel(),
      orig.m_smoke_alpha_scale,
      orig.m_smoke_init_size,
      orig.m_smoke_max_size,
      orig.m_smoke_lifetime,
      0   // initial rate - gets updated all the time...
      );
  }
}

Glider_engine::~Glider_engine()
{
  TRACE_METHOD_ONLY(2);
  if (m_do_smoke)
    Particle_engine::instance()->deregister_source(m_particle_id);
}

/*!
 */
void Glider_engine::get_force(Vector3 & force)
{
  TRACE_METHOD_ONLY(4);

  float control = 0;
  for (unsigned i = 1 ; 
       i < sizeof(m_control_per_chan)/sizeof(m_control_per_chan[0]) ;
       ++i)
  {
    if (m_bidirectional == true)
    {
      control += m_control_per_chan[i] * 
        m_glider->get_joystick().get_value(i);
    }
    else
    {
      control += m_control_per_chan[i] * 
        (1 + m_glider->get_joystick().get_value(i))*0.5f;
    }
  }

  float f = m_min_force + control * (m_max_force - m_min_force);
  
  float airspeed_scale = 1.0f - m_glider->get_airspeed().mag()/m_max_airspeed;
  if (airspeed_scale < 0) airspeed_scale = 0;

  f *= airspeed_scale;

  force = f * m_direction;

  if (m_absolute_direction == true)
  {
    // the API (ha ha) says the force returned is relative to the glider. 
    force = transpose(m_glider->get_orient()) * force;
  }
  m_force = force.mag();
}

void Glider_engine::post_physics(float dt)
{
  TRACE_METHOD_ONLY(4);

  if (m_do_smoke)
  {
    // update the smoke

    if ( fabs(m_max_force - m_min_force) > 0.01)
    {
      // emit particles? maybe update rate  
//       const float particle_stop_time = 
//         Sss::instance()->config().missile_smoke_time;
      const float particle_max_rate = 
        0.5 * Sss::instance()->config().missile_smoke_max_rate;
      float rate = m_smoke_rate_scale * particle_max_rate * 
        fabs( m_force / (m_max_force - m_min_force) );
      if (rate < 0.0f) rate = 0.0f;
    
      Velocity vel = 
        Environment::instance()->get_ambient_wind(m_glider->get_pos()) + 
        m_glider->get_orient() * m_smoke_extra_vel;

      Particle_engine::instance()->set_rate(m_particle_id, rate);
      Particle_engine::instance()->update_source
        (
          m_particle_id,
          dt,
          m_glider->get_pos() + m_glider->get_orient() * m_position,
          vel
          );
    }
  }
}

void Glider_engine::show()
{
  TRACE("Glider engine (default)\n");
}
