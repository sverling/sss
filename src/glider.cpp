/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file glider.cpp
*/

#include "glider.h"
#include "glider_aero.h"
#include "glider_structure.h"
#include "glider_graphics.h"
#include "glider_power.h"
#include "config_file.h"
#include "environment.h"
#include "audio.h"
#include "config.h"
#include "sss.h"
#include "physics.h"
#include "joystick.h"
#include "log_trace.h"
#include "missile.h"
#include "vario.h"
#include "race_manager.h"
#include "remote_sss_iface.h"
#include "body.h"
#include "criticaldamping.h"
#include "sss_glut.h"
#include "texture.h"

#include <stdlib.h>
using namespace std;

#ifndef unix
#define strcasecmp _stricmp
#endif


//==============================================================
// Glider
//==============================================================
Glider::Glider(Config_file & config_file,
               Position p,
               float speed,
               Joystick * joystick,
               bool local)
  : 
  Object(p),
  aero(0),
  structure(0),
  graphics(0),
  power(0),
  joystick(joystick),
  m_original_joystick(joystick),
  m_paused(false),
  m_eye(false),
  m_eye_type(CHASE),
  m_local(local),
  do_reset(false),
  m_eye_target(0.0f),
  m_eye_target_rate(0.0f),
  m_eye_offset(0.0f),
  m_desired_eye_offset(0.0f),
  m_eye_offset_rate(0.0f),
  m_cockpit_texture(0),
  m_last_update_msg(0),
  vario(0)
{
  TRACE_METHOD_ONLY(1);
  initialise_from_config(config_file, false); // don't send the config_file update
  // get audio properties if they're there, otherwise use defaults
  audio.audio_file = "glider.wav";
  
  audio.pitch_offset = 1.0f;
  audio.vol_offset = 0.0f;
  audio.pitch_scale_per_vel = 0.026f;
  audio.vol_scale_per_vel = 1.0f;
  audio.inside_vol_scale = 0.1f;
  config_file.get_value("audio_file", audio.audio_file);
  config_file.get_value("pitch_offset", audio.pitch_offset);
  config_file.get_value("vol_offset", audio.vol_offset);
  config_file.get_value("pitch_scale_per_vel", audio.pitch_scale_per_vel);
  config_file.get_value("vol_scale_per_vel", audio.vol_scale_per_vel);
  config_file.get_value("inside_vol_scale", audio.inside_vol_scale);
  
  // register for audio
  Audio::instance()->register_instance(this, 
                                       audio.audio_file,
                                       audio.vol_offset,
                                       audio.pitch_offset,
                                       25.0f);
  // if there is a race going on, register with it
  if (Race_manager::get_instance())
  {
    TRACE_FILE_IF(2)
      TRACE("Glider: registering with Race manager\n");
    Race_manager::get_instance()->register_object(this);
  }
  
  // set  the initial positions
  Environment::instance()->set_z(p);
  m_orig_pos = p;
  m_orig_speed = speed;
  reset();
  
  Physics::instance()->register_object(this);
  
  // Create the "real" joystick
  m_real_joystick = new Joystick(joystick->get_num_channels());
  
  // Create the glider veriometer if enabled -- Added by Esteban Ruiz on August 2006  - cerm78@gmail.com
  if (Sss::instance()->config().vario_enabled)
  {
    // Create variometer only in main glider if parameters says so
    bool main_only = Sss::instance()->config().vario_main_glider_only;
    if (!main_only || (main_only && (int)&Sss::instance()->glider() == 0))
      vario = new Vario(config_file, this);
  }

  // notify remote simulations of the glider (if there are any)
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
      Remote_sss_create_msg::GLIDER;
    strcpy(msg.msg.create.msg.
           glider.glider_file,
           config_file.get_file_name().c_str());
    
    // send the msg
    iface->send_create_ind(this, msg);
  }
  
  
}

void Glider::initialise_from_config(Config_file & config_file,
                                    bool send_config_file_update)
{
  TRACE_METHOD_ONLY(1);
  config_file.get_value_assert("mass", mass);
  config_file.get_value_assert("I_x",  I_x);
  config_file.get_value_assert("I_y",  I_y);
  config_file.get_value_assert("I_z",  I_z);
  
  string aero_file;
  string structure_file;
  string graphics_file;
  string engine_file;
  
  config_file.get_value_assert("aero_file",      aero_file);
  config_file.get_value_assert("structure_file", structure_file);
  config_file.get_value_assert("graphics_file",  graphics_file);
  config_file.get_value_assert("engine_file",    engine_file);
  
  string aero_type;
  string structure_type;
  string graphics_type;
  string engine_type;
  
  config_file.get_value_assert("aero_type",      aero_type);
  config_file.get_value_assert("structure_type", structure_type);
  config_file.get_value_assert("graphics_type",  graphics_type);
  config_file.get_value_assert("engine_type",    engine_type);
  
  if (aero)
    delete aero;
  if (structure)
    delete structure;
  if (graphics)
    delete graphics;
  if (power)
    delete power;
  
  aero       = Glider_aero::create(aero_file, aero_type, *this);
  structure  = Glider_structure::create(structure_file, structure_type, *this);
  graphics   = Glider_graphics::create(graphics_file, graphics_type, *this);
  power     = Glider_power::create(engine_file, engine_type, *this);
  
  // This must be done here because the crrcsim may update the values.
  I_inv_body = Matrix(1/I_x, 0, 0, 
                      0, 1/I_y, 0,
                      0, 0, 1/I_z );
  
  if (send_config_file_update)
  {
    // notify remote simulations
    Remote_sss_iface * iface;
    if ( (m_local) && 
         (0 != (iface = Remote_sss_iface::instance())) )
    {
      Remote_sss_msg msg;
      msg.msg_type = Remote_sss_msg::UPDATE;
      msg.msg.update.object_type = 
        Remote_sss_update_msg::GLIDER;    
      msg.msg.update.msg.glider.update_type = 
        Remote_sss_update_glider_msg::CONFIG_FILE;
      strcpy(msg.msg.update.msg.glider.update.config_file.glider_file,
             config_file.get_file_name().c_str());
      iface->send_update_ind(this, msg);
    }
  }
}

//==============================================================
// Glider
//==============================================================
Glider::~Glider()
{
  TRACE_METHOD_ONLY(1);
  // Deregister from audio
  Audio::instance()->deregister_instance(this);
  
  // if there is a race going on, deregister from it
  if (Race_manager::get_instance())
    Race_manager::get_instance()->deregister_object(this);
  
  // notify the remote end of our demise
  Remote_sss_iface * iface; 
  if ( (m_local) && 
       (0 != (iface = Remote_sss_iface::instance())) )
  {
    Remote_sss_msg msg;
    msg.msg_type = Remote_sss_msg::DESTROY;
    msg.msg.destroy.object_type = 
      Remote_sss_destroy_msg::GLIDER;
    
    // send the msg
    iface->send_destroy_ind(this, msg);
  }
  
  Physics::instance()->deregister_object(this);
  
  delete aero;
  delete structure;
  delete graphics;
  delete power;
  delete m_real_joystick;
  delete vario;
}


//==============================================================
// reset
//==============================================================
void Glider::reset(const Body * body)
{
  TRACE_METHOD_ONLY(1);
  if (body != 0)
  {
    m_orig_pos = body->get_pos();
    Environment::instance()->set_z(m_orig_pos);
  }
  
  // get direction from wind
  Environment::instance()->set_z(pos);
//  pos[2] += Sss::instance()->config().glider_alt;
  
  Vector dir = -Environment::instance()->get_ambient_wind(pos);
  dir[2] = 0;
  if (dir.mag2() > 0.0001f)
    dir.normalise();
  else
    dir = Vector(-1, 0, 0);
  
  float orient_dir = atan2_deg(dir[1], dir[0]);

  set_orient(gamma(orient_dir));
  
  // set velocity - note dir can be used
  set_vel(dir * m_orig_speed);
  
  // and the position
  float dist_offset = 
    0.5f + get_structural_bounding_radius();
  if (body)
    dist_offset += body->get_structural_bounding_radius();
  else
    dist_offset += 1.5;
  
  set_pos(m_orig_pos + dist_offset * dir);
  
  float ground_z = Environment::instance()->get_z(pos[0], pos[1]);
  if (pos[2] < (ground_z + Sss::instance()->config().glider_alt))
    pos[2] = ground_z + Sss::instance()->config().glider_alt;
  
  // and rotation
  set_rot(Rotation(0));
}

//==============================================================
// set_altitude
//==============================================================
void Glider::set_altitude(float alt)
{
  pos[2] = Environment::instance()->get_z(pos[0], pos[1]) + alt;
}

//==============================================================
// get_eye
//==============================================================
Position Glider::get_eye() const
{
  switch (m_eye_type)
  {
  case PILOT:
    return (pos + 0.1 * get_vec_i_d() + 0.1 * get_vec_k_d());
  case CHASE:
  case CHASE_FAR:
  case HOLD:
    return chase_pos;
  }
  TRACE("Unhandled switch: %d\n", m_eye_type);
  return pos;
}

//==============================================================
// get_eye_up
//==============================================================
Vector Glider::get_eye_up() const 
{
  switch (m_eye_type)
  {
  case PILOT:
  case CHASE:
    return get_vec_k_d();
  case CHASE_FAR:
  case HOLD:
    return Vector(0, 0, 1);
  }
  TRACE("Unhandled switch: %d\n", m_eye_type);
  return Vector(0, 0, 1);  
}
//==============================================================
// get_eye_target
//==============================================================
Position Glider::get_eye_target() const
{
  return m_eye_target;
}


//==============================================================
// is_eye
//==============================================================
void Glider::set_eye(bool is_eye)
{
  if (is_eye)
  {
    if (m_eye)
    {
      // already an eye, so toggle
      switch (m_eye_type)
      {
      case CHASE: 
        m_eye_type = CHASE_FAR; 
        Sss::instance()->set_config().align_billboards_to_eye_dir = false;
        break;
      case CHASE_FAR: 
        m_eye_type = HOLD; 
        Sss::instance()->set_config().align_billboards_to_eye_dir = false;
        break;
      case HOLD: 
        m_eye_type = PILOT; 
        Sss::instance()->set_config().align_billboards_to_eye_dir = true;
        break;
      case PILOT:
        m_eye_type = CHASE; 
        Sss::instance()->set_config().align_billboards_to_eye_dir = true;
        break;
      default: TRACE("Unknown eye type %d\n", m_eye_type); m_eye_type = CHASE; break;
      }
      return;
    }
    // only just been made an eye
    m_eye_type = CHASE;
    chase_pos = get_pos() - get_vec_i_d();
    Sss::instance()->set_config().align_billboards_to_eye_dir = true;
    m_eye = true;
    return;
  }
  m_eye = false;
}


//==============================================================
// post_physics
//==============================================================
void Glider::post_physics(float dt)
{
  TRACE_METHOD_ONLY(3);
  // ensure that nothing has gone horribly wrong
  if (validate() == false)
    reset();
  
  if (do_reset)
  {
    reset();
    do_reset = false;
  }
  
  // Update the audio - we emit a different freq and volume depending
  // on our speed.
  float speed = get_airspeed().mag();
  float rate = audio.pitch_offset + audio.pitch_scale_per_vel * speed;
  Audio::instance()->set_pitch_scale(this, rate);
  float vol = audio.vol_offset + audio.vol_scale_per_vel * speed * speed/100.0;
  if ( (m_eye) && (m_eye_type == PILOT) )
    vol *= audio.inside_vol_scale;
  Audio::instance()->set_vol_scale(this, vol);
  
  // any other actions?
  if ( (m_local) &&
       (get_joystick().get_value(5) > 0.5) )
  {
    float missile_len = get_graphical_bounding_radius()*0.25;
    float missile_mass = missile_len * (missile_len * 0.8) * (missile_len * 0.8);
    (void) new Missile(get_pos(),
                       get_vel(),
                       get_orient(),
                       get_rot(),
                       missile_len,
                       missile_mass,
                       this);
    get_joystick().set_value(5, 0);
  }

  static float eye_offset_time = 0.3f;
  SmoothCD(m_eye_offset, m_eye_offset_rate, dt, m_desired_eye_offset, eye_offset_time);

  if ( (m_eye) && ( (m_eye_type == CHASE) || (m_eye_type == CHASE_FAR)) )
  {
    float chase_dist, ground_offset, frac_scale;
    if (m_eye_type == CHASE)
    {
      chase_dist = get_graphical_bounding_radius() * 2.0f;
      ground_offset = 1.0f + Sss::instance()->config().clip_near;
      frac_scale = 0.7f;
    }
    else
    {
      chase_dist = get_graphical_bounding_radius() * 5.0f;
      ground_offset = 3.0f + Sss::instance()->config().clip_near;
      frac_scale = 0.2f;
    }

    const float chase_offset = chase_dist + get_graphical_bounding_radius() + 
      Sss::instance()->config().clip_near;
    Position ideal_chase_pos = pos - chase_offset * get_vec_i_d() + 0.1 * get_vec_k_d();
    
    if (m_eye_type == CHASE_FAR)
      ideal_chase_pos[2] = pos[2];

    // assume a time-scale to reach the ideal position of time_scale
    // sec based on a distance of, say, 4 * chase_offset.
    const float time_scale = 0.2;
    float frac = (dt / time_scale) * exp(-dt*dt/(time_scale * time_scale));
//    float frac = frac_scale * sss_min(time_scale + dt/time_scale, 1.0f);
    chase_pos = chase_pos * (1.0f - frac) + ideal_chase_pos * frac;
    
    float ground_z = Environment::instance()->get_z(chase_pos[0], chase_pos[1]);
    if ( (ground_z + ground_offset) > chase_pos[2])
      chase_pos[2] = ground_z + ground_offset;
  }
  
  // update the engine
  power->post_physics(dt);

  // eye target
  if (m_eye_type == PILOT)
  {
    static float scale = 3.0f;
    m_eye_target = (get_eye()+get_vec_i_d());
    m_eye_target += scale * m_eye_offset[0] * get_vec_j_d();
    m_eye_target += scale * m_eye_offset[1] * get_vec_k_d();
  }
  else
  {
    Position target_pos = get_pos();
    float lag_time = Sss::instance()->config().glider_view_lag_time;
    target_pos += lag_time * get_vel();
    SmoothCD(m_eye_target, m_eye_target_rate, dt, target_pos, lag_time);
  }
}

//==============================================================
// calc_new_pos_and_orient
//==============================================================
void Glider::calc_new_pos_and_orient(float dt)
{
  TRACE_METHOD_ONLY(3);
  if (m_paused == true)
    return;
  
  pos += dt * vel;
  
//    Matrix w_star(0, rot[2], -rot[1],
//                  -rot[2], 0, rot[0],
//                  rot[1], -rot[0], 0 );
  
  Orientation new_orient = get_orient() +
    dt * Matrix(0, rot[2], -rot[1],
                -rot[2], 0, rot[0],
                rot[1], -rot[0], 0 ) * get_orient();
  new_orient.orthonormalise();
  
  set_orient(new_orient);
  
  if (validate() == false)
    reset();
}

//==============================================================
// get_airspeed
//==============================================================
Velocity Glider::get_airspeed() const
{
  const Velocity wind_static_rel(-dot(get_vel(), get_vec_i_d()),
                                 -dot(get_vel(), get_vec_j_d()),
                                 -dot(get_vel(), get_vec_k_d()) );
  
  const Velocity ambient_wind = Environment::instance()->get_non_turbulent_wind(get_pos());
  const Velocity ambient_wind_rel(dot(ambient_wind, get_vec_i_d()),
                                  dot(ambient_wind, get_vec_j_d()),
                                  dot(ambient_wind, get_vec_k_d()) );
  
  return (ambient_wind_rel + wind_static_rel);
}


//============================================================================
// Do all the aerodynamics in this fn.
//============================================================================
/*! We invoke the aerodynamics, structure and engine force/moment
  calculations from here. We then sum the result, and use this to
  integrate our velocity and rotation parameters. */
void Glider::calc_object_specific_force_and_moment(Vector & force, 
                                                   Vector & moment)
{
  TRACE_METHOD_ONLY(3);
  force.set_to(0);
  moment.set_to(0);
  
  if (m_paused == true)
    return;
  
  TRACE_FILE_IF(7)
    show();

  Position origPos = get_pos();
  
  Vector3 aero_force;
  Vector3 aero_moment;
  
  aero->get_force_and_moment(aero_force, aero_moment);
  force += aero_force;
  moment += aero_moment;
  
  TRACE_FILE_IF(4)
    TRACE("force mag = %f\n", force.mag());
  
  Vector3 struct_force;
  Vector3 struct_moment;
  structure->get_force_and_moment(struct_force, struct_moment);
  force += struct_force;
  moment += struct_moment;
  
  Vector3 power_force;
  Vector3 power_moment;
  power->get_force_and_moment(power_force, power_moment);
  force += power_force;
  moment += power_moment;
  
  //================== gravity ========================================
  float gravity = Sss::instance()->config().gravity;
  // use reduced gravity for under water...
  if (get_pos()[2] < Environment::instance()->get_sea_altitude())
    gravity *= 0.2;
  
  force += 
    Vector3(0, 0, -gravity * mass);
  
  // sanity checks - these occur when the position etc are still
  // finite, but stupidly large
  if (force.sensible() == false) {
    TRACE("calculated force is bad - resetting\n"); 
    force.set_to(0);
    moment.set_to(0);
    reset();
    do_reset = true;
    return;
  }
  if (moment.sensible() == false) {
    TRACE("calculated moment is bad- resetting\n"); 
    force.set_to(0);
    moment.set_to(0);
    reset();
    do_reset = true;
    return;
  }
  
  if (validate() == false)
    reset();
}

//==============================================================
// calc_new_vel_and_rot
//==============================================================
void Glider::calc_new_vel_and_rot(float dt,
                                  const Vector & force,
                                  const Vector & moment)
{
  TRACE_METHOD_ONLY(3);
  
  if (m_paused == true)
    return;
  
  //=================================================================
  // integrate vel
  //=================================================================
  vel += force * (dt / mass);
  
  //=================================================================
  // integrate rot
  //=================================================================
  
  // I_inv_world is the inverse inertia tensor in the world coords
  Matrix I_inv_world = orient * I_inv_body * transpose(orient);

  // estimate the new orientation so that we can calculate the change
  // in angular momentum
  Orientation new_orient = get_orient() +
    dt * Matrix(0, rot[2], -rot[1],
                -rot[2], 0, rot[0],
                rot[1], -rot[0], 0 ) * get_orient();
  new_orient.orthonormalise();
  
  // get the extra angular momentum
  Vector3 L_E(0.0f, 0.0f, 0.0f);
  structure->get_extra_ang_mom(*m_real_joystick, L_E);
  Vector3 L_E2(L_E);
  L_E2 = new_orient * L_E;
  L_E = get_orient() * L_E;

//   TRACE("========================\n");
//   orient.show("orient");
//   rot.show("rot");
//   L_E2.show("L_E2");
//   L_E.show("L_E");

//  rot += dt * I_inv_world * (moment - (L_E2 - L_E) * (1.0f/dt));
  rot += dt * I_inv_world * (moment - cross(rot, L_E));
  
  // plain version - no extra angular momentum
//  rot += dt * I_inv_world * moment;
  
  if (validate() == false)
    reset();
}

void Glider::draw_cockpit()
{
  if (!m_cockpit_texture)
  {
    if (Sss::instance()->config().cockpit_texture_file == "none")
    {
      return;
    }
    else
    {
      m_cockpit_texture = new Rgba_file_texture(Sss::instance()->config().cockpit_texture_file, 
                                                Rgba_file_texture::CLAMP,
                                                Rgba_file_texture::RGBA);
    }
  }


  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  
  glLoadIdentity();
  gluOrtho2D(0, 1, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);

  glEnable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glBindTexture(GL_TEXTURE_2D, m_cockpit_texture->get_high_texture());

  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(  0, 0);
  glTexCoord2f(1, 0);
  glVertex2f(  1, 0);
  glTexCoord2f(1, 1);
  glVertex2f(  1, 1);
  glTexCoord2f(0, 1);
  glVertex2f(  0, 1);
  glEnd();

  // undo
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
}


void Glider::draw_cross_hair() const
{
  if (!Sss::instance()->config().draw_crosshair)
    return;

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  
  glLoadIdentity();
  gluOrtho2D(0, 1, 0, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  
  // output a white cross-hair
  glDisable(GL_FOG);
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_BLEND);
  
  const float x0 = 0.5f;
  const float x1 = 0.48f;
  const float x2 = 1.0f - x1;
  
  const float g = 0.7f;
  glColor3f(g, g, g);
  glBegin(GL_LINES);
  glVertex2f(x1, x0);
  glVertex2f(x2, x0);
  glVertex2f(x0, x1);
  glVertex2f(x0, x2);
  glEnd();
  
  // undo
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  glPopAttrib();
}

void Glider::draw(Draw_type draw_type)
{
  basic_draw();
  
  if (draw_type == NORMAL)
  {
    if ( m_eye )
    {
      if ( (get_pos()-get_eye()).mag() > get_graphical_bounding_radius() )
      {
        // eye, so only draw if the eye is sufficiently far away
        graphics->draw(Glider_graphics::NORMAL);
      }
      else
      {
        draw_cockpit();
        draw_cross_hair();
      }
    }
    else
    {
      // not eye, so draw anyway
      graphics->draw(Glider_graphics::NORMAL);
    }
  }  
  else
  {
    graphics->draw(Glider_graphics::SHADOW);
  }
}

float Glider::get_graphical_bounding_radius() const 
{
  return graphics->get_bounding_radius(); 
}

float Glider::get_structural_bounding_radius() const 
{
  return structure->get_bounding_radius(); 
}

float Glider::get_collision_sphere_radius() const
{
  return structure->get_bounding_radius() * structure->get_bounding_sphere_scale();
}

bool Glider::is_on_ground() const
{
  return structure->is_on_ground();
}

void Glider::show()
{
  TRACE("Glider\n");
  Object::show();
  aero->show();
  structure->show();
  graphics->show();
  power->show();
  TRACE("Paused state = %d\n", m_paused);
}

void Glider::toggle_paused()
{
  m_paused = m_paused == true ? false : true;
}

void Glider::send_remote_update()
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
    msg.msg.update.object_type = Remote_sss_update_msg::GLIDER;
    
    msg.msg.update.msg.glider.update_type = 
      Remote_sss_update_glider_msg::NORMAL;
    
    msg.msg.update.msg.glider.update.normal.paused = m_paused;    
    
    get_joystick().get_vals(
      msg.msg.update.msg.glider.update.normal.joystick,
      msg.msg.update.msg.glider.update.normal.trims);
    
    // send the msg
    iface->send_update_ind(this, msg);
  }
}

void Glider::pre_physics(float dt)
{
  // update the joystick
  unsigned i;
  // servo speed - goes from min(-1) to max(+1) in, say, 0.7 sec
  const float servo_speed = 3.0f;
  for (i = 0 ; i < joystick->get_num_channels() ; ++i)
  {
    float diff =  joystick->get_value(i) - m_real_joystick->get_value(i);
    float diff_to_do = dt * servo_speed;
    if (fabs(diff_to_do) > fabs(diff))
    {
      m_real_joystick->set_value(i, joystick->get_value(i));
    }
    else
    {
      if (diff < 0)
        diff_to_do = - diff_to_do;
      m_real_joystick->set_value(i, m_real_joystick->get_value(i) + diff_to_do);
    }
  }
  
  /// update the structure...
  structure->update_structure(*m_real_joystick);
  
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

void Glider::recv_remote_update(Remote_sss_msg & msg,
                                float dt) // dt indicates how late the message is
{
  switch (msg.msg.update.msg.glider.update_type)
  {
  case Remote_sss_update_glider_msg::NORMAL:
  {
    // pause and joystick are cheap, and we don't want to miss them...
    m_paused = msg.msg.update.msg.glider.update.normal.paused;
    
    get_joystick().set_vals(
      msg.msg.update.msg.glider.update.normal.joystick,
      msg.msg.update.msg.glider.update.normal.trims);
    
    // just store it - overwrite any existing msg
    if (!m_last_update_msg)
      m_last_update_msg = new Remote_sss_msg;
    
    *m_last_update_msg = msg;
    m_last_update_dt = dt;
    return;
  }
  case Remote_sss_update_glider_msg::CONFIG_FILE:
  {
    string new_glider_file(
      msg.msg.update.msg.glider.update.config_file.glider_file);
    TRACE_FILE_IF(1)
      TRACE("using new glider file: %s\n", new_glider_file.c_str());
    bool success;
    Config_file glider_config_file(new_glider_file,
                                   success);
    assert2(success, "Unable to open glider file");
    // get rid of any out-of-date updates
    if (m_last_update_msg)
      delete m_last_update_msg;
    initialise_from_config(glider_config_file);
    return;
  }
  case Remote_sss_update_glider_msg::MISSILE_HIT:
  {
    TRACE_FILE_IF(2)
      TRACE("Remote missile hit\n");

    float missile_mass = msg.msg.update.msg.glider.update.missile_hit.missile_mass;
    Position missile_pos;
    missile_pos[0] = msg.msg.update.msg.glider.update.missile_hit.missile_pos[0];
    missile_pos[1] = msg.msg.update.msg.glider.update.missile_hit.missile_pos[1];
    missile_pos[2] = msg.msg.update.msg.glider.update.missile_hit.missile_pos[2];
    Velocity missile_vel;
    missile_vel[0] = msg.msg.update.msg.glider.update.missile_hit.missile_vel[0];
    missile_vel[1] = msg.msg.update.msg.glider.update.missile_hit.missile_vel[1];
    missile_vel[2] = msg.msg.update.msg.glider.update.missile_hit.missile_vel[2];

    respond_to_missile_hit(missile_mass, missile_pos, missile_vel);

    return;
  }
  case Remote_sss_update_glider_msg::RESET_IN_RACE:
    if (Race_manager::get_instance())
    {
      TRACE_FILE_IF(2)
        TRACE("Resetting glider %p in race\n", this);
      Race_manager::get_instance()->reset_object(this); 
    }
    return;
  }
  TRACE("Invalid switch!\n");
}

void Glider::reset_in_race()
{
  if (Race_manager::get_instance())
  {
    Race_manager::get_instance()->reset_object(this);
    // also send a remote reset in race in an update message
    // next time we send one
    // notify remote simulations
    Remote_sss_iface * iface;
    if ( (m_local) && 
         (0 != (iface = Remote_sss_iface::instance())) )
    {
      TRACE_FILE_IF(2)
        TRACE("Sending remote reset in race for glider %p\n", this);
      Remote_sss_msg msg;
      msg.msg_type = Remote_sss_msg::UPDATE;
      msg.msg.update.object_type = 
        Remote_sss_update_msg::GLIDER;    
      msg.msg.update.msg.glider.update_type = 
        Remote_sss_update_glider_msg::RESET_IN_RACE;
      iface->send_update_ind(this, msg);
    }
  }
}

void Glider::take_control(Joystick * new_joystick)
{
  TRACE_METHOD_ONLY(1);
  if (new_joystick)
    joystick = new_joystick;
  else
    joystick = m_original_joystick;
}

void Glider::respond_to_missile_hit(
  float missile_mass,
  const Position & missile_pos,
  const Velocity & missile_vel)
{
  TRACE_METHOD_ONLY(3);
  
  // we will pretend to the recipient that there is a 
  // force that is applied over 1 second to account for 
  // the halting of the missile
  float dt = 1.0f;
  Vector force = 20*missile_mass * missile_vel / dt;
  // random moment!
  float moment_mag = force.mag() * 
    get_structural_bounding_radius() * 
    ranged_random(-3, 3);
  Vector moment(0, 0, 1);
  moment = moment * moment_mag;
  calc_new_vel_and_rot(dt, force, moment);
}

void Glider::handle_missile_hit(
  float missile_mass,
  const Position & missile_pos,
  const Velocity & missile_vel)
{
  TRACE_METHOD_ONLY(3);
  
  if (m_local)
  {
    respond_to_missile_hit(missile_mass, missile_pos, missile_vel);
  }
  else
  {
    // send update
    Remote_sss_iface * iface;
    if (0 != (iface = Remote_sss_iface::instance()) )
    {
      Remote_sss_msg msg;
      msg.msg_type = Remote_sss_msg::UPDATE;
      msg.msg.update.object_type = 
        Remote_sss_update_msg::GLIDER;    

      msg.msg.update.msg.glider.update_type = 
        Remote_sss_update_glider_msg::MISSILE_HIT;

      msg.msg.update.msg.glider.update.missile_hit.missile_mass = missile_mass;
      msg.msg.update.msg.glider.update.missile_hit.missile_pos[0] = missile_pos[0];
      msg.msg.update.msg.glider.update.missile_hit.missile_pos[1] = missile_pos[1];
      msg.msg.update.msg.glider.update.missile_hit.missile_pos[2] = missile_pos[2];
      msg.msg.update.msg.glider.update.missile_hit.missile_vel[0] = missile_vel[0];
      msg.msg.update.msg.glider.update.missile_hit.missile_vel[1] = missile_vel[1];
      msg.msg.update.msg.glider.update.missile_hit.missile_vel[2] = missile_vel[2];
    
      // send the msg
      iface->send_update_ind_remote(this, msg);
    }

  }
}


