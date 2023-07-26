/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file aerofoil.cpp
*/
#include "aerofoil.h"
#include "sss.h"
#include "joystick.h"
#include "config_file.h"
#include "log_trace.h"
#include "environment.h"

#include "sss_glut.h"

#include <stdio.h>
#include <stdlib.h>

using namespace std;

#ifndef unix
#define strcasecmp _stricmp
#endif

void Aerofoil::calculate_hard_points()
{
  hard_points.resize(6);
  // the corners  - front-left, front-right, back-right, back-left
  hard_points[0].pos = Position(chord/2, span/2, 0);
  hard_points[1].pos = Position(chord/2, -span/2, 0);
  hard_points[2].pos = Position(-chord/2, -span/2, 0);
  hard_points[3].pos = Position(-chord/2, span/2, 0);
  // and the middle
  hard_points[4].pos = Position(0, span/2, 0);
  hard_points[5].pos = Position(0, span/2, 0);
  
  for (int i = 0 ; i < 6 ; ++i)
  {
    hard_points[i].pos = alpha(rotation) * hard_points[i].pos;
    hard_points[i].pos = hard_points[i].pos + position;
    // direction independant
    hard_points[i].min_friction_dir = Vector(1, 0, 0);
    hard_points[i].mu_max = 0.8f;
    hard_points[i].mu_min = 0.8f;
    hard_points[i].hardness = 1.0f;
  }
}

void Aerofoil::calculate_constants() 
{
  CL_0_r   = -CL_0   * CL_reverse_scale;
  CL_max_r = -CL_min * CL_reverse_scale;
  CL_min_r = -CL_max * CL_reverse_scale;
  
  CL_max_d = CL_max * CL_drop_scale; // what CL drops down to
  CL_min_d = CL_min * CL_drop_scale; 
  CL_max_r_d = CL_max_r * CL_drop_scale; 
  CL_min_r_d = CL_min_r * CL_drop_scale;
  
  //   alpha_CL_0     = 0; // alpha at CL_0
  alpha_CL_max   = (CL_max - CL_0) / CL_per_alpha;
  alpha_CL_max_d = alpha_CL_max + 10;
  alpha_CL_min   = (CL_min - CL_0) / CL_per_alpha;
  alpha_CL_min_d = alpha_CL_min - 10;
  //   alpha_CL_0_r   = 180; // must be +ve
  alpha_CL_max_r = -180 + (CL_max_r - CL_0_r) / CL_per_alpha;
  alpha_CL_max_r_d = alpha_CL_max_r + 10;
  alpha_CL_min_r = 180 + (CL_min_r - CL_0_r) / CL_per_alpha;
  alpha_CL_min_r_d = alpha_CL_min_d - 10;

  const float _x[] = 
    {
      alpha_CL_min_r-360,
      -180,
      alpha_CL_max_r, alpha_CL_max_r_d,
      -90,
      alpha_CL_min_d,   alpha_CL_min,
      0,
      alpha_CL_max,   alpha_CL_max_d,
      90,
      alpha_CL_min_r_d, alpha_CL_min_r,
      180
    };
  const float _y[] = 
    {
      CL_min_r,
      CL_0_r,
      CL_max_r, CL_max_r_d,
      0,
      CL_min_d,   CL_min,
      CL_0,
      CL_max,   CL_max_d,
      0,
      CL_min_r_d, CL_min_r,
      CL_0_r
    };
  
  // 1 = forward flight, 0 = stalled, -1 = reverse "flight"
  const Flying _flight[] = // value refers to the following line segment
    {
      STALLED,
      REVERSE, 
      REVERSE, STALLED,
      STALLED,
      FORWARD, FORWARD,
      FORWARD,
      FORWARD, STALLED,
      STALLED,
      REVERSE, REVERSE,
      REVERSE
    };

  const int n = sizeof(x) / sizeof(x[0]);
  assert1(n == NUM_POINTS);
  for (unsigned i = 0 ; i < NUM_POINTS ; ++i)
  {
    x[i]=_x[i];
    y[i]=_y[i];
    flight[i]=_flight[i];
  }
}

/*!  data_file is passed in and will be such that any subsequent reads
  will refer to this aerofoil, until we reach the attribute
  aerofoil_end The fields must be in the right order, and complete.
  
  However, if this is a copy, the copy attribute will appear first,
  and then there will be a series of optional attr/values. Otherwise,
  name will appear first.

  A list of previously-created aerofoils needs to be passed in so that
  copies can be made.  */
Aerofoil::Aerofoil(Config_file & aero_config, 
                   const vector<Aerofoil> & aerofoils)
  :
  num_sections(1),
  init_moving(false)
{
  TRACE_METHOD_ONLY(1);

  Config_file::Config_attr_value attr_value = 
    aero_config.get_next_attr_value();
  unsigned int i;
  
  if (attr_value.attr == "copy")
  {
    assert1(attr_value.num > 0);
    assert1(attr_value.value_type == Config_file::Config_attr_value::STRING);
    for (i = 0 ; i < aerofoils.size() ; ++i)
    {
      if (aerofoils[i].get_name() == attr_value.values[0].string_val)
      {
        *this = aerofoils[i];
        position = aerofoils[i].position; // not sure why this is needed...
        break;
      }
    }
    if (i == aerofoils.size())
    {
      TRACE("Can't find aerofoil %s to copy\n", 
            attr_value.values[0].string_val.c_str());
      assert1(!"Error!");
    }
 
    // now allow the values to be modified
    aero_config.get_next_value_assert("name",     name               );
    aero_config.get_next_value("num_sections",    num_sections       );
    aero_config.get_next_value("position_x",      position[0]        );
    aero_config.get_next_value("position_y",      position[1]        );
    aero_config.get_next_value("position_z",      position[2]        );
    aero_config.get_next_value("offset_forward",  x_offset[0]        );
    aero_config.get_next_value("offset_stalled",  x_offset[1]        );
    aero_config.get_next_value("offset_reverse",  x_offset[2]        );
    aero_config.get_next_value("chord",           chord              );
    aero_config.get_next_value("span",            span               );
    aero_config.get_next_value("rotation",        rotation           );
    aero_config.get_next_value("inc",             inc                );
    aero_config.get_next_value("CL_drop_scale",   CL_drop_scale      );
    aero_config.get_next_value("CL_rev_scale",    CL_reverse_scale   );
    aero_config.get_next_value("CL_per_alpha",    CL_per_alpha       );
    aero_config.get_next_value("CL_0",            CL_0               );
    aero_config.get_next_value("CL_max",          CL_max             );
    aero_config.get_next_value("CL_min",          CL_min             );
    aero_config.get_next_value("CD_prof",         CD_prof            );
    aero_config.get_next_value("CD_induced_frac", CD_induced_frac    );
    aero_config.get_next_value("CD_max",          CD_max             );
    aero_config.get_next_value("CM_0",            CM_0               );
    aero_config.get_next_value("CL_per_deg",      CL_offset_per_deg  );
    aero_config.get_next_value("CD_prof_per_deg", CD_prof_per_deg    );
    aero_config.get_next_value("CM_per_deg",      CM_per_deg         );
    aero_config.get_next_value("inc_per_deg",     inc_offset_per_deg );
    aero_config.get_next_value("control_per_chan_1",  control_per_chan_1);
    aero_config.get_next_value("control_per_chan_2",  control_per_chan_2);
    aero_config.get_next_value("control_per_chan_3",  control_per_chan_3);
    aero_config.get_next_value("control_per_chan_4",  control_per_chan_4);
    aero_config.get_next_value("control_per_chan_5",  control_per_chan_5);
    aero_config.get_next_value("control_per_chan_6",  control_per_chan_6);
    aero_config.get_next_value("control_per_chan_7",  control_per_chan_7);
    aero_config.get_next_value("control_per_chan_8",  control_per_chan_8);
  }
  else if (attr_value.attr == "name")
  {
    assert1(attr_value.num > 0);
    assert1(attr_value.value_type == Config_file::Config_attr_value::STRING);
    name = attr_value.values[0].string_val;

    control_per_chan_1 = 0;
    control_per_chan_2 = 0;
    control_per_chan_3 = 0;
    control_per_chan_4 = 0;
    control_per_chan_5 = 0;
    control_per_chan_6 = 0;
    control_per_chan_7 = 0;
    control_per_chan_8 = 0;
    
    aero_config.get_next_value("num_sections",    num_sections       );
    aero_config.get_next_value_assert("position_x",      position[0]        );
    aero_config.get_next_value_assert("position_y",      position[1]        );
    aero_config.get_next_value_assert("position_z",      position[2]        );
    aero_config.get_next_value_assert("offset_forward",  x_offset[0]        );
    aero_config.get_next_value_assert("offset_stalled",  x_offset[1]        );
    aero_config.get_next_value_assert("offset_reverse",  x_offset[2]        );
    aero_config.get_next_value_assert("chord",           chord              );
    aero_config.get_next_value_assert("span",            span               );
    aero_config.get_next_value_assert("rotation",        rotation           );
    aero_config.get_next_value_assert("inc",             inc                );
    aero_config.get_next_value_assert("CL_drop_scale",   CL_drop_scale      );
    aero_config.get_next_value_assert("CL_rev_scale",    CL_reverse_scale   );
    aero_config.get_next_value_assert("CL_per_alpha",    CL_per_alpha       );
    aero_config.get_next_value_assert("CL_0",            CL_0               );
    aero_config.get_next_value_assert("CL_max",          CL_max             );
    aero_config.get_next_value_assert("CL_min",          CL_min             );
    aero_config.get_next_value_assert("CD_prof",         CD_prof            );
    aero_config.get_next_value_assert("CD_induced_frac", CD_induced_frac    );
    aero_config.get_next_value_assert("CD_max",          CD_max             );
    aero_config.get_next_value_assert("CM_0",            CM_0               );
    aero_config.get_next_value_assert("CL_per_deg",      CL_offset_per_deg  );
    aero_config.get_next_value_assert("CD_prof_per_deg", CD_prof_per_deg    );
    aero_config.get_next_value_assert("CM_per_deg",      CM_per_deg         );
    aero_config.get_next_value_assert("inc_per_deg",     inc_offset_per_deg );
    
    aero_config.get_next_value("control_per_chan_1",  control_per_chan_1);
    aero_config.get_next_value("control_per_chan_2",  control_per_chan_2);
    aero_config.get_next_value("control_per_chan_3",  control_per_chan_3);
    aero_config.get_next_value("control_per_chan_4",  control_per_chan_4);
    aero_config.get_next_value("control_per_chan_5",  control_per_chan_5);
    aero_config.get_next_value("control_per_chan_6",  control_per_chan_6);
    aero_config.get_next_value("control_per_chan_7",  control_per_chan_7);
    aero_config.get_next_value("control_per_chan_8",  control_per_chan_8);
  }
  else
  {
    TRACE("Invalid first attribute: %s\n", attr_value.attr.c_str());
    assert1(!"Error!");
  }
  
  calculate_hard_points();
  calculate_constants();
}

Aerofoil::Aerofoil(const string & name,
                   const Position & pos,
                   float rotation,
                   float inc,
                   float span,
                   float chord)
  :
  name(name),
  num_sections(1),
  position(pos),
  x_offset(chord*0.25f, 0, -chord*0.25f), 
  chord(chord),
  span(span),
  rotation(rotation),
  inc(inc),
  CL_drop_scale(0.5f),
  CL_reverse_scale(1),
  CL_per_alpha(0.1f),
  CL_0(0),
  CL_max(1.5f),
  CL_min(-1.5f),
  CD_prof(0.01f),
  CD_induced_frac(0.04f),
  CD_max(3),
  CM_0(0),
  CL_offset_per_deg(0.0f),
  CD_prof_per_deg(0),
  CM_per_deg(0),
  inc_offset_per_deg(0),
  control_per_chan_1(0),
  control_per_chan_2(0),
  control_per_chan_3(0),
  control_per_chan_4(0),
  control_per_chan_5(0),
  control_per_chan_6(0),
  control_per_chan_7(0),
  control_per_chan_8(0),
  init_moving(false)
{
  TRACE_METHOD_ONLY(1);
  if (num_sections <= 0)
    num_sections = 1;
  calculate_hard_points();
  calculate_constants();
}


// helper fn for get_CL
inline float interp(float x0, float y0, float x1, float y1, float x)
{
  return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

/*
  Calculate CL based on the angle of attack. This assumes that when
  reversed (alpha approx 180 deg), the wing behaves in exactly the same
  way, but just less so (scaled). We calculate various points, then
  linearly interpolate.
  
  here, alpha includes the aerofoil inclination, but doesn't include the
  effect of the control surface 
*/
float Aerofoil::get_CL(float alpha, float control_input, 
                       Flying & flying, // returned
                       float & flying_float)
{
  const float CL_offset = control_input * CL_offset_per_deg;
  const float inc_offset = control_input * inc_offset_per_deg;
  
  // defaults...
  flying = FORWARD;
  flying_float = 0.0f;

  alpha += inc_offset;

  // make sure input is in correct range
  if (alpha > 180)
    alpha -= 360;
  else if (alpha < -180)
    alpha += 360;
  
//    for (int i = 0 ; i < n ; i++)
//      TRACE("x = %f, y = %f\n", x[i], y[i]);
  
  if (alpha < x[1])
  {
    TRACE("alpha out of range %f\n", alpha);
    alpha = 0.0f;
//    return 0;
//    abort();
  }
  
  float CL = -999;
  
  for (int i = 2 ; i < NUM_POINTS ; ++i)
  {
    if (alpha <= x[i])
    {
      CL = CL_offset + interp(x[i-1], y[i-1], x[i], y[i], alpha);
      flying_float = 1.0f - 
        interp(x[i-1], flight[i-1], x[i], flight[i], alpha);
      flying = flight[i-1];
      break;
    }
  }
  
  //  flying_float = 1-flying_float;

  if (CL != -999)
  {
    return CL;
  }
  else
  {
    TRACE("alpha is out of range %f\n", alpha);
    return 0;
    //      abort();
  }
}

// alpha is angle of attack, including inc
float Aerofoil::get_CD(float alpha, float control_input, Flying flying, float CL)
{
  float sin_deg_alpha = sin_deg(alpha);
  float CD_stall = (flying == STALLED) * ( CD_max * sin_deg_alpha * sin_deg_alpha);
  float CD_induced = CL * CL * CD_induced_frac; // 0.05f from crrcsim?
  
  return CD_prof + CD_prof_per_deg * fabs(control_input) + CD_induced + CD_stall;
}

// This function easily wins when run under a profiler - gprof puts it
// at around 17%. Almost all of this is this function - not any
// (profiled) function it calls.
void Aerofoil::get_lift_and_drag(const Velocity & wind_rel_in, // in
                                 const Joystick & joystick,
                                 Vector3 & linear_force, // force
                                 Position & linear_position, // loc of force  
                                 Vector3 & pitch_force,
                                 Position & pitch_position,
                                 float density
  )
{
  float CL, CD, lift_force, drag_force;
  
  // rotate incoming wind_rel by -inc about  axis
  // rotate incoming wind_rel by -rotation about x axis
  const Matrix alpha_beta(alpha(-rotation) * beta(inc));
  const Matrix alpha_beta_inv(transpose(alpha_beta));

  const Velocity wind_rel(alpha_beta * wind_rel_in);
  //  Velocity wind_rel = rotation_matrix(inc, Vector(0,1,0)) * wind_rel_in;
  //  wind_rel = alpha(-rotation) * wind_rel;
  //  wind_rel = rotation_matrix(-rotation, Vector(1,0,0)) * wind_rel;
  
  float alph = atan2_deg(wind_rel[2], -wind_rel[0]); // angle of inclination
  float speed2 = wind_rel[2] * wind_rel[2] + wind_rel[0] * wind_rel[0];
  float speed = sqrt(speed2);

  if (alph > 180)
    alph -= 360;
  else if (alph < -180)
    alph += 360;
  
  Flying flying;
  float flying_float;
  // calculate and store the control input (might be used in the
  // display)
  float control_input = 
    control_per_chan_1 * joystick.get_value(1) + 
    control_per_chan_2 * joystick.get_value(2) + 
    control_per_chan_3 * joystick.get_value(3) + 
    control_per_chan_4 * joystick.get_value(4) + 
    control_per_chan_5 * joystick.get_value(5) + 
    control_per_chan_6 * joystick.get_value(6) + 
    control_per_chan_7 * joystick.get_value(7) + 
    control_per_chan_8 * joystick.get_value(8);
  
  CL = get_CL(alph, control_input, flying, flying_float); // this sets flying
  CD = get_CD(alph, control_input, flying, CL);

  float force_scale = get_area() * 0.5f * speed2;
//  TRACE("%s %p, chord = %f, span = %f\n", get_name().c_str(), this, chord, span);
  // HACK - sqrt(density) is normally just 1... but we can't handle
  // the large values from being under water. Can't even handle sqrt(100)...
  lift_force = force_scale * CL; // main variation in density is air/water
  drag_force = density * force_scale * CD;

  float flying_offset;
  if (flying_float > 0)
  {
    flying_offset = x_offset[(int) STALLED] + 
      flying_float * (x_offset[(int) FORWARD] - x_offset[(int) STALLED]);
  }
  else
  {
    flying_offset = x_offset[(int) STALLED] + 
      -flying_float * (x_offset[(int) REVERSE] - x_offset[(int) STALLED]);
  }

//  TRACE("flying = %d, %f: alph = %f, CL = %f, CD = %f, lift = %f, drag = %f\n", 
//    flying, flying_float, alph, CL, CD, lift_force, drag_force);
  
  // we need cos(alph) etc. Rather than using trig, we can calculate the
  // value directly...
//  const float cos_deg_alph = cos_deg(alph);
//  const float sin_deg_alph = sin_deg(alph);
  bool doit = (fabs(speed) > 0.01);
  const float cos_deg_alph = doit ? -wind_rel[0] / speed : 0.0f;
  const float sin_deg_alph = doit ? wind_rel[2] / speed : 0.0f;;

  const float force_up = lift_force * cos_deg_alph + 
    drag_force * sin_deg_alph;
  const float force_forward = lift_force * sin_deg_alph - 
    drag_force * cos_deg_alph;
  
  // the following are returned by reference
  linear_force = Vector3(force_forward, 0.0f, force_up); 
  linear_position = position + Position(flying_offset, 0, 0);

  // calculate the pitching moment - represented by a single 
  // force at the trailing edge
  float pitching_force = get_area() * 0.5f * wind_rel[0] * fabs(wind_rel[0]) * 
    (CM_0 + CM_per_deg * control_input); 
  pitch_force = Vector3(0.0f, 0.0f, pitching_force);
  pitch_position = position;
  pitch_position[0] -= 0.5f*chord;

  // need to rotate the force back...
  //  Matrix rot_m = rotation_matrix(rotation, Vector(1,0,0));
//    Matrix rot_m = alpha(rotation);
//    linear_force = rot_m * linear_force;
//    pitch_force = rot_m * pitch_force;

//    //  rot_m = rotation_matrix(-inc, Vector(0,1,0));
//    rot_m = beta(-inc);
//    linear_force = rot_m * linear_force;
//    pitch_force = rot_m * pitch_force;

  //  Matrix rot_beta_alpha = beta(-inc) * alpha(rotation);
  linear_force = alpha_beta_inv * linear_force;
  pitch_force = alpha_beta_inv * pitch_force;
}

//////////////////////////////////////////////////////////////

void Aerofoil::draw_non_moving(Draw_type draw_type)
{
  
  static const Vector3 col1(1,0,0); // red
  static const Vector3 col2(0,0,1); // blue
  static const Vector3 col3(0,1,0); // blue
  
  float f1 = fabs(rotation/180.0f);
  float f2 = 1-f1;
  float f3 = fabs(rotation/90.0f);
  
  Vector3 col_top = f2 * col1 + f3 * col3;
  Vector3 col_bot = f2 * col2 + f3 * col3;
  Vector3 col_end = (col_top + col_bot) * 0.5f;
  
  const int nx = 5;
  static float x[nx], z[nx];
  static bool init = false;
  if (init == false)
  {
    const float zscale = 0.04f;
    
    init = true;
    
    x[0]=0;
    x[1]=0.1f;
    x[2]=0.3f;
    x[3]=0.6f;
    x[4]=1;
    
    z[0]=zscale * 0;
    z[1]=zscale * 0.7f;
    z[2]=zscale * 1.0f;
    z[3]=zscale * 0.7f;
    z[4]=zscale * 0;
  }
  
  glPushMatrix();
  
  glTranslatef(position[0], position[1], position[2]);
  
  glRotatef(rotation, 1, 0, 0);
  glRotatef(-inc,      0, 1, 0);
  
  glBegin(GL_QUADS);
  
  float x0 = chord/2;
  float dx = -chord;
  float dz = chord;
  
  // top
  if (draw_type == NORMAL)
    glColor4f(col_top[0], col_top[1], col_top[2], 1);
  int i;
  for (i = 0 ; i < nx-1 ; i++)
  {
    Vector3 v1(x0 + dx * x[i],    span/2, dz * z[i]);
    Vector3 v2(x0 + dx * x[i],   -span/2, dz * z[i]);
    Vector3 v3(x0 + dx * x[i+1], -span/2, dz * z[i+1]);
    Vector3 v4(x0 + dx * x[i+1],  span/2, dz * z[i+1]);
    
    Vector3 vn = cross(v3-v1, v2-v4).normalise();
    glNormal3fv(vn.get_data());
    glVertex3fv(v4.get_data());
    glVertex3fv(v3.get_data());
    glVertex3fv(v2.get_data());
    glVertex3fv(v1.get_data());
  }
  
  // bottom
  
  if (draw_type == NORMAL)
    glColor4f(col_bot[0], col_bot[1], col_bot[2], 1);
  for (i = 0 ; i < nx-1 ; i++)
  {
    Vector3 v1(x0 + dx * x[i],    span/2, -dz * z[i]);
    Vector3 v2(x0 + dx * x[i],   -span/2, -dz * z[i]);
    Vector3 v3(x0 + dx * x[i+1], -span/2, -dz * z[i+1]);
    Vector3 v4(x0 + dx * x[i+1],  span/2, -dz * z[i+1]);
    
    Vector3 vn = cross(v3-v1, v4-v2).normalise();
    glNormal3fv(vn.get_data());
    glVertex3fv(v1.get_data());
    glVertex3fv(v2.get_data());
    glVertex3fv(v3.get_data());
    glVertex3fv(v4.get_data());
  }
  
  glEnd();
  
  // ends
  Vector3 v1(x0 + dx * x[0],   span/2, dz * z[0]);
  Vector3 v2(x0 + dx * x[1],   span/2, dz * z[1]);
  Vector3 v3(x0 + dx * x[1],   span/2, -dz * z[1]);
  Vector3 vn = cross(v3-v1, v2-v1).normalise();
  
  if (draw_type == NORMAL)
    glColor4f(col_end[0], col_end[1], col_end[2], 1);
  
  // left
  glBegin(GL_POLYGON);
  glNormal3fv(vn.get_data());
  for (i = 0 ; i < nx ; i++)
  {
    glVertex3f(x0 + dx * x[i],    span/2, -dz * z[i]);
  }
  for (i = nx-2 ; i > 0 ; i--)
  {
    glVertex3f(x0 + dx * x[i],    span/2, dz * z[i]);
  }
  glEnd();
  
  //right
  vn = cross(v2-v1, v3-v1).normalise();
  glBegin(GL_POLYGON);
  glNormal3fv(vn.get_data());
  for (i = 0 ; i < nx ; i++)
  {
    glVertex3f(x0 + dx * x[i],    -span/2, dz * z[i]);
  }
  for (i = nx-2 ; i > 0 ; i--)
  {
    glVertex3f(x0 + dx * x[i],    -span/2, -dz * z[i]);
  }
  glEnd();
  
  glPopMatrix();
}

//////////////////////////////////////////////////////////////////////

void Aerofoil::draw_moving(Draw_type draw_type,
                           const Joystick & joystick)
{
  TRACE_METHOD_ONLY(4);

  static const Vector3 col1(1,0,0); // red
  static const Vector3 col2(0,0,1); // blue
  static const Vector3 col3(0,1,0); // blue
  
  float f1 = fabs(rotation/180.0f);
  float f2 = 1-f1;
  float f3 = fabs(rotation/90.0f);
  
  Vector3 col_top = f2 * col1 + f3 * col3;
  Vector3 col_bot = f2 * col2 + f3 * col3;
  Vector3 col_end = (col_top + col_bot) * 0.5f;
  
  // we use a "standard" aerofoil shape, but insert an extra (pair of)
  // point(s) in the hinge location. Then when we come to draw it we
  // modify just the last section

  if (init_moving == false)
  {
    const float zscale = 0.04f;
    init_moving = true;

    ax[0]=0;
    ax[1]=0.1f;
    ax[2]=0.3f;
    ax[3]=0.6f;
    ax[4]=1;
    assert1(4 == NUM_AEROFOIL_POINTS - 2);

    az[0]=zscale * 0;
    az[1]=zscale * 0.7f;
    az[2]=zscale * 1.0f;
    az[3]=zscale * 0.7f;
    az[4]=zscale * 0;

    // initialise the pivot so that it won't be used
    pivot_index = NUM_AEROFOIL_POINTS + 10;

    // now insert the extra point
    // find the pivot index
    for (int i = 0 ; i < NUM_AEROFOIL_POINTS-1 ; ++i)
    {
      if (ax[i] > (1 - inc_offset_per_deg))
      {
        // we have overshot. copy everything up one, before
        // writing the extra value into here.
        pivot_index = i;
        for ( i = NUM_AEROFOIL_POINTS-1 ; i > pivot_index ; --i)
        {
          ax[i] = ax[i-1];
          az[i] = az[i-1];
        }
        ax[pivot_index] = (1 - inc_offset_per_deg);
        // interpolate z
        if (pivot_index == 0)
          az[pivot_index] = az[0];
        else if (pivot_index == (NUM_AEROFOIL_POINTS - 1) )
          az[pivot_index] = az[NUM_AEROFOIL_POINTS-1];
        else
        {
          az[pivot_index] = az[pivot_index-1] + 
            (az[pivot_index + 1] - az[pivot_index-1]) * 
            ( (1-inc_offset_per_deg) - ax[pivot_index-1] ) /
            (ax[pivot_index + 1] - ax[pivot_index-1]) ;
        }
        // done eveything
        goto done;
      }
    }
    // dropped off the end...
    ax[NUM_AEROFOIL_POINTS-1] = ax[NUM_AEROFOIL_POINTS-2];
    az[NUM_AEROFOIL_POINTS-1] = az[NUM_AEROFOIL_POINTS-2];
  } // initialise stuff

 done:

  float control_input = 
    control_per_chan_1 * joystick.get_value(1) + 
    control_per_chan_2 * joystick.get_value(2) + 
    control_per_chan_3 * joystick.get_value(3) + 
    control_per_chan_4 * joystick.get_value(4) + 
    control_per_chan_5 * joystick.get_value(5) + 
    control_per_chan_6 * joystick.get_value(6) + 
    control_per_chan_7 * joystick.get_value(7) + 
    control_per_chan_8 * joystick.get_value(8);

  TRACE_FILE_IF(5)
    TRACE("Aerofoil %s, control_input = %f\n", name.c_str(), control_input);

  // we will have already worked out the basic aerofoil, with the
  // extra pivot point. Now make a tweaked copy for this control
  // input.

  float x[NUM_AEROFOIL_POINTS2];
  float z[NUM_AEROFOIL_POINTS2];

  int i;
  float x0 = chord/2;
  float dx = -chord;
  float dz = chord;
  float x_offset = 0;
  float z_offset = 0;

  for (i = 0 ; i < NUM_AEROFOIL_POINTS ; i++)
  {
    float delta_x = (ax[i] - (1-inc_offset_per_deg));
    if (delta_x > 0)
    {
      x_offset = delta_x * (1.0f - cos_deg(control_input));
      z_offset = delta_x * sin_deg(control_input);
    }
    else
    {
      x_offset=0;
      z_offset=0;
    }

    x[i] = x0 + dx * ax[i] + dx * x_offset;
    x[NUM_AEROFOIL_POINTS2 - i - 1] = x[i];
    z[i] = dz * az[i] + dx * z_offset;
    z[NUM_AEROFOIL_POINTS2 - i - 1] = -dz * az[i] + dx * z_offset;
  }
      
  glPushMatrix();
  
  glTranslatef(position[0], position[1], position[2]);
  
  glRotatef(rotation, 1, 0, 0);
  glRotatef(-inc,      0, 1, 0);
  
  glBegin(GL_QUADS);
  
  // top colour
  if (draw_type == NORMAL)
    glColor4f(col_top[0], col_top[1], col_top[2], 1);

  // top and bottom
  for (i = 0 ; i < NUM_AEROFOIL_POINTS2-1 ; ++i)
  {
    if (i == NUM_AEROFOIL_POINTS)
      if (draw_type == NORMAL)
        glColor4f(col_bot[0], col_bot[1], col_bot[2], 1);

    Vector3 v1(x[i],    span/2, z[i]);
    Vector3 v2(x[i],   -span/2, z[i]);
    Vector3 v3(x[i+1], -span/2, z[i+1]);
    Vector3 v4(x[i+1],  span/2, z[i+1]);
    
    Vector3 vn = cross(v3-v1, v2-v4);
    if (vn.mag2() > 0.0f)
    {
      vn.normalise();
      glNormal3fv(vn.get_data());
    }
    glVertex3fv(v4.get_data());
    glVertex3fv(v3.get_data());
    glVertex3fv(v2.get_data());
    glVertex3fv(v1.get_data());
  }
  
  glEnd();
  
  if (draw_type == NORMAL)
    glColor4f(col_end[0], col_end[1], col_end[2], 1);
  
  // left
  glBegin(GL_QUADS);
  glNormal3f(0.0f, 1.0f, 0.0f);
  for (i = 0 ; i < NUM_AEROFOIL_POINTS-1 ; ++i)
  {
    glVertex3f(x[i+1], span/2, z[i+1]);
    glVertex3f(x[i], span/2, z[i]);
    glVertex3f(x[NUM_AEROFOIL_POINTS2-i-1], span/2, z[NUM_AEROFOIL_POINTS2-i-1]);
    glVertex3f(x[NUM_AEROFOIL_POINTS2-i-2], span/2, z[NUM_AEROFOIL_POINTS2-i-2]);
  }
  glEnd();
  
  //right
  glBegin(GL_QUADS);
  glNormal3f(0.0f, -1.0f, 0.0f);
  for (i = 0 ; i < NUM_AEROFOIL_POINTS-1 ; ++i)
  {
    glVertex3f(x[i], -span/2, z[i]);
    glVertex3f(x[i+1], -span/2, z[i+1]);
    glVertex3f(x[NUM_AEROFOIL_POINTS2-i-2], -span/2, z[NUM_AEROFOIL_POINTS2-i-2]);
    glVertex3f(x[NUM_AEROFOIL_POINTS2-i-1], -span/2, z[NUM_AEROFOIL_POINTS2-i-1]);
  }
  glEnd();
  
  glPopMatrix();
}

void Aerofoil::show() const
{
  TRACE("Aerofoil: %s\n", name.c_str());
  position.show("Position");
  TRACE("Chord = %f, span = %f, inc = %f, rot = %f, CL_0 = %f\n",
        chord, span, inc, rotation, CL_0);
}

vector<Aerofoil> Aerofoil::split_and_get() const
{
  vector<Aerofoil> result;

  // populate the result
  int i;
  for (i = 0 ; i < num_sections ; ++i)
  {
    result.push_back(*this);
  }
  
  // spanwise vector
  Vector v0 = span * Position(0, cos_deg(rotation), sin_deg(rotation));

  Position p0 = position - 0.5f * v0;
  
  for (i = 0 ; i < num_sections ; ++i)
  {
    result[i].position = p0 + v0 * ((i + 0.5f) / num_sections );
    result[i].span = span/num_sections;
    result[i].num_sections = 1;
  }

  return result;
}

