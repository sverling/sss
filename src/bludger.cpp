/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file bludger.cpp
*/

#include "bludger.h"
#include "renderer.h"
#include "physics.h"
#include "sss.h"
#include "environment.h"
#include "body.h"
#include "log_trace.h"

#include <algorithm>
using namespace std;

Bludger::Bludger(Position p, bool local) 
  : 
  Object(p), m_mass(50), m_radius(0.15), m_local(local)
{
  m_time_remaining = 10; // 10 seconds
  m_current_object = 0;
  set_pos(p);
  Physics::instance()->register_object(this);
  Sss::instance()->add_object(this);
}

Bludger::~Bludger()
{
  Physics::instance()->deregister_object(this);
  Sss::instance()->remove_object(this);
  
}

void Bludger::calc_new_pos_and_orient(float dt)
{
  pos = pos + dt * vel;
}

Object * Bludger::choose_new_target(list<Object *> & objects)
{
  int num = objects.size();
  int i = (int) ranged_random(0, num-0.001);
  list<Object *>::const_iterator it = objects.begin();
  for (int j = 0 ; j < i ; ++j)
  {
    ++it;
  }
  return *it;
}

void Bludger::calc_object_specific_force_and_moment(Vector & force, 
                                                    Vector & moment)
{
  list<Object *> objects = Physics::instance()->get_objects();
  
  if ( (m_time_remaining < 0) ||
       (find(objects.begin(), objects.end(), m_current_object) == objects.end()) )
  {
    TRACE(" Bludger %p finding new object\n", this);
    m_current_object = this;
    while ( (m_current_object == this) ||
//            (dynamic_cast<const Body *> (m_current_object)) ||
            (dynamic_cast<const Body *> (m_current_object)) )// ignore some objects
    {
      m_current_object = choose_new_target(objects);
    }
    m_time_remaining = ranged_random(5, 20);
    
    TRACE("New object = %p\n", m_current_object);
  }
  
  force = m_current_object->get_pos() - get_pos();
  force.normalise();
  float force_mag = ranged_random(50, 150) * m_mass;
  force = force * force_mag;  
  
}

void Bludger::calc_new_vel_and_rot(float dt,
                                   const Vector & force,
                                   const Vector & moment)
{
  vel = vel + force * (dt / m_mass);
  float speed = get_vel().mag();
  if (speed > 29)
  {
    set_vel(get_vel() * 29/speed);
  }
  
}

void Bludger::post_physics(float dt)
{
  m_time_remaining -= dt;
  if (get_pos()[2] < 
      Environment::instance()->get_z(get_pos()[0], get_pos()[1]) + 
      get_structural_bounding_radius())
  {
    pos[2] = (Environment::instance()->get_z(get_pos()[0], get_pos()[1]) + 
              get_structural_bounding_radius());
    if (vel[2] < 0)
      vel[2] *= -1;
  }
}

void Bludger::draw(Draw_type draw_type)
{
  static GLuint list_num = glGenLists(1);
  static bool init = false;
  
  if (init == false)
  {
    init = true;
    glDeleteLists(list_num, 1);
    list_num = glGenLists(1);
    glNewList(list_num, GL_COMPILE);
    gluSphere(Renderer::instance()->quadric(), m_radius, 8, 8);
    glEndList();
  }
  
  glPushMatrix();
  basic_draw();
  glColor3f(0.2, 0.2, 0.2);
  glCallList(list_num);
  glPopMatrix();
}
