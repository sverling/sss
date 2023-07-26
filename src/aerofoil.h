/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef AEROFOIL_H
#define AEROFOIL_H

#include "types.h"
#include "object.h"
#include <string>
#include <stdio.h>
#include <vector>

class Config_file;
class Joystick;

// assumed to be in the x-y plane, with forward in +ve x direction

/// This class represents an aerofoils graphical, aerodynamic and
/// structural properties.
class Aerofoil
{
public:
  //! the ctor for fresh aerofoils
  Aerofoil(Config_file & aero_config, const std::vector<Aerofoil> & aerofoils);

  //! allow construction of a very plain aerofoil
  Aerofoil(const std::string & name,
           const Position & pos,
           float rotation,
           float inc,
           float span,
           float chord);

  // use the default copy ctor to copy an already-defined aerofoil
  //Aerofoil(const Aerofoil &); 
  
  // the returned force and force_position are in the original
  // reference frame (i.e. before inc and rotation have been applied),
  // so the caller need not worry about accounting for inc and
  // rotation
  void get_lift_and_drag(const Velocity & wind_rel, // in
                         const Joystick & joystick,
                         Vector3 & force, // force
                         Position & force_position, // location of force
                         Vector3 & pitch_force,
                         Position & pitch_position,
                         float density); // air density
  
  enum Draw_type { NORMAL, SHADOW };
  /// draw with moving surface
  void draw_moving(Draw_type draw_type, const Joystick & joystick);

  /// draw as one object
  void draw_non_moving(Draw_type draw_type);
  
  const Position & get_pos() const {return position;}

  float get_chord() const {return chord;}

  const std::string & get_name() const {return name;}

  /// Returns a list of points used for collision response.
  const std::vector<Hard_point> & get_hard_points() const {return hard_points;}

  /// Returns a vector of smaller aerofoils that can replace this one.
  std::vector<Aerofoil> split_and_get() const;

  /// indicates if this aerofoil could/should be split
  bool get_splittable() const {return num_sections > 1;}

  void show() const;
private: // helper methods
  
  inline float get_area() {return chord * span;}
  
  typedef enum Flying
  {
    FORWARD = 0, // note, used as an array index
    STALLED,
    REVERSE
  };

  float get_CL(float alpha, float control_input, 
               Flying & flying, // returned
               float & flying_float);
  
  float get_CD(float alpha, float control_input, Flying flying, float CL);

  void calculate_hard_points();
  void calculate_constants();
  std::vector<Hard_point> hard_points;
  
  Aerofoil() {} // don't use!
  
private:
  std::string name;
  int num_sections;
  
  Position position;
  
  Vector3 x_offset; // x position offsets when flying, stalled and reverse flying
  
  float chord, span; // average values
  float rotation;    // rotation about x axis
  float inc;         // angle of incidence (after rotation)
  
  // CL params
  float CL_drop_scale; // when we stall, CL drops by this much
  float CL_reverse_scale; // scale parms by this for wing flying backwards
  
  float CL_per_alpha; // in units per deg
  
  float CL_0; // +ve for a wing aerofoil
  float CL_max; // so we stall at CL_max / CL_per_alpha
  float CL_min; // similarly
  
  // CD params
  float CD_prof, CD_induced_frac, CD_max;
  
  // pitching moment
  float CM_0;  // pitching moment for control = 0

  // control surface params
  float CL_offset_per_deg;  // change in CL (of whole graph) 
  float CD_prof_per_deg;  // change in CD_prof
  float CM_per_deg;         // pitching moment per control
  float inc_offset_per_deg; // effective rotation of aerofoil
  float control_per_chan_1;     //control per joystick channel
  float control_per_chan_2;     //control per joystick channel
  float control_per_chan_3;     //control per joystick channel
  float control_per_chan_4;     //control per joystick channel
  float control_per_chan_5;     //control per joystick channel
  float control_per_chan_6;     //control per joystick channel
  float control_per_chan_7;     //control per joystick channel
  float control_per_chan_8;     //control per joystick channel

  float CL_0_r;
  float CL_max_r;
  float CL_min_r;
  
  float CL_max_d;
  float CL_min_d;
  float CL_max_r_d;
  float CL_min_r_d;
  
  //  float alpha_CL_0;
  float alpha_CL_max;
  float alpha_CL_max_d;
  float alpha_CL_min;
  float alpha_CL_min_d;
  //  float alpha_CL_0_r;
  float alpha_CL_max_r;
  float alpha_CL_max_r_d;
  float alpha_CL_min_r;
  float alpha_CL_min_r_d;

  enum {NUM_POINTS = 14};
  float x[NUM_POINTS];
  float y[NUM_POINTS];
  Flying flight[NUM_POINTS];

  // for drawing the moving aerofoil
  bool init_moving;
  int pivot_index;
  enum {
    NUM_AEROFOIL_POINTS = 6,
    NUM_AEROFOIL_POINTS2 = NUM_AEROFOIL_POINTS * 2
  };
  float ax[NUM_AEROFOIL_POINTS], az[NUM_AEROFOIL_POINTS];
};

#endif
