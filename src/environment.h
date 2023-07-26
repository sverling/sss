/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include "types.h"
#include "sss_glut.h"
#include "lod.h"
#include <set>
#include <vector>

class Vertex_array;
class Object;
class Thermal_manager;
class Rgba_file_texture;
class Shadow_texture;
class Flare_texture;
class Lightmap_texture;
class Detail_texture;
class Tree_collection;

/// Allow objects to register with the environment and modify the 
/// ambient wind field
class Wind_modifier
{
public:
    // ctor does not automatically register but dtor does deregister
    // (just in case)
    Wind_modifier();
    virtual ~Wind_modifier();

    // the implementer should add their contribution to the wind
    // vector passed in (done like this to minimise copies).
    virtual void get_wind(const Position & pos, Vector & wind) const = 0;
};

//! The singleton environment
/*! 
  The environment object is responsible for the terrain, sky and winds.

  It desparately needs to be split up into its components.
*/
class Environment
{
public:
  //! Any simulation can only have one environment
  static inline Environment * instance();
  ~Environment();
  
  //! Returns the wind at the point specified. Turbulence is
  //! calculated using the object scale. Includes wind modifiers.
  Velocity get_wind(const Position & pos,
                    const Object * object) const;

  /// Allow objects to register to modify the wind field
  void register_wind_modifier(const Wind_modifier * modifier);
  void deregister_wind_modifier(const Wind_modifier * modifier);

  //! Returns the non-turbulent component of the wind field at the
  //! point specified.
  Velocity get_non_turbulent_wind(const Position & pos) const;

  //! Returns the non-turbulent, non-thermal component of the wind
  //! field at the point specified.
  Velocity get_ambient_wind(const Position & pos) const;

  //! returns the nearest point on the terrrain surface to pos (only
  //! accurate when it is close), and the local normal vector.
  void get_local_terrain(const Position & pos,
                         Position & terrain_pos,
                         Vector3 & normal) const;
  
  //! Returns terrain height
  float get_z(float x1, float y1) const;
  //! Sets the height in the position object
  void set_z(Position & pos) const;

  float get_sea_altitude() const {return sea_altitude;}

  /// Returns the first intersection with the ground between pos0 and
  /// pos1
  Position get_ground_intersection(const Position & pos0,
                                   const Position & pos1) const;

  /// Returns the "air" density. This should actually be water density 
  /// if under water, but that breaks things!
  float get_air_density(const Position & pos);

  //! Attempts to work out a reasonable standing position - must be
  //called after the terrain an wind-field have been set up.
  Position estimate_best_start_position();

  //! Calculates the shadow for the passed-in blocker (will disfigure
  //! any existing rendering context.
  void calculate_shadow(Object * shadow_maker);
  void draw_ground(Object * blocker);
  void draw_sky();
  void draw_lens_flare();
  void draw_thermals() const;

  void setup_lighting();

  /// indicates how many triangles were used to drawn the terrain
  unsigned get_terrain_triangles() const;

  //! does the (infrequent) updates of state
  void update_environment(float dt);
  
  // calculates the array index for the internal wind representation -
  // only used by the external function used to create the winds...
  int calc_index(int i, int j, int k) const
    { return i + wind_nx * j + wind_nx * wind_ny * k;}

  Thermal_manager * get_thermal_manager() const 
    { return m_thermal_manager;}

private:
  Environment();

  Velocity get_turbulent_wind(const Position & pos, 
                              const Velocity & orig_vel,
                              bool use_i_and_j,
                              float i, float j,
                              float lambda,
                              int rand_offset) const;
  Velocity get_background_wind(const Position & pos) const;
  Velocity get_background_wind(const Position & pos, 
                               float i, 
                               float j, 
                               bool thermal = true) const;
  Velocity get_background_wind_and_ij(const Position & pos, 
                                      float & i, 
                                      float & j) const;

  Velocity get_background_wind_from_terrain(const Position & pos,
                                            bool thermal = true) const;
  
  float interp(float * x, float i, float j, float k) const;
  /// returns float index (e.g. 3.4) for p wrt x
  float find_index(float * x, int nx, float p) const;
  /// Returns (by reference) the index for the quad surrounding x0, y0.
  /// The other indexes are i11+1 etc
  void get_index(float x0, float y0,
                 unsigned & i11, unsigned & j11,
                 const Position & pos_min, 
                 const Position & delta) const;
  
  void draw_lens_flare_element(float x, float y,
                               float radius_x, float radius_y);

  void calculate_terrain_lightmap();

  void draw_terragen_ground(Object* blocker);
  
  bool load_terragen_terrain(std::vector<Vertex>& vertex);

  static Environment * m_instance;

  int nx, ny;
  Vertex_array * ground_vertex_array;
  
  // wind field
  float boundary_layer_depth;
  int wind_nx, wind_ny, wind_nz;
  float * wind_x;
  float * wind_y;
  float * wind_z;
  float * wind_u;
  float * wind_v;
  float * wind_w;

  // wind modifiers
  typedef std::set<const Wind_modifier *> Wind_modifiers;
  Wind_modifiers m_wind_modifiers;

  // thermals
  Thermal_manager * m_thermal_manager;

  GLfloat light_pos[4];
  GLdouble sun_win_x, sun_win_y, sun_win_z; // in screen

  // trees
  Tree_collection * m_tree_collection;

  float sea_altitude;

  float m_air_density, m_water_density;

  // textures - get created when needed
  Rgba_file_texture * sea_texture;
  Rgba_file_texture * sand_texture;
  Rgba_file_texture * ground_texture;
  Rgba_file_texture * cloud_texture;
  Rgba_file_texture * sun_texture;
  Detail_texture * ground_texture_detail;
  Lightmap_texture * terrain_lightmap_texture;
  Shadow_texture * shadow_texture;
  Flare_texture * flare_texture;
  Rgba_file_texture * panorama_top_texture;
  Rgba_file_texture * panorama_sides_texture;
  Rgba_file_texture * sky_textures[6];
};

Environment * Environment::instance()
{
  return ( m_instance ? m_instance : (m_instance = new Environment));
}


#endif
