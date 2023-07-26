/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#include "physics.h"
#include "particle_engine.h"
#include "types.h"
#include "sss.h"
#include "config.h"
#include "object.h"
#include "sss_assert.h"
#include "log_trace.h"
#include <stdio.h>
#include <algorithm>
using namespace std;

Physics * Physics::m_instance = 0;

Physics::Physics() 
{
  TRACE_METHOD_ONLY(1);
}

Physics::~Physics()
{
  TRACE_METHOD_ONLY(1);
}

void Physics::register_object(Object * object)
{
  TRACE_METHOD_ONLY(2);
  m_objects.push_back(object);
}

int Physics::deregister_object(const Object * object)
{
  TRACE_METHOD_ONLY(2);
  Objects_it it = find(m_objects.begin(),
                       m_objects.end(),
                       object);
  if (it != m_objects.end())
  {
    m_objects.erase(it);
    return 0;
  }
  else
  {
    printf("Could not find object 0x%x to deregister", (size_t) object);
    object->show();
    return -1;
  }
}

void Physics::calc_collision_force(const Object * obja,
                                   Vector & force,
                                   Vector & moment)
{
  float obja_mass;
  Object::Mass_type obja_mass_type = obja->get_mass(obja_mass);
  if (obja_mass_type != Object::MASS)
    return;
  
  float radiusa = obja->get_collision_sphere_radius();
  float min_dist, min_dist2; // minimum distance that two objects are allowed to get
  float actual_dist, actual_dist2;
  float objb_mass;
  Object::Mass_type objb_mass_type;
  for (Objects_it it = m_objects.begin() ; it != m_objects.end() ; ++it)
  {
    if ((*it) == obja) 
      continue;
    if ((objb_mass_type = (*it)->get_mass(objb_mass)) == Object::MASS_TRANSPARENT)
      continue;
    min_dist = radiusa + (*it)->get_collision_sphere_radius();
    min_dist2 = min_dist * min_dist;
    Vector vec_b_a = obja->get_pos() - (*it)->get_pos(); // in dir of force
    actual_dist2 = vec_b_a.mag2();
    if (actual_dist2 < min_dist2)
    {
      // collision!
      float force_scale = 100.0f;
      actual_dist = sqrt(actual_dist2);
      if (actual_dist < 0.05f)
      {
        // the objects are effectively sitting on top of each other,
        // so the direction between them may be undefined. So, set the
        // actual distance to a small value, and use a heuristic to
        // calculate the force vector direction (which should be
        // opposite for different objects).
        actual_dist = 0.05f;
        if ( (*it) > obja )
          vec_b_a = Vector(1, 0, 0);
        else
          vec_b_a = Vector(-1, 0, 0);
      }
      else
      {
        vec_b_a.normalise();
      }
      float f = force_scale * obja_mass * min_dist/actual_dist;
      force += f * vec_b_a;
      
      // fake the moment
      float moment_mag = f * 
        obja->get_collision_sphere_radius() * 
        ranged_random(-1, 1);
      moment += moment_mag * Vector(0, 0, 1);
    }
  }
}

/*!
  Does the timestep for a particular object
*/
void Physics::real_do_timestep(Object * object, float dt)
{
  TRACE_METHOD_ONLY(3);
  
  if (object->use_physics())
  { 
    Config::Physics_mode mode = 
      Sss::instance()->config().physics_mode;
    
    static Vector moment;
    static Vector force;
    
    switch (mode)
    {
    case Config::EULER:
    {
      // ask each object to calculate its object-specific forces
      // (e.g. aerodynamics + ground collision)
      object->calc_object_specific_force_and_moment(force, moment);
      // calculate inter-object forces here...
      calc_collision_force(object, force, moment);
      // objects move in object-specific ways
      object->calc_new_pos_and_orient(dt);
      // Objects rotate and accelerate in object-specific ways
      object->calc_new_vel_and_rot(dt, force, moment);
      break;
    }
    case Config::MOD_EULER:
    {
      // ask each object to calculate its object-specific forces
      // (e.g. aerodynamics + ground collision)
      object->calc_object_specific_force_and_moment(force, moment);
      // calculate inter-object forces here...
      calc_collision_force(object, force, moment);
      // half a step
      object->calc_new_vel_and_rot(0.5f*dt, force, moment);
      // objects move in object-specific ways, using the average vel/rotation
      object->calc_new_pos_and_orient(dt);
      // Objects rotate and accelerate in object-specific ways
      object->calc_new_vel_and_rot(0.5f*dt, force, moment);
      break;
    }
    case Config::RK2:
    {
      const Position orig_pos = object->get_pos();
      const Orientation orig_orient = object->get_orient();
      const Velocity orig_vel = object->get_vel();
      const Rotation orig_rot = object->get_rot();
      
      // move to mid-point based on the original time
      object->calc_object_specific_force_and_moment(force, moment);
      calc_collision_force(object, force, moment);
      object->calc_new_pos_and_orient(dt*0.5f);
      object->calc_new_vel_and_rot(dt*0.5f, force, moment);
      
      // Calculate the derivatives there
      object->calc_object_specific_force_and_moment(force, moment);
      calc_collision_force(object, force, moment);
      
      // now reset the object position (not vel) and move it the whole
      // timestep, using the more accurate vel
      object->set_pos(orig_pos);
      object->set_orient(orig_orient);
      object->calc_new_pos_and_orient(dt);
      
      // now reset the object vel and move it the whole timestep,
      // using the more accurate forces.
      object->set_vel(orig_vel);
      object->set_rot(orig_rot);
      object->calc_new_vel_and_rot(dt, force, moment);
      break;
    }
    case Config::MOD_RK2:
    {
      const Position orig_pos = object->get_pos();
      const Orientation orig_orient = object->get_orient();
      const Velocity orig_vel = object->get_vel();
      const Rotation orig_rot = object->get_rot();
      
      // objects move in object-specific ways
      object->calc_object_specific_force_and_moment(force, moment);
      calc_collision_force(object, force, moment);
      // get the new vel/rot 1/4 of the way...
      object->calc_new_vel_and_rot(dt*0.25f, force, moment);
      // use the new vel/rot to move to the mid-point
      object->calc_new_pos_and_orient(dt*0.5f);
      object->calc_new_vel_and_rot(dt*0.25f, force, moment);
      
      // ask each object to calculate its object-specific forces
      // (e.g. aerodynamics + ground collision)
      object->calc_object_specific_force_and_moment(force, moment);
      
      // calculate inter-object forces here...
      calc_collision_force(object, force, moment);
      
      // now reset the object and move it the whole timestep
      object->set_pos(orig_pos);
      object->set_orient(orig_orient);
      object->set_vel(orig_vel);
      object->set_rot(orig_rot);
      // Objects rotate and accelerate in object-specific ways
      object->calc_new_vel_and_rot(dt*0.5f, force, moment);
      object->calc_new_pos_and_orient(dt);
      object->calc_new_vel_and_rot(dt*0.5f, force, moment);
      break;
    }
    default:
      TRACE("Unhandled switch %d\n", mode);
      assert1(!"Error!");
    } // switch on the physics mode
  }
  else
  {
    // just get it to do the nasty stuff
    object->calc_new_pos_and_orient(dt);
  }
}

/*!
  Does the timestep for a particular object
*/
void Physics::do_timestep(Object * object, float dt0)
{
  int ndt = 1 + (int) (dt0 * Sss::instance()->config().physics_freq);
  float dt = dt0/ndt;
  for (int i = 0 ; i < ndt ; i++)
  {
    real_do_timestep(object, dt);
  }
}


void Physics::do_timestep(float dt0)
{
  // dt is the time since the last update. We do the physics at
  // (probably) a higher frequency.
  
  int ndt = 1 + (int) (dt0 * Sss::instance()->config().physics_freq);
  float dt = dt0/ndt;
  
  Objects_it it;
  
  // let objects do something... they might delete themselves. Note
  // that we couldn't use a vector for the list of objects - the
  // incremented iterator would get invalidated by any deregistration.
  for (it = m_objects.begin() ; it != m_objects.end() ;)
  {
    (*(it++))->pre_physics(dt0);
  }
  
  for (int i = 0 ; i < ndt ; i++)
  {
    for (it = m_objects.begin() ; it != m_objects.end() ; ++it)
    {
      real_do_timestep((*it), dt);
    }
  }
  
  // let objects do something... they might delete themselves
  for (it = m_objects.begin() ; it != m_objects.end() ;)
  {
    (*(it++))->post_physics(dt0);
  }
  
  // kick the particle engine
  Particle_engine::instance()->move_particles(dt0);
}





