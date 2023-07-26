/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#include "propeller.h"
#include "joystick.h"
#include "renderer.h"
#include "config_file.h"
#include "gyro.h"
#include "glider_aero_component.h"

using namespace std;

Propeller::Propeller(Config_file & prop_config)
{
  TRACE_METHOD_ONLY(1);
  unsigned i;
  
  for (i = 0 ; i < MAX_CHANNEL ; ++i)
  {
    m_rotation_speed_per_control[i] = 0.0f;
    m_inc_fwd_per_control[i] = 0.0f;
    m_inc_left_per_control[i] = 0.0f;
    m_inc_per_control[i] = 0.0f;
  }
  
  m_gyro_name = "none";

  m_last_speed = 0.0f;
  m_last_inc = 0.0f;
  m_last_pos.set_to(0.0f);
  m_last_force_z = 0.0f;
  m_wash = true;

  prop_config.get_next_value_assert("name", m_name);
  prop_config.get_next_value_assert("position_x", m_position[0]);
  prop_config.get_next_value_assert("position_y", m_position[1]);
  prop_config.get_next_value_assert("position_z", m_position[2]);
  prop_config.get_next_value_assert("rot_x", m_rot_x);
  prop_config.get_next_value_assert("rot_y", m_rot_y);
  prop_config.get_next_value_assert("rot_z", m_rot_z);
  prop_config.get_next_value_assert("inertia", m_moment_of_inertia);
  prop_config.get_next_value_assert("radius", m_radius);
  prop_config.get_next_value_assert("blade_chord", m_blade_chord);
  prop_config.get_next_value_assert("offset_angle", m_offset_angle);
  prop_config.get_next_value_assert("radius_frac", m_radius_frac);
  prop_config.get_next_value_assert("CL_0", m_CL_0);
  prop_config.get_next_value_assert("CL_per_inc", m_CL_per_inc);
  prop_config.get_next_value_assert("inc_0", m_inc_0);
  prop_config.get_next_value_assert("speed_0", m_speed_0);
  
  prop_config.get_next_value("speed_per_chan_1", m_rotation_speed_per_control[0]);
  prop_config.get_next_value("speed_per_chan_2", m_rotation_speed_per_control[1]);
  prop_config.get_next_value("speed_per_chan_3", m_rotation_speed_per_control[2]);
  prop_config.get_next_value("speed_per_chan_4", m_rotation_speed_per_control[3]);
  prop_config.get_next_value("speed_per_chan_5", m_rotation_speed_per_control[4]);
  prop_config.get_next_value("speed_per_chan_6", m_rotation_speed_per_control[5]);
  prop_config.get_next_value("speed_per_chan_7", m_rotation_speed_per_control[6]);
  prop_config.get_next_value("speed_per_chan_8", m_rotation_speed_per_control[7]);
  
  prop_config.get_next_value("inc_fwd_per_chan_1", m_inc_fwd_per_control[0]);
  prop_config.get_next_value("inc_fwd_per_chan_2", m_inc_fwd_per_control[1]);
  prop_config.get_next_value("inc_fwd_per_chan_3", m_inc_fwd_per_control[2]);
  prop_config.get_next_value("inc_fwd_per_chan_4", m_inc_fwd_per_control[3]);
  prop_config.get_next_value("inc_fwd_per_chan_5", m_inc_fwd_per_control[4]);
  prop_config.get_next_value("inc_fwd_per_chan_6", m_inc_fwd_per_control[5]);
  prop_config.get_next_value("inc_fwd_per_chan_7", m_inc_fwd_per_control[6]);
  prop_config.get_next_value("inc_fwd_per_chan_8", m_inc_fwd_per_control[7]);
  
  prop_config.get_next_value("inc_left_per_chan_1", m_inc_left_per_control[0]);
  prop_config.get_next_value("inc_left_per_chan_2", m_inc_left_per_control[1]);
  prop_config.get_next_value("inc_left_per_chan_3", m_inc_left_per_control[2]);
  prop_config.get_next_value("inc_left_per_chan_4", m_inc_left_per_control[3]);
  prop_config.get_next_value("inc_left_per_chan_5", m_inc_left_per_control[4]);
  prop_config.get_next_value("inc_left_per_chan_6", m_inc_left_per_control[5]);
  prop_config.get_next_value("inc_left_per_chan_7", m_inc_left_per_control[6]);
  prop_config.get_next_value("inc_left_per_chan_8", m_inc_left_per_control[7]);
  
  prop_config.get_next_value("inc_per_chan_1", m_inc_per_control[0]);
  prop_config.get_next_value("inc_per_chan_2", m_inc_per_control[1]);
  prop_config.get_next_value("inc_per_chan_3", m_inc_per_control[2]);
  prop_config.get_next_value("inc_per_chan_4", m_inc_per_control[3]);
  prop_config.get_next_value("inc_per_chan_5", m_inc_per_control[4]);
  prop_config.get_next_value("inc_per_chan_6", m_inc_per_control[5]);
  prop_config.get_next_value("inc_per_chan_7", m_inc_per_control[6]);
  prop_config.get_next_value("inc_per_chan_8", m_inc_per_control[7]);

  prop_config.get_next_value("attach_to_gyro", m_gyro_name);
  
  // calculate the rotation axis, initially in the z dir.
  m_axis = Vector(0.0f, 0.0f, 1.0f);
  m_axis = alpha(m_rot_x) * m_axis;
  m_axis = beta(m_rot_y) * m_axis;
  m_axis = gamma(m_rot_z) * m_axis;
  
  calculate_hard_points();
}

void Propeller::calculate_hard_points()
{
  const unsigned num = 8;
  unsigned i;
  
  Matrix matrix = matrix3_identity();
  
  matrix = alpha(m_rot_x) * matrix;
  matrix = beta(m_rot_y) * matrix;
  matrix = gamma(m_rot_z) * matrix;
  
  for (i = 0 ; i < num ; ++i)
  {
    float angle = i * 360.0f / num;
    Vector pos(m_radius * cos_deg(angle), m_radius * sin_deg(angle), 0.0f);

    pos = matrix * pos;

    Vector dir = cross(m_axis, pos);
    dir.normalise();
    
    pos += m_position;
    
    m_hard_points.push_back(pos);
    m_hard_points[i].m_vel = m_speed_0 * dir;
    m_hard_points[i].mu_min = 0.9f;
    m_hard_points[i].mu_max = 0.9f;
    m_hard_points[i].hardness = 200.0f;
    m_hard_points[i].min_friction_dir = Vector(1.0f, 0.0f, 0.0f);
  }
}


// Calculate the forces by calculating the force at a number of positions around the 
// propeller arc. Then take the average.
// some of this should be moved to pre_physics so that the control stuff gets done just once per frame
void Propeller::get_forces(const Velocity & wind_rel, // in
                           const Position & glider_pos,
                           const Orientation & glider_orient,
                           const Vector & local_rot, // in - the parent rotation in local coords
                           const Joystick & joystick,
                           const Glider_aero_component & glider, // to get the gyros
                           Vector3 & force,  // force out
                           Vector3 & moment, // moment out
                           Position & force_position, // location of force out
                           float density)
{
  // Initialise the result
  force.set_to(0.0f);
  force_position = m_position;
  moment.set_to(0.0f);
  
  // Do the calculations by splitting the propeller up into a number
  // of segments.
  const unsigned num_segments = 8;
  unsigned i;
  Vector rot(m_axis);
  
  float speed = m_speed_0;
  float blade_area = m_radius * m_blade_chord;
  
  float inc_fwd_control = 0.0f;
  float inc_left_control = 0.0f;
  float inc_control = m_inc_0;
  for (i = 0 ; i < MAX_CHANNEL ; ++i)
  {
    speed += m_rotation_speed_per_control[i] * joystick.get_value(i+1) ;
    inc_fwd_control += m_inc_fwd_per_control[i] * joystick.get_value(i+1);
    inc_left_control += m_inc_left_per_control[i] * joystick.get_value(i+1);
    inc_control += m_inc_per_control[i] * joystick.get_value(i+1);
  }

  // also allow one gyro to affect the collective
  const Gyro * gyro = glider.get_gyro(m_gyro_name);

  if (gyro)
  {
    float gyro_control = gyro->get_output(joystick);
    inc_control -= gyro_control;
  }

  rot *= speed * TWO_PI;
  
  // in order to avoid over-correction by gyros we need to limit the rate of travel
  // this is nasty and frame-rate dependant
  float delta_inc = inc_control - m_last_inc;
  const float max_inc_change = 0.5f;

  if (delta_inc > max_inc_change)
    delta_inc = max_inc_change;
  else if (delta_inc < -max_inc_change)
    delta_inc = -max_inc_change;

  inc_control = m_last_inc + delta_inc;

  // limit the inclination
  const float max_inc = 45.0f;
  if (inc_control > max_inc)
    inc_control = max_inc;
  else if (inc_control < -max_inc)
    inc_control = -max_inc;

  // store the result
  m_last_speed = speed;
  m_last_inc = inc_control;
  m_last_pos = glider_pos + glider_orient * m_position;
  m_last_orient = glider_orient;
  m_last_wind_rel = wind_rel;

  Matrix matrix = matrix3_identity();
  matrix = alpha(m_rot_x) * matrix;
  matrix = beta(m_rot_y) * matrix;
  matrix = gamma(m_rot_z) * matrix;
  
  for (i = 0 ; i < num_segments ; ++i)
  {
    float angle = i * 360.0f / num_segments;
    Vector pos(
      m_radius_frac * m_radius * cos_deg(angle), 
      m_radius_frac * m_radius * sin_deg(angle), 
      0.0f);
    // note that pos is relative to the propeller origin, 
    pos = matrix * pos;
    Velocity blade_vel = cross(rot, pos);
    Velocity blade_rel_wind_vel = wind_rel - blade_vel;
    Vector   blade_dir(blade_vel);
    blade_dir.normalise();
    
    // make pos the full glider-relative position
    pos += m_position;
    Vector rot_headwind = -cross(local_rot, pos);
    
    float blade_headwind = - dot((blade_rel_wind_vel + rot_headwind), blade_dir);
    float blade_vertwind = dot((blade_rel_wind_vel + rot_headwind), m_axis);
    
    float wind_alpha = atan2_deg(blade_vertwind, blade_headwind);
    
    float inc = wind_alpha + inc_control + 
      inc_fwd_control * cos_deg(angle - m_offset_angle) + 
      inc_left_control * sin_deg(angle - m_offset_angle);
    
    // have enough for CL (assume no stalling etc!)
    float CL = m_CL_0 + inc * m_CL_per_inc;
    
    float CD = 0.02 + CL * CL * 0.05f; // 0.05f from crrcsim?
    CD = 0.0f;
    float lift_force = blade_area * 0.5f * blade_headwind * blade_headwind * CL;
    float drag_force = density * blade_area * 0.5f * blade_headwind * blade_headwind * CD;
    
    Vector total_force = lift_force * m_axis - drag_force * blade_dir;
    total_force *= (2.0f / num_segments); // factor of two because there are two blades
    
    force += total_force;
    moment += cross(pos, total_force);
  }
  
  m_last_force_z = dot(force, m_axis);

  if (!moment.sensible())
  {
    TRACE("Propeller calculated moment is bad\n"); 
    moment.set_to(0.0f);
  }
  if (!force.sensible())
  {
    TRACE("Propeller calculated force is bad\n");
    force.set_to(0.0f);
  }

}


/// Return the angular momentum  in the object reference frame
void Propeller::get_ang_mom(const Joystick & joystick,
                            Vector3 & ang_mom) const
{
  float speed = m_speed_0;
  unsigned i;
  
  for (i = 0 ; i < 8 ; ++i)
  {
    speed += m_rotation_speed_per_control[i];
  }
  
  ang_mom += TWO_PI * speed * m_moment_of_inertia * m_axis;
}

// draw (very basic)
void Propeller::draw(Draw_type draw_type)
{
  const int num = 32;
  
/*
  glPushMatrix();
  glTranslatef(m_position[0], m_position[1], m_position[2]);
  glBegin(GL_LINES);
  glVertex3f(0.0f, 0.0f, 0.0f);
  glVertex3fv(&m_axis[0]);
  glEnd();
  glPopMatrix();
*/
  glPushMatrix();
  glTranslatef(m_position[0], m_position[1], m_position[2]);
  glRotatef(m_rot_z, 0, 0, 1);
  glRotatef(m_rot_y, 0, 1, 0);
  glRotatef(m_rot_x, 1, 0, 0);
  if (draw_type == NORMAL)
    glColor4f(1.0f, 0.0f, 0.0f, 1.0f);
  gluDisk (Renderer::instance()->quadric(),
           0.0f,
           m_radius, 
           num, 8);  
  glRotatef(180.0f, 1, 0, 0);
  if (draw_type == NORMAL)
    glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
  gluDisk (Renderer::instance()->quadric(),
           0.0f,
           m_radius, 
           num, 8);  
  glPopMatrix();
}

Vector calculate_wash(const Position & pos, 
                      const Position & prop_pos,
                      Vector3 axis, 
                      float downdraft,
                      float radius)
{
  Vector3 offset = pos - prop_pos;
  
  float vertical_offset = dot(offset, axis);
  float hor_offset = (offset - vertical_offset * axis).mag();

  hor_offset -= radius;
  if (hor_offset < 0.0f)
    hor_offset = 0.0f;
  
#define SQR(a) ((a) * (a))
  float scale = 
    exp(-SQR(hor_offset/radius)) * 
    exp(-0.15f * fabs(SQR(vertical_offset/radius)));

  Vector3 extra = -downdraft * scale * axis;

  return extra;
}

void Propeller::get_wind(const Position & pos, Vector & wind) const
{
  float downdraft = sqrt(fabs(m_last_force_z) / (PI * m_radius * m_radius) );
  if (m_last_force_z < 0.0f)
    downdraft = -downdraft;
  if (!is_finite(downdraft))
    downdraft = 0.0f;
  Vector3 axis = m_last_orient * m_axis;
  
  float translation_factor = 0.0f;
  if (!m_wash && (fabs(downdraft) > 0.0001f))
  {
    // calculate the translation lift factor - this is 1.0 if the air coming into the prop is 
    // fresh, and 0.0 if it is stale (e.g. hover). Scaling is done based on the downdraft
    float wind_rel_in_plane = (m_last_wind_rel - dot(m_last_wind_rel, m_axis) * m_axis).mag();
    translation_factor = 1.0f - fabs(wind_rel_in_plane / downdraft);
    if (translation_factor > 1.0f)
      translation_factor = 1.0f;
    else if (translation_factor < 0.0f)
      translation_factor = 0.0f;

    translation_factor *= 0.5f;
    /*
      if (m_name == "rotor")
      {
      TRACE("speed = %f, fac = %f\n", wind_rel_in_plane, translation_factor);
      }
    */
  }
  wind += (m_wash ? 1.0f : translation_factor) * calculate_wash(pos, m_last_pos, axis, downdraft, m_radius);

  // calculate the ground back-wash by reflecting the axis in the ground - 
  // assume that if we're close enough to the ground for this to be significant
  // then the ground slope immediately below is appropriate

  Position terrain_pos;
  Vector3 normal;
  Environment::instance()->get_local_terrain(m_last_pos, terrain_pos, normal);

  // get equation of plane
  float dist = -dot(terrain_pos, normal);

  // get dist of prop above plane
  float prop_dist = dot(normal, m_last_pos) + dist;
  if (prop_dist < 0.0f)
    return;

  Position mirror_pos = m_last_pos - 2.0f * prop_dist * normal;
  Vector mirror_axis = axis - 2 * dot(axis, normal) * normal;

  Vector3 mirror_wash = calculate_wash(pos, mirror_pos, mirror_axis, downdraft, m_radius);

  wind += mirror_wash;
}
