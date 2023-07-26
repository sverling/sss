/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file glider_graphics_component.cpp
*/
#include "glider_graphics_component.h"
#include "aerofoil.h"
#include "fuselage.h"
#include "sss.h"
#include "config_file.h"
#include "config.h"
#include "glider.h"
#include "log_trace.h"

#include "sss_glut.h"

using namespace std;

Glider_graphics_component::Glider_graphics_component(
  Config_file graphics_config,
  Glider & glider)
  :
  m_glider(glider),
  current_shade_model(GL_INVALID_VALUE),
  list_num(glGenLists(2))
{
  TRACE_METHOD_ONLY(1);
  // don't split the aerofoils
  while (graphics_config.find_new_block("aerofoil"))
  {
    Aerofoil new_aerofoil(graphics_config, m_aerofoils);
    m_aerofoils.push_back(new_aerofoil);
  }
  
  graphics_config.reset();
  
  while (graphics_config.find_new_block("fuselage"))
  {
    Fuselage new_fuselage(graphics_config);
    m_fuselages.push_back(new_fuselage);
  }
  // I can't spell - can anyone else?
  graphics_config.reset();
  while (graphics_config.find_new_block("fusalage"))
  {
    Fuselage new_fuselage(graphics_config);
    m_fuselages.push_back(new_fuselage);
  }
  
  graphics_config.reset();
  while (graphics_config.find_new_block("propeller"))
  {
    m_propellers.push_back(Propeller(graphics_config));
  }
  
  calculate_max_dist();
  
}

/*! Works through the list of hard points and calculates the maximum
  distance of any from the glider origin. */
void Glider_graphics_component::calculate_max_dist()
{
  unsigned int i, j;
  max_dist = 0;
  // aerofoils
  for (i = 0 ; i < m_aerofoils.size() ; ++i)
  {
    vector<Hard_point> hps = m_aerofoils[i].get_hard_points();
    for (j = 0 ; j < hps.size() ; ++j)
    {
      max_dist = sss_max(max_dist, hps[j].pos.mag());
    }
  }
  // fuselages
  for (i = 0 ; i < m_fuselages.size() ; ++i)
  {
    vector<Hard_point> hps = m_fuselages[i].get_hard_points();
    for (j = 0 ; j < hps.size() ; ++j)
    {
      max_dist = sss_max(max_dist, hps[j].pos.mag());
    }
  }
  // props
  for (i = 0 ; i < m_propellers.size() ; ++i)
  {
    vector<Hard_point> hps = m_propellers[i].get_hard_points();
    for (j = 0 ; j < hps.size() ; ++j)
    {
      max_dist = sss_max(max_dist, hps[j].pos.mag());
    }
  }
  
  // scale it up a bit just to be safe...
  max_dist *= 1.1f;
}

void Glider_graphics_component::draw_glider(Draw_type draw_type)
{
  unsigned int i;
  // work out the tail and nose positions.
  
  // can only cull when we draw the aerofoil (etc) ends
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  
  for ( i = 0 ; i < m_fuselages.size() ; i++)
  {
    m_fuselages[i].draw(draw_type == NORMAL ? Fuselage::NORMAL : Fuselage::SHADOW);
  }
  
  for ( i = 0 ; i < m_propellers.size() ; i++)
  {
    m_propellers[i].draw(draw_type == NORMAL ? Propeller::NORMAL : Propeller::SHADOW);
  }
  
  for ( i = 0 ; i < m_aerofoils.size() ; i++)
  {
    if (Sss::instance()->config().moving_control_surfaces)
    {
      m_aerofoils[i].draw_moving(draw_type == NORMAL ? Aerofoil::NORMAL : Aerofoil::SHADOW,
                                 m_glider.get_real_joystick());
    }
    else
    {
      m_aerofoils[i].draw_non_moving(draw_type == NORMAL ? Aerofoil::NORMAL : Aerofoil::SHADOW);
    }
  }
  glDisable(GL_CULL_FACE);
  
}

/*!
  We assume that Object::basic_draw() has been called
*/
void Glider_graphics_component::draw(Draw_type draw_type)
{
  // if we are showing moving control surfaces we have to draw each
  // aerofoil individually. In this case we rely on the aerofoils to
  // use their own display lists.
  if (Sss::instance()->config().moving_control_surfaces)
  {
    draw_glider(draw_type);
  }
  else
  {
    if (Sss::instance()->config().shade_model != current_shade_model)
    {
      current_shade_model = Sss::instance()->config().shade_model;
      glShadeModel(Sss::instance()->config().shade_model);
      glDeleteLists(list_num, 2);
      list_num = glGenLists(2);
      glNewList(list_num, GL_COMPILE);
      draw_glider(NORMAL);
      glEndList();
      glNewList(list_num+1, GL_COMPILE);
      draw_glider(SHADOW);
      glEndList();
    }
    
    glPushMatrix();
    
    if (draw_type == NORMAL)
      glCallList(list_num);
    else
      glCallList(list_num+1);
    
    glPopMatrix();
    
  }
}

void Glider_graphics_component::show()
{
  TRACE("Glider_graphics_component\n");
}
