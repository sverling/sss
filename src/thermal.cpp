/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file thermal.cpp
*/

#include "thermal.h"
#include "environment.h"
#include "renderer.h"
#include "sss.h"
#include "config.h"

#include <math.h>
#include "sss_assert.h"

using namespace std;

float Thermal::calc_flux(float A, float B, float ra, float rb) const
{
  return
    (TWO_PI * B * rb * rb * 0.5f + TWO_PI * A * rb * rb * rb / 3.0f) -
    (TWO_PI * B * ra * ra * 0.5f + TWO_PI * A * ra * ra * ra / 3.0f);
}

Thermal::Thermal(Position start_pos,
                 float updraft,
                 float updraft_radius,
                 float zero_radius,
                 float downdraft_radius,
                 float outer_radius,
                 float height,
                 float inflow_height,
                 float lifetime,
                 float lifecycle)
  :
  Object(start_pos),
  m_time_elapsed(lifecycle * lifetime),
  m_updraft(updraft),
  m_updraft_radius(updraft_radius),
  m_zero_radius(zero_radius),
  m_downdraft_radius(downdraft_radius),
  m_outer_radius(outer_radius),
  m_height(height),
  m_inflow_height(inflow_height),
  m_range2(outer_radius*outer_radius),
  m_lifetime(lifetime),
  m_strength(0),
  m_active(true),
  m_list_num(0)
{
  // need to calculate m_downdraft. We do this by integrating the
  // fluxes, and ensuring that the sum is equal to zero.
  // use subscripts so that r0 is the center, r1 = updraft_radius etc;

  float r0 = 0;
  float r1 = m_updraft_radius;
  float r2 = m_zero_radius;
  float r3 = m_downdraft_radius;
  float r4 = m_outer_radius;
  
  float w0 = updraft;
  float w1 = updraft;
  float w2 = 0;
  float w3; // this is what we want to work out - ie m_downdraft
  float w4 = 0;
  
  // evaluate the fluxes in each ring in turn. Could implify many of
  // these, but I think it's clearer keeping all the terms the same in
  // each ring

  // in the integration, represent w = Ar + B.
  /*
    r1/
    flux = | 2 pi r (A r + B) dr 
    r0/                     
  */

  // between r0 and r1
  A01 = (w1 - w0)/(r1 - r0);
  B01 = w0 - r0 * (w1 - w0)/(r1 - r0);
  flux01 = calc_flux(A01, B01, r0, r1);
  
  // between r1 and r2
  A12 = (w2 - w1)/(r2 - r1);
  B12 = w1 - r1 * (w2 - w1)/(r2 - r1);
  flux12 = calc_flux(A12, B12, r1, r2);
  
  float flux_up = flux01 + flux12;

  // Since I don't feel like solving for w3 analytically just at the
  // moment, iteratively find it, picking a vaguely sensible value to
  // start with.

  w3 = - w1 * (r1 * r1) / (r3 * r3); // just a starting point.

  float flux_down = flux_up;

  unsigned i;
  for (i = 0 ; i < 5 ; ++i)
  {
    // adjust w3
    w3 = w3 * flux_up/flux_down;

    // between r2 and r3
    A23 = (w3 - w2)/(r3 - r2);
    B23 = w2 - r2 * (w3 - w2)/(r3 - r2);
    flux23 = calc_flux(A23, B23, r2, r3);
    
    // between r3 and r4
    A34 = (w4 - w3)/(r4 - r3);
    B34 = w3 - r3 * (w4 - w3)/(r4 - r3);
    flux34 = calc_flux(A34, B34, r3, r4);
    
    flux_down = -(flux23 + flux34);
  }

  m_downdraft = -w3;

}

// Updraft is pretty easy - use w = Ar + B in each zone, then scale it
// so that so that there is a 50m region at the bottom (and top?).
// For the inflow, the peak flow rate is derived from the total flux
// integrated over the region inside the point. This is done assuming
// a triangle inflow distribution of 50m (or inflow_height). So, the
// peak flow (at a height of 25m) is twice the average.
Velocity Thermal::get_wind(const Position & pos) const
{
  float radius = hypot(pos[0] - get_pos()[0], pos[1] - get_pos()[1]);
  
  if (radius > m_outer_radius)
  {
    return Velocity(0, 0, 0);
  }

  float u, v, w;
  float inner_flux = 0; // total flux inside this point

  float altitude = pos[2] -  Environment::instance()->get_z(pos[0], pos[1]);
  const float outflow_height = m_inflow_height;

  if (altitude > m_height)
  {
    return Velocity(0, 0, 0);
  }
  
  if (radius < m_updraft_radius)
  {
    w = A01 * radius + B01;
    if ( (altitude < m_inflow_height) || 
         (altitude > (m_height - outflow_height) ) )
      inner_flux = calc_flux(A01, B01, 0, radius);
  }
  else if (radius < m_zero_radius)
  {
    w = A12 * radius + B12;
    if ( (altitude < m_inflow_height) || 
         (altitude > (m_height - outflow_height) ) )
      inner_flux = flux01 + 
        calc_flux(A12, B12, m_updraft_radius, radius);
  }
  else if (radius < m_downdraft_radius)
  {
    w = A23 * radius + B23;
    if ( (altitude < m_inflow_height) || 
         (altitude > (m_height - outflow_height) ) )
      inner_flux = flux01 + flux12 +
        calc_flux(A23, B23, m_zero_radius, radius);
  }
  else
  {
    assert1(radius <= m_outer_radius);
    w = A34 * radius + B34;
    if ( (altitude < m_inflow_height) || 
         (altitude > (m_height - outflow_height) ) )
      inner_flux = flux01 + flux12 + flux23 +
        calc_flux(A34, B34, m_downdraft_radius, radius);
  }

  if (altitude < m_inflow_height)
  {
    // calculate inflow
    const float inflow_area = 2 * PI * radius * m_inflow_height;
    // 2 is because this is the peak, not the mean
    float peak_inflow = 2 * inner_flux/inflow_area;
    float inflow;
    const float inflow_height2 = 0.5f*m_inflow_height;
    if (altitude < inflow_height2)
    {
      inflow = peak_inflow * altitude/inflow_height2;
    }
    else
    {
      inflow = peak_inflow - 
        peak_inflow * (altitude-inflow_height2)/inflow_height2;
    }
    Vector dir = get_pos() - pos;
    dir[2] = 0;
    dir.normalise();
    u = dir[0] * inflow;
    v = dir[1] * inflow;
    // This is a hack - scale w
    if (altitude < inflow_height2)
      w *= altitude/inflow_height2;
    // \todo add on the ground slope contribution
    Position terrain_pos;
    Vector3 normal;
    Environment::instance()->get_local_terrain(pos, 
                                               terrain_pos,
                                               normal);
    // theta = deviation of normal from up.
    float tan_theta;
    // x-dir first
    tan_theta = -normal[0]/normal[2];
    w += u * tan_theta;
    // y-dir
    tan_theta = -normal[1]/normal[2];
    w += v * tan_theta;
  }
  else if (altitude > (m_height - outflow_height))
  {
    //calculate outflow
    const float outflow_area = 2 * PI * radius * outflow_height;
    // 2 is because this is the peak, not the mean
    float peak_outflow = 2 * inner_flux/outflow_area;
    float outflow;
    const float outflow_height2 = 0.5f*outflow_height;
    if (altitude < (m_height - outflow_height2))
    {
      outflow = peak_outflow * 
        (altitude - (m_height - outflow_height)) / 
        outflow_height2;
    }
    else
    {
      outflow = peak_outflow - 
        peak_outflow * 
        (altitude-(m_height - outflow_height2)) /
        outflow_height2;
    }
    Vector dir = get_pos() - pos;
    dir[2] = 0;
    dir.normalise();
    u = -dir[0] * outflow;
    v = -dir[1] * outflow;
    // This is a hack - scale w
    w *= (m_height - altitude)/outflow_height;
  }
  else
  {
    u = v = 0;
  }
  
  // scale it all by the thermal strength
  return m_strength * Velocity(u, v, w);
}

void Thermal::update(float dt)
{
  m_time_elapsed += dt;

  // calculate new locate
  const float steering_height = m_height*0.5f;
  float x = get_pos()[0];
  float y = get_pos()[1];
  
  float z = Environment::instance()->get_z(x, y) + steering_height;
  
  Position new_pos = 
    get_pos() + 
    dt * Environment::instance()->get_ambient_wind(Position(x, y, z));

  new_pos[2] = z - steering_height; // i.e. location
  
  set_pos(new_pos);

  // calculate strength
  m_strength = pow(fabs(sin( (m_time_elapsed/m_lifetime) * PI )), 0.3f);
//  cout << "strength = " << m_strength << endl;
  
  if (m_time_elapsed > m_lifetime)
  {
    m_active = false;
  }
}

void Thermal::draw() const
{
  if (m_list_num == 0)
  {
    m_list_num = glGenLists(2);
    if (m_list_num == 0)
    {
      assert1(!"Error creating display list");
    }
    
    // ===== the solid one ==========
    glNewList(m_list_num, GL_COMPILE);

    glTranslatef(0, 0, -m_height);
//      glEnable(GL_CULL_FACE);
//      glCullFace(GL_FRONT); // draw the back first
//      gluCylinder( Renderer::instance()->quadric(),
//                   m_updraft_radius,
//                   m_updraft_radius,
//                   1000.0f,
//                   8, 
//                   3);
//      glDisable(GL_CULL_FACE);
    
    gluCylinder( Renderer::instance()->quadric(),
                 1,
                 1,
                 m_height * 2,
                 12, 
                 2);

//    glutSolidTetrahedron();

    glEndList();

    // ===== The translucent one ==========
    glNewList(m_list_num+1, GL_COMPILE);

    glTranslatef(0, 0, -m_height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT); // draw the back first
    gluCylinder( Renderer::instance()->quadric(),
                 m_updraft_radius,
                 m_updraft_radius,
                 m_height * 2,
                 12, 
                 2);
    
    glCullFace(GL_BACK); // then the front
    gluCylinder( Renderer::instance()->quadric(),
                 m_updraft_radius,
                 m_updraft_radius,
                 m_height * 2,
                 12, 
                 2);
    glDisable(GL_CULL_FACE);
    
    glDisable(GL_BLEND);
    
    glEndList();

  }
    
  glPushMatrix();
  basic_draw();
  glColor4f(0, 1, 0, m_strength * 0.5f);
  if (Sss::instance()->config().thermal_show_type == Config::SOLID)
  {
    glCallList(m_list_num);
  }
  else
  {
    glCallList(m_list_num + 1);
  }
  
  glPopMatrix();
}

void Thermal::reset(const Position & start_pos)
{
  set_pos(start_pos);
  m_strength = 0;
  m_active = true;
  m_time_elapsed = 0;
}
