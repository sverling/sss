/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file graphics_3ds.cpp
*/

#include "sss.h"
#include <string>
#include "graphics_3ds.h"
#include "config.h"

#include "object.h"
#include "3ds.h"
#include "log_trace.h"

#include "sss_glut.h"

/*!
  \todo support textures
  \todo calculate correct bounding radius
*/

Graphics_3ds::Graphics_3ds(const char * strFilename_3ds)
  :
  max_dist(6),
  load3ds(new CLoad3DS),
  model(new t3DModel),
  current_shade_model(GL_INVALID_VALUE),
  list_num(glGenLists(2))
{
  TRACE_METHOD_ONLY(1);
  
  string filename_3ds = strFilename_3ds;
  cull_backface = false;

  TRACE_FILE_IF(2)
    TRACE("Loading %s\n", filename_3ds.c_str());
  filename_3ds = "gliders/" + filename_3ds;
  
  // Load our .3DS file into our model structure
  load3ds->Import3DS(model, filename_3ds.c_str()); 
  
  // assume no textures for now
  
  max_dist = calc_bounding_radius();
}

Graphics_3ds::~Graphics_3ds()
{
  TRACE_METHOD_ONLY(1);
  delete load3ds;
  delete model;
}

void Graphics_3ds::draw_thing(Draw_type draw_type)
{
  bool force_colour_update = true;
  
  if (cull_backface)
  {
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
  }
  else
  {
    glDisable(GL_CULL_FACE);
  }
  
  glDisable(GL_TEXTURE_2D);
  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_BLEND);
  
  glPushMatrix();
  
  glRotatef(90,  1, 0, 0);
  glRotatef(-90, 0, 1, 0);
  // Since we know how many objects our model has, go through each of them.
  for(int i = 0; i < model->numOfObjects; i++)
  {
    // Make sure we have valid objects just in case. (size() is in the vector class)
    if(model->pObject.size() <= 0) break;
    
    // Get the current object that we are displaying
    t3DObject *pObject = &model->pObject[i];
    
    Rgba_file_texture * texture = 0;
    
    if ( pObject->bHasTexture && model->pMaterials.size() && 
         pObject->materialID >= 0) 
    {
      // Bind the texture map to the object by it's materialID
      texture = model->pMaterials[pObject->materialID].rgba_texture;
    } 
    
    if (texture)
    {
      glEnable(GL_TEXTURE_2D);
      force_colour_update = true;
      glColor3f(1.0f, 1.0f, 1.0f);
      glBindTexture(GL_TEXTURE_2D, texture->get_high_texture());
    }
    else 
    {
      // Turn off texture mapping and turn on color
      glDisable(GL_TEXTURE_2D);
      // Reset the color to normal again
      force_colour_update = true;
    }

    // since we're allowing transparency on a per-vertex basis, have to enable blending
    // since we're limited with what we can do between glBegin and glEnd.
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // This determines if we are in wireframe or normal mode
    glBegin(GL_TRIANGLES);

    int count = 0;
    
    // Go through all of the faces (polygons) of the object and draw them
    for(int j = 0; j < pObject->numOfFaces; j++)
    {
      // Go through each corner of the triangle and draw it.
      for(int whichVertex = 0; whichVertex < 3; whichVertex++)
      {
        // Get the index for each point of the face
        int index = pObject->pFaces[j].vertIndex[whichVertex];
        
        // only do normal if it's different to last time. 
        static GLfloat last_nx = pObject->pNormals[ index ].x+1;
        static GLfloat last_ny = pObject->pNormals[ index ].y+1;
        static GLfloat last_nz = pObject->pNormals[ index ].z+1;
        if ( (last_nx != pObject->pNormals[ index ].x) ||
             (last_ny != pObject->pNormals[ index ].y) ||
             (last_nz != pObject->pNormals[ index ].z) )
        {
          last_nx = pObject->pNormals[index].x;
          last_ny = pObject->pNormals[index].y;
          last_nz = pObject->pNormals[index].z;
          
          // Give OpenGL the normal for this vertex.
          glNormal3f(pObject->pNormals[ index ].x,
                     pObject->pNormals[ index ].y,
                     pObject->pNormals[ index ].z);
        }

        // Make sure there is a valid material/color assigned to this object.
        // You should always at least assign a material color to an object, 
        // but just in case we want to check the size of the material list.
        // if the size is at least one, and the material ID != -1,
        // then we have a valid material.
        if (draw_type == NORMAL) 
        {
          if(texture) 
          {
            // Make sure there was a UVW map applied to the object or
            // else it won't have tex coords.
            if(pObject->pTexVerts) 
            {
              float x = pObject->pTexVerts[index].x;
              float y = pObject->pTexVerts[index].y;
//              TRACE("%f, %f\n", x, y);
              glTexCoord2f(x, y);
            }
          } 
          else
          {
            if(model->pMaterials.size() && pObject->materialID >= 0) 
            {
              // Get and set the color that the object is, since it must
              // not have a texture
              unsigned char *pColor = 
                model->pMaterials[pObject->materialID].color;
              
              // only do colour if it's different to last time. 
              static unsigned char last_r = pColor[0]+1;
              static unsigned char last_g = pColor[1]+1;
              static unsigned char last_b = pColor[2]+1;
              static unsigned char last_a = pColor[3]+1;
              if ( force_colour_update ||
                   (pColor[0] != last_r) ||
                   (pColor[1] != last_g) ||
                   (pColor[2] != last_b) ||
                   (pColor[3] != last_a) )
              {
                force_colour_update = false;
                last_r = pColor[0];
                last_g = pColor[1];
                last_b = pColor[2];
                last_a = pColor[3];
                // Assign the current color to this model
                glColor4ub(pColor[0], pColor[1], pColor[2], pColor[3]);
              }
            }
          }
        }
        // Pass in the current vertex of the object (Corner of current face)
        glVertex3f(pObject->pVerts[ index ].x,
                   pObject->pVerts[ index ].y,
                   pObject->pVerts[ index ].z);
        ++count;
      }
    }
    glEnd();        // End the drawing
  } // loop over objects
  
  glPopMatrix();
  
  glDisable(GL_CULL_FACE);
  glDisable(GL_BLEND);
}

/*!
  We assume that Object::basic_draw() has been called
*/
void Graphics_3ds::draw(Draw_type draw_type)
{
  if (
    (Sss::instance()->config().shade_model != current_shade_model)
    )
  {
    current_shade_model = Sss::instance()->config().shade_model;
    glShadeModel(Sss::instance()->config().shade_model);
    glDeleteLists(list_num, 2);
    list_num = glGenLists(2);
    glNewList(list_num, GL_COMPILE);
    draw_thing(NORMAL);
    glEndList();
    glNewList(list_num+1, GL_COMPILE);
    draw_thing(SHADOW);
    glEndList();
  }
  
  glPushMatrix();
  
  if (draw_type == NORMAL)
  {
    glCallList(list_num);
  }
  else
    glCallList(list_num+1);
  
  glPopMatrix();
}

void Graphics_3ds::show()
{
  TRACE("Graphics_3ds\n");
}


float Graphics_3ds::calc_bounding_radius() const
{
  float radius = 1.0;
  // Since we know how many objects our model has, go through each of them.
  for(int i = 0; i < model->numOfObjects; i++)
  {
    // Make sure we have valid objects just in case.
    if(model->pObject.size() <= 0) break;
    
    // Get the current object
    t3DObject *pObject = &model->pObject[i];
    
    // Go through all of the faces (polygons) of the object
    for(int j = 0; j < pObject->numOfFaces; j++)
    {
      // Go through each corner of the triangle.
      for(int whichVertex = 0; whichVertex < 3; whichVertex++)
      {
        // Get the index for each point of the face
        int index = pObject->pFaces[j].vertIndex[whichVertex];
        
        Position pos(pObject->pVerts[ index ].x,
                     pObject->pVerts[ index ].y,
                     pObject->pVerts[ index ].z);
        
        float r = pos.mag();
        
        radius = (radius > r ? radius : r);
      }
    }
    
  } // loop over objects
  // add a bit extra for safety...
  radius *= 1.05f;
  return radius;
}

