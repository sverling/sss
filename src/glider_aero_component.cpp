/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file glider_aero_component.cpp
*/
#include "glider_aero_component.h"
#include "aerofoil.h"
#include "config_file.h"
#include "environment.h"
#include "glider.h"
#include "joystick.h"
#include "fuselage.h"
#include "log_trace.h"

using namespace std;

/*!  Needs to add to the vector of aerofoils based on what is in the
  config file.  */
Glider_aero_component::Glider_aero_component(Config_file & aero_config,
                                             Glider & glider)
  :
  m_glider(glider)
{
  TRACE_METHOD_ONLY(1);
  vector<Aerofoil> aerofoils;
  
  while (aero_config.find_new_block("aerofoil"))
  {
    Aerofoil new_aerofoil(aero_config, aerofoils);
    aerofoils.push_back(new_aerofoil);
  }
  
  // now split them if necessary
  unsigned i;
  for (i = 0 ; i < aerofoils.size() ; ++i)
  {
    if (aerofoils[i].get_splittable() == false)
      m_aerofoils.push_back(aerofoils[i]);
    else
    {
      vector<Aerofoil> new_aerofoils = aerofoils[i].split_and_get();
      m_aerofoils.insert(m_aerofoils.begin(), 
                         new_aerofoils.begin(), 
                         new_aerofoils.end());
    }
  }
  
  aero_config.reset();
  
  // also add a "vanilla" pair of aerofoils for every fuselage
  while (aero_config.find_new_block("fuselage"))
  {
    Fuselage fuselage(aero_config);
    
    float span = fuselage.get_average_radius();
    float chord = fuselage.get_length();
    Position mid_point = fuselage.get_mid_point();
    
    Aerofoil fus1("fuselage1", mid_point, 0, 0, span, chord);
    Aerofoil fus2("fuselage2", mid_point, 90, 0, span, chord);
    m_aerofoils.push_back(fus1);
    m_aerofoils.push_back(fus2);
  }
  
  // I can't spell
  while (aero_config.find_new_block("fusalage"))
  {
    Fuselage fuselage(aero_config);
    
    float span = fuselage.get_average_radius();
    float chord = fuselage.get_length();
    Position mid_point = fuselage.get_mid_point();
    
    Aerofoil fus1("fusalage1", mid_point, 0, 0, span, chord);
    Aerofoil fus2("fusalage2", mid_point, 90, 0, span, chord);
    m_aerofoils.push_back(fus1);
    m_aerofoils.push_back(fus2);
  }
  
  // get they gyros before the props
  aero_config.reset();
  while (aero_config.find_new_block("gyro"))
  {
    Gyro gyro(aero_config, glider);
    m_gyros.insert(std::make_pair(gyro.get_name(), gyro));
  }
  
  aero_config.reset();
  while (aero_config.find_new_block("propeller"))
  {
    Propeller * prop = new Propeller(aero_config);
    m_propellers.push_back(prop);
    Environment::instance()->register_wind_modifier(prop);    
  }
  
  // make sure we have at least one aerofoil to play with.
//  assert2(m_aerofoils.size() > 0, "Need at least one aerofoil");
}

Glider_aero_component::~Glider_aero_component()
{
  unsigned i;
  for (i = 0 ; i < m_propellers.size() ; ++i)
  {
    delete m_propellers[i];
  }
}

/*!
  Accumulate all the forces from the constituent aerofoils.
*/
void Glider_aero_component::get_force_and_moment(Vector3 & force,
                                                 Vector3 & moment)
{
  unsigned i;
  
  const Velocity wind_static_rel(
    -dot(m_glider.get_vel(), m_glider.get_vec_i_d()),
    -dot(m_glider.get_vel(), m_glider.get_vec_j_d()),
    -dot(m_glider.get_vel(), m_glider.get_vec_k_d()) );
  
  // accumulate the forces and moments into:
  Vector3 l_force(0); // forces in local frame
  Vector3 l_moment(0); // moments in local frame
  
  // temporary vectors
  Vector3 linear_force; // use fn parameter as a temporary
  Position linear_position;
  Vector3 pitch_force; // use parameter as a temporary
  Position pitch_position;
  
  // rotation rate in the body frame
  const Vector3 rot_body = transpose(m_glider.get_orient()) * m_glider.get_rot();
  
  for (i = m_aerofoils.size() ; i-- != 0 ; )
  {
    // head wind, sidewind etc. All values are in the positive x (etc)
    // directions, relative to the glider.
    Position aerofoil_pos = m_glider.get_pos() + 
      m_aerofoils[i].get_pos()[0] * m_glider.get_vec_i_d() +
      m_aerofoils[i].get_pos()[1] * m_glider.get_vec_j_d() +
      m_aerofoils[i].get_pos()[2] * m_glider.get_vec_k_d();
    
    const Velocity ambient_wind = Environment::instance()->get_wind(aerofoil_pos, 
                                                                    &m_glider);
//    aerofoil_pos.show(m_aerofoils[i].get_name().c_str());
    const Velocity ambient_wind_rel(dot(ambient_wind, m_glider.get_vec_i_d()),
                                    dot(ambient_wind, m_glider.get_vec_j_d()),
                                    dot(ambient_wind, m_glider.get_vec_k_d()) );
    
    const Velocity wind_rel = ambient_wind_rel + wind_static_rel;
    
    // we need to work out the relative wind velocity due to the
    // glider's rotation.
    
    const Position & pos = m_aerofoils[i].get_pos();
    
    // do the cross in the opposite direction, so we don't have to
    // negate the result
    Velocity wind_rel_rot = cross(pos, rot_body);
    
//      if (wind_rel_rot.sensible() == false)
//        TRACE("wind_rel_rot is bad\n");
    
    Velocity wind_rel_aerofoil = wind_rel + wind_rel_rot;
    
//      if (wind_rel_aerofoil.sensible() == false)
//        TRACE("wind_rel_aerofoil is bad\n");
    float density = Environment::instance()->get_air_density(aerofoil_pos);
    
    m_aerofoils[i].get_lift_and_drag(wind_rel_aerofoil, 
                                     m_glider.get_real_joystick(),
                                     linear_force, linear_position,
                                     pitch_force,  pitch_position,
                                     density);
    
    // accumulate forces and moments
    l_force += linear_force;
    l_moment += cross(linear_position, linear_force);
    // bit of a hack here
    l_moment += cross(pitch_position, pitch_force);
  }
  
  // and the propellers. These need the glider rotation in the glider's frame
  Vector glider_rot = transpose(m_glider.get_orient()) * m_glider.get_rot();
  
  for (i = m_propellers.size() ; i-- != 0 ; )
  {
    // disable the propeller's own wash....
    m_propellers[i]->disable_main_wash();

    // head wind, sidewind etc. All values are in the positive x (etc)
    // directions, relative to the glider.
    Position propeller_pos = m_glider.get_pos() + 
      m_propellers[i]->get_pos()[0] * m_glider.get_vec_i_d() +
      m_propellers[i]->get_pos()[1] * m_glider.get_vec_j_d() +
      m_propellers[i]->get_pos()[2] * m_glider.get_vec_k_d();
    
    const Velocity ambient_wind = 
      Environment::instance()->get_wind(propeller_pos, 
                                        &m_glider);
//    propeller_pos.show(m_propellers[i].get_name().c_str());
    const Velocity ambient_wind_rel(dot(ambient_wind, m_glider.get_vec_i_d()),
                                    dot(ambient_wind, m_glider.get_vec_j_d()),
                                    dot(ambient_wind, m_glider.get_vec_k_d()) );
    
    const Velocity wind_rel = ambient_wind_rel + wind_static_rel;
    
    // we need to work out the relative wind velocity due to the
    // glider's rotation.
    
    const Position & pos = m_propellers[i]->get_pos();
    
    // do the cross in the opposite direction, so we don't have to
    // negate the result
    Velocity wind_rel_rot = cross(pos, rot_body);
    
//      if (wind_rel_rot.sensible() == false)
//        TRACE("wind_rel_rot is bad\n");
    
    Velocity wind_rel_propeller = wind_rel + wind_rel_rot;
    
//      if (wind_rel_propeller.sensible() == false)
//        TRACE("wind_rel_propeller is bad\n");
    float density = Environment::instance()->get_air_density(propeller_pos);
    
    Vector extra_moment(0);
    
    m_propellers[i]->get_forces(wind_rel_propeller, 
                                m_glider.get_pos(),
                                m_glider.get_orient(),
                                glider_rot,
                                m_glider.get_real_joystick(),
                                *this,
                                linear_force, extra_moment, linear_position,
                                density);
    
    // accumulate forces and moments
    l_force += linear_force;
    l_moment += cross(linear_position, linear_force);
    l_moment += extra_moment;

    m_propellers[i]->enable_main_wash();
  }
  
  // convert the local vectors to world coords
  moment = m_glider.get_orient() * l_moment;
  force  = m_glider.get_orient() * l_force;
}


void Glider_aero_component::show()
{
  TRACE("Glider_aero_component\n");
  
  unsigned int i;
  for (i = 0 ; i < m_aerofoils.size() ; ++i)
  {
    m_aerofoils[i].show();
  }
}

const Gyro * Glider_aero_component::get_gyro(const std::string name) const
{
  const Gyros::const_iterator it = m_gyros.find(name);
  if (it != m_gyros.end())
    return &it->second;
  else
    return 0;
}

