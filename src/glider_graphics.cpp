/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file glider_graphics.cpp
*/

#include "glider_graphics.h"
#include "glider_graphics_crrcsim.h"
#include "glider_graphics_component.h"
#include "glider_graphics_3ds.h"
//#include "glider_graphics_3ds_plib.h"
#include "config_file.h"
#include "log_trace.h"

Glider_graphics * Glider_graphics::create(const string & graphics_file,
                                          const string & graphics_type,
                                          Glider & glider)
{
  bool success;

  Config_file graphics_config("gliders/" + graphics_file, success);
  assert2(success, "Unable to open graphics glider file");

  if (graphics_type == "component")
  {
    return new Glider_graphics_component(graphics_config, glider);
  }
  else if (graphics_type == "3ds")
  {
    return new Glider_graphics_3ds(graphics_config, glider);
  }
//    else if (graphics_type == "3ds_plib")
//    {
//      return new Glider_graphics_3ds_plib(graphics_config, glider);
//    }
  else if (graphics_type == "crrcsim")
  {
    return new Glider_graphics_crrcsim(graphics_config, glider);
  }
  else
  {
    TRACE("Unknown graphics type: %s\n", graphics_type.c_str());
    assert1(!"Error");
    return 0;
  }
  
}

