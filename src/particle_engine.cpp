/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#include "particle_engine.h"
#include "particle_source.h"
#include "sss.h"
#include "config.h"
#include "log_trace.h"
#include "sss_assert.h"
#include "object.h"

using namespace std;

Particle_engine * Particle_engine::m_instance = 0;

Particle_engine::Particle_engine()
{
  TRACE_METHOD_ONLY(1);

  // initialise the free id list. Note that if we run out of ids we 
  // can always add more.
  for (unsigned i = 0 ; i < 256 ; ++i)
    free_ids.push_back(i);
}

Particle_engine::~Particle_engine()
{
  TRACE_METHOD_ONLY(1);

  for (Particle_source_map::iterator it = particle_sources.begin();
       it != particle_sources.end();
       ++it)
  {
    delete (it->second);
  }

}

int Particle_engine::register_source(Particle_type type,
                                     int num,
                                     const Position & pos,
                                     const Velocity & vel,
                                     float alpha_scale,
                                     float init_size,
                                     float max_size,
                                     float lifetime,
                                     float rate)
{
  TRACE_METHOD_ONLY(2);

  if (free_ids.empty())
  {
    /// Rather than expanding the id pool, just don't allocate a source
    /// If there are that many sources, who will notice?!
    return -1;
    // all the ids must be in the map, so make some more starting
    // at the size of the map. double it.
    unsigned num = particle_sources.size();
    for (unsigned i = 0 ; i < num ; ++i)
    {
      free_ids.push_back(num + i);
    }
  }

  assert1(!free_ids.empty());

  int id = free_ids.back();
  free_ids.pop_back();

  Particle_source * new_source = new Particle_source(
    type,
    num,
    pos,
    vel,
    alpha_scale,
    init_size,
    max_size,
    lifetime,
    rate);
  particle_sources[id] = new_source;
  return id;
}

void Particle_engine::set_rate(int id,
                               float new_rate)
{
  TRACE_METHOD_ONLY(4);
  // make sure it exists
  if (id == -1)
    return;

  Particle_source_map::iterator it = particle_sources.find(id);
  assert1(it != particle_sources.end());

  it->second->set_rate(new_rate);
}

void Particle_engine::deregister_source(int id)
{
  TRACE_METHOD_ONLY(2);
  // make sure it exists
  if (id == -1)
    return;
  Particle_source_map::iterator it = particle_sources.find(id);
  assert1(it != particle_sources.end());

  // just disable it - when all the particles have expired it will 
  // get cleaned up.
  it->second->disable();
}

void Particle_engine::update_source(int id,
                                    float dt,
                                    const Position & source_pos,
                                    const Velocity & source_vel,
                                    float vel_jitter_mag)
{
  if (id == -1)
    return;
  // if particles are disabled then if we don't update the source (i.e.
  // add any new particles), they'll go away when the source is
  // deregistered.
  if ( (Sss::instance()->config().use_particles == false) ||
       (Sss::instance()->config().texture_level < 3) )
    return;

  // make sure it exists
  Particle_source_map::iterator it = particle_sources.find(id);
  assert1(it != particle_sources.end());

  it->second->update_source(dt, source_pos, source_vel, vel_jitter_mag);
}

void Particle_engine::move_particles(float dt)
{
  for (Particle_source_map::iterator it = particle_sources.begin();
       it != particle_sources.end();
    )
  {
    it->second->move_particles(dt);
    if (it->second->dead())
    {
      TRACE_FILE_IF(4)
        TRACE("Removing particle source %p\n", it->second);
      delete it->second;
      free_ids.push_back(it->first);
      particle_sources.erase(it++);
      // remove the source
    }
    else
    {
      ++it;
    }
  }

}

void Particle_engine::draw_particles(const Object * eye)
{
  if ( (Sss::instance()->config().use_particles == false) ||
       (Sss::instance()->config().texture_level < 3) )
    return;
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glDisable(GL_LIGHTING);
  glEnable(GL_BLEND);
  glDepthMask(GL_FALSE);
  for (Particle_source_map::iterator it = particle_sources.begin();
       it != particle_sources.end();
       ++it)
  {
    it->second->draw_particles(eye);
  }
  glPopAttrib();
  glDisable(GL_BLEND);
  glDepthMask(GL_TRUE);
}

