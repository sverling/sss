/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  Variometer code added by Esteban Ruiz on August 2006  - cerm78@gmail.com
*/

#ifndef VARIO_H
#define VARIO_H

#include "object.h"
#include "environment.h"
#include "config.h"

using namespace std;

class Glider;
class Config_file;

/// Represents a variometer.
class Vario : public Object
{
public:
	Vario(Config_file & config_file, const Glider * glider);
	virtual ~Vario();

	void draw(Draw_type draw_type){};
	void pre_physics(float dt);
	void post_physics(float dt);
	float get_graphical_bounding_radius() const { return 0; }
	float get_structural_bounding_radius() const { return 0; }
	// Obtains the current variometer location (EYE, GLIDER)
	Config::Vario_location get_location() const { return location; }
	// Sets the current variometer location (EYE, GLIDER)
	void set_location(Config::Vario_location new_location) { location = new_location; }
	// Gets the current variometer silent state
	float get_silence() const { return silence; }
	// Sets the current variometer silent state. If silent = true, then
	// the vario stops beeping
	void set_silence(bool silent) { silence = silent; }

private:
	const Glider * parent_glider;	// Glider the veriometer is on. Each glider haves its own variometer
	float last_vertical_speed;	// Last known glider vertical speed
	bool silence;						// Set to silence the vario beep

	float volume;						// Default vario volume
	float speed_variation;	// Effective speed variation. If the new vertical speed is over or under the last known vertical speed by this amount, vario state is updated.
	float max_speed;					// Maximum climb speed allowed. If vertical speed is over this limit, the sound update is ignored.
	float min_speed;					// Minimum sink speed allowed. If vertical speed is below this limit, the sound update is ignored.
	float speed_div;					// This is the speed divisor that will convert the vertical speed in a fraction to add or remove from the base rate. The graater the divisor, the lesser the effect of the speed change on the sound frequency.
	float rate_base;					// This is the base rate at wich the vario will beep with zero vertical speed (remember that it will quit down in the dead zone)
	float dz_max;						// Upper limit of the silent "dead zone". The vario beep will quiet down when vertical speed is in this range.
	float dz_min;						// Lower limit of the silent "dead zone". The vario beep will quiet down when vertical speed is in this range.
	float dz_vol;						// Vario volume when in the "dead zone". This is the vario volume when vertical speed is between dz_min and dz_max.
	Config::Vario_location location;	// Sets the variometer position at: EYE = current eye position, Glider = at glider position
};

#endif // file included
