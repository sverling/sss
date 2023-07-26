/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

/*!  This file provides a number of functions used to generate terrain
  for use in the simulation.  */

#include "types.h"
#include "race_manager.h"

#include <vector>

#include <stdio.h>

struct Vertex;

//! Loads a terrain from file. vertex_it is allocated by this function
void file_terrain(FILE * terrain_file,
                  Vertex *& vertex_in, // returned
                  int & nx, int & ny, // returned
                  bool do_zero_edges);

//! Creates a terrain based on gaussian "peaks". vertex_in must
//! already be allocated
void gaussian_peaks_terrain(Vertex * vertex_in, //!< modified on output
                            int nx, int ny,
                            float dx, float dy,
                            float xmin, float ymin,
                            float range,
                            float peak,
                            float n_peaks,
                            int seed);

//! Creates a terrain based on mid-point displacement. vertex_in must
//! already be allocated
void midpoint_displacement_terrain(
  Vertex * vertex_in, //!< modified on output
  int nx, int ny,
  float dx, float dy,
  float xmin, float ymin,
  float d_height,
  float rough,
  int filter_iter,
  int seed);

//! Creates a single ridge. vertex_in must already be allocated
void ridge_terrain(Vertex * vertex_in, //!< modified on output
                   int nx, int ny,
                   float dx, float dy,
                   float xmin, float ymin,
                   float height, float width);

/// Creates a single plateau (with two cliffs). vertex_in must already be allocated
void plateau_terrain(Vertex * vertex_in, //!< modified on output
                     int nx, int ny,
                     float dx, float dy,
                     float xmin, float ymin,
                     float height, 
                     float ridge_width, 
                     float plateau_width);

/// Creates a terrain suitable for sledging. Returns the start
/// position and some race gates.
struct Gate
{
  Gate(float lx, float ly, float rx, float ry)
    : left_x(lx), left_y(ly), right_x(rx), right_y(ry) {}
  float left_x, left_y, right_x, right_y;
};

Position piste_terrain(Vertex * vertex_in, //!< modified on output
                       int nx, int ny,
                       float dx, float dy,
                       float xmin, float ymin,
                       float slope,
                       int seed,
                       std::vector<Gate> & gates);

#endif // file included
