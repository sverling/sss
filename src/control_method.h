/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef CONTROL_METHOD_H
#define CONTROL_METHOD_H

#include "config_file.h"

#include <string>
#include <vector>

class Joystick;

/// This class represents a mapping between control inputs (mouse, joystick
/// etc) and "channels". These channels get translated into control-surface 
/// movements in the glider/aerofoil classes
class Control_method
{
public:
  Control_method(Config_file & config);
  
  enum Control_input
  {
    JOYSTICK_1,
    JOYSTICK_2,
    JOYSTICK_3,
    JOYSTICK_4,
    JOYSTICK_5,
    JOYSTICK_6,
    JOYSTICK_BUTTON_1,
    JOYSTICK_BUTTON_2,
    MOUSE_X,
    MOUSE_Y,
    MOUSE_BUTTON_LEFT,
    MOUSE_BUTTON_MIDDLE,
    MOUSE_BUTTON_RIGHT,
    CONST_MINUS,
    CONST_ZERO,
    CONST_PLUS,
    MAX_CONTROL_INPUT
  };
  //! Returns the channel associated with the input type, or -1 if
  //! none is defined.
  int get_channel(Control_input input) const {return control_inputs[input];}

  //! warps the input value according to the warping for the
  //! particular input type.
  float get_warped_value(Control_input input, float orig);
  
  //! sets the appropriate channels in joystick to the constant 
  //! value of -1, 0 or 1
  void set_const_channels(Joystick * joystick);

  const string & get_name() const {return name;}

private:
  void handle_attr_value(
    Config_file::Config_attr_value & attr_value,
    Control_input input);
  void handle_warp(
    Config_file::Config_attr_value & attr_value,
    Control_input input);
  void handle_const(
    Config_file::Config_attr_value & attr_value,
    Control_input input);
  
  int control_inputs[MAX_CONTROL_INPUT];
  float warp[MAX_CONTROL_INPUT];
  float zero[MAX_CONTROL_INPUT];
  float scale[MAX_CONTROL_INPUT];

  //! vector of channels to be set to -1
  std::vector<int> const_minus;
  std::vector<int> const_zero;
  std::vector<int> const_plus;
  
  std::string name;
};

#endif
