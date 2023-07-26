/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef VERTEX_H
#define VERTEX_H

#include "types.h"
#include "lod.h"
#include "plane.h"
#include "sss_assert.h"

#include "sss_glut.h"
#include <vector>

class Object;

/// Deals with the terrain mesh. Support automatic per-vertex colouring
/// and saving the calculated vertices (for multi-pass rundering)
class Vertex_array
{
public:
  // ctor sets up x, y, z, delta, r
  Vertex_array(int nx, int degree, const Vertex * vertex);
  ~Vertex_array() {}
  
  inline Position get_pos(int i, int j) const; // i and j are "x" and "y"
  
  int get_nx() const {return nx;}
  
  // mesh refinement fns
  //! Top-level refinement function - no view-frustum clipping
  void mesh_refine(const Position & eye_pos,
                   float lod, 
                   bool do_colour = false,
                   bool save = false);
  
  //! Enables clipping, and sets parameters
  void set_clipping(const Object & eye,
                    double fovy,
                    double aspect,
                    double Near, // how I hate Win32
                    double Far);
  //! Just enables/disables clipping
  void set_clipping(bool doit) {clip = doit;}
  
  //! Plots the flat region of land
  void plot_plain(bool save = false);
  
  //! Plots the (steeply) sloping region of beach
  /*! depth indicates the depth to which the beach should descend */
  void plot_beach(float depth);
  
  /// plots the saved points
  void plot_saved() const;
  void clear_saved();

  /// returns the number of triangles in the last terrain render
  unsigned get_triangles() const;

private:
  
  const Vertex & sw() {return vertex[0];}
  const Vertex & se() {return vertex[1];}
  const Vertex & ne() {return vertex[2];}
  const Vertex & nw() {return vertex[3];}
  
  const Vertex & w() {return vertex[5];}
  const Vertex & s() {return vertex[6];}
  const Vertex & e() {return vertex[7];}
  const Vertex & n() {return vertex[8];}
  
  const Vertex & c() {return vertex[4];}
  
  inline void plot_plain_edge(int v);
  inline void plot_beach_edge(int v, float depth);
  
  void submesh_refine(int i, int j, int l, float plot_z_i, unsigned clip_mask);
  
  inline void tstrip_append(int v, int p, float plot_z);
  inline void append_vertex(int v, float plot_z);
  
  enum {M=-7};
// The following are calculated in place... slightly more efficiently
//inline int cl(int pq, int pg) {return(4*pq + ((2*pq + pg + M + 1) % 4) + M);}
//inline int cr(int pq, int pg) {return(4*pq + ((2*pq + pg + M + 2) % 4) + M);}
  
  inline bool active(int v, float & plot_z, unsigned & clip_mask) const;
  
  const int degree; // the degree
  const int nx;
  const Vertex * vertex;
  Vertex_array();
  // internal variables for the mesh refinement
  int vn0; // ptr to V(n-0)
  int vn1; // ptr to V(n-1)
  
  float plot_z0;
  float plot_z1;
  
  int parity;
  
  float eye_x, eye_y, eye_z;
  float lod;
  
  // clipping planes
  Plane plane[6];
  bool clip;

  //! colour in the terrain when drawing it?
  bool do_colour;

  /// Save the results as we go (like a display list)
  bool do_save;
  struct Pos
  {
    Pos(float x, float y, float z) : x(x), y(y), z(z) {}
    float x, y, z;
  };
  std::vector<Pos> saved_points;
  std::vector<Pos> saved_plain_points;

  unsigned vertex_count;
  
public:
  const float xmid, ymid;
};

//------------- Inline functions ------------------

inline Position Vertex_array::get_pos(int i, int j) const
{
  int index = vertex[Lod::calc_index(i, j, nx)].index;
//  assert1(index >=0); // never gone off... and this does get hit
  
  return Position(vertex[index].x, vertex[index].y, vertex[index].z);
}



#endif
