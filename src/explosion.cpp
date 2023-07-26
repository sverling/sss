/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file explosion.cpp
*/

#include "explosion.h"

#include "misc.h"
#include "physics.h"
#include "sss.h"
#include "config.h"
#include "audio.h"
#include "particle_engine.h"

#include <iostream>
using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Explosion::Explosion(const Position & pos, 
                     const Velocity & vel, 
                     float max_size,
                     bool local)
  :
  Object(pos),
  max_radius(max_size),
  lifetime(4),
  sec_running(0),
  radius(0),
  m_local(local),
  m_ambient_wind(0)
{
  TRACE_METHOD_ONLY(1);
  Environment::instance()->register_wind_modifier(this);
  // register for audio
  string explosion_audio_file("explosion.wav");
  Audio::instance()->register_instance(this, 
                                       explosion_audio_file,
                                       1.0f,
                                       1.0f,
                                       400.0f,
                                       false);
  Physics::instance()->register_object(this);
  Sss::instance()->add_object(this);

  m_particle_id_flame = Particle_engine::instance()->register_source(
    PARTICLE_SMOKE_DARK,
    100, // initial number
    get_pos(),
    Velocity(0),
    30 * Sss::instance()->config().missile_smoke_alpha_scale,
    .5, 10, 1,
    0   // initial rate - gets updated all the time...
    );
  m_particle_id_smoke = Particle_engine::instance()->register_source(
    PARTICLE_SMOKE_MEDIUM,
    400, // initial number
    get_pos(),
    Velocity(0),
    20 * Sss::instance()->config().missile_smoke_alpha_scale,
    3, 10, 1,
    0   // initial rate - gets updated all the time...
    );
}

Explosion::~Explosion()
{
  TRACE_METHOD_ONLY(1);
  // Deregister from audio
  Audio::instance()->deregister_instance(this);
  Physics::instance()->deregister_object(this);
  Sss::instance()->remove_object(this);
  Particle_engine::instance()->deregister_source(m_particle_id_flame);
  Particle_engine::instance()->deregister_source(m_particle_id_smoke);
}
 
void Explosion::post_physics(float dt)
{
  sec_running += dt;
  if (sec_running > lifetime)
  {
    delete this;
    return;
  }

  if (Sss::instance()->config().use_particles)
  {
    // emit particles? maybe update rate  
    const float particle_stop_time = 0.3;
    const float particle_max_rate = Sss::instance()->config().missile_smoke_max_rate;
    float rate = (1.0f - sec_running / particle_stop_time) * particle_max_rate;
    if (rate < 0.0f) rate = 0.0f;

    Particle_engine::instance()->set_rate(m_particle_id_flame, rate*1.5f);
    Particle_engine::instance()->update_source(
      m_particle_id_flame,
      dt,
      get_pos(),
      m_ambient_wind + Velocity(0, 0, 5),
      6.0f); // jitter

    Particle_engine::instance()->set_rate(m_particle_id_smoke, rate);
    Particle_engine::instance()->update_source(
      m_particle_id_smoke,
      dt,
      get_pos(),
      m_ambient_wind + Velocity(0, 0, 10),
      12.0f); // jitter
  }
}

void Explosion::draw(Draw_type draw_type)
{
  if (Sss::instance()->config().use_particles)
  {
    return;
  }
  radius = max_radius * sec_running / lifetime;
  float alpha = 1.0 - pow((double) sec_running / lifetime, 0.5);
  float r = 1.0;
  float g = alpha;
  float b = 0.0;

  glPushMatrix();
  basic_draw();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//  glBlendFunc(GL_SRC_ALPHA, GL_ONE);

  glColor4f(r, g, b, alpha);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT); // draw the back first
  glutSolidSphere(radius, 12, 12);
  glCullFace(GL_BACK); // then the front
  glutSolidSphere(radius, 12, 12);
  glDisable(GL_CULL_FACE);

  glDisable(GL_BLEND);
  glDisable(GL_CULL_FACE);
  glPopMatrix();
}

void Explosion::get_wind(const Position & pos, Vector & wind) const
{
  Vector dir = (pos - get_pos());
  float dist2 = dir.mag2();
  
  // return ASAP if possible
  if (dist2 > (max_radius * max_radius * 2))
    return;

  float dist = sqrt((float) dist2);

  // represent it as a finite-width shell that expands with the
  // displayed shell.

  float r = max_radius * sec_running / lifetime;

  // scale the expansion speed for greater effect...
  float s = 3.0f * max_radius / lifetime;

  // also scale it inversely according to how big the shell has got
  // (this scaling is not realistic!)
  s *= (1-r/max_radius);

  dir.normalise();

  float fac = (r-dist)/(0.1 * max_radius);
  wind += s * exp(-fac * fac) * dir;
}

