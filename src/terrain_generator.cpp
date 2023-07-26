/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file terrain_generator.cpp
*/
#include "terrain_generator.h"
#include "lod.h"
#include "vertex.h"
#include "misc.h"
#include "sss_random.h"
#include "log_trace.h"
#include "array_2d.h"

#include <time.h>
#include <stdlib.h>
#include "sss_assert.h"
#if defined(__ppc__)
#include "sss_ppc.h"
#endif

using namespace std;

static Sss_random terrain_random(10);

// local functions
static void zero_edges(Vertex * vertex_in, 
                       int nx, int ny,
                       int n_offset = 4)
{
  int i, j;
  
  for (int offset = 0 ; offset < n_offset ; ++offset)
  {
    float scale = (float) offset/n_offset;
    
    for (i = offset ; i < (nx-offset)-1 ; ++i)
    {
      vertex_in[Lod::calc_index(i, offset, nx)].z *= scale;
      vertex_in[Lod::calc_index(i+1, (ny-1)-offset, nx)].z *= scale;
    }
    
    for (j = offset ; j < (ny-offset)-1 ; ++j)
    {
      vertex_in[Lod::calc_index(offset, j+1, nx)].z *= scale;
      vertex_in[Lod::calc_index((nx-1)-offset, j, nx)].z *= scale;
    }
  }
}

/*!  Reads terrain from file, and allocates memory for an array
  (vertex_in) which it populates. Also reads and sets nx and ny.  */
void file_terrain(FILE * terrain_file,
                  Vertex *& vertex_in,
                  int & nx, int & ny,
                  bool do_zero_edges)
{
  size_t rv;
  rv = fread(&nx, sizeof(nx), 1, terrain_file);
  rv = fread(&ny, sizeof(ny), 1, terrain_file);
#if defined(__ppc__)
  LSWAP(nx);
  LSWAP(ny);
#endif
  
  TRACE("Terrain nx = %d, ny = %d\n", nx, ny);
  
  float * orog_x = new float[nx];
  float * orog_y = new float[ny];
  float * orog_z = new float[nx*ny];

  fread(&orog_x[0], sizeof(orog_x[0]), nx, terrain_file);
  fread(&orog_y[0], sizeof(orog_y[0]), ny, terrain_file);
  fread(&orog_z[0], sizeof(orog_z[0]), nx * ny, terrain_file);
  
#if defined(__ppc__)
  for (int i = 0 ; i < nx ; ++i) { FSWAP(orog_x[i]); }
  for (int i = 0 ; i < ny ; ++i) { FSWAP(orog_y[i]); }
  for (int i = 0 ; i < nx*ny ; ++i) { FSWAP(orog_z[i]); }
#endif
  // re-arrange the data to pass to the lod class.
  
  vertex_in = new Vertex[nx*ny];
  for (int i = 0 ; i < nx ; ++i)
  {
    for (int j = 0 ; j < ny ; ++j)
    {
      vertex_in[Lod::calc_index(i, j, nx)] = 
        Vertex(orog_x[i], orog_y[j], orog_z[i + nx * j]);
      //printf("%5f %5f %5f\n", orog_x[i], orog_y[j], orog_z[i + nx * j]);
    }
  }
  
  delete [] orog_x;
  delete [] orog_y;
  delete [] orog_z;
  
  if (do_zero_edges) zero_edges(vertex_in, nx, ny);
}



/*!  Adds a gaussian peak to the array, and attenuates it towards the
  edge of the array.  */
void add_peak(Vertex * vertex_in, 
              int nx, int ny, float dx, float dy,
              float xmid, float ymid, float height, float range)
{
  int i, j;
  const float edge_dist = dx * nx * 0.1f;
  const float edge_dist2 = edge_dist * edge_dist;
  
  float xmin = vertex_in[Lod::calc_index(0, 0, nx)].x;
  float ymin = vertex_in[Lod::calc_index(0, 0, nx)].y;
  
  int i_min = (int) (((xmid-3*range)-xmin)/dx);
  int i_max = (int) (((xmid+3*range)-xmin)/dx);
  int j_min = (int) (((ymid-3*range)-ymin)/dy);
  int j_max = (int) (((ymid+3*range)-ymin)/dy);
  
  i_min = sss_max(sss_min(i_min, nx-1), 1);
  i_max = sss_max(sss_min(i_max, nx-1), 1);
  j_min = sss_max(sss_min(j_min, ny-1), 1);
  j_max = sss_max(sss_min(j_max, ny-1), 1);
  
  for (i = i_min ; i < i_max ; ++i)
  {
    for (j = j_min ; j < j_max ; ++j)
    {
      float x = vertex_in[Lod::calc_index(i, j, nx)].x;
      float y = vertex_in[Lod::calc_index(i, j, nx)].y;
      float dist_to_edge = sss_min(x-xmin,y-ymin);
      dist_to_edge = sss_min(dist_to_edge, xmin + (nx-1)*dx - x);
      dist_to_edge = sss_min(dist_to_edge, ymin + (ny-1)*dy - y);
      float edge_scale = 1 - exp(-dist_to_edge * dist_to_edge/edge_dist2);
      
      float dz = edge_scale * height * 
        exp(-( ((x-xmid) * (x-xmid)) + ((y-ymid) * (y-ymid)) ) /
            ( range*range ) );
      
      vertex_in[Lod::calc_index(i, j, nx)].z += dz;
      
    }
  }
}

//! seed the random number generator, using time if the seed = 0
void seed_rand(int seed)
{
  if (seed == 0)
  {
    seed = (int) time( NULL );
    TRACE("Add builtin_terrain_seed = %d to sss.cfg to reproduce\n",
          seed);
  }
  else
  {
    TRACE("Using builtin_terrain_seed = %d (from sss.cfg)\n",seed);
  }
  
//  srand((unsigned) seed);
  terrain_random.set_seed((long) seed);
}

/*! 
  vertex_in is modified on output. on input it must contain (nx*ny) entries.
*/
void gaussian_peaks_terrain(Vertex * vertex_in,
                            int nx, int ny,
                            float dx, float dy,
                            float xmin, float ymin,
                            float range,
                            float peak,
                            float n_peaks,
                            int seed)
{
  TRACE("Calculating peaks terrain\n");
  
  seed_rand(seed);
  
  int i, j;
  // Set the x and y values (not the z yet)
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float x = xmin + dx*i;
      float y = ymin + dy*j;
      assert1(Lod::calc_index(i, j, nx) < (nx*ny));
      vertex_in[Lod::calc_index(i, j, nx)] = Vertex(x, y, 0);;
    }
  }
  
  // can specify that a certain set of peaks always exist the
  // numbers here are normalised by range and peak above, and x/y
  // are in the range {0-1}
  float peak_x[] = { 0.5f };
  float peak_y[] = { 0.5f };
  float peak_h[] = { 0.7f };
  float peak_r[] = { 1.0f };
  
  for (i = 0 ; i < (int) (sizeof(peak_x)/sizeof(peak_x[0])) ; ++i)
  {
    add_peak(vertex_in, nx, ny, dx, dy,
             xmin + peak_x[i]*nx*dx,
             ymin + peak_y[i]*ny*dy,
             peak_h[i]*peak, 
             peak_r[i]*range);
  }
  
  float peak_max = 0.5f;
  float peak_min = -0.6f;
  float range_offset = 1.3f;
  
  for (i = 0 ; i < n_peaks ; ++i)
  {
    if (i % 20 == 0)
    {
      TRACE(".");
      fflush(NULL);
    }
    
    float h;
    float r;
    do
    {
      h = peak * (peak_min + (peak_max -peak_min) *
                  terrain_random.rand());
      r = fabs(h * (range_offset + terrain_random.rand()));
    } while (r > (nx * dx * 0.5f));
    
    float x = xmin + r + (nx*dx - 2*r) * terrain_random.rand();
    float y = ymin + r + (ny*dy - 2*r) * terrain_random.rand();
    
    add_peak(vertex_in, nx, ny, dx, dy,
             x, y, h, r);
  }
  
  zero_edges(vertex_in, nx, ny);
  TRACE("\nDone calculating terrain\n");
}

#define Z(i, j) vertex_in[Lod::calc_index(i, j, nx)].z

//! Applies a filter to the array passed in
static void apply_filter(Vertex * vertex_in, //!< modified on output
                         int nx, int ny,
                         float filter)
{
  int i, j;
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      int i1 = (i + 1) % nx;
      int j1 = (j + 1) % ny;
      int i0 = (i + nx - 1) % nx;
      int j0 = (j + ny - 1) % ny;
      
      Z(i, j) = (1-filter) * Z(i, j) + 
        (Z(i0, j) + Z(i1, j) + Z(i, j0) + Z(i, j1))*0.25f * filter;
    }
  }
}


//! Creates a terrain based on mid-point displacement. On entry,
//! vertex_in should be allocated.
void midpoint_displacement_terrain(Vertex * vertex_in, //!< modified on output
                                   int nx, int ny,
                                   float dx, float dy,
                                   float xmin, float ymin,
                                   float d_height,
                                   float rough,
                                   int filter_iter,
                                   int seed)
{
  TRACE("Calculating midpoint displacement terrain\n");
  seed_rand(seed);
  
  float r = (float)pow(2.0,-1.0*rough);
  
  int i, j;
  // Set the x and y values (not the z yet)
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float x = xmin + dx*i;
      float y = ymin + dy*j;
      assert1(Lod::calc_index(i, j, nx) < (nx*ny));
      vertex_in[Lod::calc_index(i, j, nx)] = Vertex(x, y, 0);;
    }
  }
  
  int size = nx-1;
  int rect_size = size;
  
  while (rect_size > 0)
  {
    // diamond step
    for (i = 0 ; i < size ; i += rect_size)
    {
      for (j = 0 ; j < size ; j += rect_size)
      {
        int ni = (i + rect_size) % nx;
        int nj = (j + rect_size) % ny;
        
        int mi = (i + rect_size/2);
        int mj = (j + rect_size/2);        
        
        // ensure that we go up in the middle
        float offset;
        if (rect_size == size)
        {
          offset = terrain_random.rand(-d_height/2.0f, d_height);
        }
        else
        {
          offset = terrain_random.rand(-d_height, d_height);
        }
        
        Z(mi, mj) = ( Z(i, j) + Z(ni, j) + Z(i, nj) + Z(ni, nj) ) * 0.25f + 
          offset;
      }
    }
    
    // square step. Note that we have to be more careful about the
    // boundary conditions.
    
    for (i = 0 ; i < size ; i += rect_size)
    {
      for (j = 0 ; j < size ; j += rect_size)
      {
        int ni = (i + rect_size) % nx;
        int nj = (j + rect_size) % ny;
        
        int mi = (i + rect_size/2);
        int mj = (j + rect_size/2);
        
        int pmi = (i-rect_size/2 + nx) % nx;
        int pmj = (j-rect_size/2 + ny) % ny;
        
        /*
          Calculate the square value for the top side of the rectangle
        */
        Z(mi, j) = ( Z(i, j) + Z(ni, j) + Z(mi, pmj) + Z(mi, mj) ) * 0.25f +
          terrain_random.rand(-d_height, d_height);
        
        /*
          Calculate the square value for the left side of the rectangle
        */
        Z(i, mj) = ( Z(i, j) + Z(i, nj) + Z(pmi, mj) + Z(mi, mj) ) * 0.25f +
          terrain_random.rand(-d_height, d_height);
      }
    }
    
    rect_size /= 2;
    d_height *= r;
  }
  
  zero_edges(vertex_in, nx, ny);
  
  for (i = 0 ; i < filter_iter ; ++i)
  {
    apply_filter(vertex_in, nx, ny, 0.5f);
  }
  
  zero_edges(vertex_in, nx, ny);
  TRACE("Done calculating terrain\n");
}
#undef Z






//! Creates a terrain based on mid-point displacement. On entry,
//! vertex_in should be allocated.
void ridge_terrain(Vertex * vertex_in, //!< modified on output
                   int nx, int ny,
                   float dx, float dy,
                   float xmin, float ymin,
                   float height, float width)
{
  TRACE("Calculating ridge terrain - height = %f, width = %f\n",
        height, width);
  
  float xmax = xmin + nx * dx;
//  float ymax = ymin + ny * dy;
  
  float xmid = 0.5f * (xmin + xmax);
//  float ymid = 0.5f * (ymin + ymax);
  
  int i, j;
  // Set the x and y values (not the z yet)
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float x = xmin + dx*i;
      float y = ymin + dy*j;
      float d = x - xmid;
      float z = height * exp( -d*d/(width*width) );
      assert1(Lod::calc_index(i, j, nx) < (nx*ny));
      vertex_in[Lod::calc_index(i, j, nx)] = Vertex(x, y, z);;
    }
  }
  zero_edges(vertex_in, nx, ny);
  TRACE("Done calculating terrain\n");
}

//! Creates a terrain based on mid-point displacement. On entry,
//! vertex_in should be allocated.
void plateau_terrain(Vertex * vertex_in, //!< modified on output
                     int nx, int ny,
                     float dx, float dy,
                     float xmin, float ymin,
                     float height, float ridge_width, float plateau_width)
{
  TRACE("Calculating plateau terrain\n");
  
  float xmax = xmin + nx * dx;
  //  float ymax = ymin + ny * dy;
  
  float xmid = 0.5f * (xmin + xmax);
//  float ymid = 0.5f * (ymin + ymax);
  
  int i, j;
  // Set the x and y values (not the z yet)
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float x = xmin + dx*i;
      float y = ymin + dy*j;
      float d = x - xmid;
      if (d > plateau_width*0.5f)
      {
        d -= (plateau_width*0.5f);
      }
      else if (d < -plateau_width*0.5f)
      {
        d += (plateau_width*0.5f);
      }
      else
        d = 0;
      
      float z = height * exp( -d*d/(ridge_width*ridge_width) );
      assert1(Lod::calc_index(i, j, nx) < (nx*ny));
      vertex_in[Lod::calc_index(i, j, nx)] = Vertex(x, y, z);
    }
  }
  zero_edges(vertex_in, nx, ny);
  TRACE("Done calculating terrain\n");  
}

/// Structure used in draw_gaussian_lines
struct Point
{
  Point(float x, float y, float d, float w) : x(x), y(y), d(d), w(w) {};
  float x, y;
  float d; // the depth of the point
  float w; // the width of the gaussian at the point
};

/// Applies the gaussian pattern at a single point, only increasing
/// the values (which represent depth)
void apply_gaussian(Array_2D<float> & orig_array,
                    float xmin, float ymin, float dx, float dy,
                    const Point & point)
{
  const int nx = orig_array.get_nx();
  const int ny = orig_array.get_ny();
  
  /// work out the approx index of the center
  const int i0 = (int) ((point.x - xmin) / dx);
  const int j0 = (int) ((point.y - ymin) / dy);
  // how big a range?
  const int delta_i = (int) (2 * (point.w / dx));
  const int delta_j = (int) (2 * (point.w / dy));
  const float w2 = point.w * point.w;
  
  int i, j;
  for (i = i0 - delta_i ; i < i0 + delta_i ; ++i)
  {
    for (j = j0 - delta_j ; j < j0 + delta_j ; ++j)
    {
      // check in range
      if ( (i >= 0) && (i < nx) &&
           (j >= 0) && (j < ny) )
      {
        float x = xmin + i * dx;
        float y = ymin + j * dy;
        float r2 = (x - point.x) * (x - point.x) + 
          (y - point.y) * (y - point.y);
        
        float new_depth = point.d * exp(-r2 / w2);
        if (new_depth > orig_array.at(i, j))
          orig_array.at(i, j) = new_depth;
      }
    }
  }
}


/// Helper for the piste terrain. We want to be able to "draw" the
/// track by specifying a series of points. We do this by
/// interpolating between the points, and at each interpolated
/// location drawing a guassian blob onto a new array... but we only
/// update the new array of the depth is greater than the previous
/// value. At the end we subtract this array from the original.
void draw_gaussian_lines(Array_2D<float> & orig_array,
                         float xmin, float ymin, float dx, float dy,
                         const vector<Point> & points)
{
  int nx = orig_array.get_nx();
  int ny = orig_array.get_ny();
  
  if (points.size() < 2)
    return;
  
  Array_2D<float> depth(nx, ny, 0.0f);
  
  unsigned index;
  for (index = 1 ; index < points.size() ; ++index)
  {
    Point p0 = points[index-1];
    Point p1 = points[index];
    // "draw" the line by brute force - draw gaussians at n positions
    // between the points, where n is the max number of points in
    // either the x or y dir
    float delta_x = p1.x - p0.x;
    float delta_y = p1.y - p0.y;
    float num_x = fabs(delta_x / dx);
    float num_y = fabs(delta_y / dy);
    int num = (int) (sss_max(num_x + 1, num_y + 1));
//    TRACE("num = %d\n", num);
    for (int i = 0 ; i < num ; ++i)
    {
      Point p(0, 0, 0, 0);
      p.x = p0.x + (p1.x - p0.x) * (float) i/(num-1);
      p.y = p0.y + (p1.y - p0.y) * (float) i/(num-1);
      p.d = p0.d + (p1.d - p0.d) * (float) i/(num-1);
      p.w = p0.w + (p1.w - p0.w) * (float) i/(num-1);
      apply_gaussian(depth, xmin, ymin, dx, dy, p);
    }
  }
//  depth.dump("depth.txt");
  
  // now apply depth to the original
  orig_array.subtract(depth);
}



Position piste_terrain(Vertex * vertex_in, //!< modified on output
                       int nx, int ny,
                       float dx, float dy,
                       float xmin, float ymin,
                       float slope,
                       int seed,
                       std::vector<Gate> & gates)
{
  TRACE("Calculating piste terrain\n");
  seed_rand(seed);
  
  float xmax = xmin + nx * dx;
  float ymax = ymin + ny * dy;
  
//  float xmid = 0.5f * (xmin + xmax);
//  float ymid = 0.5f * (ymin + ymax);
  
  float zmin = 0.0f;
  
  Array_2D<float> array(nx, ny, 0.0f);
  
  int i, j;
  float x0 = xmin + 0.15 * (xmax - xmin);
  float x1 = xmin + 0.85 * (xmax - xmin);
  
  // Set the x and y values (not the z yet)
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float x = xmin + dx*i;
//      float y = ymin + dy*j;
      if (x > x1)
        x = x1;
      if (x < x0)
        x = x0;
      float z = zmin + (x - x0) * slope;
      array.at(i, j) = z;
    }
  }
  
//  array.gaussian_filter(nx/60, 2*nx/60);
  
  // that's the basic hill. Now draw the "track"
  const float min_depth = 20;
  const float max_depth = 40;
  const float min_width = 25; // varying width doesn't work well...
  const float max_width = 25;
  
  vector<Point> points;
  
  points.push_back(Point(xmax, 
                         (ymax + ymin) * 0.5f, 
                         terrain_random.rand(min_depth, max_depth),
                         terrain_random.rand(min_width, max_width)));
  // this gets updated in the loop until x < x1
  Position start_pos(x1, points.back().y, 0);
  
  float end_x = x0;
//  float y0 = ymin + 0.35 * (ymax - ymin);
//  float y1 = ymax - 0.35 * (ymax - ymin);
  float max_transverse_slope = 2.0; // dy/dx
  
  float dir = 0;
  const float max_dir = fabs(atan_deg(max_transverse_slope));
  const float ddir_dl = 1.0f; // i.e. ddir_dl = 1 means 1 deg per m
  const float dl = 5.0f; // length of each piece
  const float max_section_len = 70.0f;
  const float max_dw_dl = 0.1f;
  const float max_dd_dl = 0.5f;
  
  const float ddir_per_section = fabs(ddir_dl * dl);
  
  float next_dir = terrain_random.rand(-max_dir, max_dir);
  float section_length = terrain_random.rand(0.0f, max_section_len);
  
  float last_x = points.back().x;
  float last_y = points.back().y;
  float last_w = points.back().w;
  float last_d = points.back().d;
  float current_section_dist = 0.0f;
  
  while (last_x > end_x)
  {
//    TRACE("last_x = %f, end_x = %f\n", last_x, end_x);
    float x = last_x - dl * cos_deg(dir);
    float y = last_y + dl * sin_deg(dir);
    float w = last_w + terrain_random.rand(-max_dw_dl * dl, max_dw_dl * dl);
    float d = last_d + terrain_random.rand(-max_dd_dl * dl, max_dd_dl * dl);
    if (w < min_width) w = min_width;
    else if (w > max_width) w = max_width;
    if (d < min_depth) d = min_depth;
    else if (d > max_depth) d = max_depth;
    
    points.push_back(Point(x, y, w, d));
    if (x > x1)
      start_pos = Position(points.back().x, points.back().y, 0);
    
    last_x = points.back().x;
    last_y = points.back().y;
    last_w = points.back().w;
    last_d = points.back().d;
    current_section_dist += dl;
    
    // now update the direction to aim for
    if ( (fabs(dir - next_dir) < ddir_per_section) &&
         (current_section_dist > section_length) )
    {
      // avoid downhill...
      float r = terrain_random.rand(0.0001f, 1.0f);
      r = pow(r, 0.1f);
      float r2 = terrain_random.rand(-1, 1);
      next_dir = r * max_dir * (r2 > 0 ? 1 : -1);
//      next_dir = terrain_random.rand(-max_dir, max_dir);
      current_section_dist = 0.0f;
    }
    
    float total_ddir = next_dir - dir;
    if (total_ddir > 0)
      dir += ddir_per_section;
    else
      dir -= ddir_per_section;
  }
  
  draw_gaussian_lines(array, 
                      xmin, ymin, dx, dy,
                      points);
  
  // now update the checkpoints/gates
  float dist = 0.0f;
  float sec_len = 200.0f;
  float next_cp = -1.0f;
  for (i = 1 ; i < (int) (points.size() - 1) ; ++i)
  {
    if ( points[i].x < start_pos[0])
    {
      bool last_one = false;
      if ( points[i].x < (x0 + max_depth * 1.0/slope) )
      {
        last_one = true;
      }
          
      if ( (dist > next_cp) || last_one )
      {
        // do a gate
        Vector3 dir(points[i+1].x - points[i-1].x,
                    points[i+1].y - points[i-1].y,
                    0);
        Vector3 gate_dir = cross(dir, Vector3(0, 0, 1)).normalise();
        Position pos_left = Position(points[i].x,
                                     points[i].y,
                                     0) - gate_dir * points[i].w * 0.8;
        Position pos_right = Position(points[i].x,
                                      points[i].y,
                                      0) + gate_dir * points[i].w * 0.8;
        gates.push_back(Gate(pos_left[0], pos_left[1],
                             pos_right[0], pos_right[1]));
        
        TRACE_FILE_IF(2)
          {
            TRACE("Adding gate (%f, %f),(%f, %f):\n",
                  gates.back().left_x,
                  gates.back().left_y,
                  gates.back().right_x,
                  gates.back().right_y);
          }
        // calculate the next checkpoint
        next_cp = dist + sec_len;
      }
      // increment the distance so far...
      dist += hypot(points[i].x - points[i-1].x,
                    points[i].y - points[i-1].y);
      if (last_one)
        goto no_more_gates;
    }
  }
 no_more_gates:
  TRACE_FILE_IF(2)
    TRACE("Created %d checkpoint gates\n", gates.size());
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float x = xmin + dx*i;
      float y = ymin + dy*j;
      vertex_in[Lod::calc_index(i, j, nx)] = Vertex(x, y, array.at(i, j));
    }
  }
  
  zero_edges(vertex_in, nx, ny, nx/10);
  TRACE("Done calculating terrain\n");  
  return start_pos;
}

