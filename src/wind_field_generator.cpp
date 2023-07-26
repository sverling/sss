/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file wind_field_generator.cpp
*/
#include "wind_field_generator.h"
#include "vertex.h"
#include "environment.h"
#include "fft2d.h"
#include "log_trace.h"
#if defined(__ppc__)
#include "sss_ppc.h"
#endif

using namespace std;

#include <math.h>
#include <stdlib.h>

//! Dumps a west-east transect to file (for debugging)
static bool dump_transect(int wind_nx, int wind_ny, int wind_nz, 
                          float * wind_x, float * wind_z,
                          float * wind_u, float * wind_w,
                          const Environment * env) // for calculating the indices
{
  int i, j, k;

  FILE * file = fopen("transect.dat", "w");
  if (!file)
  {
    TRACE("Unable to open data.dat for writing\n");
    return false;
  }
  
  fwrite(&wind_nx, sizeof(wind_nx), 1, file);
  fwrite(&wind_nz, sizeof(wind_nz), 1, file);

  // write out coords
  for (i = 0 ; i < wind_nx ; ++i)
    fwrite(&wind_x[i], sizeof(wind_x[i]), 1, file);
  for (i = 0 ; i < wind_nz ; ++i)
    fwrite(&wind_z[i], sizeof(wind_z[i]), 1, file);
  
  // write out the middle transect
  j = wind_ny/2;
  for (k = 0 ; k < wind_nz ; ++k)
  {
    for (i = 0 ; i < wind_nx ; ++i)
    {
      fwrite(&wind_u[env->calc_index(i, j, k)], sizeof(wind_u[0]), 1, file);
    }
  }
  for (k = 0 ; k < wind_nz ; ++k)
  {
    for (i = 0 ; i < wind_nx ; ++i)
    {
      fwrite(&wind_w[env->calc_index(i, j, k)], sizeof(wind_w[0]), 1, file);
    }
  }
  
  fclose(file);
  return true;

}

//! dumps a Complex_array to file for debugging
static bool dump_array(const char * file_name,
                       const Complex_array & array)
{
  int i, j;

  FILE * file = fopen(file_name, "w");
  if (!file)
  {
    TRACE("Unable to open %s for writing\n", file_name);
    return false;
  }
  
  int nx = array.get_nx();
  int ny = array.get_ny();
  
  fwrite(&nx, sizeof(nx), 1, file);
  fwrite(&ny, sizeof(ny), 1, file);

  // write out coords
  for (i = 0 ; i < nx ; ++i)
  {
    float val = (float) i;
    fwrite(&val, sizeof(val), 1, file);
  }
  for (i = 0 ; i < ny ; ++i)
  {
    float val = (float) i;
    fwrite(&val, sizeof(val), 1, file);
  }
  
  // write out the array (magnitude)

  for (j = 0 ; j < ny ; ++j)
  {
    for (i = 0 ; i < nx ; ++i)
    {
      float val = hypot(array(i, j).real, array(i, j).imag);
      fwrite(&val, sizeof(val), 1, file);
    }
  }
  
  fclose(file);
  return true;

}

//! Reads wind data from file
int wind_from_file(const string & file_name,
                   float *& wind_x, 
                   float *& wind_y, 
                   float *& wind_z, 
                   float *& wind_u, 
                   float *& wind_v, 
                   float *& wind_w, 
                   int & wind_nx,
                   int & wind_ny, 
                   int & wind_nz)
{
  unsigned int num;
  
  FILE * winds_file = fopen(file_name.c_str(), "rb");
  if (!winds_file)
  {
    TRACE("Unable to open %s\n", file_name.c_str());
    return -1;
  }

  if (1 != fread(&wind_nx, sizeof(wind_nx), 1, winds_file)) goto err;
  if (1 != fread(&wind_ny, sizeof(wind_ny), 1, winds_file)) goto err;
  if (1 != fread(&wind_nz, sizeof(wind_nz), 1, winds_file)) goto err;
  
#if defined(__ppc__)
  LSWAP(wind_nx);
  LSWAP(wind_ny);
  LSWAP(wind_nz);
#endif
  
  wind_x = new float[wind_nx];
  wind_y = new float[wind_ny];
  wind_z = new float[wind_nz];
  
  if (wind_nx != (int) fread(&wind_x[0], sizeof(wind_x[0]), wind_nx, winds_file)) goto err;
  if (wind_ny != (int) fread(&wind_y[0], sizeof(wind_y[0]), wind_ny, winds_file)) goto err;
  if (wind_nz != (int) fread(&wind_z[0], sizeof(wind_z[0]), wind_nz, winds_file)) goto err;
  
#if  defined(__ppc__)
  for(int i=0; i<wind_nx; i++) { FSWAP(wind_x[i]); }
  for(int i=0; i<wind_ny; i++) { FSWAP(wind_y[i]); }
  for(int i=0; i<wind_nz; i++) { FSWAP(wind_z[i]); }
#endif

  wind_u = new float[wind_nx * wind_ny * wind_nz];
  wind_v = new float[wind_nx * wind_ny * wind_nz];
  wind_w = new float[wind_nx * wind_ny * wind_nz];
  
  num = wind_nx * wind_ny * wind_nz;
  if (num != fread(&wind_u[0], sizeof(wind_u[0]), num, winds_file)) goto err;
  if (num != fread(&wind_v[0], sizeof(wind_v[0]), num, winds_file)) goto err;
  if (num != fread(&wind_w[0], sizeof(wind_w[0]), num, winds_file)) goto err;

#if defined(__ppc__)
  for(int i=0; i<num; i++) { 
    FSWAP(wind_u[i]); 
    FSWAP(wind_v[i]); 
    FSWAP(wind_w[i]); 
  }
#endif

  fclose(winds_file);
  return 0;
  
 err:
  fclose(winds_file);
  TRACE("Error reading from %s\n", file_name.c_str());
  return -1;
}

/*! The terrain calculation uses just the west-east gradient and a
  constant west-east wind speed to generate a wind velocity that
  follows the contours, with a scaling with height. */
void simple_wind_from_terrain(const Environment * environment,
                              const Vertex_array * ground_vertex_array,
                              float *& wind_x, 
                              float *& wind_y, 
                              float *& wind_z, 
                              float *& wind_u, 
                              float *& wind_v, 
                              float *& wind_w, 
                              int & wind_nx,
                              int & wind_ny, 
                              int & wind_nz)
{
  const int nx = ground_vertex_array->get_nx();
  const int ny = nx;
  
  // calculate the wind field (hack alert!)
  
  wind_nx = 1 + (nx-1)/4;
  wind_ny = 1 + (ny-1)/4;
  wind_nz = 16;
  
  float dx = ground_vertex_array->get_pos(1, 0)[0] - 
    ground_vertex_array->get_pos(0, 0)[0];
  float dy = ground_vertex_array->get_pos(0, 1)[1] - 
    ground_vertex_array->get_pos(0, 0)[1];
  float xmin = ground_vertex_array->get_pos(0, 0)[0];
  float ymin = ground_vertex_array->get_pos(0, 0)[1];
  
  float peak = 200; // used for the scaling
  
  float wind_dx = dx*4;;
  float wind_dy = dy*4;;
  float wind_dz = 2 * peak/wind_nz;
  
  wind_x = new float[wind_nx];
  wind_y = new float[wind_ny];
  wind_z = new float[wind_nz];
  
  int i, j, k;
  
  for (i = 0 ; i < wind_nx ; ++i)
  {
    wind_x[i] = xmin + wind_dx * i;
  }
  for (j = 0 ; j < wind_ny ; ++j)
  {
    wind_y[j] = ymin + wind_dy * j;
  }
  for (k = 0 ; k < wind_nz ; ++k)
  {
    wind_z[k] = wind_dz * k;
  }
  
  wind_u = new float[wind_nx * wind_ny * wind_nz];
  wind_v = new float[wind_nx * wind_ny * wind_nz];
  wind_w = new float[wind_nx * wind_ny * wind_nz];
  
  float basic_u = 5; // west-east wind of 5 m/s
  float vertical_scale = peak*2.0f; // scale vertical velocity perturbation
  
  for (i = 0 ; i < wind_nx ; ++i)
  {
    for (j = 0 ; j < wind_ny ; ++j)
    {
      for (k = 0 ; k < wind_nz ; ++k)
      {
        wind_v[environment->calc_index(i, j, k)] = 0;
        if ((i == 0) || (i==wind_nx-1))
        {
          wind_u[environment->calc_index(i, j, k)] = basic_u;
          wind_w[environment->calc_index(i, j, k)] = 0;
        }
        else
        {
          // calculate height gradient
          float z1 = ground_vertex_array->get_pos(i*4-1, j*4)[2];
          float z2 = ground_vertex_array->get_pos(i*4+1, j*4)[2];
//          float z0 = ground_vertex_array->get_pos(i*4  , j*4)[2];
          float slope = (z2-z1)/(2*dx);
          slope *= exp(-(wind_z[k] * wind_z[k]) /
                       (vertical_scale*vertical_scale));
          wind_u[environment->calc_index(i, j, k)] = 
            basic_u / sqrt(1+slope*slope);
          wind_w[environment->calc_index(i, j, k)] = wind_u[environment->calc_index(i, j, k)] * slope;
        }
      }
    }
  }
//    dump_transect(wind_nx, wind_ny, wind_nz,
//                  wind_x, wind_z,
//                  wind_u, wind_w,
//                  environment);
}


//! Imposes a Butterworth filter with cutoff freq c, and power n
void fft_filter(Complex_array & array, float c, float n)
{
  const int & nx = array.get_nx();
  const int & ny = array.get_ny();
  const int nx2 = nx/2;
  const int ny2 = ny/2;
  assert1(nx2*2 == nx);
  assert1(ny2*2 == ny);

  const double c2 = c * c;

  for (int i = 0 ; i < nx ; ++i)
  {
    for (int j = 0 ; j < ny ; ++j)
    {
      if ( (i != nx2) || (j != ny2) )
      {
        float k2 = (i-nx2)*(i-nx2) + (j-ny2)*(j-ny2);
        float scale = 1.0f/(1.0f + pow(k2/c2, (double) n));
        array(i, j) *= scale;
      }
    }
  }
}

//! Calculates wind field from terrain
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
                                  int & wind_nz)
{
  const int nx = ground_vertex_array->get_nx();
  const int ny = nx;
  
  // calculate the wind field (hack alert!)
  
  const int skip = 1;

  wind_nx = 1 + (nx-1)/skip;
  wind_ny = 1 + (ny-1)/skip;
  wind_nz = 16;
  
  float dx = ground_vertex_array->get_pos(1, 0)[0] - 
    ground_vertex_array->get_pos(0, 0)[0];
  
  float peak = 200;
  
  float wind_dz = 2 * peak/wind_nz;
  
  wind_x = new float[wind_nx];
  wind_y = new float[wind_ny];
  wind_z = new float[wind_nz];
  
  int i, j, k;
  
  j = 0;
  for (i = 0 ; i < wind_nx ; ++i)
  {
    wind_x[i] = ground_vertex_array->get_pos(i*skip, j*skip)[0];
  }
  i = 0;
  for (j = 0 ; j < wind_ny ; ++j)
  {
    wind_y[j] = ground_vertex_array->get_pos(i*skip, j*skip)[1];
  }
  for (k = 0 ; k < wind_nz ; ++k)
  {
    wind_z[k] = wind_dz * k;
  }
  
  wind_u = new float[wind_nx * wind_ny * wind_nz];
  wind_v = new float[wind_nx * wind_ny * wind_nz];
  wind_w = new float[wind_nx * wind_ny * wind_nz];
  
  float basic_u = 5; // west-east wind of 7 m/s
  float vertical_scale = peak*2.0f; // scale vertical velocity perturbation
  
  // Copy the terrain data into a form suitable for passing to the FFT
  // note that the original terrain is 2^n + 1 - we need 2^n
  const int terrain_nx = ground_vertex_array->get_nx() - 1;
  const int terrain_ny = terrain_nx;
  
  Complex_array terrain(terrain_nx,
                        terrain_nx);
  for (i = 0 ; i < terrain_nx ; ++i)
  {
    for (j = 0 ; j < terrain_nx ; ++j)
    {
      terrain(i, j) = Complex(ground_vertex_array->get_pos(i, j)[2]);
    }
  }

  dump_array("orig.dat", terrain);

  Complex_array terrain_fft(terrain);
  FFT2D(terrain_fft, terrain_nx, terrain_ny, 1);
  // rotate here so that filtering is easier
  // remember to rotate again before inverse ffting
  terrain_fft.rotate();
  
  dump_array("orig_fft.dat", terrain_fft);

  // Start the loops
  for (k = 0 ; k < wind_nz ; ++k)
  {
    // copy the original terrain.
    TRACE("FFT\n");

    // scale the fft
    fft_filter(terrain_fft, 5, 1);
    dump_array("after_fft.dat", terrain_fft);

    Complex_array terrain_new(terrain_fft);
    
    terrain_new.rotate();
    FFT2D(terrain_new, terrain_nx, terrain_ny, -1);
    
    dump_array("after.dat", terrain_new);
    if (k == 0)
      exit(0);
    
    for (i = 0 ; i < wind_nx ; ++i)
    {
      for (j = 0 ; j < wind_ny ; ++j)
      {
        wind_v[environment->calc_index(i, j, k)] = 0;
        if ((i == 0) || (i==wind_nx-1))
        {
          wind_u[environment->calc_index(i, j, k)] = basic_u;
          wind_w[environment->calc_index(i, j, k)] = 0;
        }
        else
        {
          // calculate height gradient
          float z1 = ground_vertex_array->get_pos(i*skip-1, j*skip)[2];
          float z2 = ground_vertex_array->get_pos(i*skip+1, j*skip)[2];
//          float z0 = ground_vertex_array->get_pos(i*skip  , j*skip)[2];
          float slope = (z2-z1)/(2*dx);
          slope *= exp(-(wind_z[k] * wind_z[k]) /
                       (vertical_scale*vertical_scale));
          wind_u[environment->calc_index(i, j, k)] = 
            basic_u / sqrt(1+slope*slope);
          wind_w[environment->calc_index(i, j, k)] = wind_u[environment->calc_index(i, j, k)] * slope;
        }
      }
    }
  }

  dump_transect(wind_nx, wind_ny, wind_nz,
                wind_x, wind_z,
                wind_u, wind_w,
                environment);
}

//! Repeatedly imposes a "diffusion" on the array
#define INDEX(i, j) (i + terrain_nx * j)
static void filter_terrain(float * terrain, 
                           int terrain_nx, int terrain_ny, 
                           float frac, unsigned int iter)
{
  const int ip[] = {-1, 0, 1, 1, 1, 0, -1, -1};
  const int jp[] = {1, 1, 1, 0, -1, -1, -1, 0};
  int i, j;
  unsigned n;
  
  for ( ; iter != 0 ; --iter)
  {
    for (i = 0 ; i < terrain_nx ; ++i)
    {
      for (j = 0 ; j < terrain_ny ; ++j)
      {
        float weight = 1;
        float total = terrain[INDEX(i, j)];
        for (n = 0 ; n < sizeof(ip)/sizeof(ip[0]) ; ++n)
        {
          int ii = i + ip[n];
          if ( (ii > 0) && (ii < terrain_nx-1) )
          {
            int jj = j + jp[n];
            if ( (jj > 0) && (jj < terrain_ny-1) )
            {
              weight += frac;
              total += terrain[INDEX(ii, jj)];
            }
          }
        }
        terrain[INDEX(i, j)] /= weight;
      }
    }
  }
}


//! dumps an array of floats
bool dump_array(const char * file_name,
                const float * array,
                int terrain_nx, int terrain_ny)
{
  int i, j;

  FILE * file = fopen(file_name, "w");
  if (!file)
  {
    TRACE("Unable to open %s for writing\n", file_name);
    return false;
  }
  
  int nx = terrain_nx;
  int ny = terrain_ny;
  
#if defined(__ppc__)
  {
    int swnx = SwapInt32( nx );
    int swny = SwapInt32( ny );
    fwrite(&swnx, sizeof(swnx), 1, file);
    fwrite(&swny, sizeof(swny), 1, file);
  }
#else
  fwrite(&nx, sizeof(nx), 1, file);
  fwrite(&ny, sizeof(ny), 1, file);
#endif

  // write out coords
#if defined(__ppc__)
#define LITTLEFLOAT(x) SwapFloat32((float)(x))
#else
#define LITTLEFLOAT(x) (float)(x)
#endif

  for (i = 0 ; i < nx ; ++i)
  {
    float val = LITTLEFLOAT(i);
    fwrite(&val, sizeof(val), 1, file);
  }
  for (i = 0 ; i < ny ; ++i)
  {
    float val = LITTLEFLOAT(i);
    fwrite(&val, sizeof(val), 1, file);
  }
  
  // write out the array (magnitude)

  for (j = 0 ; j < ny ; ++j)
  {
    for (i = 0 ; i < nx ; ++i)
    {
      float val = LITTLEFLOAT(array[INDEX(i, j)]);
      fwrite(&val, sizeof(val), 1, file);
    }
  }
  
  fclose(file);
  return true;

}

//! Calculates wind field by filtering the terrain
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
                                int & wind_nz)
{
  TRACE("Calculating wind from terrain...\n");
  
  const int nx = ground_vertex_array->get_nx();
  const int ny = nx;
  
  // calculate the wind field (hack alert!)
  
  const int skip = 2;

  wind_nx = 1 + (nx-1)/skip;
  wind_ny = 1 + (ny-1)/skip;
  wind_nz = 50;
  
  float dx = ground_vertex_array->get_pos(1, 0)[0] - 
    ground_vertex_array->get_pos(0, 0)[0];
  
  float peak = 200;
  
  float wind_dz = 2 * peak/wind_nz;
  
  wind_x = new float[wind_nx];
  wind_y = new float[wind_ny];
  wind_z = new float[wind_nz];
  
  int i, j, k;
  
  j = 0;
  for (i = 0 ; i < wind_nx ; ++i)
  {
    wind_x[i] = ground_vertex_array->get_pos(i*skip, j*skip)[0];
  }
  i = 0;
  for (j = 0 ; j < wind_ny ; ++j)
  {
    wind_y[j] = ground_vertex_array->get_pos(i*skip, j*skip)[1];
  }
  wind_z[0] = 0;
  TRACE("wind zs: ");
  for (k = 1 ; k < wind_nz ; ++k)
  {
    wind_z[k] = wind_z[k-1] + wind_dz;
    TRACE("%f ", wind_z[k]);
//    wind_dz *= 1.2f;
  }
  TRACE("\n");
  
  wind_u = new float[wind_nx * wind_ny * wind_nz];
  wind_v = new float[wind_nx * wind_ny * wind_nz];
  wind_w = new float[wind_nx * wind_ny * wind_nz];
  
  float basic_u = 5; // west-east wind of 5 m/s
  
  // create a terrain array that we can modify
  const int terrain_nx = ground_vertex_array->get_nx();
  const int terrain_ny = terrain_nx;
  

  float * terrain = new float[terrain_nx * terrain_ny];

  for (j = 0 ; j < terrain_nx ; ++j)
  {
    for (i = 0 ; i < terrain_nx ; ++i)
    {
      terrain[INDEX(i, j)] = ground_vertex_array->get_pos(i, j)[2];
    }
  }

//  dump_array("orig.dat", terrain, terrain_nx, terrain_ny);

  // Start the loops
  for (k = 0 ; k < wind_nz ; ++k)
  {
    const float height_scale_top = boundary_layer_depth;
    const float height_scale_bot = 0.4f; // effective height of ground (for scaling)

    float f = 1 - exp(-(height_scale_bot + wind_z[k])/height_scale_top);
    float basic_u1 = basic_u * f;
    for (j = 0 ; j < wind_ny ; ++j)
    {
      for (i = 0 ; i < wind_nx ; ++i)
      {
        wind_v[environment->calc_index(i, j, k)] = 0;
        if ((i == 0) || (i==wind_nx-1))
        {
          wind_u[environment->calc_index(i, j, k)] = basic_u1;
          wind_w[environment->calc_index(i, j, k)] = 0;
        }
        else
        {
          // calculate height gradient
          float z1 = terrain[INDEX((i*skip)-1, j*skip)];
          float z2 = terrain[INDEX((i*skip)+1, j*skip)];
          float slope = (z2-z1)/(2*dx);
          wind_u[environment->calc_index(i, j, k)] = 
            basic_u1 / sqrt(1+slope*slope);
          wind_w[environment->calc_index(i, j, k)] = 
            wind_u[environment->calc_index(i, j, k)] * slope;
        }
      }
    }
    filter_terrain(terrain, terrain_nx, terrain_ny, 0.005f, 5); // weight, and iter
  }

  delete [] terrain;

//    dump_array("after.dat", terrain, terrain_nx, terrain_ny);

//    dump_transect(wind_nx, wind_ny, wind_nz,
//                  wind_x, wind_z,
//                  wind_u, wind_w,
//                  environment);
  TRACE("... Done calculating wind from terrain\n");

}


//! Calculates wind field by filtering the terrain and "simulating"
//! separation regions downwind of peaks.
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
  int & wind_nz)
{
  TRACE("Calculating filtered separated wind from terrain...\n");
  
  const int nx = ground_vertex_array->get_nx();
  const int ny = nx;
  
  // calculate the wind field (hack alert!)
  
  const int skip = 1;

  wind_nx = 1 + (nx-1)/skip;
  wind_ny = 1 + (ny-1)/skip;
  wind_nz = 50;
  
  float dx = ground_vertex_array->get_pos(1, 0)[0] - 
    ground_vertex_array->get_pos(0, 0)[0];
  
  float peak = 200;
  
  float wind_dz = 0.02f * 2 * peak/wind_nz;
  
  wind_x = new float[wind_nx];
  wind_y = new float[wind_ny];
  wind_z = new float[wind_nz];
  
  int i, j, k;
  
  j = 0;
  for (i = 0 ; i < wind_nx ; ++i)
  {
    wind_x[i] = ground_vertex_array->get_pos(i*skip, j*skip)[0];
  }
  float wind_dx = wind_x[1] - wind_x[0];
  i = 0;
  for (j = 0 ; j < wind_ny ; ++j)
  {
    wind_y[j] = ground_vertex_array->get_pos(i*skip, j*skip)[1];
  }

  wind_z[0] = 0;
  TRACE("wind zs: ");
  for (k = 1 ; k < wind_nz ; ++k)
  {
    wind_z[k] = wind_z[k-1] + wind_dz;
    TRACE("%f ", wind_z[k]);
    wind_dz *= 1.2f;
  }
  TRACE("\n");
  
  wind_u = new float[wind_nx * wind_ny * wind_nz];
  wind_v = new float[wind_nx * wind_ny * wind_nz];
  wind_w = new float[wind_nx * wind_ny * wind_nz];
  
  float basic_u = 5; // west-east wind of 5 m/s
  
  // create a terrain array that we can modify
  const int terrain_nx = ground_vertex_array->get_nx();
  const int terrain_ny = terrain_nx;
  

  float * terrain = new float[terrain_nx * terrain_ny];
  float * adjusted_terrain = new float[terrain_nx * terrain_ny];

  // rate at which the separation layer decreases with
  // distance. Normally positive!
  float slope_offset = separation_slope * wind_dx;
  
  for (j = 0 ; j < terrain_ny ; ++j)
  {
    for (i = 0 ; i < terrain_nx ; ++i)
    {
      terrain[INDEX(i, j)] = ground_vertex_array->get_pos(i, j)[2];
      adjusted_terrain[INDEX(i, j)] = terrain[INDEX(i, j)];
      if ( ( i != 0) &&
           (adjusted_terrain[INDEX(i, j)] < 
            (adjusted_terrain[INDEX(i-1, j)]) - fabs(slope_offset)) )
      {
        adjusted_terrain[INDEX(i, j)] = 
          adjusted_terrain[INDEX(i-1, j)] - slope_offset;
        if (adjusted_terrain[INDEX(i, j)] < terrain[INDEX(i, j)])
        {
          adjusted_terrain[INDEX(i, j)] = terrain[INDEX(i, j)];
        }
      }
    }
  }

//  dump_array("orig.dat", terrain, terrain_nx, terrain_ny);

  // Start the loops
  for (k = 0 ; k < wind_nz ; ++k)
  {
    for (j = 0 ; j < wind_ny ; ++j)
    {
      for (i = 0 ; i < wind_nx ; ++i)
      {
        // account for the separation, plus the boundary layer
        float separation_depth = 
          adjusted_terrain[INDEX(i, j)] - terrain[INDEX(i, j)];
        
        wind_v[environment->calc_index(i, j, k)] = 0;
        if ((i == 0) || (i==wind_nx-1))
        {
          wind_u[environment->calc_index(i, j, k)] = basic_u;
          wind_w[environment->calc_index(i, j, k)] = 0;
        }
        else
        {
          // calculate height gradient
          float slope;
          if ( (separation_depth < 0.1f) || (wind_z[k] > separation_depth) )
          {
            float z1 = adjusted_terrain[INDEX((i*skip)-1, j*skip)];
            float z2 = adjusted_terrain[INDEX((i*skip)+1, j*skip)];
            slope = (z2-z1)/(2*dx);
          }
          else
          {
            float z1_adj = adjusted_terrain[INDEX((i*skip)-1, j*skip)];
            float z2_adj = adjusted_terrain[INDEX((i*skip)+1, j*skip)];
            float z1 = terrain[INDEX((i*skip)-1, j*skip)];
            float z2 = terrain[INDEX((i*skip)+1, j*skip)];
            float slope_adj = (z2_adj-z1_adj)/(2*dx);
            float slope_norm = (z2-z1)/(2*dx);
            float frac = wind_z[k]/separation_depth;
            slope = frac * slope_adj + (1 - frac) * slope_norm;
          }            
          wind_u[environment->calc_index(i, j, k)] = 
            basic_u / sqrt(1+slope*slope);
          wind_w[environment->calc_index(i, j, k)] = 
            wind_u[environment->calc_index(i, j, k)] * slope;

          // Now scale to take into account the wind reversal
          float scale; //the normalised wind
          scale = 0.5f * (1 + separation_wind_scale) +
            0.5f * (1 - separation_wind_scale) * 
            tanh((wind_z[k] - separation_depth)/boundary_layer_depth);

          // need to make sure we don't replicate the boundary layer - i.e. 
          // phase this in as the separation depth > boundary_layer_depth
          float frac = 1.0f;
          if (separation_depth < boundary_layer_depth)
          {
            frac = separation_depth/boundary_layer_depth;
          }
          scale = frac * scale + (1-frac) * 1.0f;

          wind_u[environment->calc_index(i, j, k)] *= scale;
          wind_w[environment->calc_index(i, j, k)] *= scale;
        }

        // finally scale according to ground proximity
        // have a small offset so that the scale != 0 at the ground.
        const float height_offset = 0.4f;
        float scale = tanh((wind_z[k] + height_offset)/
                           (boundary_layer_depth + height_offset));
        wind_u[environment->calc_index(i, j, k)] *= scale;
        wind_w[environment->calc_index(i, j, k)] *= scale;


//          if (j == 64)
//          {
//            TRACE("i = %d, j = %d, k = %d, sep = %f wind_z[k] = %f\n", 
//                   i, j, k, 
//                   separation_depth,
//                   wind_z[k]);
          
//            TRACE("     u = %f, w = %f\n", 
//                   wind_u[environment->calc_index(i, j, k)],
//                   wind_w[environment->calc_index(i, j, k)]);
//          }
      }
    }
//    filter_terrain(terrain, terrain_nx, terrain_ny, 0.005f, 5); // weight, and iter
  }

  delete [] terrain;
  delete [] adjusted_terrain;

//    dump_array("after.dat", terrain, terrain_nx, terrain_ny);

//    dump_transect(wind_nx, wind_ny, wind_nz,
//                  wind_x, wind_z,
//                  wind_u, wind_w,
//                  environment);
  TRACE("... Done calculating wind from terrain\n");

}




