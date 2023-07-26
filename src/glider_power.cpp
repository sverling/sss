/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file glider_power.cpp
*/

#include "glider_power.h"
#include "config_file.h"
#include "glider.h"
#include "matrix_vector3.h"
#include "sss_assert.h"
#include "log_trace.h"

using namespace std;

/*!
  Currently only one type of glider power supply is supported.
*/
Glider_power * Glider_power::create(const string & engine_file,
                                    const string & engine_type,
                                    Glider & glider)
{
  bool success;
  Config_file engine_config("gliders/" + engine_file, success);
  assert2(success, "Unable to open engine glider file");
  
  if (engine_type == "default")
  {
    return new Glider_power(engine_config, glider);
  }
  else
  {
    TRACE("Unknown engine type: %s\n", engine_type.c_str());
    assert1(!"Error");
    return 0;
  }
}

Glider_power::Glider_power(Config_file & engine_config,
                           Glider & glider)
  :
  m_glider(glider)
{
  while (engine_config.find_new_block("engine"))
  {
    Glider_engine new_engine(engine_config, glider);
    m_engines.push_back(new_engine);
  }
//  assert2(m_engines.size() > 0, "Each glider needs at least one engine (strange!)");
}

/*!
  Accumulate all the forces from the constituent engines
*/
void Glider_power::get_force_and_moment(Vector3 & force,
                                        Vector3 & moment)
{
  // accumulate the forces and moments into:
  Vector3 l_force(0); // forces in local frame
  Vector3 l_moment(0); // moments in local frame

  Vector3 linear_force; // temporary

  for (unsigned int i = 0 ; i < m_engines.size() ; ++i)
  {
    m_engines[i].get_force(linear_force);

    // accumulate forces and moments
    l_force += linear_force;
    l_moment += cross(m_engines[i].m_position, linear_force);
  }
  // convert the local vectors to world coords
  moment = m_glider.get_orient() * l_moment;
  force  = m_glider.get_orient() * l_force;
}

void Glider_power::post_physics(float dt)
{
  for (unsigned int i = 0 ; i < m_engines.size() ; ++i)
  {
    m_engines[i].post_physics(dt);
  }
}

void Glider_power::show()
{
  for (unsigned int i = 0 ; i < m_engines.size() ; ++i)
  {
    m_engines[i].show();
  }
}


