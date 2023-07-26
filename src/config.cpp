/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file config.cpp
*/
#include "config.h"
#include "config_file.h"
#include "control_method.h"
#include "log_trace.h"

#ifdef WITH_PLIB
#include <plib/ul.h>
#endif

#include "sss_glut.h"

#include <string>
#include <stdlib.h>
#include <stdio.h>

#ifndef unix
#define strcasecmp _stricmp
#endif

/*!
  Handles the overall configuration of the Sss simulation
*/
Config::Config(string configuration_file)
  :
  texture_level(5),
  shadow(false),
  lod(300),
  target_frame_rate(35),
  shade_model(GL_SMOOTH),
  fog(true),
  use_particles(true),
  text_overlay(TEXT_LITTLE),
  auto_zoom(true),
  zoom_x2_dist(100),
  body_view_lag_time(0.0f),
  glider_view_lag_time(0.0f),
  remote_update_freq(30.0),
  jitter_correct(true),
  lag_correct(true),
  timer_method(TIMER_GLUT),
  clip(true),
  fov(45),
  depth_bits(-1),
  clip_near(-1), clip_far(-1), // <0 means calc defaults using GLUT
  window_x(600),
  window_y(500),
  // trace
  trace_enabled(false),
  trace_level(0),
  trace_all_strings(false),
  close_console(false),
  wind_scale(1.0),
  wind_dir(270.0),
  gravity(9.81),
  turbulence(false),
  turbulence_scale(5.0),
  turbulence_shear_offset(0.04f),
  boundary_layer_depth(4.0),
  wind_slope_smoothing(1.0f),
  wind_wave_upwind_shift(0.3f),
  wind_slope_influence_height(100.0f),
  separation_slope(0.1),
  separation_wind_scale(-0.2),
  missile_smoke_alpha_scale(1.0),
  missile_smoke_max_rate(200.0f),
  missile_smoke_time(1.0f),
  interactive_startup(true),
  time_scale(1.0f),
  physics_freq(150),
  physics_mode(EULER),
  dts_smooth(8),
  fullscreen(false),
  shadow_size(256),
  tracer_count(0),
  start_x(0),
  start_y(0),
  glider_alt(1.5),
  glider_speed(10),
  sss_mode(NORMAL_MODE),
  f3f_penalty(10.0),
  num_robots(0),
  do_movie(false),
  movie_dt(0.1),
  tree_config_file("none"),
  align_billboards_to_eye_dir(true),
  // global audio
  use_audio(true),
  global_audio_scale(2.0),
  // terrain
  builtin_terrain_seed(0),
  terrain_zero_edges(true),
  builtin_terrain_type(TERRAIN_MIDPOINT),
  builtin_terrain_size(129),
  builtin_terrain_dx(10),
  builtin_terrain_lumps(50),
  builtin_terrain_peak_height(200),
  builtin_terrain_peak_radius(220),
  builtin_terrain_height(150.0),
  builtin_terrain_roughness(1.0),
  builtin_terrain_filter(2),
  builtin_terrain_ridge_height(60),
  builtin_terrain_ridge_width(50),
  builtin_terrain_plateau_width(100),
  builtin_terrain_piste_slope(0.5),
  translucent_sea(false),
  coast_enhancement(5.0),
  sky_mode(SKY_DYNAMIC),
  dynamic_cloud_layers(1),
  panorama_texture("none"),
  skybox_texture("none"),
  use_terragen_terrain(false),
  terragen_terrain("none"),
  // thermals
  thermal_density(5.0e-5),
  thermal_mean_updraft(1.5),
  thermal_sigma_updraft(0.4),
  thermal_mean_radius(18),
  thermal_sigma_radius(5),
  thermal_height(400),
  thermal_inflow_height(50),
  thermal_mean_lifetime(300),
  thermal_sigma_lifetime(60),
  thermal_show_type(TRANSLUCENT),
  thermal_show_all(false),
  // textures
  tile_ground_texture(true),
  sea_texture_file("sea_128.rgb"),
  sand_texture_file("sand_128.rgb"),
  ground_texture_file("ground_b_128.rgb"),
  ground_detail_texture_file("ground_c_128.rgb"),
  cloud_texture_file("cloud.rgb"),
  sun_texture_file("sun.rgb"),
  sun_bearing(240.0f),
  sun_elevation(40.0f),
  glider_file("glider_aileron.dat"),
  terrain_file("none"),
  wind_file("none"),
  robot_file("none"),
  body_3ds_file("none"),
  body_3ds_cull_backface(true),
  cockpit_texture_file("none"),
  draw_crosshair(true),
  moving_control_surfaces(true),
  tx_audio(false),
  arrow_keys_for_primary_control(true),
  use_heli_controller(false),
  plib_joystick_num(0),
  running_as_screensaver(false),
  vario_enabled(false),
  vario_main_glider_only(true),
  vario_audio_file("vario_beep.wav"),
  vario_speed_variation(0.5f),
  vario_volume(0.8),
  vario_dz_max(0.5f),
  vario_dz_min(-0.5f),
  vario_dz_vol(0.15f),
  vario_max_speed(15),
  vario_min_speed(-15),
  vario_rate_base(1),
  vario_speed_div(10)
{
  TRACE_METHOD_ONLY(1);
  read_config(configuration_file);
}

void Config::read_config(string & configuration_file)
{
  TRACE_METHOD_ONLY(1);
  bool success;
  Config_file config_file(configuration_file,
                          success);
  assert2(success, "Unable to open config file");

  // boolean values
  config_file.get_value("fullscreen", fullscreen);
  config_file.get_value("fog", fog);
  config_file.get_value("use_particles", use_particles);
  config_file.get_value("translucent_sea", translucent_sea);
  config_file.get_value("turbulence", turbulence);
  config_file.get_value("shadow", shadow);
  config_file.get_value("clip", clip);
  config_file.get_value("jitter_correct", jitter_correct);
  config_file.get_value("lag_correct", lag_correct);
  config_file.get_value("thermal_show_all", thermal_show_all);
  config_file.get_value("auto_zoom", auto_zoom);
  config_file.get_value("tile_ground_texture", tile_ground_texture);
  config_file.get_value("use_audio", use_audio);
  config_file.get_value("tx_audio", tx_audio);
  config_file.get_value("trace_enabled", trace_enabled);
  config_file.get_value("trace_all_strings", trace_all_strings);
  config_file.get_value("moving_control_surfaces", moving_control_surfaces);
  config_file.get_value("terrain_zero_edges", terrain_zero_edges);
  config_file.get_value("use_terragen_terrain", use_terragen_terrain);
  config_file.get_value("close_console", close_console);
  config_file.get_value("body_3ds_cull_backface", body_3ds_cull_backface);
  config_file.get_value("draw_crosshair", draw_crosshair);
  config_file.get_value("do_movie", do_movie);
  config_file.get_value("interactive_startup", interactive_startup);
  config_file.get_value("running_as_screensaver", running_as_screensaver);

  // string values to convert
  string shading_t;
  if (true == config_file.get_value("shading", shading_t))
  {
    shade_model = (shading_t == "smooth") ? GL_SMOOTH : GL_FLAT;
  }
  
  string terrain_t;
  if (true == config_file.get_value("builtin_terrain_type", terrain_t))
  {
    if (terrain_t == "midpoint")
      builtin_terrain_type = TERRAIN_MIDPOINT;
    else if (terrain_t == "peaks")
      builtin_terrain_type = TERRAIN_PEAKS;
    else if (terrain_t == "ridge")
      builtin_terrain_type = TERRAIN_RIDGE;
    else if (terrain_t == "plateau")
      builtin_terrain_type = TERRAIN_PLATEAU;
    else if (terrain_t == "piste")
      builtin_terrain_type = TERRAIN_PISTE;
    else
      TRACE("Invalid terrain type: %s\n", terrain_t.c_str());
  }

  string timer_method_t;
  if (true == config_file.get_value("timer_method", timer_method_t))
  {
    if (timer_method_t == "auto")
      timer_method = TIMER_AUTO;
    else if (timer_method_t == "glut")
      timer_method = TIMER_GLUT;
    else if (timer_method_t == "perf")
      timer_method = TIMER_PERF;
    else
      TRACE("Invalid timer method: %s\n", timer_method_t.c_str());
  }
  
  string sky_mode_t;
  if (true == config_file.get_value("sky_mode", sky_mode_t))
  {
    if (sky_mode_t == "none")
      sky_mode = SKY_NONE;
    else if (sky_mode_t == "skybox")
      sky_mode = SKY_SKYBOX;
    else if (sky_mode_t == "panorama")
      sky_mode = SKY_PANORAMA;
    else if (sky_mode_t == "dynamic")
      sky_mode = SKY_DYNAMIC;
    else
      TRACE("Invalid sky_mode type: %s\n", sky_mode_t.c_str());
  }
  
  string thermal_t;
  if (true == config_file.get_value("thermal_show_type", thermal_t))
  {
    if (thermal_t == "solid")
      thermal_show_type = SOLID;
    else if (thermal_t == "translucent")
      thermal_show_type = TRANSLUCENT;
    else
      thermal_show_type = NONE;
  }
  
  string mode_t;
  if (true == config_file.get_value("mode", mode_t))
  {
    if (mode_t == "quidditch")
      sss_mode = QUIDDITCH_MODE;
    else if (mode_t == "f3f")
      sss_mode = F3F_MODE;
    else if (mode_t == "race")
      sss_mode = RACE_MODE;
    else if (mode_t == "demo")
      sss_mode = DEMO_MODE;
    else if (mode_t == "sledge")
      sss_mode = SLEDGE_MODE;
    else
      sss_mode = NORMAL_MODE;
  }
  
  string physics_mode_t;
  if (true == config_file.get_value("physics_mode", physics_mode_t))
  {
    if (physics_mode_t == "euler")
      physics_mode = EULER;
    else if (physics_mode_t == "mod_euler")
      physics_mode = MOD_EULER;
    else if (physics_mode_t == "rk2")
      physics_mode = RK2;
    else if (physics_mode_t == "mod_rk2")
      physics_mode = MOD_RK2;
    else if (physics_mode_t == "rk4")
      physics_mode = RK4;
    else
    {
      TRACE("Unkown physics mode %s - using euler\n", physics_mode_t.c_str());
      physics_mode = EULER;
    }
  }
  
  // string values 
  config_file.get_value("sea_texture", sea_texture_file);
  config_file.get_value("sand_texture", sand_texture_file);
  config_file.get_value("ground_texture", ground_texture_file);
  config_file.get_value("detail_texture", ground_detail_texture_file);
  config_file.get_value("cloud_texture", cloud_texture_file);
  config_file.get_value("sun_texture", sun_texture_file);
  config_file.get_value("glider_file", glider_file);
  config_file.get_value("terrain_file", terrain_file);
  config_file.get_value("wind_file", wind_file);
  config_file.get_value("robot_file", robot_file);
  config_file.get_value("body_3ds_file", body_3ds_file);
  config_file.get_value("cockpit_texture_file", cockpit_texture_file);
  config_file.get_values("trace_strings", trace_strings);
  config_file.get_value("skybox_texture", skybox_texture);
  config_file.get_value("panorama_texture", panorama_texture);
  config_file.get_value("tree_config_file", tree_config_file);
  if (sky_mode != SKY_NONE && skybox_texture == "none") 
    sky_mode = SKY_DYNAMIC;
  config_file.get_value("terragen_terrain", terragen_terrain);
  if (terragen_terrain == "none") use_terragen_terrain = false;
  
  // integer values
  config_file.get_value("window_x", window_x);
  config_file.get_value("window_y", window_y);
  config_file.get_value("texture_level", texture_level);
  config_file.get_value("shadow_size", shadow_size);
  config_file.get_value("tracer_count", tracer_count);
  config_file.get_value("dts_smooth", dts_smooth);
  config_file.get_value("builtin_terrain_seed", builtin_terrain_seed);
  config_file.get_value("builtin_terrain_size", builtin_terrain_size);
  config_file.get_value("builtin_terrain_lumps", builtin_terrain_lumps);
  config_file.get_value("builtin_terrain_filter", builtin_terrain_filter);
  config_file.get_value("trace_level", trace_level);
  config_file.get_value("num_robots", num_robots);
  config_file.get_value("depth_bits", depth_bits);
  config_file.get_value("dynamic_cloud_layers", dynamic_cloud_layers);
  config_file.get_value("plib_joystick_num", plib_joystick_num);

  // float values
  config_file.get_value("movie_dt", movie_dt);
  config_file.get_value("remote_update_freq", remote_update_freq);
  config_file.get_value("clip_near", clip_near);
  config_file.get_value("clip_far", clip_far);
  config_file.get_value("physics_freq", physics_freq);
  config_file.get_value("time_scale", time_scale);
  config_file.get_value("wind_scale", wind_scale);
  config_file.get_value("wind_dir", wind_dir);
  config_file.get_value("turbulence_scale", turbulence_scale);
  config_file.get_value("turbulence_shear_offset", turbulence_shear_offset);
  config_file.get_value("boundary_layer_depth", boundary_layer_depth);
  config_file.get_value("wind_slope_smoothing", wind_slope_smoothing);
  config_file.get_value("wind_wave_upwind_shift", wind_wave_upwind_shift);
  config_file.get_value("wind_slope_influence_height", wind_slope_influence_height);
  config_file.get_value("separation_wind_scale", separation_wind_scale);
  config_file.get_value("separation_slope", separation_slope);
  config_file.get_value("gravity", gravity);
  config_file.get_value("target_frame_rate", target_frame_rate);
  config_file.get_value("level_of_detail", lod);
  config_file.get_value("fov", fov);
  config_file.get_value("start_x", start_x);
  config_file.get_value("start_y", start_y);
  config_file.get_value("glider_alt", glider_alt);
  config_file.get_value("global_audio_scale", global_audio_scale);
  config_file.get_value("glider_speed", glider_speed);
  config_file.get_value("coast_enhancement", coast_enhancement);
  config_file.get_value("builtin_terrain_dx", builtin_terrain_dx);
  config_file.get_value("builtin_terrain_height", builtin_terrain_height);
  config_file.get_value("builtin_terrain_roughness", builtin_terrain_roughness);
  config_file.get_value("builtin_terrain_peak_height", builtin_terrain_peak_height);
  config_file.get_value("builtin_terrain_peak_radius", builtin_terrain_peak_radius);
  config_file.get_value("builtin_terrain_ridge_height", builtin_terrain_ridge_height);
  config_file.get_value("builtin_terrain_ridge_width", builtin_terrain_ridge_width);
  config_file.get_value("builtin_terrain_plateau_width", builtin_terrain_plateau_width);
  config_file.get_value("builtin_terrain_piste_slope", builtin_terrain_piste_slope);
  config_file.get_value("thermal_density", thermal_density);
  config_file.get_value("thermal_mean_updraft", thermal_mean_updraft);
  config_file.get_value("thermal_sigma_updraft", thermal_sigma_updraft);
  config_file.get_value("thermal_mean_radius", thermal_mean_radius);
  config_file.get_value("thermal_sigma_radius", thermal_sigma_radius);
  config_file.get_value("thermal_height", thermal_height);
  config_file.get_value("thermal_inflow_height", thermal_inflow_height);
  config_file.get_value("thermal_mean_lifetime", thermal_mean_lifetime);
  config_file.get_value("thermal_sigma_lifetime", thermal_sigma_lifetime);
  config_file.get_value("zoom_x2_dist", zoom_x2_dist);
  config_file.get_value("body_view_lag_time", body_view_lag_time);
  config_file.get_value("glider_view_lag_time", body_view_lag_time);
  config_file.get_value("missile_smoke_alpha_scale", missile_smoke_alpha_scale);
  config_file.get_value("missile_smoke_max_rate", missile_smoke_max_rate);
  config_file.get_value("missile_smoke_time", missile_smoke_time);
  config_file.get_value("f3f_penalty", f3f_penalty);
  config_file.get_value("sun_bearing", sun_bearing);
  config_file.get_value("sun_elevation", sun_elevation);

  // get the possible control methods
  string control_method_name;
  config_file.get_value_assert("control", control_method_name);

  config_file.reset();
  while (config_file.find_new_block("control_method"))
  {
    Control_method * new_control_method = new Control_method(config_file);
    control_methods[new_control_method->get_name()] = new_control_method;
  }
  assert2(control_methods.size() > 0, "Need at least one control method");

  Control_methods_it it = control_methods.find(control_method_name);
  if (it == control_methods.end())
  {
    TRACE("Cannot find control method %s\n", control_method_name.c_str());
    exit(-1);
  }
  control_method = it->second;

  config_file.get_value("arrow_keys_for_primary_control", arrow_keys_for_primary_control);
  config_file.get_value("use_heli_controller", use_heli_controller);

  // Configuration settings for the variometer. Each glider can override this settings
  config_file.get_value("vario_audio_file", vario_audio_file);
  config_file.get_value("vario_enabled", vario_enabled);
  config_file.get_value("vario_main_glider_only", vario_main_glider_only);
  config_file.get_value("vario_volume", vario_volume);
  config_file.get_value("vario_speed_var", vario_speed_variation);
  config_file.get_value("vario_dz_max", vario_dz_max);
  config_file.get_value("vario_dz_min", vario_dz_min);
  config_file.get_value("vario_dz_vol", vario_dz_vol);
  config_file.get_value("vario_max_speed", vario_max_speed);
  config_file.get_value("vario_min_speed", vario_min_speed);
  config_file.get_value("vario_rate_base", vario_rate_base);
  config_file.get_value("vario_speed_div", vario_speed_div);
  string vl = "eye";
  config_file.get_value("vario_location", vl);
  if (vl == "eye")
  {
    vario_location = Config::EYE;
    vario_volume = vario_volume + 0.5f;
  }
  else if (vl ==  "glider")
    vario_location = Config::GLIDER;

  // *********************************************************

#ifdef WITH_PLIB  
  char cd[UL_NAME_MAX];
  ulGetCWD(cd, UL_NAME_MAX);
  string gd = cd + string("/gliders");
  ulDir * glider_dir = ulOpenDir(gd.c_str());
  if (glider_dir)
  {
    ulDirEnt * dp;
    while ( (dp = ulReadDir(glider_dir)) != 0)
    {
      if (dp->d_isdir == false)
      {
        string file = dp->d_name;
        string::size_type beg = file.find("glider_");
        string::size_type end = file.rfind(".dat");
        if ( (beg == 0) && (end == (file.length() -4)) )
        {
          glider_files.push_back(file);
        }
      }
    }
    ulCloseDir(glider_dir);
  }
  else
  {
    TRACE("Cannot open glider dir!\n");
  }
#else
  config_file.get_values("glider_files", glider_files);
  if (glider_files.empty())
    glider_files.push_back(glider_file);

#endif  
}



