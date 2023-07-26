/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#include "control_method.h"
#include "config_file.h"
#include "joystick.h"
#include "log_trace.h"

#include <math.h>

#include <iostream>
using namespace std;

Control_method::Control_method(Config_file & config)
{
  TRACE_METHOD_ONLY(1);
  // initialise the lookup array
  for (unsigned i = 0 ; i < MAX_CONTROL_INPUT ; ++i)
  {
    control_inputs[i] = -1;
    warp[i] = 1.0;
    scale[i] = 1.0;
  }

  config.get_next_value_assert("name", name);

  while (true)
  {
    Config_file::Config_attr_value attr_value = config.get_next_attr_value();
    if (attr_value.attr == "end")
      return;
    else if (attr_value.attr == "joystick_1")
      handle_attr_value(attr_value, JOYSTICK_1);
    else if (attr_value.attr == "joystick_2")
      handle_attr_value(attr_value, JOYSTICK_2);
    else if (attr_value.attr == "joystick_3")
      handle_attr_value(attr_value, JOYSTICK_3);
    else if (attr_value.attr == "joystick_4")
      handle_attr_value(attr_value, JOYSTICK_4);
    else if (attr_value.attr == "joystick_5")
      handle_attr_value(attr_value, JOYSTICK_5);
    else if (attr_value.attr == "joystick_6")
      handle_attr_value(attr_value, JOYSTICK_6);
    else if (attr_value.attr == "joystick_button1")
      handle_attr_value(attr_value, JOYSTICK_BUTTON_1);
    else if (attr_value.attr == "joystick_button2")
      handle_attr_value(attr_value, JOYSTICK_BUTTON_2);
    else if (attr_value.attr == "mouse_x")
      handle_attr_value(attr_value, MOUSE_X);
    else if (attr_value.attr == "mouse_y")
      handle_attr_value(attr_value, MOUSE_Y);
    else if (attr_value.attr == "mouse_button_left")
      handle_attr_value(attr_value, MOUSE_BUTTON_LEFT);
    else if (attr_value.attr == "mouse_button_middle")
      handle_attr_value(attr_value, MOUSE_BUTTON_MIDDLE);
    else if (attr_value.attr == "mouse_button_right")
      handle_attr_value(attr_value, MOUSE_BUTTON_RIGHT);
    else if (attr_value.attr == "joystick_1_exp")
      handle_warp(attr_value, JOYSTICK_1);
    else if (attr_value.attr == "joystick_2_exp")
      handle_warp(attr_value, JOYSTICK_2);
    else if (attr_value.attr == "joystick_3_exp")
      handle_warp(attr_value, JOYSTICK_3);
    else if (attr_value.attr == "joystick_4_exp")
      handle_warp(attr_value, JOYSTICK_4);
    else if (attr_value.attr == "joystick_5_exp")
      handle_warp(attr_value, JOYSTICK_5);
    else if (attr_value.attr == "joystick_6_exp")
      handle_warp(attr_value, JOYSTICK_6);
    else if (attr_value.attr == "mouse_x_exp")
      handle_warp(attr_value, MOUSE_X);
    else if (attr_value.attr == "mouse_y_exp")
      handle_warp(attr_value, MOUSE_Y);
    else if (attr_value.attr == "const_minus_one")
      handle_const(attr_value, CONST_MINUS);
    else if (attr_value.attr == "const_zero")
      handle_const(attr_value, CONST_ZERO);
    else if (attr_value.attr == "const_plus_one")
      handle_const(attr_value, CONST_PLUS);
    else
    {
      TRACE("invalid attribute: %s\n", attr_value.attr.c_str());
      return;
    }
  }
}

void Control_method::handle_attr_value(
  Config_file::Config_attr_value & attr_value, 
  Control_input input)
{
  if (attr_value.value_type != Config_file::Config_attr_value::FLOAT)
  {
    TRACE("Invalid value type\n");
    return;
  }
  control_inputs[input] = (int) (attr_value.values[0].float_val);
}

void Control_method::handle_warp(
  Config_file::Config_attr_value & attr_value, 
  Control_input input)
{
  if (attr_value.value_type != Config_file::Config_attr_value::FLOAT)
  {
    TRACE("Invalid value type\n");
    return;
  }
  
  // let the user reverse the sense
  scale[input] = (attr_value.values[0].float_val) < 0.0f ? -1.0f : 1.0f;

  warp[input] = fabs((attr_value.values[0].float_val));
  if (warp[input] < 0.1)
  {
    TRACE("input warp is very small!!\n");
    warp[input] = 0.1F;
  }
}

void Control_method::handle_const(
  Config_file::Config_attr_value & attr_value, 
  Control_input input)
{
  if (attr_value.value_type != Config_file::Config_attr_value::FLOAT)
  {
    TRACE("Invalid value type\n");
    return;
  }
  
  int chan = (int) (attr_value.values[0].float_val);

  switch (input)
  {
  case CONST_MINUS:
//    cout << this << ": Setting channel " << chan << " to constant -1\n";
    const_minus.push_back(chan);
    break;
  case CONST_ZERO:
//    cout << this << ": Setting channel " << chan << " to constant 0\n";
    const_zero.push_back(chan);
    break;
  case CONST_PLUS:
//    cout << this << ": Setting channel " << chan << " to constant +1\n";
    const_plus.push_back(chan);
    break;
  default:
    TRACE("Invalid input!\n");
  }
}

float Control_method::get_warped_value(Control_input input, float orig)
{
  return (float) 
    (pow(fabs(orig), warp[input]) *  (orig > 0.0 ? 1.0 : -1.0)) * scale[input];
}

void Control_method::set_const_channels(Joystick * joystick)
{
  unsigned int i;
//  cout << this << ": minus size = " << const_minus.size() << endl;
  
  for (i = 0 ; i < const_minus.size() ; ++i)
  {
//    cout << this << ": setting channel " << const_minus[i] << " to -1\n";
    joystick->set_value(const_minus[i], -1);
  }
  for (i = 0 ; i < const_zero.size() ; ++i)
  {
//    cout << this << ": setting channel " << const_zero[i] << " to 0\n";
    joystick->set_value(const_zero[i], 0);
  }
  for (i = 0 ; i < const_plus.size() ; ++i)
  {
//    cout << this << ": setting channel " << const_plus[i] << " to +1\n";
    joystick->set_value(const_plus[i], 1);
  }
  
  
}

