/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef PILOT_H
#define PILOT_H

class Glider;
class Object;

/// Really a minimal base class on which the specific pilots are built.
/// Two fundamental pilots types would be Human_pilot and Robot_pilot.
/// Also consider if we want Remote_pilot.
class Pilot
{
public:
  Pilot() {};
  virtual ~Pilot() {};

  /// Returns the glider that this pilot controls
  virtual Glider * get_glider() = 0;

  /// prompts the pilot to reset, based on a focal point (interpret
  /// how it likes!).
  virtual void reset(const Object * obj) = 0;

  /*! This will be called at a rate suiable for doing AI code etc
  (probably at the frame-rate - certainly not as fast as the physics
  unless the frame-rate itself is very high). */
  virtual void update(float dt) {};
};


#endif
