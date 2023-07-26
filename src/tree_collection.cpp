#include "tree_collection.h"
#include "tree.h"
#include "texture.h"
#include "config_file.h"
#include "sss.h"
#include "config.h"
#include "environment.h"

using namespace std;

Tree_collection * Tree_collection::s_instance = 0;

Tree_collection * Tree_collection::create_instance(const string & tree_config_file)
{
  s_instance = new Tree_collection(tree_config_file);
  return s_instance;
}

Tree_collection::Tree_collection(const string & tree_config_file)
  :
  Object(0),
  m_list_num(0)
{
  if (tree_config_file == "none")
    return;
  
  bool success;
  Config_file tree_config("terrains/" + tree_config_file, success);
  assert1(success);
  
  while (tree_config.find_new_block("tree_collection"))
  {
    int num;
    tree_config.get_next_value_assert("num", num);
    
    // will store this later...
    Tree_config cfg;
    cfg.orig_num = num;
    
    // get the texture and size
    tree_config.get_next_value_assert("texture_file", cfg.texture_file);
    tree_config.get_next_value_assert("texture_min_size", cfg.texture_min_size);
    tree_config.get_next_value_assert("texture_max_size", cfg.texture_max_size);
    tree_config.get_next_value_assert("structure_scale", cfg.structure_scale);
    tree_config.get_next_value_assert("num_planes", cfg.num_planes);
    string orient_type;
    tree_config.get_next_value_assert("orientation_type", orient_type);
    if (orient_type == "fixed")
      cfg.orient_type = Tree::FIXED;
    else if (orient_type == "vertical_billboard")
      cfg.orient_type = Tree::VERTICAL_BILLBOARD;
    else if (orient_type == "full_billboard")
      cfg.orient_type = Tree::FULL_BILLBOARD;
    else 
    {
      TRACE("Impossible tree orientation type: %s", orient_type.c_str());
      assert1(!"Error");
    }
    
    Rgba_file_texture * texture = new Rgba_file_texture(cfg.texture_file,
                                                        Rgba_file_texture::CLAMP_TO_EDGE,
                                                        Rgba_file_texture::RGBA);
    assert1(texture);
    
    // The position
    string pos_type_s;
    tree_config.get_next_value_assert("pos_type", pos_type_s);
    
    if (pos_type_s == "random_pos")
    {
      cfg.pos_type = Tree_config::RANDOM_POS;
      tree_config.get_next_value_assert("x_min", cfg.x_min);
      tree_config.get_next_value_assert("y_min", cfg.y_min);
      tree_config.get_next_value_assert("x_max", cfg.x_max);
      tree_config.get_next_value_assert("y_max", cfg.y_max);
    }
    else if (pos_type_s == "random_range")
    {
      cfg.pos_type = Tree_config::RANDOM_RANGE;
      tree_config.get_next_value_assert("r_min", cfg.r_min);
      tree_config.get_next_value_assert("r_max", cfg.r_max);
    }
    else if (pos_type_s == "fixed_pos")
    {
      cfg.pos_type = Tree_config::FIXED_POS;
      tree_config.get_next_value_assert("pos_x", cfg.pos_x);
      tree_config.get_next_value_assert("pos_y", cfg.pos_y);    
    }
    else
    {
      assert1(!"For tree, must specify pos_type");
    }
    
    float texture_size = ranged_random(cfg.texture_min_size, cfg.texture_max_size);
    
    // loop, creating the trees
    for (int i = 0 ; i < num ; ++i)
    {
      // try to stop the tree being under water...
      int tries = 0;
      bool ok = false;
      while (ok == false)
      {
        float x, y;
        if (cfg.pos_type == Tree_config::RANDOM_POS)
        {
          x = ranged_random(cfg.x_min, cfg.x_max);
          y = ranged_random(cfg.y_min, cfg.y_max);
        }
        else if (cfg.pos_type == Tree_config::RANDOM_RANGE)
        {
          float range = ranged_random(cfg.r_min, cfg.r_max);
          float dir = ranged_random(0, 360);
          x = Sss::instance()->config().start_x + range * sin_deg(dir);
          y = Sss::instance()->config().start_y + range * cos_deg(dir);
        }
        else if (cfg.pos_type == Tree_config::FIXED_POS)
        {
          x = cfg.pos_x;
          y = cfg.pos_y;
        }
        else
        {
          assert1(!"For tree, must specify pos_type");
        }
        
        // set the position according to the terrain - need to check for underwater!
        Position pos(x, y, 0);
        Position terrain_pos;
        Vector3 terrain_normal;
        Environment::instance()->get_local_terrain(pos,
                                                   terrain_pos,
                                                   terrain_normal);
        pos = terrain_pos + Position(0, 0, texture_size*0.5);
        // adjust for the terrain slope - assume the trunk base is frac
        // times the total width
        const float frac = 0.25;
        float offset = hypot(terrain_normal[0], terrain_normal[1]) * 
          0.5 * frac * texture_size;
        pos[2] -= offset;

        // check it's not underwater...
        ok = true;
        if ( (Environment::instance()->get_sea_altitude() > 
              (pos[2] - texture_size*0.5)) &&
             (tries < 100) )
        {
          // try again
          ok = false;
          ++tries;
        }
        else
        {
          
          // Create a tree
          Tree * new_tree = new Tree(pos,
                                     texture_size, 
                                     texture_size * cfg.structure_scale,
                                     cfg.orient_type);
          cfg.trees.push_back(new_tree);
          
          // And record the texture for it
          m_texture_maps[new_tree] = texture;
        }
        
      } // loop looking for a decent place
    } // tree creating loop
    
    // Finally store what we know, so that we can create things later
    m_tree_configs.push_back(cfg);
    
  } // loop over tree blocks
  
  // register for drawing
  Sss::instance()->add_object(this);
  
}

void Tree_collection::draw(Draw_type draw_type)
{
  const Position eye_dir = 
    ( Sss::instance()->eye().get_eye_target() - 
      Sss::instance()->eye().get_eye() ).normalise();
  
  const Position eye_pos = Sss::instance()->eye().get_eye();

  bool use_eye_dir = Sss::instance()->config().align_billboards_to_eye_dir;
  
  // set up texture stuff
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
  // alpha testing
  glEnable(GL_ALPHA_TEST);
  glAlphaFunc(GL_GREATER, 0.6f);
  // texture only - no lighting
  glDisable(GL_LIGHTING);
  
//   if (m_list_num != 0)
//   {
//     glCallList(m_list_num);
//     return;
//   }
//   else
//   {
//     m_list_num = glGenLists(1);
//     glNewList(m_list_num, GL_COMPILE);
  
  Rgba_file_texture * current_texture = 0;
  
  for (Tree_configs::iterator cfg_it = m_tree_configs.begin() ; 
       cfg_it != m_tree_configs.end() ; 
       ++cfg_it)
  {
    for (Trees::iterator it = (*cfg_it).trees.begin() ; 
         it != (*cfg_it).trees.end() ; 
         ++it)
    {
      glPushMatrix();
      if (use_eye_dir)
        (*it)->prepare_for_draw_dir(eye_dir);
      else
        (*it)->prepare_for_draw_pos(eye_pos);

      // need to change texture?
      Texture_maps::iterator texture_it = m_texture_maps.find(*it);
      if ( texture_it->second != current_texture)
      {
        current_texture = texture_it->second;
        glBindTexture(GL_TEXTURE_2D, texture_it->second->get_high_texture());
      }
      
      for (int i = 0 ; i < cfg_it->num_planes ; ++i)
      {
        if ( i != 0)
          glRotatef(180.0f + 180.0f / cfg_it->num_planes, 0, 0, 1);
        // draw in the x/z plane
        float s = (*it)->get_graphical_bounding_radius();
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0);      
        glVertex3f(-s, 0, -s);
        glTexCoord2f(0, 1);      
        glVertex3f(-s, 0,  s);
        glTexCoord2f(1, 1);      
        glVertex3f( s, 0,  s);
        glTexCoord2f(1, 0);      
        glVertex3f( s, 0, -s);
        glEnd();
      }
      glPopMatrix();
    }
//     }
//     glEndList();
//     glCallList(m_list_num);
  }
  
}


