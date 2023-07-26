/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  A very significant part of this code is taken from the Crrcsim
  project - Many thanks to Jan Kansky and Mark Drela for licensing
  that project under the GPL.

  \file glider_graphics_crrcsim.cpp */
#include "glider_graphics_crrcsim.h"
#include "sss.h"
#include "config.h"
#include "renderer.h"
#include "glider.h"
#include "log_trace.h"
 
#include <math.h>

using namespace std;

//==============================================================
// Glider_graphics_crrcsim
//==============================================================
Glider_graphics_crrcsim::Glider_graphics_crrcsim(
  Config_file graphics_config,
  Glider & glider)
  :
  m_glider(glider),
  current_shade_model(GL_INVALID_VALUE),
  list_num(glGenLists(2)),
  extrusions(0),
  cylinders(0),
  spheres(0),
  triangles(0)
{
  TRACE_METHOD_ONLY(1);
  TRACE("Using CRRCSIM graphics\n");
  
  Config_file::Config_attr_value attr_value;
  Config_file::Config_attr_value t_attr_value;
  
  bool done_parsing_extrusion; // Hit the end_extrusion keyword
  bool done_parsing_cylinder; // Hit the end_cylinder keyword
  bool done_parsing_sphere; // Hit the end_sphere keyword
  bool done_parsing_triangles; // Hit the end_triangle keyword
  Crrcsim_extrusion *extrusion_ptr; // Pointer to the current extrusion being read
  Crrcsim_extrusion *last_extrusion=NULL; //Pointer to the last filled extrusion struct
  Crrcsim_cylinder *cylinder_ptr;      // Pointer to the current cylinder being read
  Crrcsim_cylinder *last_cylinder=NULL;  // Pointer to the last filled cylinder struct
  Crrcsim_sphere *sphere_ptr;      // Pointer to the current sphere being read
  Crrcsim_sphere *last_sphere=NULL;  // Pointer to the last filled sphere struct
  Crrcsim_triangle *triangle_ptr=NULL;  //Pointer to the current triangles struct
  Crrcsim_triangle *last_triangle=NULL; // Pointer to the last filled triangle struct
  int loop;                   // Loop counter for extrusion parsing
  float x_component,y_component,magnitude;
  float dx1,dy1,dz1,dx2,dy2,dz2,normx,normy,normz,amplitude;  
  float units_scale = 1.0;
  
  // start reading the file, line by line
  attr_value = graphics_config.get_next_attr_value();
  
  while (attr_value.value_type != Config_file::Config_attr_value::INVALID)
  {
    if (attr_value.attr == "units_scale")
    {
      units_scale = attr_value.values[0].float_val;
    }
    else if ( attr_value.attr == "triangles" )
    {
      // read triangles
      TRACE_FILE_IF(2)
        TRACE("In triangles\n");  

      triangle_ptr=new Crrcsim_triangle;
      triangle_ptr->triangle_name="none";

      triangle_ptr->num_triangles=(unsigned long int)attr_value.values[0].float_val;
      triangle_ptr->vertices=(GLfloat **)malloc(triangle_ptr->
                                                num_triangles*sizeof(GLfloat *));
      triangle_ptr->normals=(GLfloat **)malloc(triangle_ptr->
                                               num_triangles*sizeof(GLfloat *));
      triangle_ptr->vertices[0]=(GLfloat *)malloc(triangle_ptr->
                                                  num_triangles*sizeof(GLfloat)*9);
      triangle_ptr->normals[0]=(GLfloat *)malloc(triangle_ptr->
                                                 num_triangles*sizeof(GLfloat)*3);
      for (loop=1;loop<(int)triangle_ptr->num_triangles;loop++)
      {
        triangle_ptr->vertices[loop]=triangle_ptr->vertices[0]+ loop*9;
        triangle_ptr->normals[loop]=triangle_ptr->normals[0]+ loop*3;
      }
      triangle_ptr->next_triangle=NULL;
      if (last_triangle==NULL)
        triangles=triangle_ptr;
      else
        last_triangle->next_triangle=triangle_ptr;
      last_triangle=triangle_ptr;

      done_parsing_triangles=false;
      vector<float> values;
      while(!done_parsing_triangles)
      {
        attr_value = graphics_config.get_next_attr_value();
        TRACE_FILE_IF(2)
          TRACE("    %s\n", attr_value.attr.c_str());

        if (attr_value.attr == "translate")
        {
          graphics_config.get_next_values(values, 3);
          triangle_ptr->translate_x=units_scale * values[0];
          triangle_ptr->translate_y=units_scale * values[1];
          triangle_ptr->translate_z=units_scale * values[2];
        }      
        else if (attr_value.attr == "rotate")
        {
          graphics_config.get_next_values(values, 3);
          triangle_ptr->rotate_x=values[0];
          triangle_ptr->rotate_y=values[1];
          triangle_ptr->rotate_z=values[2];
        }
        else if ( (attr_value.attr == "color") ||
                  (attr_value.attr == "colour") )
        {
          graphics_config.get_next_values(values, 4);
          triangle_ptr->colors[0]=values[0];
          triangle_ptr->colors[1]=values[1];
          triangle_ptr->colors[2]=values[2];
          triangle_ptr->colors[3]=values[3];
        }
        else if (attr_value.attr == "vertices")
        {
          for (loop=0;loop<(int)triangle_ptr->num_triangles;loop++)
          {
            graphics_config.get_next_values(values, 3);
            triangle_ptr->vertices[loop][0]=units_scale * values[0];
            triangle_ptr->vertices[loop][1]=units_scale * values[1];
            triangle_ptr->vertices[loop][2]=units_scale * values[2];
            graphics_config.get_next_values(values, 3);
            triangle_ptr->vertices[loop][3]=units_scale * values[0];
            triangle_ptr->vertices[loop][4]=units_scale * values[1];
            triangle_ptr->vertices[loop][5]=units_scale * values[2];
            graphics_config.get_next_values(values, 3);
            triangle_ptr->vertices[loop][6]=units_scale * values[0];
            triangle_ptr->vertices[loop][7]=units_scale * values[1];
            triangle_ptr->vertices[loop][8]=units_scale * values[2];
              
            dx1=triangle_ptr->vertices[loop][3]-
              triangle_ptr->vertices[loop][0];
            dy1=triangle_ptr->vertices[loop][1]-
              triangle_ptr->vertices[loop][4];
            dz1=triangle_ptr->vertices[loop][2]-
              triangle_ptr->vertices[loop][5];
            dx2=triangle_ptr->vertices[loop][3]-
              triangle_ptr->vertices[loop][6];
            dy2=triangle_ptr->vertices[loop][4]-
              triangle_ptr->vertices[loop][7];
            dz2=triangle_ptr->vertices[loop][5]-
              triangle_ptr->vertices[loop][8];
            normx=dy1*dz2-dz1*dy2;
            normy=dz1*dx2-dx1*dz2;
            normz=dx1*dy2-dy1*dx2;
            amplitude=sqrt(normx*normx+normy*normy+normz*normz);
            triangle_ptr->normals[loop][0]=normx/amplitude;
            triangle_ptr->normals[loop][1]=normy/amplitude;
            triangle_ptr->normals[loop][2]=normz/amplitude;
          }
        }
        else if (attr_value.attr == "end_triangles")
        {
          done_parsing_triangles=true;
        } 
      }
    }
    else if ( attr_value.attr == "sphere" )
    {
      // read a sphere
      TRACE_FILE_IF(2)
        TRACE("In sphere\n");  
      sphere_ptr=new Crrcsim_sphere;
      sphere_ptr->sphere_name="none";
      sphere_ptr->next_sphere=NULL;
      if (last_sphere==NULL)
        spheres=sphere_ptr;
      else
        last_sphere->next_sphere=sphere_ptr;
      last_sphere=sphere_ptr;

      done_parsing_sphere=false;
      vector<float> values;
      while(!done_parsing_sphere)
      {
        attr_value = graphics_config.get_next_attr_value();
        TRACE_FILE_IF(2)
          TRACE("    %s\n", attr_value.attr.c_str());

        if (attr_value.attr == "translate")
        {
          graphics_config.get_next_values(values, 3);
          sphere_ptr->translate_x=units_scale * values[0];
          sphere_ptr->translate_y=units_scale * values[1];
          sphere_ptr->translate_z=units_scale * values[2];
        }
        else if (attr_value.attr == "rotate")
        {
          graphics_config.get_next_values(values, 3);
          sphere_ptr->rotate_x=values[0];
          sphere_ptr->rotate_y=values[1];
          sphere_ptr->rotate_z=values[2];
        }
        else if (attr_value.attr == "dimension")
        {
          graphics_config.get_next_values(values, 1);
          sphere_ptr->radius=units_scale * values[0];
        }
        else if (attr_value.attr == "grid")
        {
          graphics_config.get_next_values(values, 2);
          sphere_ptr->slices=(int)values[0];
          sphere_ptr->stacks=(int)values[1];
        }
        else if ( (attr_value.attr == "color") ||
                  (attr_value.attr == "colour") )
        {
          graphics_config.get_next_values(values, 4);
          sphere_ptr->colors[0]=values[0];
          sphere_ptr->colors[1]=values[1];
          sphere_ptr->colors[2]=values[2];
          sphere_ptr->colors[3]=values[3];
        } 
        else if (attr_value.attr == "end_sphere")
        {
          done_parsing_sphere=true;
        }     
      }
    }
    else if ( attr_value.attr == "extrusion" )
    {
      // read a extrusion
      TRACE_FILE_IF(2)
        TRACE("In extrusion\n");  
      extrusion_ptr = new Crrcsim_extrusion;
      extrusion_ptr->num_points_on_path= 2 + (int)attr_value.values[0].float_val;
      extrusion_ptr->extrusion_name="none";

      extrusion_ptr->point_array=(gleDouble **)malloc(extrusion_ptr->
                                                      num_points_on_path*sizeof(gleDouble *));
      extrusion_ptr->point_array[0]=(gleDouble *)malloc(3*extrusion_ptr->
                                                        num_points_on_path*sizeof(gleDouble));
      extrusion_ptr->colors=(GLfloat **)malloc(extrusion_ptr->
                                               num_points_on_path*sizeof(GLfloat *));
      extrusion_ptr->colors[0]=(GLfloat *)malloc(3*extrusion_ptr->
                                                 num_points_on_path*sizeof(GLfloat));
      extrusion_ptr->scaling=(gleAffine *)malloc(extrusion_ptr->
                                                 num_points_on_path*sizeof(gleAffine));
      for (loop=1;loop<extrusion_ptr->num_points_on_path;loop++)
      {
        extrusion_ptr->point_array[loop]=extrusion_ptr->point_array[0]+
          loop*3;
        extrusion_ptr->colors[loop]=extrusion_ptr->colors[0]+
          loop*3;
      }
      extrusion_ptr->next_extrusion=NULL;
      if (last_extrusion==NULL)    // If this is the first parsed extrusion
        extrusions=extrusion_ptr;  
      else
        last_extrusion->next_extrusion=extrusion_ptr;
      last_extrusion=extrusion_ptr;

      done_parsing_extrusion=false;
      vector<float> values;
      while(!done_parsing_extrusion)
      {
        attr_value = graphics_config.get_next_attr_value();
        TRACE_FILE_IF(2)
          TRACE("    %s\n", attr_value.attr.c_str());

        if (attr_value.attr == "translate")
        {
          graphics_config.get_next_values(values, 3);
          extrusion_ptr->translate_x=units_scale * values[0];
          extrusion_ptr->translate_y=units_scale * values[1];
          extrusion_ptr->translate_z=units_scale * values[2];
        }
        if (attr_value.attr == "rotate")
        {
          graphics_config.get_next_values(values, 3);
          extrusion_ptr->rotate_x=values[0];
          extrusion_ptr->rotate_y=values[1];
          extrusion_ptr->rotate_z=values[2];
        }
        if (attr_value.attr == "up_vector")
        {
          graphics_config.get_next_values(values, 3);
          extrusion_ptr->up[0]=values[0];
          extrusion_ptr->up[1]=values[1];
          extrusion_ptr->up[2]=values[2];
        }
        if (attr_value.attr == "path")
        {
          for (loop=1;loop<extrusion_ptr->num_points_on_path-1;loop++)
          {
            graphics_config.get_next_values(values, 3);
            extrusion_ptr->point_array[loop][0]=units_scale * values[0];
            extrusion_ptr->point_array[loop][1]=units_scale * values[1];
            extrusion_ptr->point_array[loop][2]=units_scale * values[2];
          }
          extrusion_ptr->point_array[0][0]=extrusion_ptr->
            point_array[1][0];
          extrusion_ptr->point_array[0][1]=extrusion_ptr->
            point_array[1][1];
          extrusion_ptr->point_array[0][2]=extrusion_ptr->
            point_array[1][2];
          extrusion_ptr->point_array[extrusion_ptr->
                                     num_points_on_path-1][0]=extrusion_ptr->
            point_array[extrusion_ptr->num_points_on_path-2][0];
          extrusion_ptr->point_array[extrusion_ptr->
                                     num_points_on_path-1][1]=extrusion_ptr->
            point_array[extrusion_ptr->num_points_on_path-2][1];
          extrusion_ptr->point_array[extrusion_ptr->
                                     num_points_on_path-1][2]=extrusion_ptr->
            point_array[extrusion_ptr->num_points_on_path-2][2];
        }
        if ( (attr_value.attr == "colors") ||
             (attr_value.attr == "colours") )
        {
          for (loop=1;loop<extrusion_ptr->num_points_on_path-1;loop++)
          {
            graphics_config.get_next_values(values, 3);
            extrusion_ptr->colors[loop][0]=values[0];
            extrusion_ptr->colors[loop][1]=values[1];
            extrusion_ptr->colors[loop][2]=values[2];
          }
          extrusion_ptr->colors[0][0]=extrusion_ptr->
            colors[1][0];
          extrusion_ptr->colors[0][1]=extrusion_ptr->
            colors[1][1];
          extrusion_ptr->colors[0][2]=extrusion_ptr->
            colors[1][2];
          extrusion_ptr->colors[extrusion_ptr->
                                num_points_on_path-1][0]=extrusion_ptr->
            colors[extrusion_ptr->num_points_on_path-2][0];
          extrusion_ptr->colors[extrusion_ptr->
                                num_points_on_path-1][1]=extrusion_ptr->
            colors[extrusion_ptr->num_points_on_path-2][1];
          extrusion_ptr->colors[extrusion_ptr->
                                num_points_on_path-1][2]=extrusion_ptr->
            colors[extrusion_ptr->num_points_on_path-2][2];
        }
        if (attr_value.attr == "scaling")
        {
          for (loop=1;loop<extrusion_ptr->num_points_on_path-1;loop++)
          {
            graphics_config.get_next_values(values, 6);
            extrusion_ptr->scaling[loop][0][0]=values[0];
            extrusion_ptr->scaling[loop][0][1]=values[1];
            extrusion_ptr->scaling[loop][0][2]=units_scale * values[2];
            extrusion_ptr->scaling[loop][1][0]=values[3];
            extrusion_ptr->scaling[loop][1][1]=values[4];
            extrusion_ptr->scaling[loop][1][2]=units_scale * values[5];
          }
          extrusion_ptr->scaling[0][0][0]=extrusion_ptr->
            scaling[1][0][0];
          extrusion_ptr->scaling[0][0][1]=extrusion_ptr->
            scaling[1][0][1];
          extrusion_ptr->scaling[0][0][2]=extrusion_ptr->
            scaling[1][0][2];
          extrusion_ptr->scaling[0][1][0]=extrusion_ptr->
            scaling[1][1][0];
          extrusion_ptr->scaling[0][1][1]=extrusion_ptr->
            scaling[1][1][1];
          extrusion_ptr->scaling[0][1][2]=extrusion_ptr->
            scaling[1][1][2];
          extrusion_ptr->scaling[extrusion_ptr->
                                 num_points_on_path-1][0][0]=extrusion_ptr->
            scaling[extrusion_ptr->num_points_on_path-2][0][0];
          extrusion_ptr->scaling[extrusion_ptr->
                                 num_points_on_path-1][0][1]=extrusion_ptr->
            scaling[extrusion_ptr->num_points_on_path-2][0][1];
          extrusion_ptr->scaling[extrusion_ptr->
                                 num_points_on_path-1][0][2]=extrusion_ptr->
            scaling[extrusion_ptr->num_points_on_path-2][0][2];
          extrusion_ptr->scaling[extrusion_ptr->
                                 num_points_on_path-1][1][0]=extrusion_ptr->
            scaling[extrusion_ptr->num_points_on_path-2][1][0];
          extrusion_ptr->scaling[extrusion_ptr->
                                 num_points_on_path-1][1][1]=extrusion_ptr->
            scaling[extrusion_ptr->num_points_on_path-2][1][1];
          extrusion_ptr->scaling[extrusion_ptr->
                                 num_points_on_path-1][1][2]=extrusion_ptr->
            scaling[extrusion_ptr->num_points_on_path-2][1][2];
        }
        if (attr_value.attr == "section")
        {
          graphics_config.get_next_values(values, 1);
          extrusion_ptr->num_contour_elements=(int)values[0];
          
          extrusion_ptr->contour=(gleDouble **)malloc(extrusion_ptr->
                                                      num_contour_elements*sizeof(gleDouble *));
          extrusion_ptr->contour[0]=(gleDouble *)malloc(extrusion_ptr->
                                                        num_contour_elements*2*sizeof(gleDouble));
          extrusion_ptr->contour_normal_vectors=(gleDouble **)
            malloc(extrusion_ptr->num_contour_elements*
                   sizeof(gleDouble *));
          extrusion_ptr->contour_normal_vectors[0]=(gleDouble *)
            malloc(extrusion_ptr->num_contour_elements*2*
                   sizeof(gleDouble));
            
          for (loop=1;loop<extrusion_ptr->num_contour_elements;loop++)
          {
            extrusion_ptr->contour[loop]=extrusion_ptr->contour[0]+
              2*loop;
            extrusion_ptr->contour_normal_vectors[loop]=
              extrusion_ptr->contour_normal_vectors[0]+
              2*loop;
          }
          for (loop=0;loop<extrusion_ptr->num_contour_elements;loop++)
          {
            graphics_config.get_next_values(values, 2);
            extrusion_ptr->contour[loop][0]=values[0];
            extrusion_ptr->contour[loop][1]=values[1];
          }
          for (loop=2;loop<extrusion_ptr->num_contour_elements;loop++)
          {
            x_component=(extrusion_ptr->contour[loop][0]-
                         extrusion_ptr->contour[loop-1][0]);
            y_component=(extrusion_ptr->contour[loop][1]-
                         extrusion_ptr->contour[loop-1][1]);
            magnitude=sqrt(x_component*x_component+
                           y_component*y_component);
            extrusion_ptr->contour_normal_vectors[loop-1][0]=
              y_component/magnitude;
            extrusion_ptr->contour_normal_vectors[loop-1][1]=
              -1*x_component/magnitude;
              
            x_component=(extrusion_ptr->contour[loop-1][0]-
                         extrusion_ptr->contour[loop-2][0]);
            y_component=(extrusion_ptr->contour[loop-1][1]-
                         extrusion_ptr->contour[loop-2][1]);
            magnitude=sqrt(x_component*x_component+
                           y_component*y_component);
              
            extrusion_ptr->contour_normal_vectors[loop-1][0]+=
              y_component/magnitude;
            extrusion_ptr->contour_normal_vectors[loop-1][1]+=
              -1*x_component/magnitude;
            magnitude=sqrt(
              extrusion_ptr->contour_normal_vectors[loop-1][0]*
              extrusion_ptr->contour_normal_vectors[loop-1][0]+
              extrusion_ptr->contour_normal_vectors[loop-1][1]*
              extrusion_ptr->contour_normal_vectors[loop-1][1]);
            extrusion_ptr->contour_normal_vectors[loop-1][0]/=
              magnitude;
            extrusion_ptr->contour_normal_vectors[loop-1][1]/=
              magnitude;
          }
          x_component=(extrusion_ptr->contour[1][0]-
                       extrusion_ptr->contour[0][0]);
          y_component=(extrusion_ptr->contour[1][1]-
                       extrusion_ptr->contour[0][1]);
          magnitude=sqrt(x_component*x_component+
                         y_component*y_component);
          extrusion_ptr->contour_normal_vectors[0][0]=y_component/
            magnitude;
          extrusion_ptr->contour_normal_vectors[0][1]= -1*x_component/
            magnitude;
            
          x_component=(extrusion_ptr->contour[extrusion_ptr->
                                              num_contour_elements-1][0]-
                       extrusion_ptr->contour[extrusion_ptr->
                                              num_contour_elements-2][0]);
          y_component=(extrusion_ptr->contour[extrusion_ptr->
                                              num_contour_elements-1][1]-
                       extrusion_ptr->contour[extrusion_ptr->
                                              num_contour_elements-2][1]);
          magnitude=sqrt(x_component*x_component+
                         y_component*y_component);
          extrusion_ptr->contour_normal_vectors[extrusion_ptr->
                                                num_contour_elements-1][0]=y_component/
            magnitude;
          extrusion_ptr->contour_normal_vectors[extrusion_ptr->
                                                num_contour_elements-1][1]= -1*x_component/
            magnitude;
        }
        if (attr_value.attr == "end_extrusion")
        {
          done_parsing_extrusion=true;
        }
      }
    }
    else if ( attr_value.attr == "cylinder" )
    {
      // read a cylinder
      TRACE_FILE_IF(2)
        TRACE("In cylinder\n");  
      cylinder_ptr=new Crrcsim_cylinder;
      cylinder_ptr->cylinder_name="none";
      cylinder_ptr->next_cylinder=NULL;
      
      if (last_cylinder==NULL)
        cylinders=cylinder_ptr;
      else
        last_cylinder->next_cylinder=cylinder_ptr;
      
      last_cylinder=cylinder_ptr;
      
      done_parsing_cylinder=false;
      
      while(!done_parsing_cylinder)
      { 
        attr_value = graphics_config.get_next_attr_value();
        TRACE_FILE_IF(2)
          TRACE("    %s\n", attr_value.attr.c_str());
        
        vector<float> values;
        if (attr_value.attr == "translate")
        {
          graphics_config.get_next_values(values, 3);
          cylinder_ptr->translate_x=units_scale * values[0];
          cylinder_ptr->translate_y=units_scale * values[1];
          cylinder_ptr->translate_z=units_scale * values[2];
        }
        else if (attr_value.attr == "rotate")
        {
          graphics_config.get_next_values(values, 3);
          cylinder_ptr->rotate_x=values[0];
          cylinder_ptr->rotate_y=values[1];
          cylinder_ptr->rotate_z=values[2];
        }
        else if (attr_value.attr == "dimension")
        {
          graphics_config.get_next_values(values, 3);
          cylinder_ptr->radius_1=units_scale * values[0];
          cylinder_ptr->radius_2=units_scale * values[1];
          cylinder_ptr->length=units_scale * values[2];
        }
        else if (attr_value.attr == "grid")
        {
          graphics_config.get_next_values(values, 2);
          cylinder_ptr->radial_sampling=(int)values[0];
          cylinder_ptr->longitudinal_sampling=(int)values[1];
        }
        else if ( (attr_value.attr == "color") ||
                  (attr_value.attr == "colour") )
        {
          graphics_config.get_next_values(values, 4);
          cylinder_ptr->colors[0]=values[0];
          cylinder_ptr->colors[1]=values[1];
          cylinder_ptr->colors[2]=values[2];
          cylinder_ptr->colors[3]=values[3];
        } 
        else if (attr_value.attr == "end_cylinder")
        {
          done_parsing_cylinder=true;
        }     
        
      }
    }
    attr_value = graphics_config.get_next_attr_value();
  }

  calculate_max_dist();
}

//==============================================================
// draw
//==============================================================
void Glider_graphics_crrcsim::draw(Draw_type draw_type)
{
  if (
    (Sss::instance()->config().shade_model != current_shade_model)
    )
  {
    current_shade_model = Sss::instance()->config().shade_model;
//    glShadeModel(Sss::instance()->config().shade_model);
    glDeleteLists(list_num, 2);
    list_num = glGenLists(2);
    glNewList(list_num, GL_COMPILE);
    draw_glider(NORMAL);
    glEndList();
    glNewList(list_num+1, GL_COMPILE);
    draw_glider(SHADOW);
    glEndList();
  }
  
  glPushMatrix();
  
  if (draw_type == NORMAL)
    glCallList(list_num);
  else
    glCallList(list_num+1);

  glPopMatrix();
}


//==============================================================
// show
//==============================================================
void Glider_graphics_crrcsim::show()
{
}


//==============================================================
// calculate_max_dist
//==============================================================
void Glider_graphics_crrcsim::calculate_max_dist()
{
  max_dist = m_glider.get_structural_bounding_radius() * 1.2; // !!
}

//==============================================================
// draw_glider
//==============================================================
void Glider_graphics_crrcsim::draw_glider(Draw_type draw_type)
{
  Crrcsim_extrusion *extrusion_ptr;
  Crrcsim_cylinder *cylinder_ptr;
  Crrcsim_sphere *sphere_ptr;
  Crrcsim_triangle *triangle_ptr;
  int loop;
  GLUquadricObj * quadric = Renderer::instance()->quadric();
  
  extrusion_ptr=extrusions;
  cylinder_ptr=cylinders;
  sphere_ptr=spheres;
  triangle_ptr=triangles;
  
  glPushAttrib(GL_ALL_ATTRIB_BITS);
#ifdef linux
  glEnable(GL_RESCALE_NORMAL);
#else
  glEnable(GL_NORMALIZE);
#endif
  glPushMatrix(); 
  // convert from feet to meters
  glScalef(0.305f, 0.305f, 0.305f); 
  glRotatef(90,0,1,0);
  glRotatef(90,0,0,1);
  if (draw_type == SHADOW)
  {
    GLfloat shadow_ambient[] = {0.0, 0.0, 0.0, 1.0};
    GLfloat shadow_diffuse[] = {0.0, 0.0, 0.0, 0.3};
    GLfloat shadow_shiny[] = {0.0};
    GLfloat shadow_specular[] = {0.0, 0.0, 0.0, 1.0};
    glMaterialfv(GL_FRONT, GL_AMBIENT, shadow_ambient);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, shadow_diffuse);
    glMaterialfv(GL_FRONT, GL_SPECULAR, shadow_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, shadow_shiny);
    glColor4f(0,0,0,1.0);
    glDisable(GL_COLOR_MATERIAL);
  }
  else
    glEnable(GL_COLOR_MATERIAL);

  while (extrusion_ptr != NULL)
  {
    glPushMatrix();
    glTranslatef(extrusion_ptr->translate_x,extrusion_ptr->translate_y,
                 extrusion_ptr->translate_z);
    glRotatef(extrusion_ptr->rotate_x,1,0,0);
    glRotatef(extrusion_ptr->rotate_y,0,1,0);
    glRotatef(extrusion_ptr->rotate_z,0,0,1);
    float * color_array;
    if (draw_type == NORMAL)
      color_array = (*(extrusion_ptr->colors));
    else
      color_array = 0;
#ifdef WITH_GLE    
    gleSuperExtrusion(extrusion_ptr->num_contour_elements,
                      (gleDouble (*) [2])(*(extrusion_ptr->contour)),
                      (gleDouble (*) [2])(*(extrusion_ptr->
                                            contour_normal_vectors)),extrusion_ptr->up,
                      extrusion_ptr->
                      num_points_on_path,(gleDouble (*) [3])
                      (*(extrusion_ptr->point_array)),(float (*) [3])
                      color_array,
                      (gleAffine (*)) (*(extrusion_ptr->scaling)));
#endif    
    glPopMatrix();
    extrusion_ptr=extrusion_ptr->next_extrusion;
  }
  if (draw_type == NORMAL)
    glDisable(GL_COLOR_MATERIAL);

  while(cylinder_ptr!=NULL)
  {
    glPushMatrix();
    if (draw_type == NORMAL)
      glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,
                   cylinder_ptr->colors);
    
    glTranslatef(cylinder_ptr->translate_x,cylinder_ptr->translate_y,
                 cylinder_ptr->translate_z);
    glRotatef(cylinder_ptr->rotate_x,1,0,0);
    glRotatef(cylinder_ptr->rotate_y,0,1,0);
    glRotatef(cylinder_ptr->rotate_z,0,0,1);
    gluCylinder(quadric,cylinder_ptr->radius_1,cylinder_ptr->radius_2,
                cylinder_ptr->length,cylinder_ptr->radial_sampling,
                cylinder_ptr->longitudinal_sampling);
    glPopMatrix();
    cylinder_ptr=cylinder_ptr->next_cylinder;
  }
  while(sphere_ptr!=NULL)
  {
    glPushMatrix();
    if (draw_type == NORMAL)
      glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,
                   sphere_ptr->colors);
    glTranslatef(sphere_ptr->translate_x,sphere_ptr->translate_y,
                 sphere_ptr->translate_z);
    glRotatef(sphere_ptr->rotate_x,1,0,0);
    glRotatef(sphere_ptr->rotate_y,0,1,0);
    glRotatef(sphere_ptr->rotate_z,0,0,1);
    gluSphere(quadric,sphere_ptr->radius,sphere_ptr->slices,
              sphere_ptr->stacks);
    glPopMatrix();
    sphere_ptr=sphere_ptr->next_sphere;
  }
  
  glEnable(GL_CULL_FACE);
  while (triangle_ptr!=NULL)
  {
    glPushMatrix();
    if (draw_type == NORMAL)
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, 
                   triangle_ptr->colors);
    glTranslatef(triangle_ptr->translate_x,triangle_ptr->translate_y,
                 triangle_ptr->translate_z);
    glRotatef(triangle_ptr->rotate_x,1,0,0);
    glRotatef(triangle_ptr->rotate_y,0,1,0);
    glRotatef(triangle_ptr->rotate_z,0,0,1);
    glBegin(GL_TRIANGLES);
    for (loop=0;loop<(int)triangle_ptr->num_triangles;loop++)
    {
      glNormal3f(triangle_ptr->normals[loop][0],
                 triangle_ptr->normals[loop][1],
                 triangle_ptr->normals[loop][2]);
      glVertex3f(triangle_ptr->vertices[loop][0],
                 triangle_ptr->vertices[loop][1],
                 triangle_ptr->vertices[loop][2]);
      glVertex3f(triangle_ptr->vertices[loop][3],
                 triangle_ptr->vertices[loop][4],
                 triangle_ptr->vertices[loop][5]);
      glVertex3f(triangle_ptr->vertices[loop][6],
                 triangle_ptr->vertices[loop][7],
                 triangle_ptr->vertices[loop][8]);
    }
    glEnd();   
    triangle_ptr=triangle_ptr->next_triangle;
    glPopMatrix();
  }
  glDisable(GL_CULL_FACE);
  glPopMatrix();
  glPopAttrib();
}







