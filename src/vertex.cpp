/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file vertex.cpp
*/
#include "vertex.h"
#include "plane.h"
//! \todo remove reference to this "application" file
#include "object.h"

#include "sss_glut.h"
#include <stdio.h>
#include <math.h>

template<class T>
inline T max2(T t1, T t2) { return t1 > t2 ? t1: t2; }

template<class T>
inline T max3(T t1, T t2, T t3) { return max2(t1, max2(t2, t3)); }

Vertex_array::Vertex_array(int nx, int degree, const Vertex * vertex)
  :
  degree(degree), nx(nx), vertex(vertex),
  clip(false),
  vertex_count(0),
  xmid(c().x), ymid(c().x)
{
}

/*! 
  
plot_z is set to the z-value that needs to be plotted (i.e. for
morphing) - only relevant if the return value is true.  clip_mask is
0 if the point is (potentially) in the region of that plane, or 1 if
it is definitely not (i.e. the point is definitely in the view
frustrum according to that plane).

*/
inline bool Vertex_array::active(int v, 
                                 float & plot_z, 
                                 unsigned & clip_mask) const
{
  const Vertex & cur_vertex = vertex[v];
  
  if (clip)
  {
    // effectively move the clipping planes out by dx, otherwise we
    // get popping at the screen edge.
    static const float dx = sqrt(3.0f) * (get_pos(1,0)[0] - get_pos(0,0)[0]);
    
    // pre-calculate some stuff
// the following is very "conservative"
    const float r0 = -(dx + fabs(cur_vertex.z_m - cur_vertex.z) + 
                       cur_vertex.r);
    
    // if any of the planes indicate that the point is well outside,
    // we can return.  Note that we can make a small optimisation by
    // _not_ clipping against the near plane. This is because the
    // top/bottom/side planes meet anyway, so they're sufficient to
    // exclude regions behind the viewpoint. Similarly, in practice
    // none of the terrains I've generated are so big that they go
    // beyond the far clipping plane, so that can be excluded (should
    // make this configurable). Full clipping would use 6 in the loop
    // below.
    for (unsigned int i = 4 ; i-- != 0 ; )
    {
      if ( !(clip_mask & (1u<<i)) )
      {
        float dist = plane[i].distance_to_point(
          cur_vertex.x, cur_vertex.y, cur_vertex.z);
        if (dist < r0)
        {
          // point is on outside of plane
          return false;
        }
        else if (dist > cur_vertex.r)
        {
          // point is on inside of plane - don't need to check it in
          // future (i.e. lower in the tree)
          clip_mask |= (1u<<i);
        }
        // otherwise point "straddles the plane" - need to check it in
        // future, so don't modify clip_mask
      } // clip_mask
    }
  }   
  
  // now check for activity according to the LOD algorithm
  const float a = (lod * cur_vertex.d + cur_vertex.r);
  const float b = (eye_x-cur_vertex.x);
  const float c = (eye_y-cur_vertex.y);
  const float d = (eye_z-cur_vertex.z);
  float metric = a*a / ( b*b + c*c + d*d);
  const float met_min = 1.0f; // always 1
  
  if (metric < met_min)
  {
//    plot_z = -100; // always ignored
    return false;
  }

  const float met_max = 1.5f;
  if (metric > met_max)
  {
    plot_z = cur_vertex.z;
    return true;
  }
  
//   static const float met_scale = 1.0f/(met_max-met_min);
  // define it by hand in case the compiler can't work it out from
  // above...
  const float met_scale = 2.0f;
  // morph
  metric = (metric - met_min) * met_scale;
  plot_z = metric * cur_vertex.z + (1.0f-metric) * cur_vertex.z_m;
  return true;
}

static inline void set_colour(float x, float y, float z)
{
  glColor3f(0.4f + 0.1f * sin(x/50),
            0.4f + 0.15f * sin(y/50),
            0.3f + 0.1f * sin(z/20));
}


inline void Vertex_array::append_vertex(int v, float plot_z)
{
  vn1 = vn0; // move stuff along
  vn0 = v;
  plot_z1 = plot_z0;
  plot_z0 = plot_z;
  
  if (do_colour) 
    set_colour(vertex[v].x, vertex[v].y, plot_z);
  
  if (do_save)
    saved_points.push_back(Pos(vertex[v].x, vertex[v].y, plot_z));
  
  ++vertex_count;
  
  glVertex3f( vertex[v].x,
              vertex[v].y,
              plot_z );
}

inline void Vertex_array::tstrip_append(int v, int p, float plot_z)
{
  if ( (v != vn1) && (v != vn0) )
  {
    if (p != parity)
      parity = p;
    else
      append_vertex(vn1, plot_z1);
    
    // append new vertex
    append_vertex(v, plot_z);
  }
}

// note - we move the test up one level, as suggested in the original paper
void Vertex_array::submesh_refine(int i, int j, int level, 
                                  float plot_z_i, 
                                  unsigned clip_mask)
{
  float plot_z_j;
  bool doit = ( (level-1 > 0) && active(j, plot_z_j, clip_mask) );
  
  if (doit)
  {
    int t0 = 4*i + M;
    int tl = (2*i + j + M + 1) % 4;
    int tr = (tl+1) % 4;
    submesh_refine(j, t0+tl, level-1, plot_z_j, clip_mask);
    tstrip_append(i, level % 2, plot_z_i);
    submesh_refine(j, t0+tr, level-1, plot_z_j, clip_mask);
  }
  else
  {
    tstrip_append(i, level % 2, plot_z_i);
  }
  
//      submesh_refine(j, cl(i,j), level-1);
//      tstrip_append(i, level % 2);
//      submesh_refine(j, cr(i,j), level-1);
//  }
}

//! \todo take the statics here into the class (to avoid hypot call)
inline void Vertex_array::plot_beach_edge(int v, float depth)
{
//  const float range = 2000;
  static const float range1 = 1.2f * hypot(xmid-sw().x, ymid-sw().y);
  static const float range2 = range1 + depth*4;
  
  float scale1 = range1 / hypot(vertex[v].x-xmid, vertex[v].y-ymid);
  float scale2 = scale1 * range2 / range1;
  
  float x1 = xmid + (vertex[v].x-xmid)*scale1;
  float y1 = ymid + (vertex[v].y-ymid)*scale1;
  float x2 = xmid + (vertex[v].x-xmid)*scale2;
  float y2 = ymid + (vertex[v].y-ymid)*scale2;
  
  ++vertex_count;
  glVertex3f(x1, y1, 0);
  ++vertex_count;
  glVertex3f(x2, y2, -depth);
}

inline void Vertex_array::plot_plain_edge(int v)
{
//  const float range = 2000;
  static const float range = 1.2f * hypot(xmid-sw().x, ymid-sw().y);
  
  if (do_save)
    saved_plain_points.push_back(Pos(vertex[v].x, vertex[v].y, vertex[v].z));
  ++vertex_count;
  glVertex3fv(&vertex[v].x);
  
  float scale = range / hypot(vertex[v].x-xmid, vertex[v].y-ymid);
  
  float x = xmid + (vertex[v].x-xmid)*scale;
  float y = ymid + (vertex[v].y-ymid)*scale;
  
  // now "correct" for the eye not being at xmid, ymid
//    x += eye_x - xmid;
//    y += eye_y - ymid;
//    glVertex3f(x, y, 0);
  
  if (do_save)
    saved_plain_points.push_back(Pos(x, y, 0));
  
  ++vertex_count;
  glVertex2f(x, y);
}

void Vertex_array::plot_beach(float depth)
{
  //  now do "edge"
  
  glBegin(GL_QUAD_STRIP);
  
  int i, j;
  int v;
  float plot_z; // ignored
  unsigned clip_mask;
  
  j = nx-1;
  plot_beach_edge(vertex[Lod::calc_index(0,j, nx)].index, depth);
  for (i = 0 ; i < nx ; i++)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_beach_edge(v, depth);
  }
  
  i = nx-1;
  plot_beach_edge(vertex[Lod::calc_index(i,nx-1, nx)].index, depth);
  for (j = nx-1 ; j >= 0 ; j--)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_beach_edge(v, depth);
  }
  
  j = 0;
  plot_beach_edge(vertex[Lod::calc_index(nx-1,j, nx)].index, depth);
  for (i = nx-1 ; i >= 0 ; i--)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_beach_edge(v, depth);
  }
  
  i = 0;
  plot_beach_edge(vertex[Lod::calc_index(i,0, nx)].index, depth);
  for (j = 0 ; j < nx ; j++)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_beach_edge(v, depth);
  }
  
//    // complete things...
  j = nx-1;
  plot_beach_edge(vertex[Lod::calc_index(0,j, nx)].index, depth);
  for (i = 0 ; i < nx ; i++)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
    {
      plot_beach_edge(v, depth);
      break;
    }
  }
  
  glEnd();
}

void Vertex_array::plot_plain(bool save)
{
  //  now do "edge"
  do_save = save; 
  glBegin(GL_QUAD_STRIP);
  
  int i, j;
  int v;
  float plot_z;
  unsigned clip_mask;
  
  j = nx-1;
  plot_plain_edge(vertex[Lod::calc_index(0,j, nx)].index);
  for (i = 0 ; i < nx ; i++)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_plain_edge(v);
  }
  
  i = nx-1;
  plot_plain_edge(vertex[Lod::calc_index(i,nx-1, nx)].index);
  for (j = nx-1 ; j >= 0 ; j--)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_plain_edge(v);
  }
  
  j = 0;
  plot_plain_edge(vertex[Lod::calc_index(nx-1,j, nx)].index);
  for (i = nx-1 ; i >= 0 ; i--)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_plain_edge(v);
  }
  
  i = 0;
  plot_plain_edge(vertex[Lod::calc_index(i,0, nx)].index);
  for (j = 0 ; j < nx ; j++)
  {
    v = vertex[Lod::calc_index(i,j, nx)].index;
    clip_mask = 0u;
    if (active(v, plot_z, clip_mask))
      plot_plain_edge(v);
  }
  
//    // complete things...
  j = nx-1;
  plot_plain_edge(vertex[Lod::calc_index(0,j, nx)].index);
  
  glEnd();
}


void Vertex_array::mesh_refine(const Position & eye_pos, 
                               float _lod, 
                               bool _do_colour,
                               bool _do_save)
{
  eye_x = eye_pos[0];
  eye_y = eye_pos[1];
  eye_z = eye_pos[2];
  lod = _lod;
  do_colour = _do_colour;
  do_save = _do_save;
  
  glBegin(GL_TRIANGLE_STRIP);
  
  vertex_count = 0;
  
  parity = 0;
  
  append_vertex(0, vertex[0].z); // sw
  append_vertex(0, vertex[0].z); // sw
  unsigned clip_mask = 0u;
  submesh_refine(4, 6, degree, vertex[4].z, clip_mask);
  tstrip_append(1, 1, vertex[1].z);
  clip_mask = 0u;
  submesh_refine(4, 7, degree, vertex[4].z, clip_mask);
  tstrip_append(2, 1, vertex[2].z);
  clip_mask = 0u;
  submesh_refine(4, 8, degree, vertex[4].z, clip_mask);
  tstrip_append(3, 1, vertex[3].z);
  clip_mask = 0u;
  submesh_refine(4, 5, degree, vertex[4].z, clip_mask);
  append_vertex(0, vertex[0].z);
  
  glEnd();
  
  TRACE_FILE_IF(4)
    TRACE("triangles = %d\n", vertex_count - 2);
}

unsigned Vertex_array::get_triangles() const
{
  return vertex_count - 2;
}

void Vertex_array::set_clipping(const Object & eye, 
                                double fovy,
                                double aspect,
                                double Near,
                                double Far)
{
  clip = true;
  
  // calculate the clipping planes.
  // first convert the fovy and aspect into the "focal length"
  
  double fovx = aspect * fovy;
  double focalx = 1.0/tan_deg(fovx*0.5);
  double focaly = 1.0/tan_deg(fovy*0.5);
  
  // clipping planes in eye coords
  // Note that the Planes have their normals pointing into the view frustrum.
  // Taken (almost) from Lengyel p. 93
  double e1x = sqrt(focalx*focalx + 1.0);
  double e1y = sqrt(focaly*focaly + 1.0);
  Plane near0  ( 1,         0,           0,         -Near);
  Plane far0   (-1,         0,           0,          Far);
  Plane left0  ( 1/e1x,    -focalx/e1x,  0,          0);
  Plane right0 ( 1/e1x,     focalx/e1x,  0,          0);
  Plane bottom0( 1/e1y,     0,           focaly/e1y, 0);
  Plane top0   ( 1/e1y,     0,          -focaly/e1y, 0);
  
  
  // p88 describes how to transform these planes using a 3x3 rotation matrix M
  // and translation vector T.
  Vector T = eye.get_eye();
  
  // Note also that my coordinate system is... er... different
  Vector x((eye.get_eye_target() - T).normalise());
  Vector z((eye.get_eye_up()).normalise());
  Vector y(cross(z, x));
  Matrix M(x, y, z);
  
  Vector minus_M_inv_T = -(transpose(M) * T);
  
  Matrix4 F_inv_trans(
    M(0, 0),          M(0, 1),          M(0, 2),          0,
    M(1, 0),          M(1, 1),          M(1, 2),          0,
    M(2, 0),          M(2, 1),          M(2, 2),          0,
    minus_M_inv_T(0), minus_M_inv_T(1), minus_M_inv_T(2), 1 );
  
  plane[0] = F_inv_trans * bottom0;
  plane[1] = F_inv_trans * left0;
  plane[2] = F_inv_trans * right0;
  plane[3] = F_inv_trans * top0;
  plane[4] = F_inv_trans * far0;
  plane[5] = F_inv_trans * near0;
}

void Vertex_array::plot_saved() const
{
  unsigned num = saved_points.size();
  unsigned i;
  glBegin(GL_TRIANGLE_STRIP);
  for (i = 0 ; i < num ; ++i)
  {
    glVertex3fv(&saved_points[i].x);
  }
  glEnd();
  
  num = saved_plain_points.size();
  glBegin(GL_QUAD_STRIP);
  for (i = 0 ; i < num ; ++i)
  {
    glVertex3fv(&saved_plain_points[i].x);
  }
  glEnd();
}
void Vertex_array::clear_saved()
{
  saved_points.clear();
  saved_plain_points.clear();
}



