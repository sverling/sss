/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file glider_structure_crrcsim.cpp
*/
#include "glider_structure_crrcsim.h"
#include "config_file.h"
#include "environment.h"
#include "sss.h"
#include "config.h"
#include "glider.h"
#include "log_trace.h"

/*!  Needs to add to the vector of hard points based on the aerofoils
  and fuselages in the config file.  */
Glider_structure_crrcsim::Glider_structure_crrcsim(
  Config_file & structure_config,
  Glider & glider)
  :
  m_glider(glider)
{
  TRACE_METHOD_ONLY(1);

  bounding_sphere_scale = 1.0f;
  structure_config.get_value("bounding_sphere_scale", bounding_sphere_scale);
  
  Config_file::Config_attr_value attr_value;

  attr_value = structure_config.get_next_attr_value();

  while (attr_value.value_type != Config_file::Config_attr_value::INVALID)
  {
    if ( attr_value.attr == "gear" )
    {
      TRACE_FILE_IF(2)
        TRACE("In gear\n");
      
      int num = (int) attr_value.values[0].float_val;

      // now look for locations
      attr_value = structure_config.get_next_attr_value();
      
      while (attr_value.value_type != Config_file::Config_attr_value::INVALID)
      {
        if ( attr_value.attr == "locations" )
        {
          TRACE_FILE_IF(2)
            TRACE("In locations\n");
          
          vector<float> values;
          for (int i = 0 ; i < num ; ++i)
          {
            structure_config.get_next_values(values, 3);
            
            Position pos(values[0], values[1], -values[2]);
            pos = pos * 0.305f; // feet to m
            m_hard_points.push_back(pos);
          }
          
          calculate_max_dist();
          
          // all done - return
          return;
        }
        attr_value = structure_config.get_next_attr_value();
      }
    }
    attr_value = structure_config.get_next_attr_value();
  }
}

/*! Works through the list of hard points and calculates the maximum
  distance of any from the glider origin. */
void Glider_structure_crrcsim::calculate_max_dist()
{
  unsigned int i;
  max_dist = 0;
  for (i = 0 ; i < m_hard_points.size() ; ++i)
  {
    max_dist = sss_max(max_dist, m_hard_points[i].mag());
  }
}

//! Return the net force and moment in world coordinates
void Glider_structure_crrcsim::get_force_and_moment(Vector3 & force,
                                                    Vector3 & moment)
{
  force.set_to(0);
  moment.set_to(0);
  Vector3 temp;
  on_ground = false;

  if (m_glider.get_pos()[2] < 
      Environment::instance()->get_z(m_glider.get_pos()[0], m_glider.get_pos()[1]) + 
      get_bounding_radius())
  {
    Position terrain_pos;
    Vector3 normal;
    
    const float force_scale = 70 * m_glider.get_mass();
    // ensure that the reset position is at around z = 0, whatever the mass etc
    // assume 4 points count.
    const float descent_offset = m_glider.get_mass() * 
      Sss::instance()->config().gravity / (3 * force_scale);
    
    for (vector<Position>::iterator pos_it = m_hard_points.begin() ;
         pos_it != m_hard_points.end() ; 
         ++pos_it)
    {
      Position point_pos = m_glider.get_pos() + m_glider.get_orient() * *pos_it;
      
      Environment::instance()->get_local_terrain(point_pos, terrain_pos, normal);
      float descent = -dot(point_pos-terrain_pos, normal);
      descent += descent_offset;
      
      if (descent > 0)
      {
        on_ground = true;
        // perpendicular part
        float rebound_force = descent*force_scale;
        const Vector point_vel = m_glider.get_vel() +
          cross(m_glider.get_rot(),
                m_glider.get_orient() * *pos_it);
        
        float perp_vel = dot(point_vel, normal);
        if (perp_vel > 0)
        {
          rebound_force *= 1.0f/(1 + perp_vel);
          //          rebound_force = sss_max(rebound_force * (1 - perp_vel/3), 0.0f);
        }
        
        temp = rebound_force * normal;
        force += temp;
        moment += cross(point_pos - m_glider.get_pos(), temp);
        
        // parallel part
        Vector3 par_vel = point_vel - perp_vel * normal;
        
        float friction_force = 0.8f * rebound_force;
        if (friction_force > 0.001f) // ensures the normalise is OK
        {
          //          friction_force = sss_min(friction_force, 40.0f);
          par_vel.normalise();
          temp = par_vel* (-friction_force);
          force += temp;
          moment += cross(point_pos - m_glider.get_pos(), temp);
        }
      } // descent > 0
    } // loop over hard points
  } // glider near ground
}

void Glider_structure_crrcsim::show()
{
  TRACE("Glider_structure_crrcsim\n");
}
