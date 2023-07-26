/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_AERO_COMPONENT_H
#define GLIDER_AERO_COMPONENT_H

#include "glider_aero.h"
#include "aerofoil.h"
#include "propeller.h"
#include "gyro.h"

#include <vector>
#include <string>
#include <map>

class Config_file;

//! Concrete class for representing the aerodynamics as the sum of
//! component aerofoils
/*!
  Uses the Aerofoil class for the fine resolution
*/
class Glider_aero_component : public Glider_aero
{
public:
  //! constructor
  Glider_aero_component(Config_file & aero_config, Glider & glider);
  
  /// dtor
  ~Glider_aero_component();

  //! calculates net force and moment from all aerofoils
  void get_force_and_moment(Vector3 & force,
                            Vector3 & moment);

  const Gyro * get_gyro(const std::string name) const;
  
  virtual void show();
private:
  typedef std::map<std::string, Gyro> Gyros;

  std::vector<Aerofoil> m_aerofoils;
  std::vector<Propeller*> m_propellers;
  Gyros m_gyros;

  Glider & m_glider;
};


#endif
