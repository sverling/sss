/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file fuselage.cpp
*/

#include "fuselage.h"
#include "renderer.h"
#include <stdio.h>
#include "config_file.h"
#include "sss_glut.h"
#include "log_trace.h"

using namespace std;

#ifndef unix
#define strcasecmp _stricmp
#endif

Fuselage::Fuselage(const string & name)
  :
  name(name)
{
}

Fuselage::Fuselage(Config_file & aero_config)
{
  TRACE_METHOD_ONLY(1);
  aero_config.get_value_assert("name",            name);

  Config_file::Config_attr_value attr_value;
  
  while (true)
  {
    attr_value = aero_config.get_next_attr_value();
    if (attr_value.value_type == Config_file::Config_attr_value::INVALID)
    {
      assert1(!"Invalid attribute/value");
    }
    assert1(attr_value.num > 0);
    
    if (attr_value.attr == "end")
    {
      if ( (attr_value.values[0].string_val == "fuselage") ||
           (attr_value.values[0].string_val == "fusalage") )
      {
        calculate_hard_points();
        return;
      }
      TRACE("Unexpected end: %s\n", attr_value.values[0].string_val.c_str());
    } 
    else if (attr_value.attr == "segment")
    {
      assert1(attr_value.num == 4);
      assert1(attr_value.value_type == Config_file::Config_attr_value::FLOAT);
      float x = attr_value.values[0].float_val;
      float y = attr_value.values[1].float_val;
      float z = attr_value.values[2].float_val;
      float r = attr_value.values[3].float_val;
      locations.push_back(Location(Position(x, y, z), r));
    }
    else if (attr_value.attr == "name")
    {
      assert1(attr_value.num == 1);
      assert1(attr_value.value_type == Config_file::Config_attr_value::STRING);
      name = attr_value.values[0].string_val;
    }
    else
    {
      TRACE("Unknown attribute: %s\n", attr_value.attr.c_str());
    }
  }
}

void Fuselage::add_point(const Position & pos, float radius)
{
  locations.push_back(Location(pos, radius));
  calculate_hard_points();
}

void Fuselage::calculate_hard_points()
{
  hard_points.resize(0);
  const float mu_max = 0.9;
  const float mu_min = 0.2;
  const float hardness = 1.0;

  for (unsigned int i = 0 ; i < locations.size() ; ++i)
  {
    // only need multiple points if they're significantly spread out
    // for now hard code each hard point so that it has quite a low
    // friction in the direction of the glider (+ve x)
    if (locations[i].radius > 0.01)
    {
      hard_points.push_back(
        Hard_point(locations[i].pos + Position(0, locations[i].radius, 0),
                   Vector(1, 0, 0),
                   mu_min,
                   mu_max,
                   hardness));
      hard_points.push_back(
        Hard_point(locations[i].pos - Position(0, locations[i].radius, 0),
                   Vector(1, 0, 0),
                   mu_min,
                   mu_max,
                   hardness));
      hard_points.push_back(
        Hard_point(locations[i].pos + Position(0, 0, locations[i].radius),
                   Vector(1, 0, 0),
                   mu_min,
                   mu_max,
                   hardness));
      hard_points.push_back(
        Hard_point(locations[i].pos - Position(0, 0, locations[i].radius),
                   Vector(1, 0, 0),
                   mu_min,
                   mu_max,
                   hardness));
    }
    else 
    {
      hard_points.push_back(Hard_point(locations[i].pos,
                                       Vector(1, 0, 0),
                                       mu_min,
                                       mu_max,
                                       hardness));
    }
  }
}

void Fuselage::draw(Draw_type draw_type)
{
  if (draw_type == NORMAL)
    glColor4f(0.7,0.7,0.7,1);

  for (unsigned int i = 0 ; i < locations.size()-1 ; ++i)
  {
    glPushMatrix();
    glTranslatef(locations[i].pos[0], locations[i].pos[1], locations[i].pos[2]);
    glRotatef(90,0,1,0);
    gluCylinder(Renderer::instance()->quadric(),
                locations[i].radius, locations[i+1].radius,
                locations[i+1].pos[0] - locations[i].pos[0], 
                16, 2);
    glPopMatrix();
  }
}

float Fuselage::get_average_radius() const
{
  float total = 0;
  float dist_total = 0;
  unsigned int i;
  float dist;
  // do the middle bits
  const unsigned num = locations.size();
  assert1(num > 1);
  for (i = 1 ; i < num-1 ; ++i)
  {
    dist = 0.5 * 
      (locations[i+1].pos - locations[i-1].pos).mag();
    total += locations[i].radius * dist;
    dist_total += dist;
  }
  // now the ends
  dist = (locations.front().pos - locations[0].pos).mag();
  total += locations[0].radius * dist;
  dist_total += dist;

  dist = (locations[num-1].pos - locations[num-2].pos).mag();
  total += locations[num-1].radius * dist;
  dist_total += dist;

  return (total/dist_total);
}

float Fuselage::get_length() const
{
  assert1(!locations.empty());
  return (locations.front().pos - locations.back().pos).mag();
}

Position Fuselage::get_mid_point() const
{
  assert1(!locations.empty());
  return (locations.front().pos + locations.back().pos)*0.5;
}

