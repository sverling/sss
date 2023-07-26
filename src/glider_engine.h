/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_ENGINE_H
#define GLIDER_ENGINE_H

#include <string>
#include "types.h"

class Config_file;
class Glider;

/// This represents a single engine. The Glider_power class may
/// contain a number of engines.
class Glider_engine
{
public:
  Glider_engine(Config_file & engine_config, Glider & glider);
  Glider_engine(const Glider_engine & orig);

  ~Glider_engine();

  void get_force(Vector3 & force);
  void post_physics(float dt);
  void show();

  Position m_position; // relative to glider cog

private:
  Glider * m_glider;

  std::string m_name;
  float m_min_force;
  float m_max_force;
  float m_max_airspeed;
  bool m_bidirectional;
  bool m_absolute_direction; // true if direction is absolute, false if relative
  Vector3 m_direction;
  float m_control_per_chan[9];
  float m_force;
  int m_particle_id;
  bool m_do_smoke;
  float m_smoke_rate_scale;
  float m_smoke_init_size;
  float m_smoke_max_size;
  float m_smoke_lifetime;
  float m_smoke_alpha_scale;
  Velocity m_smoke_extra_vel;
};

#endif
