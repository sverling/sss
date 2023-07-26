/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file gui.cpp
  
  Note that we should be able to use "live" variables, but I can't
  seem to get the GUI to update properly...
*/

#include "gui.h"
#include "sss.h"
#include "config.h"
#include "renderer.h"
#include "environment.h"
#include "thermal_manager.h"
#include "config_file.h"
#include "glider.h"
#include "control_method.h"
#include "log_trace.h"

#include "sss_glut.h"

#include <iostream>
#include <algorithm>
using namespace std;

// static declarations
Gui * Gui::m_inst;
void (GLUTCALLBACK *Gui::idle_func)(void);

enum Action_id {
  QUIT_CONFIG, 
  QUIT_SSS,
  ADD_ROBOT,
  REMOVE_ROBOT,
  FOG, 
  PARTICLE,
  SHADOW,
  CLIP,
  TURBULENCE,
  TRANSLUCENT_SEA,
  LOD,
  AUDIO,
  FPS,
  FOV,
  WIND_SCALE,
  WIND_DIR,
  GRAVITY,
  TEXTURE,
  PHYSICS_MODE,
  SKY_MODE,
  DYNAMIC_CLOUD_LAYERS,
  TURBULENCE_SCALE,
  TURBULENCE_SHEAR_OFFSET,
  CLIP_NEAR,
  CLIP_FAR,
  PHYSICS_FREQ,
  SHADE,
  TEXT_OVERLAY,
  CONTROL_METHOD,
  THERMAL_SHOW_TYPE,
  THERMAL_SHOW_ALL,
  THERMAL_DENSITY,
  THERMAL_UPDRAFT,
  THERMAL_RADIUS,
  THERMAL_HEIGHT,
  THERMAL_INFLOW_HEIGHT,
  THERMAL_LIFETIME,
  THERMAL_APPLY,
  GLIDER_FILE,
  USE_AUDIO,
  USE_TX_AUDIO,
  ARROW_KEYS_FOR_PRIMARY_CONTROL,
  TIMER_METHOD,
  AUTO_ZOOM,
  ZOOM_X2_DIST,
  BODY_VIEW_LAG_TIME,
  GLIDER_VIEW_LAG_TIME
};

Gui * Gui::create_instance(int main_window) 
{
  m_inst = new Gui(main_window); 
  m_inst->update(); 
  return m_inst;
}

Gui::Gui(int main_window)
  :
  m_main_window(main_window)
{
#ifdef WITH_GLUI
  m_glui = GLUI_Master.create_glui( "SSS Configuration");
//  m_glui = GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_LEFT);
  // allow scaling of all spinner speeds to account for general CPU
  // speed
  const float spinner_speed = 0.7f;
  
  GLUI_Panel * panel = m_glui->add_panel("", GLUI_PANEL_NONE);
  
//===================================================================
// Graphics panel
//===================================================================
  GLUI_Rollout * graphics = m_glui->add_rollout_to_panel(panel, "Graphics", true);
  
  // fog
  fog_checkbox = m_glui->add_checkbox_to_panel(
    graphics, "Fog", 0, FOG, action);
  // particle engine
  particle_checkbox = m_glui->add_checkbox_to_panel(
    graphics, "Use particles", 0, PARTICLE, action);
  // shadow
  shadow_checkbox =  m_glui->add_checkbox_to_panel(
    graphics, "Glider Shadow", 0, SHADOW, action);
  // clip
  clip_checkbox =  m_glui->add_checkbox_to_panel(
    graphics, "View frustrum clipping", 0, CLIP, action);
  // translucent sea
  translucent_sea_checkbox =  m_glui->add_checkbox_to_panel(
    graphics, "Translucent sea", 0, TRANSLUCENT_SEA, action);
  // auto-zoom
  auto_zoom_checkbox =  m_glui->add_checkbox_to_panel(
    graphics, "Auto zoom", 0, AUTO_ZOOM, action);
  // zoom x2 dist
  zoom_x2_dist_spinner = m_glui->add_spinner_to_panel(
    graphics, "Distance for x2 zoom", GLUI_SPINNER_INT, 0, ZOOM_X2_DIST, action);
  zoom_x2_dist_spinner->set_int_limits(20, 1000, GLUI_LIMIT_CLAMP);
  zoom_x2_dist_spinner->set_speed(spinner_speed * 1.0);
  // body view lag time
  body_view_lag_time_text = m_glui->add_edittext_to_panel(
    graphics, "Body view lag time", GLUI_EDITTEXT_FLOAT, 0, BODY_VIEW_LAG_TIME, action);
  body_view_lag_time_text->set_float_limits(0.0f, 5.0f, GLUI_LIMIT_CLAMP);
  // glider view lag time
  glider_view_lag_time_text = m_glui->add_edittext_to_panel(
    graphics, "Glider view lag time", GLUI_EDITTEXT_FLOAT, 0, GLIDER_VIEW_LAG_TIME, action);
  glider_view_lag_time_text->set_float_limits(0.0f, 5.0f, GLUI_LIMIT_CLAMP);
  // lod
  lod_spinner = m_glui->add_spinner_to_panel(
    graphics, "LOD", GLUI_SPINNER_INT, 0, LOD, action);
  lod_spinner->set_int_limits(1, 10000, GLUI_LIMIT_CLAMP);
  lod_spinner->set_speed(spinner_speed * 1.0);
  // frame rate
  target_frame_rate_text = m_glui->add_edittext_to_panel(
    graphics, "Target frame rate", GLUI_EDITTEXT_INT, 0, FPS, action);
  target_frame_rate_text->set_int_limits(0, 200, GLUI_LIMIT_CLAMP);
  // fov
  fov_spinner = m_glui->add_spinner_to_panel(
    graphics, "FOV", GLUI_SPINNER_INT, 0, FOV, action);
  fov_spinner->set_int_limits(10, 150, GLUI_LIMIT_CLAMP);
  fov_spinner->set_speed(spinner_speed * 5.0);
  // texture
  texture_listbox = m_glui->add_listbox_to_panel(
    graphics, "Texture", 0, TEXTURE, action);
  texture_listbox->add_item(0, "Wireframe");
  texture_listbox->add_item(1, "Solid");
  texture_listbox->add_item(2, "Solid-shaded");
  texture_listbox->add_item(3, "Low");
  texture_listbox->add_item(4, "High");
  texture_listbox->add_item(5, "Multi-texture");
  texture_listbox->add_item(6, "Multi-texture/Multi-pass");
  texture_listbox->add_item(7, "Multi-texture/Multi-pass 2");
  // shade
  shade_listbox = m_glui->add_listbox_to_panel(
    graphics, "Shading", 0, SHADE, action);
  shade_listbox->add_item(GL_FLAT, "Flat");
  shade_listbox->add_item(GL_SMOOTH, "Smooth");
  // text overlay
  text_overlay_listbox = m_glui->add_listbox_to_panel(
    graphics, "Text overlay", 0, TEXT_OVERLAY, action);
  text_overlay_listbox->add_item(Config::TEXT_NONE, "None");
  text_overlay_listbox->add_item(Config::TEXT_LITTLE, "Little");
  text_overlay_listbox->add_item(Config::TEXT_LOTS, "Lots");
  // thermal show type
  thermal_show_type_listbox = m_glui->add_listbox_to_panel(
    graphics, "Thermal show", 0, THERMAL_SHOW_TYPE, action);
  thermal_show_type_listbox->add_item(Config::NONE, "None");
  thermal_show_type_listbox->add_item(Config::SOLID, "Solid");
  thermal_show_type_listbox->add_item(Config::TRANSLUCENT, "Translucent");
  // thermal show all
  thermal_show_all_checkbox =  m_glui->add_checkbox_to_panel(
    graphics, "Show all thermals (when showing)", 0, THERMAL_SHOW_ALL, action);
  // clip near
  clip_near_text = m_glui->add_edittext_to_panel(
    graphics, "Near clipping distance", GLUI_EDITTEXT_FLOAT, 0, CLIP_NEAR, action);
  clip_near_text->set_float_limits(0.01f, 5.0f, GLUI_LIMIT_CLAMP);
  // clip far
  clip_far_text = m_glui->add_edittext_to_panel(
    graphics, "Far clipping distance", GLUI_EDITTEXT_FLOAT, 0, CLIP_FAR, action);
  clip_far_text->set_float_limits(500.0f, 50000.0f, GLUI_LIMIT_CLAMP);

  // sky mode
  sky_mode_listbox = m_glui->add_listbox_to_panel(
    graphics, "Sky mode", 0, SKY_MODE, action);
  sky_mode_listbox->add_item(Config::SKY_NONE, "None");
  sky_mode_listbox->add_item(Config::SKY_DYNAMIC, "Dynamic");
  if (Sss::instance()->config().skybox_texture != "none") 
    sky_mode_listbox->add_item(Config::SKY_SKYBOX, "Skybox");
  
  // cloud layers
  dynamic_cloud_layers_spinner = m_glui->add_spinner_to_panel(
    graphics, 
    "Dynamic_cloud_layers", 
    GLUI_SPINNER_INT, 0, DYNAMIC_CLOUD_LAYERS, action);
  dynamic_cloud_layers_spinner->set_int_limits(0, 10, GLUI_LIMIT_CLAMP);
  dynamic_cloud_layers_spinner->set_speed(spinner_speed * 1.0);
  if (Sss::instance()->config().sky_mode == Config::SKY_DYNAMIC) 
    dynamic_cloud_layers_spinner->enable();
  else
    dynamic_cloud_layers_spinner->disable();

//===================================================================
// Robot panel
//===================================================================
  GLUI_Rollout * robots = m_glui->add_rollout_to_panel(panel, "Robots", true);
  m_glui->add_button_to_panel(robots, "Add robot", ADD_ROBOT, action );
  m_glui->add_column_to_panel(robots, false);
  m_glui->add_button_to_panel(robots, "Remove robot", REMOVE_ROBOT, action );
  
//===================================================================
// Glider panel
//===================================================================
  GLUI_Rollout * glider = m_glui->add_rollout_to_panel(panel, "Main Glider", true);
  glider_file_listbox = m_glui->add_listbox_to_panel(
    glider, "Glider file", 0, GLIDER_FILE, action);
  unsigned num_gliders = Sss::instance()->config().glider_files.size();
  for (unsigned int i = 0 ; i < num_gliders; ++i)
  {
    const char * g_file = Sss::instance()->config().glider_files[i].c_str();
    glider_file_listbox->add_item((int) i, (char *)g_file );
    //if(i>40)break;
  }
  
// Add a new column
  m_glui->add_column_to_panel(panel, false);
//===================================================================
// Physics panel
//===================================================================
  GLUI_Rollout * physics = m_glui->add_rollout_to_panel(panel, "Physics", true);
  
  // wind scale
  wind_scale_spinner = m_glui->add_spinner_to_panel(
    physics, "Wind scale", GLUI_SPINNER_FLOAT, 0, WIND_SCALE, action);
  wind_scale_spinner->set_float_limits(0, 5.0, GLUI_LIMIT_CLAMP);
  wind_scale_spinner->set_speed(spinner_speed * 5.0);
  // wind scale
  wind_dir_spinner = m_glui->add_spinner_to_panel(
    physics, "Wind dir", GLUI_SPINNER_FLOAT, 0, WIND_DIR, action);
  wind_dir_spinner->set_float_limits(0, 360, GLUI_LIMIT_WRAP);
  wind_dir_spinner->set_speed(spinner_speed * 10.0);
  // gravity
  gravity_spinner = m_glui->add_spinner_to_panel(
    physics, "Gravity", GLUI_SPINNER_FLOAT, 0, GRAVITY, action);
  gravity_spinner->set_float_limits(0, 20.0, GLUI_LIMIT_CLAMP);
  gravity_spinner->set_speed(spinner_speed * 5.0);
  
  // turbulence
  turbulence_checkbox =  m_glui->add_checkbox_to_panel(
    physics, "Turbulence", 0, TURBULENCE, action);
  
  // turbulence scale
  turbulence_scale_spinner = m_glui->add_spinner_to_panel(
    physics, "Turbulence scale", GLUI_SPINNER_FLOAT, 0, TURBULENCE_SCALE, action);
  turbulence_scale_spinner->set_float_limits(0.1, 100.0, GLUI_LIMIT_CLAMP);
  turbulence_scale_spinner->set_speed(spinner_speed * 5.0);
  
  // turbulence shear offset
  turbulence_shear_offset_spinner = m_glui->add_spinner_to_panel(
    physics, "Turbulence shear offset", GLUI_SPINNER_FLOAT, 0, TURBULENCE_SHEAR_OFFSET, action);
  turbulence_shear_offset_spinner->set_float_limits(0.0, 5.0, GLUI_LIMIT_CLAMP);
  turbulence_shear_offset_spinner->set_speed(spinner_speed * 5.0);

  // physics frequency
  physics_freq_text = m_glui->add_edittext_to_panel(
    physics, "Physics frequency", GLUI_EDITTEXT_INT, 0, PHYSICS_FREQ, action);
  physics_freq_text->set_int_limits(20, 500, GLUI_LIMIT_CLAMP);
  
  // physics mode
  physics_mode_listbox = m_glui->add_listbox_to_panel(
    physics, "Physics mode", 0, PHYSICS_MODE, action);
  physics_mode_listbox->add_item(Config::EULER, "Euler");
  physics_mode_listbox->add_item(Config::MOD_EULER, "Modified Euler");
  physics_mode_listbox->add_item(Config::RK2, "Range-Kutta 2");
  physics_mode_listbox->add_item(Config::MOD_RK2, "Modified Range-Kutta 2");
//  physics_mode_listbox->add_item(Config::RK4, "Range-Kutta 4");
  
//===================================================================
// Thermal panel
//===================================================================
  GLUI_Rollout * thermals = m_glui->add_rollout_to_panel(panel, "Thermals", true);
  // thermal_density
  thermal_density_text = m_glui->add_edittext_to_panel(
    thermals, "thermals per sq. km", GLUI_EDITTEXT_FLOAT, 0, THERMAL_DENSITY, action);
  thermal_density_text->set_float_limits(0, 200.0, GLUI_LIMIT_CLAMP);
  // thermal_updraft
  thermal_updraft_text = m_glui->add_edittext_to_panel(
    thermals, "updraft (m/s)", GLUI_EDITTEXT_FLOAT, 0, THERMAL_UPDRAFT, action);
  thermal_updraft_text->set_float_limits(0, 200.0, GLUI_LIMIT_CLAMP);
  // thermal_radius
  thermal_radius_text = m_glui->add_edittext_to_panel(
    thermals, "thermal core radius (m)", GLUI_EDITTEXT_FLOAT, 0, THERMAL_RADIUS, action);
  thermal_radius_text->set_float_limits(0, 200.0, GLUI_LIMIT_CLAMP);
  // thermal_height
  thermal_height_text = m_glui->add_edittext_to_panel(
    thermals, "thermal height (m)", GLUI_EDITTEXT_FLOAT, 0, THERMAL_HEIGHT, action);
  thermal_height_text->set_float_limits(0, 200.0, GLUI_LIMIT_CLAMP);
  // thermal_inflow_height
  thermal_inflow_height_text = m_glui->add_edittext_to_panel(
    thermals, "thermal inflow height (m)", GLUI_EDITTEXT_FLOAT, 0, THERMAL_INFLOW_HEIGHT, action);
  thermal_inflow_height_text->set_float_limits(0, 200.0, GLUI_LIMIT_CLAMP);
  // thermal_lifetime
  thermal_lifetime_text = m_glui->add_edittext_to_panel(
    thermals, "thermal lifetime (sec)", GLUI_EDITTEXT_FLOAT, 0, THERMAL_LIFETIME, action);
  thermal_lifetime_text->set_float_limits(0, 200.0, GLUI_LIMIT_CLAMP);
  // button to action these changes
  m_glui->add_button_to_panel(thermals, "Apply changes", THERMAL_APPLY, action );
  
//===================================================================
// Misc panel
//===================================================================
  GLUI_Rollout * misc = m_glui->add_rollout_to_panel(panel, "Misc", true);
  // texture
  
  control_method_listbox = m_glui->add_listbox_to_panel(
    misc, "Control method", 0, CONTROL_METHOD, action);
  for (Config::Control_methods_it it = Sss::instance()->set_config().control_methods.begin(); 
       it != Sss::instance()->set_config().control_methods.end(); 
       ++it)
  {
    char * control_name = new char[64];
    strcpy(control_name, it->second->get_name().c_str());
    control_method_listbox->add_item((size_t) (control_name), control_name);
  }
  
  arrow_keys_for_primary_control_checkbox = m_glui->add_checkbox_to_panel(
    misc, "Arrow keys->Primary control", 0, ARROW_KEYS_FOR_PRIMARY_CONTROL, action);

  // audio
  use_audio_checkbox = m_glui->add_checkbox_to_panel(
    misc, "Sound", 0, USE_AUDIO, action);
  // audio scale
  audio_spinner = m_glui->add_spinner_to_panel(
    misc, "AUDIO", GLUI_SPINNER_FLOAT, 0, AUDIO, action);
  audio_spinner->set_float_limits(0, 10.0, GLUI_LIMIT_CLAMP);
  audio_spinner->set_speed(spinner_speed * 1.0);

  // timer method
  timer_method_listbox = m_glui->add_listbox_to_panel(
    misc, "Timer method", 0, TIMER_METHOD, action);
  timer_method_listbox->add_item(Config::TIMER_GLUT, "GLUT");
#ifndef WIN32
  timer_method_listbox->add_item(Config::TIMER_PERF, "High res.");
#endif

  // Tx Audio Interface
  use_tx_audio_checkbox = m_glui->add_checkbox_to_panel(
    misc, "Audio Tx Interface", 0, USE_TX_AUDIO, action);
#ifndef WIN32
  use_tx_audio_checkbox->disable();
#endif
//===================================================================
// Buttons
//===================================================================
  // quit buttons
  GLUI_Panel * quit = m_glui->add_panel_to_panel(panel, "", GLUI_PANEL_NONE);
  m_glui->add_button_to_panel(quit, "Quit config", QUIT_CONFIG, action );
  m_glui->add_column_to_panel(quit, false);
  m_glui->add_button_to_panel(quit, "Quit SSS", QUIT_SSS, action );
  
  // do it all
  m_glui->set_main_gfx_window( main_window );
  m_visible = true; 
//  update();
#endif
}

void Gui::hide()
{
#ifdef WITH_GLUI
  m_glui->hide();
  m_visible = false;
#endif
}
void Gui::show()
{
#ifdef WITH_GLUI
  m_glui->show();
  m_visible = true;
#endif
}


void Gui::set_reshape_func(void (GLUTCALLBACK *func)(int width, int height))
{
  glutReshapeFunc(func);
  return;
}

void Gui::set_keyboard_func(void (GLUTCALLBACK *func)
                            (unsigned char key, int x, int y))
{
  glutKeyboardFunc(func);
  return;
}

void Gui::set_keyboard_up_func(void (GLUTCALLBACK *func)
                               (unsigned char key, int x, int y))
{
  glutKeyboardUpFunc(func);
  return;
}

void Gui::set_mouse_func(void (GLUTCALLBACK *func)
                         (int button, int state, int x, int y))
{
  glutMouseFunc(func);
  return;
}

void Gui::set_special_func(void (GLUTCALLBACK *func)(int key, int x, int y))
{
  glutSpecialFunc(func);
  return;
}

void Gui::set_special_up_func(void (GLUTCALLBACK *func)(int key, int x, int y))
{
  glutSpecialUpFunc(func);
  return;
}


void Gui::set_idle_func(void (GLUTCALLBACK *func)(void))
{
  idle_func = func;
#ifdef WITH_GLUI
  if (idle_func)
  {
    GLUI_Master.set_glutIdleFunc( idle );
  }
  else
  {
    GLUI_Master.set_glutIdleFunc( 0 );
  }
#else
  glutIdleFunc(idle);
#endif
}

//! Static wrapper around the idle - so app doesn't need to worry
//  about the window
void Gui::idle(void)
{
#ifdef WITH_GLUI
  if ( glutGetWindow() != m_inst->m_main_window) 
    glutSetWindow(m_inst->m_main_window);  
#endif
  idle_func();
}

/*! Static function that gives us notification when a value has
  changed. */
void Gui::action(int control)
{
#ifdef WITH_GLUI
  switch (control)
  {
  case QUIT_CONFIG:
    Sss::instance()->hide_config();
    break;
  case QUIT_SSS:
    exit(0);
    break;
  case ADD_ROBOT:
    Sss::instance()->add_robot();
    break;
  case REMOVE_ROBOT:
    Sss::instance()->remove_robot();
    break;
  case FOG:
    Sss::instance()->set_config().fog = 
      (1 == m_inst->fog_checkbox->get_int_val());
    break;
  case PARTICLE:
    Sss::instance()->set_config().use_particles = 
      (1 == m_inst->particle_checkbox->get_int_val());
    break;
  case USE_AUDIO:
    Sss::instance()->set_config().use_audio = 
      (1 == m_inst->use_audio_checkbox->get_int_val());
    break;
  case USE_TX_AUDIO:
    Sss::instance()->set_config().tx_audio = 
      (1 == m_inst->use_tx_audio_checkbox->get_int_val());
    break;
  case ARROW_KEYS_FOR_PRIMARY_CONTROL:
    Sss::instance()->set_config().arrow_keys_for_primary_control = 
      (1 == m_inst->arrow_keys_for_primary_control_checkbox->get_int_val());
    break;
  case TIMER_METHOD:
    Sss::instance()->set_config().timer_method = 
      (Config::Timer_method) m_inst->timer_method_listbox->get_int_val();
    break;
  case AUTO_ZOOM:
    Sss::instance()->set_config().auto_zoom = 
      (1 == m_inst->auto_zoom_checkbox->get_int_val());
    break;
  case ZOOM_X2_DIST:
    Sss::instance()->set_config().zoom_x2_dist = 
      m_inst->zoom_x2_dist_spinner->get_float_val();
    break;
  case BODY_VIEW_LAG_TIME:
    Sss::instance()->set_config().body_view_lag_time = m_inst->body_view_lag_time_text->get_float_val();
    break;
  case GLIDER_VIEW_LAG_TIME:
    Sss::instance()->set_config().glider_view_lag_time = m_inst->glider_view_lag_time_text->get_float_val();
    break;
  case SHADOW:
    Sss::instance()->set_config().shadow = 
      (1 == m_inst->shadow_checkbox->get_int_val());
    break;
  case CLIP:
    Sss::instance()->set_config().clip = 
      (1 == m_inst->clip_checkbox->get_int_val());
    break;
  case TURBULENCE:
    Sss::instance()->set_config().turbulence = 
      (1 == m_inst->turbulence_checkbox->get_int_val());
    break;
  case TURBULENCE_SCALE:
    Sss::instance()->set_config().turbulence_scale = 
      m_inst->turbulence_scale_spinner->get_float_val();
    break;
  case TURBULENCE_SHEAR_OFFSET:
    Sss::instance()->set_config().turbulence_shear_offset = 
      m_inst->turbulence_shear_offset_spinner->get_float_val();
    break;
  case TRANSLUCENT_SEA:
    Sss::instance()->set_config().translucent_sea = 
      (1 == m_inst->translucent_sea_checkbox->get_int_val());
    break;
  case THERMAL_SHOW_TYPE:
    Sss::instance()->set_config().thermal_show_type = 
      (Config::Thermal_show_type) m_inst->thermal_show_type_listbox->get_int_val();
    break;
  case THERMAL_SHOW_ALL:
    Sss::instance()->set_config().thermal_show_all = 
      (1 == m_inst->thermal_show_all_checkbox->get_int_val());
    break;
  case LOD:
    Sss::instance()->set_config().lod = 
      (float) m_inst->lod_spinner->get_int_val();
    break;
  case DYNAMIC_CLOUD_LAYERS:
    Sss::instance()->set_config().dynamic_cloud_layers = 
      m_inst->dynamic_cloud_layers_spinner->get_int_val();
    break;    
  case AUDIO:
    Sss::instance()->set_config().global_audio_scale = 
      m_inst->audio_spinner->get_float_val();
    break;
  case FPS:
    Sss::instance()->set_config().target_frame_rate = 
      (float) m_inst->target_frame_rate_text->get_int_val();
    break;
  case FOV:
    Renderer::instance()->set_fov((float) m_inst->fov_spinner->get_int_val());
    break;
  case WIND_SCALE:
    Sss::instance()->set_config().wind_scale =
      m_inst->wind_scale_spinner->get_float_val();
    break;
  case WIND_DIR:
    Sss::instance()->set_config().wind_dir =
      m_inst->wind_dir_spinner->get_float_val();
    break;
  case GRAVITY:
    Sss::instance()->set_config().gravity = 
      m_inst->gravity_spinner->get_float_val();
    break;
  case TEXTURE:
    Sss::instance()->set_config().texture_level = 
      m_inst->texture_listbox->get_int_val();
    break;
  case PHYSICS_MODE:
    Sss::instance()->set_config().physics_mode = 
      (Config::Physics_mode) m_inst->physics_mode_listbox->get_int_val();
    break;
  case SKY_MODE:
    Sss::instance()->set_config().sky_mode = 
      (Config::Sky_mode) m_inst->sky_mode_listbox->get_int_val();
    if (Sss::instance()->config().sky_mode == Config::SKY_DYNAMIC) 
      m_inst->dynamic_cloud_layers_spinner->enable();
    else
      m_inst->dynamic_cloud_layers_spinner->disable();
    break;
  case CLIP_NEAR:
    Renderer::instance()->set_clip_near(m_inst->clip_near_text->get_float_val());
    break;
  case CLIP_FAR:
    Renderer::instance()->set_clip_far(m_inst->clip_far_text->get_float_val());
    break;
  case PHYSICS_FREQ:
    Sss::instance()->set_config().physics_freq = 
      (float) m_inst->physics_freq_text->get_int_val();
    break;
  case SHADE:
    Sss::instance()->set_config().shade_model = 
      m_inst->shade_listbox->get_int_val();
    Renderer::instance()->recalculate_quadric();
    break;
  case TEXT_OVERLAY:
    Sss::instance()->set_config().text_overlay = 
      (Config::Text_overlay_enum) m_inst->text_overlay_listbox->get_int_val();
    break;
  case CONTROL_METHOD:
    Sss::instance()->set_config().control_method = 
      Sss::instance()->config().control_methods.find(string(
                                                       (char *) (m_inst->control_method_listbox->get_int_val())))->second;
    Sss::instance()->reset_joystick();
    TRACE("control method = %d\n", Sss::instance()->config().control_method);
    break;
  case THERMAL_DENSITY:
    Sss::instance()->set_config().thermal_density = 
      m_inst->thermal_density_text->get_float_val();
    break;
  case THERMAL_UPDRAFT:
    Sss::instance()->set_config().thermal_mean_updraft = 
      m_inst->thermal_updraft_text->get_float_val();
    break;
  case THERMAL_RADIUS:
    Sss::instance()->set_config().thermal_mean_radius = 
      m_inst->thermal_radius_text->get_float_val();
    break;
  case THERMAL_HEIGHT:
    Sss::instance()->set_config().thermal_height = 
      m_inst->thermal_height_text->get_float_val();
    break;
  case THERMAL_INFLOW_HEIGHT:
    Sss::instance()->set_config().thermal_inflow_height = 
      m_inst->thermal_inflow_height_text->get_float_val();
    break;
  case THERMAL_LIFETIME:
    Sss::instance()->set_config().thermal_mean_lifetime = 
      m_inst->thermal_lifetime_text->get_float_val();
    break;
  case THERMAL_APPLY:
    Environment::instance()->get_thermal_manager()->reinitialise();
    break;
  case GLIDER_FILE:
  {
    string new_glider_file(Sss::instance()->config().
                           glider_files[m_inst->glider_file_listbox->get_int_val()]);
    TRACE("using new glider file: %s\n", new_glider_file.c_str());
    new_glider_file = "gliders/" + new_glider_file;
    bool success;
    Config_file glider_config_file(new_glider_file,
                                   success);
    if (success == false)
    {
      TRACE("Unable to open glider file\n");
    }
    else
    {
      Sss::instance()->glider().initialise_from_config(glider_config_file);
    }
    break;
  }
  default:
    TRACE("Unknown control value %d\n", control);
    break;
  }
#endif
}

void Gui::update()
{
#ifdef WITH_GLUI
  m_inst->fog_checkbox->set_int_val((bool) Sss::instance()->config().fog);
  m_inst->particle_checkbox->set_int_val((bool) Sss::instance()->config().use_particles);
  m_inst->use_audio_checkbox->set_int_val((bool) Sss::instance()->config().use_audio);
  m_inst->use_tx_audio_checkbox->set_int_val((bool) Sss::instance()->config().tx_audio);
  m_inst->arrow_keys_for_primary_control_checkbox->set_int_val((bool) Sss::instance()->config().arrow_keys_for_primary_control);
  m_inst->timer_method_listbox->set_int_val(Sss::instance()->config().timer_method);
  m_inst->auto_zoom_checkbox->set_int_val((bool) Sss::instance()->config().auto_zoom);
  m_inst->zoom_x2_dist_spinner->set_float_val(Sss::instance()->config().zoom_x2_dist);
  m_inst->body_view_lag_time_text->set_float_val(Sss::instance()->config().body_view_lag_time);
  m_inst->glider_view_lag_time_text->set_float_val(Sss::instance()->config().glider_view_lag_time);
  m_inst->shadow_checkbox->set_int_val((bool) Sss::instance()->config().shadow);
  m_inst->clip_checkbox->set_int_val((bool) Sss::instance()->config().clip);
  m_inst->turbulence_checkbox->set_int_val((bool) Sss::instance()->config().turbulence);
  m_inst->translucent_sea_checkbox->set_int_val((bool) Sss::instance()->config().translucent_sea);
  m_inst->thermal_show_all_checkbox->set_int_val((bool) Sss::instance()->config().thermal_show_all);
  m_inst->lod_spinner->set_int_val((int) Sss::instance()->config().lod);
  m_inst->dynamic_cloud_layers_spinner->set_int_val(Sss::instance()->config().dynamic_cloud_layers);
  m_inst->audio_spinner->set_float_val(Sss::instance()->config().global_audio_scale);
  m_inst->target_frame_rate_text->set_int_val((int) Sss::instance()->config().target_frame_rate);
  m_inst->fov_spinner->set_int_val((int) Sss::instance()->config().fov);
  m_inst->wind_scale_spinner->set_float_val(Sss::instance()->config().wind_scale);
  m_inst->wind_dir_spinner->set_float_val(Sss::instance()->config().wind_dir);
  m_inst->gravity_spinner->set_float_val(Sss::instance()->config().gravity);
  m_inst->thermal_show_type_listbox->set_int_val(Sss::instance()->config().thermal_show_type);
  m_inst->turbulence_scale_spinner->set_float_val(Sss::instance()->config().turbulence_scale);
  m_inst->turbulence_shear_offset_spinner->set_float_val(Sss::instance()->config().turbulence_shear_offset);
  m_inst->clip_near_text->set_float_val(Sss::instance()->config().clip_near);
  m_inst->clip_far_text->set_float_val(Sss::instance()->config().clip_far);
  m_inst->physics_freq_text->set_int_val((int) Sss::instance()->config().physics_freq);
  m_inst->shade_listbox->set_int_val(Sss::instance()->config().shade_model);
  m_inst->text_overlay_listbox->set_int_val(Sss::instance()->config().text_overlay);
  m_inst->control_method_listbox->set_int_val((size_t) Sss::instance()->config().control_method);
  m_inst->texture_listbox->set_int_val(Sss::instance()->config().texture_level);
  m_inst->physics_mode_listbox->set_int_val(Sss::instance()->config().physics_mode);
  m_inst->sky_mode_listbox->set_int_val(Sss::instance()->config().sky_mode);
  
  m_inst->thermal_density_text->set_float_val(Sss::instance()->config().thermal_density);
  m_inst->thermal_updraft_text->set_float_val(Sss::instance()->config().thermal_mean_updraft);
  m_inst->thermal_radius_text->set_float_val(Sss::instance()->config().thermal_mean_radius);
  m_inst->thermal_height_text->set_float_val(Sss::instance()->config().thermal_height);
  m_inst->thermal_inflow_height_text->set_float_val(Sss::instance()->config().thermal_inflow_height);
  m_inst->thermal_lifetime_text->set_float_val(Sss::instance()->config().thermal_mean_lifetime);
  
  // glider file is more complicated -  this doesn't work. Need to find the string allocated above...
  vector<string>::const_iterator it = find(
    Sss::instance()->config().glider_files.begin(), 
    Sss::instance()->config().glider_files.end(),
    Sss::instance()->config().glider_file);

  m_inst->glider_file_listbox->set_int_val(
    it - Sss::instance()->config().glider_files.begin());

  if (Sss::instance()->config().sky_mode == Config::SKY_DYNAMIC) 
    dynamic_cloud_layers_spinner->enable();
  else
    dynamic_cloud_layers_spinner->disable();

#endif  
}

void Gui::update_lod()
{
#ifdef WITH_GLUI
  m_inst->lod_spinner->set_int_val((int) Sss::instance()->config().lod);
#endif
}

void Gui::update_fov()
{
#ifdef WITH_GLUI
  m_inst->fov_spinner->set_int_val((int) Sss::instance()->config().fov);
#endif
}

void Gui::update_text_overlay()
{
#ifdef WITH_GLUI
  m_inst->text_overlay_listbox->set_int_val(Sss::instance()->config().text_overlay);
#endif
}

void Gui::get_viewport_area(int * x, int * y, int * w, int * h) 
{
/*  if (m_visible)
    {
    GLUI_Master.get_viewport_area(x, y, w, h);
    }
    else
    {*/
  *x = 0;
  *y = 0;
  *w = glutGet(GLUT_WINDOW_WIDTH);
  *h = glutGet(GLUT_WINDOW_HEIGHT);
//  }
}
