/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef THERMAL_MANAGER_H
#define THERMAL_MANAGER_H

#include "types.h"
#include "thermal.h"

#include <vector>
#include <set>
using namespace std;

class Thermal;

/*!  Keeps track of thermals, arranging them so that they wrap
  around the domain...  */
class Thermal_manager
{
public:
  Thermal_manager(float xmin, float xmax,
                  float ymin, float ymax);
  
  ~Thermal_manager() {};

  Velocity get_wind(const Position & pos) const;

  //! restarts everything reading config again
  void reinitialise();

  void draw_thermals() const;
  
  void update_thermals(float dt);
  
private:

  vector<Thermal> thermals;
  typedef vector<Thermal>::iterator Thermals_it;
  typedef vector<Thermal>::const_iterator Thermals_const_it;
  
  set<Thermals_const_it> effective_thermals;
  typedef set<Thermals_const_it>::const_iterator Effective_thermals_const_it;
  
  int   m_num_thermals;
  float m_xmin, m_xmax, m_ymin, m_ymax;
  float m_mean_updraft;  
  float m_sigma_updraft; 
  float m_mean_radius;    
  float m_sigma_radius;   
  float m_mean_height;
  float m_mean_inflow_height;
  float m_mean_lifetime;  
  float m_sigma_lifetime;
};

#endif






















