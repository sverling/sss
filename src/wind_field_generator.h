/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef WIND_FIELD_GENERATOR_H
#define WIND_FILED_GENERATOR_H

#include <stdio.h>
#include <string>
using namespace std;

class Vertex_array;
class Environment;

//! allocates and populates the parameters passed to it, reading from
//! file_name. Returns 0 if successful, -1 if not.
int wind_from_file(const string & file_name,
                   float *& wind_x, 
                   float *& wind_y, 
                   float *& wind_z, 
                   float *& wind_u, 
                   float *& wind_v, 
                   float *& wind_w, 
                   int & wind_nx,
                   int & wind_ny, 
                   int & wind_nz);

//! Calculates a wind-field from the terrain, and allocates memory for
//! the arrays that are returned. Simple scaled-terrain following
void simple_wind_from_terrain(
  const Environment * environment,
  const Vertex_array * ground_vertex_array,
  float *& wind_x, 
  float *& wind_y, 
  float *& wind_z, 
  float *& wind_u, 
  float *& wind_v, 
  float *& wind_w, 
  int & wind_nx,
  int & wind_ny, 
  int & wind_nz);

//! Calculates a wind-field from the terrain, and allocates memory for
//! the arrays that are returned. Scaled terrain following, but the
//! terrain is filtered (using FFT)  by an amount that increases with altitude.
void scaled_fft_wind_from_terrain(const Environment * environment,
                                  const Vertex_array * ground_vertex_array,
                                  float *& wind_x, 
                                  float *& wind_y, 
                                  float *& wind_z, 
                                  float *& wind_u, 
                                  float *& wind_v, 
                                  float *& wind_w, 
                                  int & wind_nx,
                                  int & wind_ny, 
                                  int & wind_nz);

/*! 
  Calculates a wind-field from the terrain, and allocates memory for
  the arrays that are returned. Scaled terrain following, but the !
  terrain is filtered (simple "diffusion" filter) by an amount that
  increases with altitude. 
*/
void filtered_wind_from_terrain(float boundary_layer_depth,
                                const Environment * environment,
                                const Vertex_array * ground_vertex_array,
                                float *& wind_x, 
                                float *& wind_y, 
                                float *& wind_z, 
                                float *& wind_u, 
                                float *& wind_v, 
                                float *& wind_w, 
                                int & wind_nx,
                                int & wind_ny, 
                                int & wind_nz);

/*! 
  Calculates a wind-field from the terrain, and allocates memory for
  the arrays that are returned. Scaled terrain following, but the 
  terrain is filtered (simple "diffusion" filter) by an amount that
  increases with altitude. 

  Also, the terrain is internally modified to produce separation
  regions behind hills. This is done by extending peaks downwind.

*/
void filtered_separated_wind_from_terrain(
  float separation_slope,  
  float boundary_layer_depth,
  float separation_wind_scale,
  const Environment * environment,
  const Vertex_array * ground_vertex_array,
  float *& wind_x, 
  float *& wind_y, 
  float *& wind_z, 
  float *& wind_u, 
  float *& wind_v, 
  float *& wind_w, 
  int & wind_nx,
  int & wind_ny, 
  int & wind_nz);

#endif




