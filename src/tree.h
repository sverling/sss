#ifndef TREE_H
#define TREE_H

#include "object.h"
#include "types.h"

/// This defines an object that is "tree-like" - it could be a tree,
/// bush etc. Basically it will be represented by a number of planes
/// that have a fixed orientation.
class Tree : public Object
{
public:
  enum Orientation_type 
  {
    FIXED, // fixed orientation (initially random and vertical)
    VERTICAL_BILLBOARD,
    FULL_BILLBOARD
  };
  
  Tree(const Position & pos,
       float graphical_size,
       float structural_size,
       Orientation_type orientation_type);
  
  ~Tree();
  
  /// we get drawn by the collection
  void draw(Draw_type draw_type) {};
  
  /// updates the orientation if necessary, then calls basic_draw
  void prepare_for_draw_pos(const Position & eye_pos);
  void prepare_for_draw_dir(const Vector & eye_dir);

  bool use_physics() const {return false;}
  
  float get_structural_bounding_radius() const 
    {return m_structural_size2;}
  float get_graphical_bounding_radius() const 
    {return m_graphical_size2;}

  Mass_type get_mass(float & mass) const {mass = 0.0; return MASS_SOLID;}
private:
  Tree(const Tree & orig);

  float m_graphical_size2;
  float m_structural_size2;
  Orientation_type m_orientation_type;
};


#endif
