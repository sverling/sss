/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file body.cpp
*/

#include "body.h"
#include "graphics_3ds.h"
#include "environment.h"
#include "renderer.h"
#include "sss.h"
#include "config.h"
#include "physics.h"
#include "log_trace.h"
#include "criticaldamping.h"

#include "sss_glut.h"

#include <string>
using namespace std;

Body::Body(Position p,  const Object * target,  bool local) 
  : 
  Object(p), 
  m_target(target), 
  torso_length(0.7f), 
  torso_width(0.5f), torso_depth(0.25f),
  leg_length(0.6f), arm_length(0.6f), foot_length(0.3f), 
  foot_depth(0.1f), head_radius(0.15f), 
  leg_width(0.1f), arm_width(0.1f), neck_length(0.2f), 
  eye_height(foot_depth + leg_length + torso_length + neck_length + head_radius),
  m_move_decay_time(0.2f),
  m_max_vel(12.0f),
  m_accel(0),
  m_eye_target(p + orient.get_col(0)),
  m_eye_target_rate(0.0f),
  m_local(local),
  m_model_3ds(0),
  m_model_3ds_cull_backface(false)
{
  TRACE_METHOD_ONLY(1);
  
  Environment::instance()->set_z(p);
  set_pos(p);
  // get notified that we need to update our orientation etc
  Physics::instance()->register_object(this);
  
  string body_3ds_file = Sss::instance()->config().body_3ds_file;
  m_model_3ds_cull_backface = Sss::instance()->config().body_3ds_cull_backface;
  
  if (body_3ds_file != "none")
  {
    //the creation of the graphics object for 3ds version of Body
    m_model_3ds = new Graphics_3ds(body_3ds_file.c_str());
  }
}

void Body::move(Move_dir dir, bool start)
{
  const float accel_scale = 10.0f;
  
  switch (dir)
  {
  case NORTH:
    m_accel[1] = start ? accel_scale : 0;
    return;
  case SOUTH:
    m_accel[1] = start ? -accel_scale : 0;
    return;
  case WEST:
    m_accel[0] = start ? -accel_scale : 0;
    return;
  case EAST:
    m_accel[0] = start ? accel_scale : 0;
    return;
  default:
    break;
  }
  // lets hope it's a motion relative to the target
  Position displacement = m_target->get_pos() - get_pos();
  if (start)
  {
    displacement[2] = 0;
    displacement = accel_scale * displacement.normalise();
  }
  else
  {
    displacement = 0;
  }
  
  switch (dir)
  {
  case TOWARDS:
    m_accel = displacement;
    break;
  case AWAY:
    m_accel = -displacement;
    break;
  case RIGHT:
    m_accel = gamma(-90.0F) * displacement;
    break;
  case LEFT:
    m_accel = gamma(90.0F) * displacement;
    break;
  default:
    TRACE("Unknown direction!\n");
    abort();
  }
}

void Body::calc_new_pos_and_orient(float dt)
{
  // Update the position/vel based on the acceleration
  vel += m_accel * dt;
  float speed = vel.mag();
  if (speed > m_max_vel)
  {
    vel = vel * m_max_vel/speed;
    speed = m_max_vel;
  }
  
  
  pos += vel * dt;
  Environment::instance()->set_z(pos);
  
  // friction
  if (m_accel.mag() < 0.1f)
  {
    if (speed > 0.01f)
    {
      float vel_mag = speed;
      vel_mag -= m_max_vel * dt / m_move_decay_time;
      if (vel_mag < 0.05f)
        vel_mag = 0.0f;
      vel = vel * vel_mag / speed;
    }
    else
    {
      vel = 0;
    }
  }
  
  Position from_pos = get_pos() + Position(0,0,eye_height);
  float dx = m_target->get_pos()[0] - from_pos[0];
  float dy = m_target->get_pos()[1] - from_pos[1];
  
  // i is just (horizontally) towards the target
  // k is vertical
  // j is perpendicular to i and k
  
  Vector vec_i = Vector(dx, dy, 0).normalise();
  Vector vec_k = Vector(0, 0,1);
  Vector vec_j = cross(vec_k, vec_i).normalise();
  
  set_orient(vec_i, vec_j, vec_k);
}

static void draw_cuboid(float x, float y, float z)
{
  glBegin(GL_QUADS);
  // bottom
  glNormal3f(0,0,-1);
  glVertex3f(0,0,0);
  glVertex3f(x,0,0);
  glVertex3f(x,y,0);
  glVertex3f(0,y,0);
  // top
  glNormal3f(0,0,1);
  glVertex3f(0,0,z);
  glVertex3f(x,0,z);
  glVertex3f(x,y,z);
  glVertex3f(0,y,z);
  // back
  glNormal3f(-1,0,0);
  glVertex3f(0,0,0);
  glVertex3f(0,y,0);
  glVertex3f(0,y,z);
  glVertex3f(0,0,z);
  // front   
  glNormal3f(1,0,0);
  glVertex3f(x,0,0);
  glVertex3f(x,y,0);
  glVertex3f(x,y,z);
  glVertex3f(x,0,z);
  // left    
  glNormal3f(0,1,0);
  glVertex3f(0,y,0);
  glVertex3f(x,y,0);
  glVertex3f(x,y,z);
  glVertex3f(0,y,z);
  // right   
  glNormal3f(0,-1,0);
  glVertex3f(0,0,0);
  glVertex3f(x,0,0);
  glVertex3f(x,0,z);
  glVertex3f(0,0,z);
  
  glEnd();
  
}

// Yuk!! One day I'll find a decent model...
void Body::draw_body(Draw_type draw_type)
{
  const int n_slices = 12;
  if (draw_type == NORMAL)
    glColor4f(1,1,0,1);
  
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  
  // draw the feet
  //left
  glPushMatrix();
  glTranslatef(0, torso_width/4 - leg_width/2, 0);
  draw_cuboid(foot_length,leg_width,leg_width);
  glPopMatrix();
  
  //right
  glPushMatrix();
  glTranslatef(0, -torso_width/4 - leg_width/2, 0);
  draw_cuboid(foot_length,leg_width,foot_depth);
  glPopMatrix();
  
  // draw the legs
  //left
  glPushMatrix();
  glTranslatef(0, torso_width/4, foot_depth);
  gluCylinder(Renderer::instance()->quadric(),
              leg_width/2, leg_width/2, leg_length, n_slices, 2);
  glPopMatrix();
  //right
  glPushMatrix();
  glTranslatef(0, -torso_width/4, foot_depth);
  gluCylinder(Renderer::instance()->quadric(),
              leg_width/2, leg_width/2, leg_length, n_slices, 2);
  glPopMatrix();
  
  // draw the torso
  glPushMatrix();
  glTranslatef(0,0, foot_depth + leg_length );
  gluCylinder(Renderer::instance()->quadric(),
              torso_width/2, torso_width/1.8f, torso_length, n_slices, 2);
  glPopMatrix();
  
  // draw the arms
  //left
  glPushMatrix();
  glTranslatef(0, torso_width/1.8f, foot_depth + leg_length + torso_length - arm_length);
  gluCylinder(Renderer::instance()->quadric(),
              arm_width/2, arm_width/2, arm_length, n_slices, 2);
  glPopMatrix();
  //right
  glPushMatrix();
  glTranslatef(0, -torso_width/1.8f, foot_depth + leg_length + torso_length - arm_length);
  gluCylinder(Renderer::instance()->quadric(),
              arm_width/2, arm_width/2, arm_length, n_slices, 2);
  glPopMatrix();
  
  // draw the neck
  glPushMatrix();
  glTranslatef(0, 0, foot_depth + leg_length + torso_length);
  glutSolidCone(torso_width/1.8f, neck_length*1.3f, n_slices, 2);
  glPopMatrix();
  
  // and the head
  glPushMatrix();
  glTranslatef(0, 0, foot_depth + leg_length + torso_length + neck_length + head_radius);
  glutSolidSphere(head_radius, n_slices, n_slices);
  glPopMatrix();
  
  glDisable(GL_CULL_FACE);
  
}

void Body::draw(Draw_type draw_type)
{
  if (m_model_3ds)
  {
    basic_draw();
    m_model_3ds->draw(Graphics_3ds::NORMAL);
  }
  else
  {
    static GLenum current_shade_model = GL_FLAT;
    static GLuint list_num = glGenLists(2);
    
    if ( (Sss::instance()->config().shade_model != current_shade_model)
      )
    {
      current_shade_model = Sss::instance()->config().shade_model;
      glShadeModel(Sss::instance()->config().shade_model);
      glDeleteLists(list_num, 2);
      list_num = glGenLists(2);
      
      glNewList(list_num, GL_COMPILE);
      draw_body(NORMAL);
      glEndList();
      glNewList(list_num+1, GL_COMPILE);
      draw_body(SHADOW);
      glEndList();
    }
    
    glPushMatrix();
    basic_draw();
    
    if (draw_type == NORMAL)
    {
      if ( &Sss::instance()->eye() != this)
        glCallList(list_num);
    }
    else
    {
      glCallList(list_num+1);
    }
    
    glPopMatrix();
  }
}

void Body::post_physics(float dt)
{
  if (m_target)
  {
    float lag_time = Sss::instance()->config().body_view_lag_time;
    Position target_pos = m_target->get_pos();
    target_pos += 0.5f * lag_time * m_target->get_vel();
    SmoothCD(m_eye_target, m_eye_target_rate, dt, target_pos, lag_time);
  }

  if (Sss::instance()->config().use_terragen_terrain)
  {
    pos = Sss::instance()->config().terragen_pos - 
      (eye_height * get_vec_k_d() + head_radius * get_vec_i_d());
  }
}


Position Body::get_eye() const
{
  return pos + eye_height * get_vec_k_d() + head_radius * get_vec_i_d();
}

Position Body::get_eye_target() const
{
  return m_eye_target;
}

Vector Body::get_eye_up() const
{
  return get_vec_k_d();
}

void Body::set_target(const Object * new_target)
{
  TRACE_FUNCTION_ONLY(2);
  m_target = new_target;
}

void Body::set_eye(bool is_eye)
{
  if (is_eye)
    Sss::instance()->set_config().align_billboards_to_eye_dir = false;
}

