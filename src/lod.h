/*

  Interface for the facililty to preprocess a height field, the result
  being a 1D array of interleaved quad-trees (as "described" in
  Lindstrom + Pascucci).

  This header also defines the type of the output - so the final
  application will include this, so that it knows how to cast the data
  in the terrain file.

  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
 */

#ifndef LOD_H
#define LOD_H

#include <math.h>

//! An element of the Lod structure
struct Vertex
{
  //! z_m is the morphed z value
  float x, y, z, z_m;
  // the following are populated by Lod
  float d, r;
  // index contains the 1D index that you're looking for if you look
  // in calc_index(i, j, nx)
  int index; 

  Vertex(float x, float y, float z, float z_m = -100) : 
    x(x), y(y), z(z), z_m(z_m), d(0), r(-1), index(-1) {};
  Vertex() : x(0), y(0), z(0), z_m(-100), d(0), r(-1), index(-1) {};
};

enum Lod_file_type
{
  SINGLE,
  TILED
};

//! The Lod structure is stored in the file using this class
struct Lod_file
{
  enum {ID_LEN = 8};
  char identifier[ID_LEN]; // = "LODFILE"
  Lod_file_type type;
  int checksum; // used to see if this Lod_file came from a particular input
  int level, nx;
  int size;
  Vertex vertices[1];
};

//! This is used in the creation of the mesh - note that the
//! implementation is not needed when just reading the file.
class Lod
{
public:
  // User passes in a pointer to the start of an array of type
  // Vertex. The array is of size (nx*nx), where nx = 2^(level/2) +
  // 1.
  // note that the input array gets modified (d and r get populated)
  Lod(Vertex * vertex_in, int level, int nx, 
      float coast_enhancement = 0.0, float alt_zero = 0);
  
  //! dumps the output
  void display_output() const;

  //! obtains the output (normally call this after creating a Lod
  //! object).
  void get_output(Vertex *& vertex_out, int & size) const;
  
  // helper fn for user to calculate indices in the input array
  static int calc_index(int i, int j, int nx) { return (i + j * nx); }

private: 

  enum Colour { LOD_WHITE, LOD_BLACK };
  
  // Structure used by Lod
  struct Delta_r
  {
    float d, r;
    Delta_r(float d, float r) : d(d), r(r) {};
    Delta_r() : d(0), r(0) {};
  };

  void do_triangle(int i1, int j1,
                   int i2, int j2,
                   int i3, int j3,
                   int lev);
  
  void initialise_d_r();
  
  void descend_graph(int i, int j, int dist,
                     Colour colour,
                     Delta_r & dr0, Delta_r & dr1,
                     Delta_r & dr2, Delta_r & dr3 );

  void descend_quad_tree(int i, int j, int pq, int dist, Colour colour);

  void add_to_output(int i, 
                     const Vertex & vertex);
  int calc_index(int i, int j) const {return calc_index(i, j, nx);}
  bool is_valid(int i, int j) const
    { return ( (i >= 0) && (j >= 0) && (i < nx) && (j < nx) ); }

  inline float calc_r(int i0, int j0, int i1, int j1) const;
  
  inline float calc_r(Delta_r dr0, Delta_r dr1, Delta_r dr2, Delta_r dr3, 
                      int i, int j,
                      int dist, Colour colour) const;
  
  inline float calc_d(Delta_r dr0, Delta_r dr1, Delta_r dr2, Delta_r dr3, 
                      int i, int j,
                      int dist, Colour colour) const;
  
  inline float calc_d(int i, int j, int dist, Colour colour) const;

  inline bool on_sea_shore(int i, int j) const;

  Vertex * const vertex_in;
  Vertex * vertex_out;

  const int level, nx;

  const int vertex_out_size;
  int max_vertex_out_index;

  // use these to increase the resolution around a particular altitude
  float coast_enhancement;
  float alt_zero;
};

#endif
