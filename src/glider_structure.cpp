/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file glider_structure.cpp
*/

#include "glider_structure.h"
#include "glider_structure_component.h"
#include "glider_structure_crrcsim.h"
#include "glider_structure_3ds.h"
#include "config_file.h"
#include "sss_assert.h"
#include "log_trace.h"

using namespace std;

Glider_structure * Glider_structure::create(const string & structure_file,
                                            const string & structure_type,
                                            Glider & glider)
{
  bool success;
  Config_file structure_config("gliders/" + structure_file, success);
  assert2(success, "Unable to open structure glider file");

  if (structure_type == "component")
  {
    return new Glider_structure_component(structure_config, glider);
  }
  else if (structure_type == "crrcsim")
  {
    return new Glider_structure_crrcsim(structure_config, glider);
  }
  else if (structure_type == "3ds")
  {
    return new Glider_structure_3ds(structure_config, glider);
  }
  else
  {
    TRACE("Unknown structure type: %s\n",structure_type.c_str());
    assert1(!"Error");
    return 0;
  }
  
}

