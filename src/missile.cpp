/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file missile.cpp
*/

#include "missile.h"

#include "types.h"
#include "aerofoil.h"
#include "fuselage.h"
#include "glider.h"
#include "tree.h"
#include "misc.h"
#include "physics.h"
#include "sss.h"
#include "config.h"
#include "environment.h"
#include "joystick.h"
#include "explosion.h"
#include "log_trace.h"
#include "audio.h"
#include "particle_engine.h"
#include "remote_sss_iface.h"
#include <string>
using namespace std;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Missile::Missile(const Position & pos, 
                 const Velocity & vel, 
                 const Orientation & orient, 
                 const Rotation & rot,
                 float length,
                 float mass,
                 const Object * parent,
                 bool local)
  :
  Object(pos),
  m_mass(mass),
  sec_running(0),
  max_dist(length),
  m_parent(parent),
  m_local(local),
  m_last_update_msg(0),
  m_die(false),
  m_ambient_wind(0)
{
  TRACE_METHOD_ONLY(1);
  set_vel(vel);
  set_rot(rot);
  set_orient(orient);
  // set up physical properties
  float radius = length/12.0f;
  
  m_I_x = m_mass * radius * radius;
  m_I_y = m_mass * length * length * 0.75f * 0.75f;
  m_I_z = m_I_y;
  
  float inc = 0;
  float fin_span = radius;
  float fin_chord = length / 6.0f;

  float x0 = (radius + 0.5f * fin_span) * sin_deg(60);
  float y0 = (radius + 0.5f * fin_span) * cos_deg(60);

  m_aerofoils.push_back(Aerofoil("Missile fin1",
                                 Position(-0.5f*length, 0, radius), 
                                 90, // rotation
                                 inc, // inc
                                 fin_span, // span
                                 fin_chord)); // chord
  
  m_aerofoils.push_back(Aerofoil("Missile fin2",
                                 Position(-0.5f*length,x0,-y0), 
                                 -30, // rotation
                                 inc, // inc
                                 fin_span, // span
                                 fin_chord)); // chord
  
  m_aerofoils.push_back(Aerofoil("Missile fin3",
                                 Position(-0.5f*length,-x0,-y0), 
                                 30, // rotation
                                 -inc, // inc
                                 fin_span, // span
                                 fin_chord)); // chord
  
  Fuselage fuselage("Missile fuselage");
  fuselage.add_point(Position(-length*0.5f, 0, 0), 0);
  fuselage.add_point(Position(-length*0.5f, 0, 0), radius);
  fuselage.add_point(Position( length*0.35f, 0, 0), radius);
  fuselage.add_point(Position( length*0.5f, 0, 0), 0);
  
  m_fuselages.push_back(fuselage);

  // take account of the fuselage aerodynamics
  float span = 0.5f * fuselage.get_average_radius();
  float chord = fuselage.get_length();
  Position mid_point = fuselage.get_mid_point();

  Aerofoil fus1("fuselage1", mid_point, 0, 0, span, chord);
  Aerofoil fus2("fuselage2", mid_point, 90, 0, span, chord);
  m_aerofoils.push_back(fus1);
  m_aerofoils.push_back(fus2);


  Physics::instance()->register_object(this);
  Sss::instance()->add_object(this);
  

  // register for audio
  string missile_audio_file("glider.wav");
  Audio::instance()->register_instance(this, 
                                       missile_audio_file,
                                       2.0f,
                                       2.0f,
                                       100.0f);
  calculate_hard_points();

  m_particle_id = Particle_engine::instance()->register_source(
    PARTICLE_SMOKE_LIGHT,
    1000, // initial number
    get_pos(),
    get_vel(),
    Sss::instance()->config().missile_smoke_alpha_scale,
    0.5f, 10.0f, 1.0f,
    0   // initial rate - gets updated all the time...
    );

  Remote_sss_iface * iface;
  if ( (m_local) && 
       (0 != (iface = Remote_sss_iface::instance())) )
  {
    Remote_sss_msg msg;
    msg.msg_type = Remote_sss_msg::CREATE;
    msg.set_basic_info(get_pos(),
                       get_vel(),
                       get_orient(),
                       get_rot());
    msg.msg.create.object_type = 
      Remote_sss_create_msg::MISSILE;

    // missile-specific info
    msg.msg.create.msg.missile.length = length;
    msg.msg.create.msg.missile.mass = mass;
    
    // send the msg
    iface->send_create_ind(this, msg);
  }
}

Missile::~Missile()
{
  TRACE_METHOD_ONLY(1);
  // Deregister from audio
  Audio::instance()->deregister_instance(this);

  Physics::instance()->deregister_object(this);
  Sss::instance()->remove_object(this);

  // notify the remote end of our demise
  Remote_sss_iface * iface; 
  if ( (m_local) && 
       (0 != (iface = Remote_sss_iface::instance())) )
  {
    Remote_sss_msg msg;
    msg.msg_type = Remote_sss_msg::DESTROY;
    msg.msg.destroy.object_type = 
      Remote_sss_destroy_msg::MISSILE;
    
    // send the msg
    iface->send_destroy_ind(this, msg);
  }

  (void) new Explosion(get_pos(),
                       get_vel(), 
                       70 * get_graphical_bounding_radius());
  Particle_engine::instance()->deregister_source(m_particle_id);
}

void Missile::calculate_hard_points()
{
  unsigned int i, j;
  for (i = 0 ; i < m_aerofoils.size() ; ++i)
  {
    vector<Hard_point> positions = m_aerofoils[i].get_hard_points();
    for (j = 0 ; j < positions.size() ; ++j)
    {
      m_hard_points.push_back(positions[j].pos);
    }
  }
  for (i = 0 ; i < m_fuselages.size() ; ++i)
  {
    vector<Hard_point> positions = m_fuselages[i].get_hard_points();
    for (j = 0 ; j < positions.size() ; ++j)
    {
      m_hard_points.push_back(positions[j].pos);
    }
  }
}


void Missile::calc_new_pos_and_orient(float dt)
{
  if (m_die == true)
    return;
  
  pos += dt * vel;
  
  Orientation new_orient = get_orient();

  new_orient += dt * Matrix(0, rot[2], -rot[1],
                            -rot[2], 0, rot[0],
                            rot[1], -rot[0], 0 ) * orient;
  new_orient.orthonormalise();
  
  set_orient(new_orient);

  // ensure that nothing has gone horribly wrong
  if (validate() == false)
    m_die = true;
}

void Missile::post_physics(float dt)
{
  sec_running += dt;
  // emit particles? maybe update rate  
  const float particle_stop_time = Sss::instance()->config().missile_smoke_time;
  const float particle_max_rate = Sss::instance()->config().missile_smoke_max_rate;
  float rate = (1.0f - sec_running / particle_stop_time) * particle_max_rate;
  if (rate < 0.0f) rate = 0.0f;
  Particle_engine::instance()->set_rate(m_particle_id, rate);

  Particle_engine::instance()->update_source(
    m_particle_id,
    dt,
    get_pos(),
    m_ambient_wind);

  if (sec_running > 30)
    m_die = true;

  // if we're a "ghost", don't do any other behaviour.

  if (m_local == false)
    return;

  if (m_die == true)
  {
    TRACE_FILE_IF(2)
      TRACE("Missile %p expired\n", this);
    delete this;
    return;
  }
  
  if (get_pos()[2] < (get_structural_bounding_radius() + 
                      Environment::instance()->get_z(
                        get_pos()[0], get_pos()[1])) )
  {
    TRACE_FILE_IF(2)
      TRACE("Missile %p hit terrain\n", this);
    delete this;
    return;
  }

  const list<Object *> & objects = 
    Physics::instance()->get_objects();
  typedef list<Object *>::const_iterator Objects_it;
  Objects_it it;
  for (it = objects.begin() ; it != objects.end() ; ++it)
  {
    if ((*it != this) && (*it != m_parent))
    {
      float delta = ((*it)->get_pos() - get_pos()).mag();
      // factor of 2 on our radius makes it easier to hit(!)
      if (delta < (2 * get_structural_bounding_radius() + 
                   (*it)->get_structural_bounding_radius()) )
      {
        TRACE_FILE_IF(2)
          TRACE("Missile %p hit between %p (parent = %p)\n",
                this, 
                *it,
                m_parent);
        
        // if it was a glider, let it respond. Note that it might be a
        // remote glider, in which case it will handle the response in
        // its own way.
        Glider * glider_ptr = dynamic_cast<Glider *>(*it);
        if (glider_ptr)
        {
          glider_ptr->handle_missile_hit(m_mass,
                                         get_pos(),
                                         get_vel());

          // and die
          delete this;
          return;
        }

        Tree * tree_ptr = dynamic_cast<Tree *>(*it);
        if (tree_ptr)
        {
          // hit trees too
          delete this;
          return;
        }
      }
    }
  }
}

void Missile::calc_new_vel_and_rot(float dt,
                                   const Vector & force,
                                   const Vector & moment)
{
  if (m_die == true)
    return;
  
  //=================================================================
  // integrate vel
  //=================================================================
  vel += force * (dt / m_mass);
  
  //=================================================================
  // integrate rot
  //=================================================================
  const Matrix I_inv_body(1/m_I_x, 0, 0, 
                          0, 1/m_I_y, 0,
                          0, 0, 1/m_I_z );
  
  // I_inv_world is the inverse inertia tensor in the world coords
  Matrix I_inv_world = orient * I_inv_body * transpose(orient);
  
  rot += dt * I_inv_world * moment;

  // ensure that nothing has gone horribly wrong
  if (validate() == false)
    m_die = true;
}

void Missile::calc_object_specific_force_and_moment(Vector & force, 
                                                    Vector & moment)
{
  force.set_to(0);
  moment.set_to(0);
  
  Vector3 temp_force;
  Vector3 temp_moment;
  
  get_aero_force_and_moment(temp_force, temp_moment);
  force += temp_force;
  moment += temp_moment;
  
  get_structure_force_and_moment(temp_force, temp_moment);
  force += temp_force;
  moment += temp_moment;
  
  get_engine_force_and_moment(temp_force, temp_moment);
  force += temp_force;
  moment += temp_moment;
  
  //================== gravity ========================================
  force += 
    Vector3(0, 0, -Sss::instance()->config().gravity * m_mass);
}

void Missile::get_aero_force_and_moment(Vector3 & force,
                                        Vector3 & moment)
{
  force.set_to(0.0f);
  moment.set_to(0.0f);

  if (m_die == true)
    return;
  
  const Velocity wind_static_rel(-dot(get_vel(), get_vec_i_d()),
                                 -dot(get_vel(), get_vec_j_d()),
                                 -dot(get_vel(), get_vec_k_d()) );
  
  // accumulate the forces and moments into:
  Vector3 l_force(0); // forces in local frame
  Vector3 l_moment(0); // moments in local frame
  
  // temporary vectors
  Vector3 linear_force; // use fn parameter as a temporary
  Position linear_position;
  Vector3 pitch_force; // use parameter as a temporary
  Position pitch_position;
  
  // rotation rate in the body frame
  const Vector3 rot_body = transpose(get_orient()) * get_rot();

  // We approximate a bit here - the wind gradient across the missile
  // is likely to be very small, so we assume it's zero. Who's looking
  // anyway?!

  // NOTE we store the ambient wind for the smoke...
  // we need to work out the relative wind velocity due to the
  // glider's rotation.
  m_ambient_wind = Environment::instance()->get_wind(get_pos(), 
                                                     this);
  const Velocity ambient_wind_rel(dot(m_ambient_wind, get_vec_i_d()),
                                  dot(m_ambient_wind, get_vec_j_d()),
                                  dot(m_ambient_wind, get_vec_k_d()) );
    
  const Velocity wind_rel = ambient_wind_rel + wind_static_rel;

  for (unsigned int i = 0 ; i < m_aerofoils.size() ; ++i)
  {
    const Position & pos = m_aerofoils[i].get_pos();
    
    // do the cross in the opposite direction, so we don't have to
    // negate the result
    Velocity wind_rel_rot = cross(pos, rot_body);
    
    Velocity wind_rel_aerofoil = wind_rel + wind_rel_rot;
    
    static Joystick joystick(10);
    float density = Environment::instance()->get_air_density(get_pos());
    m_aerofoils[i].get_lift_and_drag(wind_rel_aerofoil, 
                                     joystick, // ignored anyway
                                     linear_force, linear_position,
                                     pitch_force,  pitch_position,
                                     density);
    
    // accumulate forces and moments
    l_force = l_force + linear_force;
    l_moment = l_moment + cross(linear_position, linear_force);
    // bit of a hack here
    l_moment = l_moment + cross(pitch_position, pitch_force);
  }
  
  // convert the local vectors to world coords
  moment = get_orient() * l_moment;
  force  = get_orient() * l_force;
}

void Missile::get_structure_force_and_moment(Vector3 & force,
                                             Vector3 & moment)
{
  force.set_to(0);
  moment.set_to(0);
  Vector3 temp;
  
  if (get_pos()[2] < 
      Environment::instance()->get_z(get_pos()[0], get_pos()[1]) + 
      get_structural_bounding_radius())
  {
    Position terrain_pos;
    Vector3 normal;
    
    const float force_scale = 60 * m_mass;
    // ensure that the reset position is at around z = 0, whatever the mass etc
    // assume 2 points count.
    const float descent_offset = m_mass * 
      Sss::instance()->config().gravity / (2 * force_scale);
    
    for (vector<Position>::iterator pos_it = m_hard_points.begin() ;
         pos_it != m_hard_points.end() ; 
         ++pos_it)
    {
      Position point_pos = get_pos() + get_orient() * *pos_it;
      
      Environment::instance()->get_local_terrain(point_pos, terrain_pos, normal);
      float descent = -dot(point_pos-terrain_pos, normal);
      descent += descent_offset;
      
      if (descent > 0)
      {
        // perpendicular part
        float rebound_force = descent*force_scale;
        
        float perp_vel = dot(get_vel(), normal);
        if (perp_vel > 0)
        {
          rebound_force = sss_max(rebound_force * (1 - perp_vel*0.7f), 0.0f);
        }
        
        temp = rebound_force * normal;
        force += temp;
        moment += cross(point_pos - get_pos(), temp);
        
        // parallel part
        Vector3 par_vel = get_vel() - dot(get_vel(), normal) * normal;
        
//        float friction_force = 0.4f * rebound_force;
        float friction_force = 0 * rebound_force;
        if (friction_force > 0.001f) // ensures the normalise is OK
        {
          friction_force = sss_min(friction_force, 40.0f);
          par_vel.normalise();
          temp = par_vel* (-friction_force);
          force += temp;
          moment += cross(point_pos - get_pos(), temp);
        }
      } // descent > 0
    } // loop over hard points
  } // missile near ground
}


void Missile::get_engine_force_and_moment(Vector3 & force,
                                          Vector3 & moment)
{
  Vector3 l_force(0);
  float scale = (sec_running < 0.2f) ? 30 : 5;

  l_force[0] = m_mass * Sss::instance()->config().gravity * 
    scale;
  
  moment.set_to(0);
  
  // now convert local force vector into vector in global ref frame
  force = get_orient() * l_force;
}

void Missile::draw(Draw_type draw_type)
{
  if (m_die == true)
    return;
  
  static GLuint list_num = glGenLists(1);
  static bool init = false;
  
  if (init == false)
  {
    init = true;
    glDeleteLists(list_num, 1);
    list_num = glGenLists(1);
    glNewList(list_num, GL_COMPILE);
    
    unsigned int i;
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    for ( i = 0 ; i < m_fuselages.size() ; i++)
    {
      m_fuselages[i].draw(draw_type == NORMAL ? Fuselage::NORMAL : Fuselage::SHADOW);
    }
    
    // only have 3 fins (don't count the fuselage aerofoils - yuk!)
    for ( i = 0 ; i < 3 ; i++)
    {
      m_aerofoils[i].draw_non_moving(draw_type == NORMAL ? Aerofoil::NORMAL : Aerofoil::SHADOW);
    }
    
    glDisable(GL_CULL_FACE);
    
    glEndList();
  }
  
  glPushMatrix();
  basic_draw();
  
  glCallList(list_num);
  glPopMatrix();
}

/*! Works through the list of hard points and calculates the maximum
  distance of any from the origin. */
void Missile::calculate_max_dist()
{
  unsigned int i, j;
  max_dist = 0;
  // aerofoils
  for (i = 0 ; i < m_aerofoils.size() ; ++i)
  {
    vector<Hard_point> positions = m_aerofoils[i].get_hard_points();
    for (j = 0 ; j < positions.size() ; ++j)
    {
      max_dist = sss_max(max_dist, positions[j].pos.mag());
    }
  }
  // fuselages
  for (i = 0 ; i < m_fuselages.size() ; ++i)
  {
    vector<Hard_point> positions = m_fuselages[i].get_hard_points();
    for (j = 0 ; j < positions.size() ; ++j)
    {
      max_dist = sss_max(max_dist, positions[j].pos.mag());
    }
  }
  
  // scale it up a bit just to be safe...
  max_dist *= 1.1f;
}

void Missile::send_remote_update()
{
  // notify remote simulations
  Remote_sss_iface * iface;
  if ( (m_local) && 
       (0 != (iface = Remote_sss_iface::instance())) )
  {
    Remote_sss_msg msg;
    msg.msg_type = Remote_sss_msg::UPDATE;
    msg.set_basic_info(get_pos(),
                       get_vel(),
                       get_orient(),
                       get_rot());
    msg.msg.update.object_type = Remote_sss_update_msg::MISSILE;
    
    // send the msg
    iface->send_update_ind(this, msg);
  }
}

void Missile::pre_physics(float dt)
{
  if (!m_last_update_msg)
    return;
  
  // Have one!
  m_last_update_msg->get_basic_info(pos, vel, orient, rot);
  set_pos(pos);
  set_vel(vel);
  set_orient(orient);
  set_rot(rot);
  
  // paused was already done
  // assume that the jitter is small and we can just integrate
  // linearly by dt
  if ( (Sss::instance()->config().jitter_correct == true) ||
       (Sss::instance()->config().lag_correct == true) )
  {
    Physics::instance()->do_timestep(this, m_last_update_dt);
  }
  
  // used the msg
  delete m_last_update_msg;
  m_last_update_msg = 0;
}

void Missile::recv_remote_update(
  Remote_sss_msg & msg,
  float dt) // dt indicates how late the message is
{
  // just store it - overwrite any existing msg
  if (!m_last_update_msg)
    m_last_update_msg = new Remote_sss_msg;
  *m_last_update_msg = msg;
  m_last_update_dt = dt;
}
