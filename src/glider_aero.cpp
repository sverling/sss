/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file glider_aero.cpp
*/

#include "glider_aero.h"
#include "glider_aero_component.h"
#include "glider_aero_crrcsim.h"
#include "config_file.h"
#include "sss_assert.h"
#include "log_trace.h"

using namespace std;

Glider_aero * Glider_aero::create(const string & aero_file,
                                  const string & aero_type,
                                  Glider & glider)
{
  TRACE_METHOD_STATIC_ONLY(1);
  bool success;
  Config_file aero_config("gliders/" + aero_file, success);
  assert2(success, "Cannot open requested glider file");
  
  if (aero_type == "component")
  {
    return new Glider_aero_component(aero_config, glider);
  }
  else if (aero_type == "crrcsim")
  {
    return new Glider_aero_crrcsim(aero_config, glider);
  }
  else
  {
    TRACE("Unknown aero type: %s\n", aero_type.c_str());
    assert1(!"Error");
    return 0;
  }
    
}

