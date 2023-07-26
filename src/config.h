/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef SSS_CONFIG_H
#define SSS_CONFIG_H

#include "sss_glut.h"
#include "types.h"

#include <vector>
#include <string>
#include <map>
using namespace std;


class Control_method;

//! Sss configuration
/*!

  This is central repository for Sss configuration. When it is
  created it parses the configuration file passed to it, and stores
  the data. It can then be queried (all data is public) and
  modified. In principle there could be many of these objects, but in
  practice only Sss will own one. Everyone else can access it
  through Sss.

  \todo make this a singleton - going through Sss is pointless.

*/
class Config
{
public:
  Config(string config_file);

  // All the data with public access

  // modifyable at runtime
  int texture_level;
  bool shadow;
  float lod;
  float target_frame_rate;
  GLenum shade_model;
  bool fog;
  bool use_particles;
  enum Text_overlay_enum {TEXT_NONE=0, TEXT_LITTLE, TEXT_LOTS, TEXT_END};
  Text_overlay_enum text_overlay;
  bool auto_zoom;
  float zoom_x2_dist; // distance for zoom * 2
	float body_view_lag_time;
	float glider_view_lag_time;

  float remote_update_freq;
  bool jitter_correct;
  bool lag_correct;

  /// AUTO lets sss choose, GLUT uses glut, PERF uses
  /// QueryPerformanceCounter (win32 only)
  /// The config GUI depends on this order...
  enum Timer_method {TIMER_GLUT, TIMER_PERF, TIMER_AUTO};
  Timer_method timer_method;

  bool clip;
  float fov;
  int depth_bits; // min bits in depth buffer. -1 means let GLUT choose
  float clip_near, clip_far;
  int window_x;
  int window_y;

  // debug
  bool trace_enabled;
  int trace_level;
  vector<string> trace_strings;
  bool trace_all_strings;
  // Only for WIN32, and only used if trace is off
  bool close_console;
  
  // environment params
  float wind_scale;
  float wind_dir;
  float gravity;
  bool  turbulence;
  float turbulence_scale;
  float turbulence_shear_offset; // add this shear everywhere 
  float boundary_layer_depth;
  float wind_slope_smoothing;
  float wind_wave_upwind_shift;
  float wind_slope_influence_height;
  float separation_slope;
  float separation_wind_scale;
  
  // For particle engine
  float missile_smoke_alpha_scale;
  float missile_smoke_max_rate;
  float missile_smoke_time;

  bool interactive_startup;
  float time_scale;
  float physics_freq;
  enum Physics_mode {EULER, MOD_EULER, RK2, MOD_RK2, RK4};
  Physics_mode physics_mode;
  int dts_smooth; //!< number of timesteps to smooth over (linux)
  bool fullscreen;
  int shadow_size;
  int tracer_count;
  float start_x, start_y; 
  float glider_alt;
  float glider_speed;
  enum Sss_mode {NORMAL_MODE, 
                 DEMO_MODE,
                 QUIDDITCH_MODE, 
                 F3F_MODE, 
                 RACE_MODE, 
                 SLEDGE_MODE};
  Sss_mode sss_mode;
  float f3f_penalty; // penalty for flying downwind in f3f
  int num_robots;
  bool do_movie;
  float movie_dt;

  string tree_config_file;
  // align billboards to eye dir, or to each individual billboard
  // position. Gets updated (maybe) when the viewpoint
  // changes... depending on the nature of the eye.
  bool align_billboards_to_eye_dir;
  
  bool use_audio;
  float global_audio_scale;

  // terrain stuff
  int builtin_terrain_seed;
  bool terrain_zero_edges;
  enum Terrain_type {TERRAIN_PEAKS,
                     TERRAIN_MIDPOINT,
                     TERRAIN_RIDGE, 
                     TERRAIN_PLATEAU,
                     TERRAIN_PISTE};
  Terrain_type builtin_terrain_type;
  int builtin_terrain_size;
  float builtin_terrain_dx;
  int builtin_terrain_lumps;
  float builtin_terrain_peak_height;
  float builtin_terrain_peak_radius;
  float builtin_terrain_height;
  float builtin_terrain_roughness;
  int builtin_terrain_filter;
  float builtin_terrain_ridge_height;
  float builtin_terrain_ridge_width;
  float builtin_terrain_plateau_width;  
  float builtin_terrain_piste_slope;
  bool translucent_sea;
  float coast_enhancement;
  // how to draw the sky
  enum Sky_mode 
  {
    SKY_NONE,
    SKY_SKYBOX,   // Simple skybox
    SKY_PANORAMA,   // Simple skybox
    SKY_DYNAMIC   // dynamic clouds etc
  } sky_mode;
  int dynamic_cloud_layers;
  string panorama_texture; // texture name without "_left.png" etc
  string skybox_texture; // texture name without "_roof.rgb" etc
  bool use_terragen_terrain;
  string terragen_terrain;
  Vector3 terragen_pos;
  Vector3 terragen_sun_pos;

  // thermals
  float thermal_density;
  float thermal_mean_updraft;
  float thermal_sigma_updraft;
  float thermal_mean_radius;
  float thermal_sigma_radius;
  float thermal_height;
  float thermal_inflow_height;
  float thermal_mean_lifetime;
  float thermal_sigma_lifetime;
  enum Thermal_show_type {NONE, SOLID, TRANSLUCENT};
  Thermal_show_type thermal_show_type;
  bool thermal_show_all;
  // texture files
  bool tile_ground_texture;
  string sea_texture_file;
  string sand_texture_file;
  string ground_texture_file;
  string ground_detail_texture_file;
  string cloud_texture_file;
  string sun_texture_file;

  float sun_bearing;
  float sun_elevation;

  string glider_file; 
  vector<string> glider_files;
  string terrain_file;
  string wind_file;
  string robot_file;

  string body_3ds_file;
  bool body_3ds_cull_backface;

  string cockpit_texture_file;
  bool draw_crosshair;

  bool moving_control_surfaces;

  bool tx_audio;
  map<string, Control_method *> control_methods;
  typedef map<string, Control_method *>::iterator Control_methods_it;
  Control_method * control_method;
  bool arrow_keys_for_primary_control;
  bool use_heli_controller;
  int plib_joystick_num;

  bool running_as_screensaver;

  // Configuration settings for the variometer. Each glider can override this settings in the glider configuration file
  // Added by Esteban Ruiz on August 2006  - cerm78@gmail.com
  string vario_audio_file;					// Default veriometer audio file
  bool  vario_enabled;						// Set to false if variometer is to be disabled
  bool	vario_main_glider_only;				// Set to true if variometer is to work only for main glider
  float vario_volume;						// Default vario volume
  float vario_speed_variation;				// Effective speed variation. If the new vertical speed is over or under the last known vertical speed by this amount, vario state is updated.
  float vario_max_speed;					// Maximum climb speed allowed. If vertical speed is over this limit, the sound update is ignored.
  float vario_min_speed;					// Minimum sink speed allowed. If vertical speed is below this limit, the sound update is ignored.
  int   vario_speed_div;					// This is the speed divisor that will convert the vertical speed in a fraction to add or remove from the base rate. The graater the divisor, the lesser the effect of the speed change on the sound frequency.
  int   vario_rate_base;					// This is the base rate at wich the vario will beep with zero vertical speed (remember that it will quit down in the dead zone)
  float vario_dz_max;						// Upper limit of the silent "dead zone". The vario beep will quiet down when vertical speed is in this range.
  float vario_dz_min;						// Lower limit of the silent "dead zone". The vario beep will quiet down when vertical speed is in this range.
  float vario_dz_vol;						// Vario volume when in the "dead zone". This is the vario volume when vertical speed is between dz_min and dz_max.
  enum  Vario_location {EYE, GLIDER};
  Vario_location vario_location;  			// Sets the variometer position at: EYE = current eye position, Glider = at glider position
private:
  void read_config(string & configuration_file);
};


#endif


