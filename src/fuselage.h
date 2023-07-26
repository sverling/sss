/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef FUSELAGE_H
#define FUSELAGE_H

#include "types.h"
#include "object.h"

#include <string>
#include <vector>
using namespace std;

class Config_file;

//! Represents a fuselage - only deals with rendering and "hard points"
class Fuselage
{
public:
  Fuselage(Config_file & aero_config);
  Fuselage(const string & name);

  //! Adds a point to the end of the fuselage
  void add_point(const Position & pos, float radius);

  //! Returns the list of hard points
  const vector<Hard_point> & get_hard_points() const {return hard_points;}

  //! Gets the average radius (weighted by segment length)
  float get_average_radius() const;

  //! Gets the total length - last point - first point
  float get_length() const;

  //! Gets the mid-point - average of first and last points
  Position get_mid_point() const;

  enum Draw_type { NORMAL, SHADOW };
  void draw(Draw_type draw_type);

private:

  void calculate_hard_points();

  //! Represents an element of the fuselage
  struct Location
  {
    Location(const Position & pos, const float radius) : 
      pos(pos), radius(radius) {};
    Position pos;
    float radius;
  };
  
  vector<Location> locations;
  vector<Hard_point> hard_points;

  string name;
};


#endif // file included


