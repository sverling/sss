/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file thermal_manager.cpp
*/

#include "thermal_manager.h"
#include "thermal.h"
#include "physics.h"
#include "object.h"
#include "sss.h"
#include "config.h"
#include "log_trace.h"

using namespace std;

Thermal_manager::Thermal_manager(float xmin, float xmax,
                                 float ymin, float ymax)
  :
  m_xmin(xmin), m_xmax(xmax),
  m_ymin(ymin), m_ymax(ymax)
{
  TRACE_METHOD_ONLY(1);

  reinitialise();
}

void Thermal_manager::reinitialise()
{
  TRACE_METHOD_ONLY(1);
  float thermal_density = Sss::instance()->config().thermal_density;
  m_mean_updraft = Sss::instance()->config().thermal_mean_updraft;
  m_sigma_updraft = Sss::instance()->config().thermal_sigma_updraft;
  m_mean_radius = Sss::instance()->config().thermal_mean_radius;
  m_sigma_radius = Sss::instance()->config().thermal_sigma_radius;
  m_mean_height = Sss::instance()->config().thermal_height;
  m_mean_inflow_height = Sss::instance()->config().thermal_inflow_height;
  m_mean_lifetime = Sss::instance()->config().thermal_mean_lifetime;
  m_sigma_lifetime = Sss::instance()->config().thermal_sigma_lifetime;

  m_num_thermals = (int) ((m_xmax - m_xmin) * (m_ymax - m_ymin) * 
                          thermal_density / 1000000.0); // density is in km^-2
  TRACE_FILE_IF(2)
    TRACE("Creating %d theramls\n", m_num_thermals);
  
  thermals.clear();

  for (int i = 0 ; i < m_num_thermals ; ++i)
  {
    float radius = m_mean_radius + m_sigma_radius * ranged_random(-1, 1);

    thermals.push_back(Thermal(
                         Position(ranged_random(m_xmin, m_xmax), 
                                  ranged_random(m_ymin, m_ymax),
                                  0),
                         m_mean_updraft + m_sigma_updraft * ranged_random(-1, 1),
                         radius,
                         radius * 2.0f,
                         radius * 6.0f,
                         radius * 10.0f,
                         m_mean_height,
                         m_mean_inflow_height,
                         m_mean_lifetime + m_sigma_lifetime * ranged_random(-1, 1),
                         ranged_random(0, 1)));
  }
}



// Update the thermal locations
void Thermal_manager::update_thermals(float dt)
{
  for (Thermals_it it = thermals.begin() ; it != thermals.end() ; ++it)
  {
    it->update(dt);
    const float x = it->get_pos()[0];
    const float y = it->get_pos()[1];
    if ( (x < m_xmin) || (x > m_xmax) || (y < m_ymin) || (y > m_ymax) )
    {
      it->reset(Position(ranged_random(m_xmin, m_xmax), 
                         ranged_random(m_ymin, m_ymax),
                         0));
    }
  }

  // Take this opportunity to build a list of all the thermals that
  // could possibly affect any objects (at least, those registered
  // with the Physics object) in this frame. Checking every
  // thermal location in get_wind is very expensive because it may get
  // called many times per physics timestep (and the physics timestep
  // may be less that 1ms!)

  effective_thermals.clear();

  // get list of physics objects
  const list<Object *> physics_objects = 
    Physics::instance()->get_objects();

  list<Object *>::const_iterator physics_it;
  for (physics_it = physics_objects.begin() ; 
       physics_it != physics_objects.end() ; 
       ++physics_it)
  {
    float x = (*physics_it)->get_pos()[0];
    float y = (*physics_it)->get_pos()[1];
    for (Thermals_const_it it = thermals.begin() ; it != thermals.end() ; ++it)
    {
      if (it->in_range(x, y))
      {
        effective_thermals.insert(it); // don't care if it fails
      }
    }
  }
}

Velocity Thermal_manager::get_wind(const Position & pos) const
{
  Velocity vel(0);
//    for (Thermals_const_it it = thermals.begin() ; it != thermals.end() ; ++it)
//    {
//      if (it->in_range(pos[0], pos[1]))
//      {
//        vel += it->get_wind(pos);
//      }
//    }
  
  for (Effective_thermals_const_it it = effective_thermals.begin() ; 
       it != effective_thermals.end() ; 
       ++it)
  {
    vel += (*it)->get_wind(pos);
  }
  return vel;
}

//! Draw the thermals. Note that we can choose to show only the
//! relevant thermals, in order to improve frame rate.
void Thermal_manager::draw_thermals() const
{
  // should really sort these
  if (Sss::instance()->config().thermal_show_all)
  {
    for (Thermals_const_it it = thermals.begin() ; it != thermals.end() ; ++it)
    {
      it->draw();
    }
  }
  else
  {
    for (Effective_thermals_const_it it = effective_thermals.begin() ; 
         it != effective_thermals.end() ; 
         ++it)
    {
      (*it)->draw();
    }
  }
  
}
