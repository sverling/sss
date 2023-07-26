/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file glider_structure_3ds.cpp
*/
#include "glider_structure_3ds.h"
#include "config_file.h"
#include "environment.h"
#include "glider.h"
#include "ski.h"
#include "sss.h"
#include "config.h"
#include "3ds.h"
#include "log_trace.h"

using namespace std;

/*!
  \todo support textures
  \todo calculate correct bounding radius
*/
Glider_structure_3ds::Glider_structure_3ds(Config_file structure_config,
                                           Glider & glider)
  :
  max_dist(6), // for now
  m_glider(glider),
  load3ds(new CLoad3DS),
  model(new t3DModel)
{
  TRACE_METHOD_ONLY(1);

  string glider_3ds_file;
  unsigned i;
  
  structure_config.get_value("3ds_file", glider_3ds_file);
  
  TRACE_FILE_IF(2)
    TRACE("Loading %s\n", glider_3ds_file.c_str());

  glider_3ds_file = "gliders/" + glider_3ds_file;

  // Load our .3FDS file into our model structure
  load3ds->Import3DS(model, glider_3ds_file.c_str()); 
  
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
  bounding_sphere_scale = 1.0f;
  structure_config.get_value("bounding_sphere_scale", bounding_sphere_scale);

  // that's it
  max_dist = calc_max_dist_and_hard_points();
}

Glider_structure_3ds::~Glider_structure_3ds()
{
  TRACE_METHOD_ONLY(1);
  delete load3ds;
  delete model;

  unsigned i;
  for (i = 0 ; i < m_skis.size() ; ++i)
  {
    delete m_skis[i];
  }  
}


void Glider_structure_3ds::show()
{
  TRACE("Glider_structure_3ds\n");
}


float Glider_structure_3ds::calc_max_dist_and_hard_points()
{
  int i, j, k;
  int ii;
  float radius = 1.0f;
  // Since we know how many objects our model has, go through each of them.
  for(i = 0; i < model->numOfObjects; i++)
  {
    // Make sure we have valid objects just in case.
    if(model->pObject.size() <= 0) break;
    
    // Get the current object
    t3DObject *pObject = &model->pObject[i];
    
    // Go through all of the faces (polygons) of the object
    for(j = 0; j < pObject->numOfFaces; j++)
    {
      // Go through each corner of the triangle.
      for(int whichVertex = 0; whichVertex < 3; whichVertex++)
      {
        // Get the index for each point of the face
        int index = pObject->pFaces[j].vertIndex[whichVertex];
        
        Position pos(pObject->pVerts[ index ].z,
                     pObject->pVerts[ index ].x,
                     pObject->pVerts[ index ].y);
        m_hard_points_3ds.push_back(Hard_point(pos));
        float r = pos.mag();
        
        radius = (radius > r ? radius : r);
      }
    }

  } // loop over objects

  // the hard_points vector will probably be far too big - it 
  // needs to be pruned. 

  // cube root of the target number of points
  const int cr_target_points = 4;
  const int target_points = cr_target_points * cr_target_points * cr_target_points;

  int nx = cr_target_points*3;

  enum Search_dir {UP, DOWN, NONE};
  Search_dir search_dir = NONE;

  vector<Hard_point> new_hard_points;

  if ((int) m_hard_points_3ds.size() < target_points)
    goto done;

  for (;;)
  {
    const float dx = 2.0f * radius / nx;
    const float dx2 = 0.5f * dx;
    const float offset = -radius + dx2;

    vector<float> distances;

    new_hard_points.clear();

    for (i = 0 ; i < nx ; ++i)
    {
      for (j = 0 ; j < nx ; ++j)
      {
        for (k = 0 ; k < nx ; ++k)
        {
          float x = offset + i * dx;
          float y = offset + j * dx;
          float z = offset + k * dx;

          bool found_one = false;
          Position pos(0);
          // find all the points in this box;
          for (ii = 0 ; ii < (int) m_hard_points_3ds.size() ; ++ii)
          {
            if ( (fabs(m_hard_points_3ds[ii].pos[0] - x) < dx2 ) &&
                 (fabs(m_hard_points_3ds[ii].pos[1] - y) < dx2 ) &&
                 (fabs(m_hard_points_3ds[ii].pos[2] - z) < dx2 ) )
            {
              found_one = true;
              // choose the position that is furthest from the origin, since 
              // this is more likely to get involved in collisions.
              if (m_hard_points_3ds[ii].pos.mag2() > pos.mag2())
              {
                pos = m_hard_points_3ds[ii].pos;
              }
            }
          }
          if (found_one == true)
            new_hard_points.push_back(Hard_point(pos));
        }
      }
    }
    TRACE("Found %d points with nx = %d: ", new_hard_points.size(), nx);

    int dnx = target_points - new_hard_points.size();
    // How many did we find?
    if (dnx == 0)
    {
      m_hard_points_3ds = new_hard_points;
      TRACE("Finished\n", nx);
      goto done;
    }
    else if (dnx > 0)
    {
      int delta = dnx / 10;
      if (delta == 0) delta = 1;
      // either need to increase nx or quit
      switch (search_dir)
      {
      case UP:
        nx += delta;
        TRACE("increasing nx to %d\n", nx);
        break;
      case DOWN:
        m_hard_points_3ds = new_hard_points;
        TRACE("Finished\n", nx);
        goto done;
      case NONE:
        search_dir = UP;
        nx += delta;
        TRACE("increasing nx to %d\n", nx);
        break;
      }
    }
    else
    {
      int delta = dnx / 10;
      if (delta == 0) delta = -1;
      // either need to decrease nx or quit
      switch (search_dir)
      {
      case DOWN:
        nx += delta;
        TRACE("decreasing nx to %d\n", nx);
        break;
      case UP:
        m_hard_points_3ds = new_hard_points;
        TRACE("Finished\n", nx);
        goto done;
      case NONE:
        search_dir = DOWN;
        nx += delta;
        TRACE("decreasing nx to %d\n", nx);
        break;
      }
    }
  }

 done:

  // whew!! Now we have the 3ds hard point positions, add them to the
  // real hard points list.
  for (i = 0 ; i < (int) m_hard_points_3ds.size() ; ++i)
  {
    m_hard_points.push_back(&m_hard_points_3ds[i]);
  }
  
  // now calculate the radius properly
  radius = 0.0f;
  for (i = 0 ; i < (int) m_hard_points.size() ; ++i)
  {
    float r = m_hard_points[i]->pos.mag();
    if (r > radius)
      radius = r;
  }

  return radius;
}

//! Return the net force and moment in world coordinates. exact copy
//of glider_structure_component.
void Glider_structure_3ds::get_force_and_moment(Vector3 & force,
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

void Glider_structure_3ds::update_structure(const Joystick & joystick)
{
  for (unsigned i = 0 ; i < m_skis.size() ; ++i)
  {
    m_skis[i]->update(joystick);
  }
}
