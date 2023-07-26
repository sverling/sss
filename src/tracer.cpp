/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file tracer.cpp
*/

#include "tracer.h"
#include "environment.h"
#include "sss.h"
#include "renderer.h"
#include "physics.h"
#include <stdlib.h>

Tracer_collection * Tracer_collection::m_instance = 0;

Tracer::Tracer(Object * focus) : Object(0), 
                                 m_radius(1.5f), m_max_hor_dist(150), m_max_ver_dist(70),
                                 m_focus(focus),
                                 valid(true),
                                 elapsed_time(0),
                                 timestep(0.05f)
{
  init_pos();
  Physics::instance()->register_object(this);
}

Tracer::~Tracer()
{
  Physics::instance()->deregister_object(this);
}
  

void Tracer::init_pos()
{
  bool valid = false;
  float dx, dy, dz;
  dx = dy = dz = 0;

  while (valid == false)
  {
    dx = ( ((float) rand()/RAND_MAX) * 2 - 1) * m_max_hor_dist;
    dy = ( ((float) rand()/RAND_MAX) * 2 - 1) * m_max_hor_dist;
    dz = ( ((float) rand()/RAND_MAX) * 2 - 1) * m_max_ver_dist;

    float terrain_z = Environment::instance()->get_z(m_focus->get_pos()[0]+dx,
                                                     m_focus->get_pos()[1]+dy);

    valid = (m_focus->get_pos()[2] + dz) > terrain_z;
  }

  set_pos(Position(m_focus->get_pos() + Position(dx, dy, dz)));
}

//! don't actually modify orient (it is unused)
void Tracer::calc_new_pos_and_orient(float dt)
{
  elapsed_time += dt;
  if (elapsed_time > timestep)
  {
    elapsed_time -= timestep;
    vel = Environment::instance()->get_wind(pos, this);
  }

  const float offset = 0.5f;
  pos += dt * vel;

  // Ensure we don't get caught up in the still air right next to the
  // ground
  if ( pos[2] < (offset + Environment::instance()->get_z(pos[0], pos[1])) )
  {
    pos[2] = offset + Environment::instance()->get_z(pos[0], pos[1]);
  }

  // reset ourselves if we get too far from our focus
  if ( ( fabs(pos[0] - m_focus->get_pos()[0]) > m_max_hor_dist ) ||
       ( fabs(pos[1] - m_focus->get_pos()[1]) > m_max_hor_dist ) ||
       ( fabs(pos[2] - m_focus->get_pos()[2]) > m_max_ver_dist ) )
  {
    init_pos();
    vel = Environment::instance()->get_wind(pos, this);
  }

  if (vel.mag2() > 0.01f)
  {
    Vector v_x = vel.normalise();
    // Choose an arbitrary direction that is unlikely to be parallel
    // to the wind (up is a bad choice!)
    Vector v_y = cross(Vector(0.3f, 0.24f, 1), v_x).normalise();
    Vector v_z = cross(v_x, v_y);
    
    set_orient(v_x, v_y, v_z);
    valid = true;
  }
  else
  {
    set_orient(Vector(1, 0, 0), Vector(0, 1, 0), Vector(0, 0, 1));
    valid = false;
  }
}

void Tracer::draw(Draw_type draw_type)
{
  static GLuint list_num = glGenLists(2);
  static bool init = false;

  if (init == false)
  {
    init = true;
    glDeleteLists(list_num, 2);
    list_num = glGenLists(2);
    // the arrow
    glNewList(list_num, GL_COMPILE);
    glRotatef(90, 0, 1, 0);
    glTranslatef(0, 0, -m_radius);
    gluCylinder(Renderer::instance()->quadric(), m_radius/10, m_radius/10, 1.7f*m_radius, 8, 2);
    glTranslatef(0, 0, m_radius*1.5f);
    gluCylinder(Renderer::instance()->quadric(), m_radius/3, 0, m_radius*0.5f, 8, 2);
    glEndList();

    // the stationary blob
    glNewList(list_num+1, GL_COMPILE);
    gluSphere(Renderer::instance()->quadric(), m_radius/10, 4, 4);
    glEndList();
  }

  glPushMatrix();
  basic_draw();

  if (valid == true)
  {
    float g = 0.5f + vel[2]/7.0f;
    glColor3f(1-g, g, 0.5f);
    glCallList(list_num);
  }
  else
  {
    glColor3f(1, 0, 0);
    glCallList(list_num+1);
  }
  glPopMatrix();
}

void Tracer_collection::add_tracer(Object * focus, int num)
{
  while (num-- > 0)
  {
    Tracer * new_tracer = new Tracer(focus);
    tracers.push_back(new_tracer);
    Sss::instance()->add_object(new_tracer);
  }
}

/*!
  Just removes the last so-many tracers added, irrespective of their focus.
*/
void Tracer_collection::remove_tracer(int num)
{
  while ( (num-- > 0) && (!tracers.empty()) )
  {
    Tracer * tracer = tracers.back();
    Sss::instance()->remove_object(tracer);
    tracers.pop_back();
    delete tracer;
  }
}

void Tracer_collection::remove_all_tracers()
{
  remove_tracer(tracers.size());
}

Tracer_collection::~Tracer_collection()
{
  list<Tracer *>::iterator it;
  
  for (it = tracers.begin() ; it != tracers.end() ; ++it)
  {
    delete *it;
  }
  m_instance = 0;
}
