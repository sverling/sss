#include "lod.h"
#include "sss_assert.h"
#include "log_trace.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>

template<class T>
inline T fmax(T a, T b) {return (a > b ? a : b);}
template<class T>
inline T fmax(T a, T b, T c) {return fmax(fmax(a,b),c);}
template<class T>
inline T fmax(T a, T b, T c, T d) {return fmax(fmax(a,b,c),d);}
template<class T>
inline T fmax(T a, T b, T c, T d, T e) {return fmax(fmax(a,b,c,d),e);}
template<class T>
inline T fmax(T a, T b, T c, T d, T e, T f) {return fmax(fmax(a,b,c,d,e),f);}

#define LOD_M (-7)

inline bool Lod::on_sea_shore(int i, int j) const
{
  bool orig = vertex_in[calc_index(i,j)].z > alt_zero;
  
  static const int i_offsets[] = {0, 1, 1, 1, 0, -1, -1, -1};
  static const int j_offsets[] = {1, 1, 0, -1, -1, -1, 0, 1};
  
  for (unsigned int c = 0 ; c < sizeof(i_offsets) / sizeof(i_offsets[0]) ; ++c)
  {
    if (is_valid(i+i_offsets[c],j+j_offsets[c])) 
    {
      bool this_one = (vertex_in[calc_index(i+i_offsets[c],
                                            j+j_offsets[c])].z > alt_zero);
      if (this_one != orig)
      {
        return true;
      }
    }
  }
  
  return false;
}

inline float Lod::calc_r(int i0, int j0, int i1, int j1) const
{
  if ( is_valid(i0, j0) && is_valid(i1, j1) )
  {
    const Vertex & v1 = vertex_in[calc_index(i0, j0)];
    const Vertex & v2 = vertex_in[calc_index(i1, j1)];
    return (float) sqrt( (v1.x-v2.x) * (v1.x-v2.x) +
                         (v1.y-v2.y) * (v1.y-v2.y) +
                         (v1.z-v2.z) * (v1.z-v2.z) );
  }
  else
  {
    return 0;
  }
}
  
inline float Lod::calc_r(Delta_r dr0, Delta_r dr1, Delta_r dr2, Delta_r dr3, 
                         int i, int j,
                         int dist, Colour colour) const
{
  int i1, j1, i2, j2, i3, j3, i4, j4;
  if (colour == LOD_WHITE)
  {
    i1 = i;    i2 = i+dist;    i3 = i;    i4 = i-dist;
    j1 = j+dist;    j2 = j;    j3 = j-dist;    j4 = j;
  }
  else
  {
    i1 = i-dist;    i2 = i+dist;    i3 = i+dist;    i4 = i-dist;
    j1 = j-dist;    j2 = j-dist;    j3 = j+dist;    j4 = j+dist;
  }

  return fmax(calc_r(i, j, i1, j1) + dr0.r,
              calc_r(i, j, i2, j2) + dr1.r,
              calc_r(i, j, i3, j3) + dr2.r,
              calc_r(i, j, i4, j4) + dr3.r);
}


inline float Lod::calc_d(int i, int j, int dist, Colour colour) const
{
  int i1, j1, i2, j2, i3, j3, i4, j4;

  if (colour == LOD_WHITE)
  {
    i1 = i;    i2 = i+dist;    i3 = i;    i4 = i-dist;
    j1 = j+dist;    j2 = j;    j3 = j-dist;    j4 = j;
  }
  else
  {
    i1 = i-dist;    i2 = i+dist;    i3 = i+dist;    i4 = i-dist;
    j1 = j-dist;    j2 = j-dist;    j3 = j+dist;    j4 = j+dist;
  }
    
  float mean_z = 0;
  int count = 0;
  
  if (is_valid(i1,j1))
  {
    mean_z += vertex_in[calc_index(i1, j1)].z;
    ++count;
  }
  if (is_valid(i2,j2))
  {
    mean_z += vertex_in[calc_index(i2, j2)].z;
    ++count;
  }
  if (is_valid(i3,j3))
  {
    mean_z += vertex_in[calc_index(i3, j3)].z;
    ++count;
  }
  if (is_valid(i4,j4))
  {
    mean_z += vertex_in[calc_index(i4, j4)].z;
    ++count;
  }
  
  assert1(count);
  
  mean_z = mean_z/count;

  float d_offset = 0.0f;
  if (coast_enhancement > 0.001f)
  {
//      float z = vertex_in[calc_index(i,j)].z - alt_zero;
//      z_scale = (float) (1/(1.01f-exp(-z*z/(scale_z*scale_z))));
    if (on_sea_shore(i, j))
    {
      d_offset = coast_enhancement;
    }
  }
  
  return (float) (d_offset + fabs(mean_z - vertex_in[calc_index(i,j)].z));
}
  
inline float Lod::calc_d(Delta_r dr0, Delta_r dr1, Delta_r dr2, Delta_r dr3, 
                         int i, int j,
                         int dist, Colour colour) const
{
  return fmax(dr0.d, dr1.d, dr2.d, dr3.d,
              calc_d(i, j, dist, colour));
}

Lod::Lod(Vertex * vertex_in, const int level, const int nx, 
         float coast_enhancement, float alt_zero)
  :
  vertex_in(vertex_in),
  vertex_out(0),
  level(level), nx(nx),
  vertex_out_size(2*nx*nx), // 2 for some leeway
  max_vertex_out_index(0),
  coast_enhancement(coast_enhancement),
  alt_zero(alt_zero)
{
  assert1(nx == (int) pow(2.0, level/2.0)+1);
  
  vertex_out = new Vertex[vertex_out_size]; // ctor zeros
  
  TRACE("level = %d, nx = %d\n", level, nx);
  
  // start things off with some "well-known" points
  int i_c, i_sw, i_s, i_se, i_e, i_ne, i_n, i_nw, i_w;
  
  i_sw = 0;
  i_se = 1;
  i_ne = 2;
  i_nw = 3;
  
  i_c = 4;
  
  i_w = 5;
  i_s = 6;
  i_e = 7;
  i_n = 8;
  
  // start things off
  
  int i_mid = (nx-1)/2;
  
  int index_sw = calc_index( 0,    0    );
  int index_se = calc_index( nx-1, 0    );
  int index_ne = calc_index( nx-1, nx-1 );
  int index_nw = calc_index( 0,    nx-1 );
  
  int index_c  = calc_index( i_mid,i_mid);
  
  int index_w = calc_index(0,     i_mid);
  int index_s = calc_index(i_mid, 0    );
  int index_e = calc_index(nx-1,  i_mid);
  int index_n = calc_index(i_mid, nx-1 );
  
  // Calculate delta and r

  initialise_d_r();
  initialise_d_r();
  initialise_d_r();
  initialise_d_r();
  
  // Re-order things, writing the result in vertex_out.
  // descend the white tree, starting from the center
  int dist = i_mid/2;

  TRACE_FILE_IF(2)
    TRACE("White tree\n");
  descend_quad_tree(i_mid, i_mid, i_c, dist, LOD_WHITE);
  add_to_output(i_c, vertex_in[index_c]);

  // descend the black trees, starting from west
  TRACE_FILE_IF(2)
    TRACE("Black w tree\n");
  descend_quad_tree(0,    i_mid,i_w, dist, LOD_BLACK);
  add_to_output(i_w, vertex_in[index_w]);
  
  TRACE_FILE_IF(2)
    TRACE("Black s tree\n");
  descend_quad_tree(i_mid,0,    i_s, dist, LOD_BLACK);
  add_to_output(i_s, vertex_in[index_s]);
  
  TRACE_FILE_IF(2)
    TRACE("Black e tree\n");
  descend_quad_tree(nx-1, i_mid,i_e, dist, LOD_BLACK);
  add_to_output(i_e, vertex_in[index_e]);
  
  TRACE_FILE_IF(2)
    TRACE("Black n tree\n");
  descend_quad_tree(i_mid,nx-1, i_n, dist, LOD_BLACK);
  add_to_output(i_n, vertex_in[index_n]);

// final bits  
  vertex_in[index_sw].r = 0;
  vertex_in[index_sw].d = 0;
  vertex_in[index_se].r = 0;
  vertex_in[index_se].d = 0;
  vertex_in[index_ne].r = 0;
  vertex_in[index_ne].d = 0;
  vertex_in[index_nw].r = 0;
  vertex_in[index_nw].d = 0;
  

  add_to_output(i_sw, vertex_in[index_sw]);
  add_to_output(i_se, vertex_in[index_se]);
  add_to_output(i_ne, vertex_in[index_ne]);
  add_to_output(i_nw, vertex_in[index_nw]);

  assert1(max_vertex_out_index < vertex_out_size);
  
  // now zap the r = -1 (improve compression)
  for (int i = 0 ; i <= max_vertex_out_index; ++i)
  {
    if (vertex_out[i].r < 0)
      vertex_out[i].r = 0;
  }
  
}

void Lod::initialise_d_r()
{
  int i_mid = (nx-1)/2;
  
//    int index_sw = calc_index( 0,    0    );
//    int index_se = calc_index( nx-1, 0    );
//    int index_ne = calc_index( nx-1, nx-1 );
//    int index_nw = calc_index( 0,    nx-1 );
  
//    int index_c  = calc_index( i_mid,i_mid);

  do_triangle(i_mid, i_mid, 
              0, 0,
              nx-1, 0, level-1);
  do_triangle(i_mid, i_mid, 
              nx-1, 0,
              nx-1, nx-1, level-1);
  do_triangle(i_mid, i_mid, 
              nx-1, nx-1,
              0, nx-1, level-1);
  do_triangle(i_mid, i_mid, 
              0, nx-1,
              0, 0, level-1);
}

void Lod::do_triangle(int i1, int j1,
                      int i2, int j2,
                      int i3, int j3,
                      int lev)
{
// we are interested in calculating r and delta at v1. 
  
  Vertex & v1 = vertex_in[calc_index(i1, j1)];
  Vertex & v2 = vertex_in[calc_index(i2, j2)];
  Vertex & v3 = vertex_in[calc_index(i3, j3)];
  
  if (lev > 0)
  {
    // go down one lev..., then calculate the value at this point
    int i4 = (i2 + i3) / 2;
    int j4 = (j2 + j3) / 2;
    Vertex & v4 = vertex_in[calc_index(i4, j4)];
    
    // call this fn recursively
    do_triangle(i4, j4, i1, j1, i2, j2, lev-1);
    do_triangle(i4, j4, i3, j3, i1, j1, lev-1);

    // calculate the morphed z value of v4
    v4.z_m = (v2.z + v3.z)*0.5F;

    // having done that, as we are not a leaf node, calculate r and
    // delta based on the current location, and (for delta) compare
    // with the values from the children. Accept the highest. 
    
    // calculate delta
    float d;
    
    // i1, j1 are in the middle
    // construct i2', j2', which are the opposites of i2, j2 etc
    
    int i2p = i1 - (i2-i1);
    int j2p = j1 - (j2-j1);
    float z2p;
    if ( (i2p >= 0) && (i2p <= nx-1) && (j2p >= 0) && (j2p <= nx-1) )
      z2p = vertex_in[calc_index(i2p, j2p)].z;
    else
      z2p = v1.z - (v2.z - v1.z);
    
    int i3p = i1 - (i3-i1);
    int j3p = j1 - (j3-j1);
    float z3p;
    if ( (i3p >= 0) && (i3p <= nx-1) && (j3p >= 0) && (j3p <= nx-1) )
      z3p = vertex_in[calc_index(i3p, j3p)].z;
    else
      z3p = v1.z - (v3.z - v1.z);
    
    d = v1.z - (v2.z + v3.z + z2p + z3p) * 0.25f;
    
    v1.d = fmax(v1.d, (float) fabs(d), v4.d); // current, new, child
    
    // and set r
    v1.r = fmax(v1.r, (float) sqrt((v4.x-v1.x) * (v4.x-v1.x) +
                                   (v4.y-v1.y) * (v4.y-v1.y) +
                                   (v4.z-v1.z) * (v4.z-v1.z)) + v4.r);
  }
  else
  {
    // got here, then we must be a leaf node. Calculate z from
    // adjacent values
    float d;
    if ( (i1 == 0) || (i1 == nx-1) )
      d = v1.z - (vertex_in[calc_index(i1, j1+1)].z + 
                  vertex_in[calc_index(i1, j1-1)].z) * 0.5f;
    else if ( (j1 == 0) || (j1 == nx-1) )
      d = v1.z - (vertex_in[calc_index(i1+1, j1)].z + 
                  vertex_in[calc_index(i1-1, j1)].z) * 0.5f;
    else 
    {
      d = v1.z - (vertex_in[calc_index(i1, j1+1)].z + 
                  vertex_in[calc_index(i1, j1-1)].z +
                  vertex_in[calc_index(i1+1, j1)].z + 
                  vertex_in[calc_index(i1-1, j1)].z   ) * 0.25f;
    }

    float d_offset = 0.0f;
    if (coast_enhancement > 0.001f)
    {
//        float z = vertex_in[calc_index(i1,j1)].z - alt_zero;
//        z_scale = (float) (1/(1.01f-exp(-z*z/(scale_z*scale_z))));
      if (on_sea_shore(i1, j1))
      {
        d_offset = coast_enhancement;
      }
    }
    // just set delta = d
    v1.d = d_offset + (float) fabs(d) ;
    // and r = 0;
    v1.r = 0;
  }
}

void Lod::descend_graph(int i, int j, int dist,
                        Colour colour,
                        Delta_r & dr0, Delta_r & dr1,
                        Delta_r & dr2, Delta_r & dr3 )
{
  int i0, j0;

  // note - opposite to in the quadtree!
  const int i_offset_black[] = {-1, 1, 1, -1};
  const int j_offset_black[] = {-1, -1, 1, 1};
  const int i_offset_white[] = {0, 1, 0, -1};
  const int j_offset_white[] = {1, 0, -1, 0};
  
  const int * i_offset;
  const int * j_offset;
  
  if (colour == LOD_WHITE)
  {
    i_offset = i_offset_white;
    j_offset = j_offset_white;
  }
  else
  {
    i_offset = i_offset_black;
    j_offset = j_offset_black;
  }
  
  Delta_r d_r[4];

  int index;
  for (index = 0 ; index < 4 ; ++index)
  {
    i0 = i + i_offset[index] * dist;
    j0 = j + j_offset[index] * dist;
  
    if (is_valid(i0, j0))
    {
      int new_dist = dist;
      Colour new_colour = (colour == LOD_WHITE ? LOD_BLACK : LOD_WHITE);
      
      if (colour == LOD_WHITE)
        new_dist = dist/2;
      
      Delta_r this_d_r;

      if (new_dist > 0)
      {
        // non-leaf - descend further
        if (vertex_in[calc_index(i0, j0)].r < 0)
        {
          // not done this one yet
          Delta_r d_r0, d_r1, d_r2, d_r3;
          descend_graph(i0, j0, new_dist,
                        new_colour,
                        d_r0, d_r1, d_r2, d_r3);
          
          this_d_r = Delta_r(
            calc_d(d_r0, d_r1, d_r2, d_r3, i0, j0, new_dist, new_colour),
            calc_r(d_r0, d_r1, d_r2, d_r3, i0, j0, new_dist, new_colour) );
        }
        else
        {
          // already done this one
          this_d_r = Delta_r(vertex_in[calc_index(i0, j0)].d, 
                             vertex_in[calc_index(i0, j0)].r);
        }
      }
      else
      {
        // leaf
        this_d_r = Delta_r(
          calc_d(i0, j0, dist, LOD_WHITE),
          0);
//            static int count = 0;
//            TRACE("%d, (%d, %d)\n", count++, i0, j0);
      }
      vertex_in[calc_index(i0, j0)].d = this_d_r.d;
      vertex_in[calc_index(i0, j0)].r = this_d_r.r;
      d_r[index] = this_d_r;
    }
    else
    {
      d_r[index] = Delta_r(0, 0);
    }
  }
  
  dr0 = d_r[0];
  dr1 = d_r[1];
  dr2 = d_r[2];
  dr3 = d_r[3];
}


void Lod::add_to_output(int i, const Vertex & vertex)
{
  if (i >= vertex_out_size)
  {
    TRACE("Error - index %d > %d\n", i, vertex_out_size);
    exit(-1);
  }
  else if (i < 0)
  {
    TRACE("Error - index %d < 0\n", i);
    exit(-1);
  }
  
  max_vertex_out_index = fmax(max_vertex_out_index, i);
  if (vertex_out[i].r > -0.5f)
  {
    TRACE("Error - entry for index %d already exists (%f, %f)\n", 
          i, 
          vertex_out[i].x,
          vertex_out[i].y );
    exit(-1);
  }

  int ind = vertex_out[i].index;
  vertex_out[i] = Vertex(vertex);
  vertex_out[i].index = ind;
  vertex_out[(int) (&vertex - &vertex_in[0])].index = i;
}

void Lod::get_output(Vertex *& _vertex_out, int & size) const
{
  assert1(vertex_out);
  _vertex_out = vertex_out;
  size = max_vertex_out_index+1;
}


void Lod::display_output() const
{
  int i;
  int count = 0;
  TRACE("######## max index = %d #########\n", max_vertex_out_index);
  for (i = 0 ; i <= max_vertex_out_index; ++i)
  {
//    if (vertex_out[i].r >= 0)
    {
      ++count;
      TRACE("%5d, %5.1f, %5.1f, %5.1f, (delta = %5.2f, r = %5.2f), index = %d\n", 
            i, 
            vertex_out[i].x, vertex_out[i].y, vertex_out[i].z,
            vertex_out[i].d, vertex_out[i].r,
            vertex_out[i].index);
    }
  }
  TRACE("#### storage efficiency = %d%% #####\n", 100*count/max_vertex_out_index);
}

void Lod::descend_quad_tree(int i, int j, int pq, int dist, Colour colour)
{
  const int i_offset_white[] = {-1, 1, 1, -1};
  const int j_offset_white[] = {-1, -1, 1, 1};
  const int i_offset_black[] = {0, 1, 0, -1};
  const int j_offset_black[] = {1, 0, -1, 0};
  
  const int * i_offset;
  const int * j_offset;
  
  if (colour == LOD_WHITE)
  {
    i_offset = i_offset_white;
    j_offset = j_offset_white;
  }
  else
  {
    i_offset = i_offset_black;
    j_offset = j_offset_black;
  }
  
  int index;
  for (index = 0 ; index < 4 ; ++index)
  {
    int c0=4*pq + index + LOD_M;
    int i0 = i + i_offset[index] * dist;
    int j0 = j + j_offset[index] * dist;
    if (is_valid(i0, j0))
    {
      if (dist/2 > 0)
      {
        descend_quad_tree(i0, j0, c0, dist/2, colour);
      }
      add_to_output(c0, vertex_in[calc_index(i0, j0)]);
    }
  }
}




