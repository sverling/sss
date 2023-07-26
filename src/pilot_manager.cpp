/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file pilot_manager.cpp
*/

#include "pilot_manager.h"
#include "pilot.h"
#include "log_trace.h"

#include <algorithm>
using namespace std;

Pilot_manager * Pilot_manager::m_instance = 0;

Pilot_manager * Pilot_manager::instance()
{
  return m_instance ? m_instance : (m_instance = new Pilot_manager);
}

void Pilot_manager::register_pilot(Pilot * pilot)
{
  pilots.insert(pilot);
}

bool Pilot_manager::deregister_pilot(Pilot * pilot)
{
  if (1 == pilots.erase(pilot))
    return true;
  TRACE("Error deregistering pilot %p\n", pilot);
  return false;
}

//! Helper functor for update_pilots
class Call_update
{
public:
  Call_update(float dt) :dt(dt) {};
  void operator()(Pilot * pilot) {pilot->update(dt);}
  float dt;
};

void Pilot_manager::update_pilots(float dt)
{
  for_each(pilots.begin(), pilots.end(), Call_update(dt));
}

