/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file glider_structure_component.cpp
*/
#include "glider_structure_component.h"
#include "config_file.h"
#include "fuselage.h"
#include "aerofoil.h"
#include "ski.h"
#include "propeller.h"
#include "environment.h"
#include "sss.h"
#include "config.h"
#include "glider.h"
#include "log_trace.h"

/*!  Needs to add to the vector of hard points based on the aerofoils
  and fuselages in the config file.  */
Glider_structure_component::Glider_structure_component(
  Config_file & structure_config,
  Glider & glider)
  :
  m_glider(glider)
{
  TRACE_METHOD_ONLY(1);
  unsigned int i;
  vector<Aerofoil> aerofoils;
  
  structure_config.reset();
  
  while (structure_config.find_new_block("aerofoil"))
  {
    
    Aerofoil * new_aerofoil = new Aerofoil(structure_config, aerofoils);
    m_aerofoils.push_back(new_aerofoil);
    aerofoils.push_back(*new_aerofoil);
    
    const vector<Hard_point> & hps = new_aerofoil->get_hard_points();
    for (i = 0 ; i < hps.size() ; ++i)
    {
      m_hard_points.push_back(&hps[i]);
    }
  }
  
  structure_config.reset();
  
  while (structure_config.find_new_block("fuselage"))
  {
    Fuselage * new_fuselage = new Fuselage(structure_config);
    m_fuselages.push_back(new_fuselage);
    const vector<Hard_point> & hps = new_fuselage->get_hard_points();
    for (i = 0 ; i < hps.size() ; ++i)
    {
      m_hard_points.push_back(&hps[i]);
    }
  }
  // I can't spell
  structure_config.reset();
  while (structure_config.find_new_block("fusalage"))
  {
    Fuselage * new_fuselage = new Fuselage(structure_config);
    m_fuselages.push_back(new_fuselage);
    const vector<Hard_point> & hps = new_fuselage->get_hard_points();
    for (i = 0 ; i < hps.size() ; ++i)
    {
      m_hard_points.push_back(&hps[i]);
    }
  }
  
  structure_config.reset();
  
  while (structure_config.find_new_block("ski"))
  {
    Ski * new_ski = new Ski(structure_config);
    m_skis.push_back(new_ski);
    const vector<Hard_point> & hps = new_ski->get_hard_points();
    for (i = 0 ; i < hps.size() ; ++i)
    {
      m_hard_points.push_back(&hps[i]);
    }
  }
  
  structure_config.reset();
  
  while (structure_config.find_new_block("propeller"))
  {
    Propeller * new_propeller = new Propeller(structure_config);
    m_propellers.push_back(new_propeller);
    const vector<Hard_point> & hps = new_propeller->get_hard_points();
    for (i = 0 ; i < hps.size() ; ++i)
    {
      m_hard_points.push_back(&hps[i]);
    }
  }
  
  bounding_sphere_scale = 1.0f;
  structure_config.get_value("bounding_sphere_scale", bounding_sphere_scale);

  calculate_max_dist();
  
}

Glider_structure_component::~Glider_structure_component()
{
  unsigned i;
  for (i = 0 ; i < m_aerofoils.size() ; ++i)
  {
    delete m_aerofoils[i];
  }
  for (i = 0 ; i < m_fuselages.size() ; ++i)
  {
    delete m_fuselages[i];
  }
  for (i = 0 ; i < m_skis.size() ; ++i)
  {
    delete m_skis[i];
  }
  for (i = 0 ; i < m_propellers.size() ; ++i)
  {
    delete m_propellers[i];
  }
  
}

void Glider_structure_component::get_extra_ang_mom(const Joystick & joystick,
                                                   Vector3 & ang_mom) const
{
  Vector3 l_ang_mom(0.0f);
  
  for (unsigned i = 0 ; i < m_propellers.size() ; ++i)
  {
    m_propellers[i]->get_ang_mom(joystick, l_ang_mom);
  }
  ang_mom = l_ang_mom;
}


/*! Works through the list of hard points and calculates the maximum
  distance of any from the glider origin. */
void Glider_structure_component::calculate_max_dist()
{
  unsigned int i;
  max_dist = 0;
  for (i = 0 ; i < m_hard_points.size() ; ++i)
  {
    max_dist = sss_max(max_dist, m_hard_points[i]->pos.mag());
  }
}

//! Return the net force and moment in world coordinates
void Glider_structure_component::get_force_and_moment(Vector3 & force,
                                                      Vector3 & moment)
{
  force.set_to(0);
  moment.set_to(0);
  Vector3 temp;
  on_ground = false;

  if (m_glider.get_pos()[2] < 
      Environment::instance()->get_z(m_glider.get_pos()[0], 
                                     m_glider.get_pos()[1]) + 
      get_bounding_radius())
  {
    Position terrain_pos;
    Vector3 normal;
    
    const float force_scale = 70 * m_glider.get_mass();
    // ensure that the reset position is at around z = 0, whatever the mass etc
    // assume 4 points count.
    const float descent_offset = m_glider.get_mass() * 
      Sss::instance()->config().gravity / (3 * force_scale);
    
    for (vector<const Hard_point *>::iterator hp_it = m_hard_points.begin() ;
         hp_it != m_hard_points.end() ; 
         ++hp_it)
    {
      Position point_pos = m_glider.get_pos() + 
        m_glider.get_orient() * (*hp_it)->pos;
      
      Environment::instance()->get_local_terrain(point_pos, terrain_pos, normal);
      float descent = -dot(point_pos-terrain_pos, normal);
      descent += descent_offset;
      
      if (descent > 0)
      {
        on_ground = true;
        // perpendicular part
        float rebound_force = descent * force_scale * (*hp_it)->hardness;
        const Vector point_vel = m_glider.get_vel() +
          m_glider.get_orient() * (*hp_it)->m_vel + 
          cross(m_glider.get_rot(),
                m_glider.get_orient() * (*hp_it)->pos);
        
        float perp_vel = dot(point_vel, normal);
        if (perp_vel > 0)
        {
          rebound_force *= 1.0f/(1 + perp_vel);
          //          rebound_force = sss_max(rebound_force * (1 - perp_vel*0.5f), 0.0f);
        }
        
        temp = rebound_force * normal;
        force += temp;
        moment += cross(point_pos - m_glider.get_pos(), temp);
        
        // parallel part
        Vector3 par_vel = point_vel - perp_vel * normal;
        
        if (par_vel.mag2() > 0.000001f) // ensures the normalise is OK
        {
          //          friction_force = sss_min(friction_force, 40.0f);
          par_vel.normalise();
          
          Vector min_friction_dir = m_glider.get_orient() * 
            (*hp_it)->min_friction_dir ;
          
          float mu_max = (*hp_it)->mu_max;
          float mu_min = (*hp_it)->mu_min;
          
          // parallel to the min friction dir has mu_min
          Vector par_vel_min = dot(par_vel, min_friction_dir) * min_friction_dir;
          float friction_force_min_mag = mu_min * rebound_force;
          Vector friction_force_min = - friction_force_min_mag * par_vel_min;
          
          // component perp to min friction is the "remainder" and has
          // mu_max
          Vector par_vel_max = par_vel - par_vel_min;
          float friction_force_max_mag = mu_max * rebound_force;
          Vector friction_force_max = - friction_force_max_mag * par_vel_max;
          
//           float mu = mu_max - (mu_max - mu_min) * 
//             fabs(dot(min_friction_dir, par_vel));
          
//           float friction_force = mu * rebound_force;
//           temp = par_vel* (-friction_force);
          
          temp = friction_force_min + friction_force_max;
          
          force += temp;
          moment += cross(point_pos - m_glider.get_pos(), temp);
        }
      } // descent > 0
    } // loop over hard points
  } // glider near ground
}

void Glider_structure_component::show()
{
  TRACE("Glider_structure_component\n");
}

void Glider_structure_component::update_structure(const Joystick & joystick)
{
  for (unsigned i = 0 ; i < m_skis.size() ; ++i)
  {
    m_skis[i]->update(joystick);
  }
}
