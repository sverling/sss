#ifndef TREE_COLLECTION_H
#define TREE_COLLECTION_H

#include "sss_assert.h"
#include "object.h"
#include "tree.h"

#include <map>
#include <string>
#include <vector>

#include "sss_glut.h"

class Rgba_file_texture;
class Tree;

// owns all the trees, and also the textures for them (so as not to
// have lots of identical textures). We draw them all, so as not to
// result in lots of state changes.
class Tree_collection : public Object
{
public:
  static Tree_collection * create_instance(const std::string & tree_config_file);
  static Tree_collection * instance() {assert1(s_instance); return s_instance;}
  
  void draw(Draw_type draw_type);

  bool use_physics() const {return false;}

  float get_graphical_bounding_radius() const 
    {return 0;}
  float get_structural_bounding_radius() const 
    {return 0;}

private:
  Tree_collection(const std::string & tree_config_file);

  static Tree_collection * s_instance;

  typedef std::map<Tree *, Rgba_file_texture *> Texture_maps;
  Texture_maps m_texture_maps;

  typedef std::vector<Tree *> Trees;
  struct Tree_config
  {
    int orig_num;
    enum Pos_type {RANDOM_RANGE, RANDOM_POS, FIXED_POS};
    Pos_type pos_type;
    float r_min, r_max; // for RANDOM_RANGE
    float x_min, x_max, y_min, y_max; // for RANDOM_POS
    float pos_x, pos_y; // for FIXED_POS
    
    std::string texture_file;
    float texture_min_size;
    float texture_max_size;
    float structure_scale;
    int num_planes;
    Tree::Orientation_type orient_type;
    
    Trees trees;
  };
  typedef std::vector<Tree_config> Tree_configs;
  Tree_configs m_tree_configs;

  GLuint m_list_num;
};


#endif
