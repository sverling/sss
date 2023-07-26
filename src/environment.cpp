/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file environment.cpp
*/
#ifdef unix
#include <sys/types.h> // dodgy headers under Mac?
#include <sys/mman.h>
#else
//#include "stdafx.h"
//#include <afxwin.h>         // MFC core and standard components
//#include <afxext.h>         // MFC extensions
// include what?
#endif
#include "environment.h"
#include "terrain_generator.h"
#include "wind_field_generator.h"
#include "lod.h"
#include "object.h"
#include "sss.h"
#include "config.h"
#include "config_file.h"
#include "renderer.h"
#include "gui.h"
#include "array_2d.h"
#include "vertex.h"
#include "types.h"
#include "texture.h"
#include "body.h"
#include "thermal_manager.h"
#include "tree_collection.h"
#include "race_manager.h"
#include "log_trace.h"
#include "sss_assert.h"

#include "sss_glut.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>

#include <iostream>
using namespace std;

Environment * Environment::m_instance = 0;

// for turbulence
#define MAX_TURB_SCALE 16
#define NUM_TURB_SAMPLES 10

Environment::~Environment() 
{
  delete ground_vertex_array; 
  delete m_thermal_manager; 
  delete sea_texture;
  delete sand_texture;
  delete ground_texture;
  delete cloud_texture;
  delete sun_texture;
  delete ground_texture_detail;
  delete terrain_lightmap_texture;
  delete shadow_texture;
  delete flare_texture;
}

Environment::Environment()
  :
  sea_altitude(-7),
  m_air_density(1.0f),
  // water density is a hack - it breaks the physics if we
  // set it to 1000
  m_water_density(50.0f),
  sea_texture(0),
  sand_texture(0),
  ground_texture(0),
  cloud_texture(0),
  sun_texture(0),
  ground_texture_detail(0),
  terrain_lightmap_texture(0),
  shadow_texture(0),
  flare_texture(0),
  panorama_top_texture(0),
  panorama_sides_texture(0)
{
  TRACE_METHOD_ONLY(1);
  m_instance = this;

  float sun_bearing = Sss::instance()->config().sun_bearing;
  float sun_elevation = Sss::instance()->config().sun_elevation;
  sun_bearing -= 90.0f;
  light_pos[0] = cos_deg(sun_bearing) * cos_deg(sun_elevation);
  light_pos[1] = -sin_deg(sun_bearing) * cos_deg(sun_elevation);
  light_pos[2] = sin_deg(sun_elevation);
  light_pos[3] = 0.0f;
  
  // list of checkpoint gates (used for racing)
  vector<Gate> checkpoint_gates;
  
  void * lod_map;
  
  if (Sss::instance()->config().terragen_terrain != "none")
  {
    // if the terrain file is none, the winds must also be none or ds,
    // whatever the config says.
    string t_wind_type = Sss::instance()->config().wind_file;
    if ( (t_wind_type != "none") &&
         (t_wind_type != "ds") && 
         (t_wind_type != "runtime") )
      Sss::instance()->set_config().wind_file = "none";
    
    // create the terrain on a regular grid
    std::vector<Vertex> vertex_in;
    if (true != load_terragen_terrain(vertex_in))
    {
      TRACE("Error loading terragen file: %s\n",
            Sss::instance()->config().terragen_terrain.c_str());
      assert1(!"Error");
    }

    Sss::instance()->set_config().start_x = Sss::instance()->config().terragen_pos[0];
    Sss::instance()->set_config().start_y = Sss::instance()->config().terragen_pos[1];


    int n = (int) floor(2 * (log(nx-1.0)/log(2.0))+0.5);
    assert1(nx == (int) floor(pow(2.0,(n/2.0)) + 1.5));
    
    
    // Last two parameters are used to increase the resolution near to
    // the sea altitude.
    Lod lod(&vertex_in[0], n, nx, false, 0.0);
    
    // now have the output available
    Vertex * vertex_out;
    int out_size;
    lod.get_output(vertex_out, out_size);
    TRACE("out_size = %d\n", out_size);
    
    ground_vertex_array = new Vertex_array(nx, n, &vertex_out[0]);

    Position terrain_pos;
    Vector3 terrain_normal;
    get_local_terrain(Sss::instance()->config().terragen_pos, terrain_pos, terrain_normal);
    float z = get_z(Sss::instance()->config().terragen_pos[0], Sss::instance()->config().terragen_pos[1]);

    z = get_z(0.0f, 0.0f);
    z = get_z(nx * 4, nx * 4);
  }
  else if (Sss::instance()->config().terrain_file != "none")
  {
    // We want to read in the terrain data from file. The first time
    // this is done on this particular terrain file, we do this by
    // reading the data in cartesian form, then creating a quadtree
    // structure which we attempt to save to file (in a Lod_file
    // structure). If this is successful, we memory map the file for
    // our run-time data, otherwise wejust use the quadtree in memory
    // (assume that we have enough memory and swap space).  Subsequent
    // reads of the terrain file will use the Lod_file structure, if
    // it exists, otherwise we go through the above procedure again.
    
    string terrain_lod_file_name("terrains/" + Sss::instance()->config().
                                 terrain_file + string(".lod"));
    
    FILE * terrain_lod_file = fopen(terrain_lod_file_name.c_str(), "rb");
    
    if (terrain_lod_file == 0)
    {
      TRACE("Unable to open %s - creating the quadtree based on %s\n",
            terrain_lod_file_name.c_str(),
            Sss::instance()->config().terrain_file.c_str());
      
      string real_terrain_file = "terrains/" + Sss::instance()->config().terrain_file;
      FILE * terrain_file = fopen(
        real_terrain_file.c_str(), 
        "rb");
      if (terrain_file == 0)
      {
        TRACE("Unable to open %s\n", 
              Sss::instance()->config().terrain_file.c_str());
        assert1(!"Error");
      }
      
      Vertex * vertex_in;
      bool do_zero_edges = Sss::instance()->config().terrain_zero_edges;
      // load the terrain from file, populating vertex_in, nx and ny.
      file_terrain(terrain_file,
                   vertex_in,
                   nx, ny,
                   do_zero_edges);
      assert2(nx==ny, "Terrain must be square");
      
      fclose(terrain_file);
      
      int level = (int) floor(2 * (log(nx-1.0)/log(2.0))+0.5);
      Lod lod(vertex_in, level, nx, 
              Sss::instance()->config().coast_enhancement, 
              sea_altitude);
      
      // now have the output available
      Vertex * vertex_out;
      int out_size;
      lod.get_output(vertex_out, out_size);
      TRACE("Lod out_size = %d\n", out_size);
      
      ground_vertex_array = new Vertex_array(nx, level, &vertex_out[0]);
      
      // we have our output, but if possible we want to write it to a
      // file, and memory map the file. If we can't write to the file,
      // we keep the version of ground_vertex_array.
      TRACE("Opening lod file for writing: %s\n", terrain_lod_file_name.c_str());
      FILE * out_file = fopen(terrain_lod_file_name.c_str(), "wb");
      if (out_file != 0)
      {
        // have an output file to write to.
        const char identifier[] = "LODFILE";
        Lod_file_type type = SINGLE;
        int checksum = 0;
        size_t rv;
        
        fwrite(&identifier[0], sizeof(identifier[0]),  Lod_file::ID_LEN, out_file);
        fwrite(&type,      sizeof(type),           1,        out_file);
        fwrite(&checksum,  sizeof(checksum),       1,        out_file);
        fwrite(&level,     sizeof(level),          1,        out_file);
        fwrite(&nx,        sizeof(nx),             1,        out_file);
        fwrite(&out_size,  sizeof(out_size),       1,        out_file);
        rv = fwrite(&vertex_out[0], sizeof(Vertex),     out_size, out_file);
        fclose(out_file);
        
        // now everything is written out, we delete the version of
        // ground_vertex_array in memory. It will get read in from
        // file in a bit...
        
        terrain_lod_file = fopen(terrain_lod_file_name.c_str(), "rb");
        if (terrain_lod_file == 0)
        {
          TRACE("Error opening %s, even though we "
                "should have just created it!\n",
                terrain_lod_file_name.c_str());
          assert1(!"error");
        }
        delete [] vertex_out;
        vertex_out = 0;
        
        delete ground_vertex_array;
        ground_vertex_array = 0;
      }
      else
      {
        // OK - we couldn't write the output file. Just keep the data
        // in memory.
        TRACE("Warning - we couldn't open the lod file for writing: %s\n",
              terrain_lod_file_name.c_str());
      }
    } // could we open terrain_lod_file?
    
    
    if (terrain_lod_file != 0)
    {
      TRACE("Memory mapping %s\n", terrain_lod_file_name.c_str());
      // we have a LOD file - memory map it. Note that if we don't
      // have a LOD file, the quadtree should already be set up in
      // ground_vertex_array.
      
      // ===================== UNIX file mapping =======================
#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
      int unix_lod_file = fileno(terrain_lod_file);
      
      struct stat statbuf;
      size_t size;
      if (fstat(unix_lod_file, &statbuf) < 0)
      {
        TRACE("Can't stat file %s\n", terrain_lod_file_name.c_str());
        assert1(!"error");
      }
      
      size = statbuf.st_size;

      TRACE("Size of mapped file = %d (%d/%d)\n", size, statbuf.st_uid, statbuf.st_gid);
      assert1(size > 0);

      if ( (lod_map = mmap(0, size, PROT_READ, 
                           MAP_FILE | MAP_SHARED, unix_lod_file, 0)) == 
           (caddr_t) -1)
      {
        TRACE("Can't map terrain file\n");
        assert1(!"error");
      }
#else
// ===================== Windoze file mapping =======================
      
      HANDLE terrain_file = CreateFile(terrain_lod_file_name.c_str(), 
                                       GENERIC_READ, FILE_SHARE_READ, 0,
                                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
      if (terrain_file == INVALID_HANDLE_VALUE)
      {
        TRACE("error opening %s\n", terrain_lod_file_name.c_str());
        assert1(!"error");
      }
      
      HANDLE map_handle = CreateFileMapping(
        terrain_file, 0, PAGE_READONLY, 0, 
        0, "Terrain map");
      
      if (map_handle == INVALID_HANDLE_VALUE)
      {
        TRACE("error mapping %s\n", terrain_lod_file_name.c_str());
        assert1(!"error");
      }
      
      TRACE("Size of mapped file = %d\n", GetFileSize(terrain_file, 0));
      
      if ( (lod_map = 
            MapViewOfFile(map_handle, FILE_MAP_READ, 0, 0, 0)) == 0)
      {
        TRACE("Can't map %s\n",
              terrain_lod_file_name.c_str());
        assert1(!"error");
      }
#endif
      
      fclose(terrain_lod_file);

      Lod_file * lod_file = (Lod_file *) lod_map;
      assert2(lod_file, "Cannot open processed terrain");
      
      nx = lod_file->nx;
      ny = lod_file->nx;
      int n = lod_file->level;
      
      assert2(nx == pow(2.0,(n/2.0)) + 1, "Terrain must be size 2^n + 1");
      
      TRACE("nx = %d, n = %d\n", nx, n);
      
      ground_vertex_array = new Vertex_array(nx, n, &lod_file->vertices[0]);
    }
    else // i.e. no Lod_file
    {
      assert1(ground_vertex_array != 0);
    }
  }
  else // i.e. the terrain file is "none"
  {
    // if the terrain file is none, the winds must also be none or ds,
    // whatever the config says.
    string t_wind_type = Sss::instance()->config().wind_file;
    if ( (t_wind_type != "none") &&
         (t_wind_type != "ds") && 
         (t_wind_type != "runtime") )
      Sss::instance()->set_config().wind_file = "none";
    
    // we need to construct the terrain.
    
    // terrain
    nx = Sss::instance()->config().builtin_terrain_size;
    ny = nx;
    int n = (int) floor(2 * (log(nx-1.0)/log(2.0))+0.5);
    assert1(nx == (int) floor(pow(2.0,(n/2.0)) + 1.5));
    
    float dx, dy;
    // ensure that the terrain always ends up the same resolution,
    // whatever nx/ny are.
    dx = dy = Sss::instance()->config().builtin_terrain_dx;
    
    float xmin = -nx*dx/2.0f;
    
    // create the terrain on a regular grid
    
    Vertex * vertex_in = new Vertex[nx*ny];
    
    switch (Sss::instance()->config().builtin_terrain_type)
    {
    case Config::TERRAIN_PEAKS:
    {
      gaussian_peaks_terrain(
        vertex_in,
        nx, ny,
        dx, dy,
        xmin, xmin,
        Sss::instance()->config().builtin_terrain_peak_radius,
        Sss::instance()->config().builtin_terrain_peak_height,
        Sss::instance()->config().builtin_terrain_lumps * nx*nx/(129*129),
        Sss::instance()->config().builtin_terrain_seed);
      break;
    }
    case Config::TERRAIN_MIDPOINT:
    {
      midpoint_displacement_terrain(
        vertex_in,
        nx, ny,
        dx, dy,
        xmin, xmin,
        Sss::instance()->config().builtin_terrain_height * (nx/65.0) * (dx/10),
        Sss::instance()->config().builtin_terrain_roughness,
        Sss::instance()->config().builtin_terrain_filter,
        Sss::instance()->config().builtin_terrain_seed);
      break;
    }
    case Config::TERRAIN_RIDGE:
    {
      ridge_terrain(
        vertex_in,
        nx, ny,
        dx, dy,
        xmin, xmin,
        Sss::instance()->config().builtin_terrain_ridge_height,
        Sss::instance()->config().builtin_terrain_ridge_width);
      break;
    }
    case Config::TERRAIN_PLATEAU:
    {
      plateau_terrain(
        vertex_in,
        nx, ny,
        dx, dy,
        xmin, xmin,
        Sss::instance()->config().builtin_terrain_ridge_height,
        Sss::instance()->config().builtin_terrain_ridge_width,
        Sss::instance()->config().builtin_terrain_plateau_width);
      break;
    }
    case Config::TERRAIN_PISTE:
    {
      Position start_pos = piste_terrain(
        vertex_in,
        nx, ny,
        dx, dy,
        xmin, xmin,
        Sss::instance()->config().builtin_terrain_piste_slope,
        Sss::instance()->config().builtin_terrain_seed,
        checkpoint_gates);
      
      Sss::instance()->set_config().start_x = start_pos[0];
      Sss::instance()->set_config().start_y = start_pos[1];
      // add the gates to the race manager later, when the terrain is
      // all ready.
      break;
    }
    default:
      TRACE("Invalid terrain type: %d\n", 
            Sss::instance()->config().builtin_terrain_type);
      assert1(!"error");
      break;
    }
    
    // Last two parameters are used to increase the resolution near to
    // the sea altitude.
    Lod lod(vertex_in, n, nx, 
            Sss::instance()->config().coast_enhancement, 
            sea_altitude);
    
    // now have the output available
    Vertex * vertex_out;
    int out_size;
    lod.get_output(vertex_out, out_size);
    TRACE("out_size = %d\n", out_size);
    
    ground_vertex_array = new Vertex_array(nx, n, &vertex_out[0]);
    
    // delete the vertex_in temporary
    delete [] vertex_in;
    
  }
  
  assert1(ground_vertex_array != 0);
  
  // Hooray - the ground stuff is all done.
  // now do the winds
  
  if (Sss::instance()->config().wind_file == "ds")
  {
    float separation_slope =
      Sss::instance()->config().separation_slope;
    boundary_layer_depth =
      Sss::instance()->config().boundary_layer_depth;
    float separation_wind_scale =
      Sss::instance()->config().separation_wind_scale;
    filtered_separated_wind_from_terrain(separation_slope,
                                         boundary_layer_depth,
                                         separation_wind_scale,
                                         this,
                                         ground_vertex_array,
                                         wind_x, wind_y, wind_z,
                                         wind_u, wind_v, wind_w,
                                         wind_nx, wind_ny, wind_nz);
  }
  else if (Sss::instance()->config().wind_file == "none")
  {
    boundary_layer_depth =
      Sss::instance()->config().boundary_layer_depth;
#if 0
    simple_wind_from_terrain(this,
                             ground_vertex_array,
                             wind_x, wind_y, wind_z,
                             wind_u, wind_v, wind_w,
                             wind_nx, wind_ny, wind_nz);
#endif
#if 1
    filtered_wind_from_terrain(
      boundary_layer_depth,
      this,
      ground_vertex_array,
      wind_x, wind_y, wind_z,
      wind_u, wind_v, wind_w,
      wind_nx, wind_ny, wind_nz);
#endif
  }
  else if (Sss::instance()->config().wind_file == "runtime")
  {
    TRACE("Using runtime wind-field\n");
    boundary_layer_depth =
      Sss::instance()->config().boundary_layer_depth;
    
    // zero the stuff we won't use;
    wind_x = wind_y = wind_z = 0;
    wind_u = wind_v = wind_w = 0;
    wind_nx = wind_ny = wind_nz = 0;
  }
  else
  {
    // read in wind stuff from file
    if (0 != wind_from_file(Sss::instance()->config().wind_file,
                            wind_x, wind_y, wind_z,
                            wind_u, wind_v, wind_w,
                            wind_nx, wind_ny, wind_nz))
    {
      TRACE("Error setting up wind-field\n");
      assert1(!"error");
    }
  }
  
  // estimate a reasonable location to stand
  if ((Sss::instance()->config().builtin_terrain_type 
       != Config::TERRAIN_PISTE) &&
      (Sss::instance()->config().terrain_file == "none") &&
      (Sss::instance()->config().use_terragen_terrain == false)
    )
  {
    //over-ride the starting positions
    Position pos = estimate_best_start_position();
    Sss::instance()->set_config().start_x = pos[0];
    Sss::instance()->set_config().start_y = pos[1];
  }
  
  // initialise the thermal field
  m_thermal_manager = new Thermal_manager(
    ground_vertex_array->get_pos(0, 0)[0],
    ground_vertex_array->get_pos(nx-1, ny-1)[0],
    ground_vertex_array->get_pos(0, 0)[1],
    ground_vertex_array->get_pos(nx-1, ny-1)[1]);
  
  // do the trees
  m_tree_collection = Tree_collection::create_instance(
    Sss::instance()->config().tree_config_file);
  
  // any gates/checkpoints? 
  
  if (!checkpoint_gates.empty())
  {
    vector<Checkpoint *> checkpoints;
    for (unsigned i = 0 ; i < checkpoint_gates.size() ; ++i)
    {
      checkpoints.push_back(new Checkpoint_gate(checkpoint_gates[i].left_x,
                                                checkpoint_gates[i].left_y,
                                                checkpoint_gates[i].right_x,
                                                checkpoint_gates[i].right_y));
    }
    
    // add the checkpoints to the race manager
    Race_manager::add_checkpoints(checkpoints);
  }
}

bool Environment::load_terragen_terrain(std::vector<Vertex>& vertex)
{

  string terrain_config_filename(
    "terrains/" + Sss::instance()->config().terragen_terrain + ".cfg");
  string terrain_raw_filename(
    "terrains/" + Sss::instance()->config().terragen_terrain + ".raw");

  bool success = false;
  Config_file terrain_config(terrain_config_filename, success);
  if (success == false)
  {
    TRACE("Unable to open terragen config file %s\n", terrain_config_filename.c_str());
    return false;
  }

  terrain_config.get_value("nx", nx);
  terrain_config.get_value("ny", ny);
  float dx = 1, dy = 1;
  terrain_config.get_value("delta", dx);
  terrain_config.get_value("delta", dy);

  terrain_config.get_value("water_level", sea_altitude);

  terrain_config.get_value("camera_x", Sss::instance()->set_config().terragen_pos[0]);
  terrain_config.get_value("camera_y", Sss::instance()->set_config().terragen_pos[1]);
  terrain_config.get_value("camera_z", Sss::instance()->set_config().terragen_pos[2]);

  // lighting
  float sun_bearing = 0.0f;
  float sun_elevation = 45.0f;
  terrain_config.get_value("sun_bearing", sun_bearing);
  terrain_config.get_value("sun_elevation", sun_elevation);
  sun_bearing -= 90.0f;
  light_pos[0] = cos_deg(sun_bearing) * cos_deg(sun_elevation);
  light_pos[1] = -sin_deg(sun_bearing) * cos_deg(sun_elevation);
  light_pos[2] = sin_deg(sun_elevation);
  light_pos[3] = 0.0f;

  float min_x = 0.0f;
  float min_y = 0.0f;

  float min_z = 0.0f;
  float max_z = 100.0f;
  terrain_config.get_value("min_z", min_z);
  terrain_config.get_value("max_z", max_z);

  // load the raw data - 16 bits
  FILE * data_file = fopen(terrain_raw_filename.c_str(), "rb");
  if (data_file == 0)
  {
    TRACE("Unable to open raw data file: %s\n", terrain_raw_filename.c_str());
    return false;
  }

  vertex.resize(nx * ny);

  TRACE("Reading raw data file: %s\n", terrain_raw_filename.c_str());
  int i, j;
  for (j = 0 ; j < ny ; ++j)
  {
    for (i = 0 ; i < nx ; ++i)
    {
      unsigned short val;
      size_t num = fread(&val, sizeof(val), 1, data_file);
      if (num == 0)
      {
        TRACE("Error reading terragen raw file: i = %d, j = %d\n", i, j);
        fclose(data_file);
        return false;
      }
      float z = min_z + (max_z - min_z) * val / (256*256 - 1);
      vertex[Lod::calc_index(i, j, nx)] = Vertex(min_x + i * dx, min_y + j * dy, z);
    }
  }

  TRACE("Finished reading the terragen data\n");
  fclose(data_file);

  // now convert things

  return true;
}


void Environment::get_index(float x0, float y0,                       
                            unsigned & i11, unsigned & j11,
                            const Position & pos_min, 
                            const Position & delta) const
{
  
  // i11 is just less than x0
  i11 = (int) ( (x0-pos_min[0]) / delta[0] );
  j11 = (int) ( (y0-pos_min[1]) / delta[1] );
  
  if ((int) i11 > (nx-2))
  {
    i11 = nx-2;
  }
  else if ((int) i11 < 0)
  {
    i11 = 0;
  }
  
  if ((int) j11 > (nx-2))
  {
    j11 = nx-2;
  }
  else if ((int) j11 < 0)
  {
    j11 = 0;
  }
}

/// Interpolates within a quad to return the height, dividing the quad
/// into two triangles. Returns the interpolated height.
/// This function comes high on the list with gprof
float interpolate_for_z(
  Position pos,
  const Position & pos11,
  const Position & pos12,
  const Position & pos21,
  const Position & pos22,
  bool upward_slope) // direction of the diagonal
{
  // P = c0*P0 + c1*P1 + c2*P2
  // arrange it so that at the end we interpolate over a triangle:
  //    /| P2
  //   / |   
  //  /__|   
  // P1   P0 
  
  // make sure pos is within triangle
  if (pos[0] < pos11[0])
    pos[0] = pos11[0];
  else if (pos[0] > pos22[0])
    pos[0] = pos22[0];
  if (pos[1] < pos11[1])
    pos[1] = pos11[1];
  else if (pos[1] > pos22[1])
    pos[1] = pos22[1];
  
  const Position * P0;
  const Position * P1;
  const Position * P2;
  
  if (upward_slope)
  {
    if ( (pos[0] - pos11[0]) > (pos[1] - pos11[1]) )
    {
      // right hand side /|
      P0 = &pos21;
      P1 = &pos11;
      P2 = &pos22;
    }
    else
    {
      // left hand side
      P0 = &pos12;
      P1 = &pos22;
      P2 = &pos11;
    }
  }
  else
  {
    if ( (pos[0] - pos11[0]) > (pos12[1] - pos[1]) )
    {
      // right hand side \|
      P0 = &pos22;
      P1 = &pos21;
      P2 = &pos12;
    }
    else
    {
      // left hand side
      P0 = &pos11;
      P1 = &pos12;
      P2 = &pos21;
    }
  }
  
//    TRACE("P0 = (%5.1f, %5.1f, %5.1f), P1 = (%5.1f, %5.1f, %5.1f), P2 = (%5.1f, %5.1f, %5.1f)\n",
//          P0[0], P0[1], P0[2],
//          P1[0], P1[1], P1[2],
//          P2[0], P2[1], P2[2]); 
//    TRACE("pos = (%5.2f, %5.2f)\n", pos[0], pos[1]);  
  
// could do this... but we don't care about z
  // const Position E1 = *P1 - *P0; 
  const float E1x = (*P1)[0] - (*P0)[0];
  const float E1y = (*P1)[1] - (*P0)[1];
  
//  E1[2] = 0;
  // const Position E2 = *P2 - *P0;
  const float E2x = (*P2)[0] - (*P0)[0];
  const float E2y = (*P2)[1] - (*P0)[1];
//  E2[2] = 0;
  // const Position E = pos - *P0;
  const float Ex = pos[0] - (*P0)[0];
  const float Ey = pos[1] - (*P0)[1];
//  E[2] = 0;
  
//    TRACE("E1 = (%5.1f, %5.1f), E2 = (%5.1f, %5.1f)\n",
//          E1[0], E1[1],
//          E2[0], E2[1]);
  
  const float e11 = E1x * E1x + E1y * E1y; //dot(E1, E1);
  const float e12 = E1x * E2x + E1y * E2y; //dot(E1, E2);
  const float e22 = E2x * E2x + E2y * E2y; //dot(E2, E2);
  const float inv_delta = 1.0f/(e11*e22 - e12*e12);
//  assert1(delta > 0.000001f);
//    TRACE("e11 = %f, e12 = %f, e22 = %f, delta = %f\n",
//          e11, e12, e22, delta);
  
  const float p1 = Ex * E1x + Ey * E1y; //dot(E, E1);
  const float p2 = Ex * E2x + Ey * E2y; //dot(E, E2);
  
  const float c1 = (e22*p1 - e12*p2) * inv_delta;
  const float c2 = (e11*p2 - e12*p1) * inv_delta;
  const float c0 = 1.0f - c1 - c2;
  
//    TRACE("c0 = %f, c1 = %f, c2 = %f\n", c0, c1, c2);
  
  return c0 * (*P0)[2] + c1 * (*P1)[2] + c2 * (*P2)[2];
}

void Environment::get_local_terrain(const Position & pos,
                                    Position & terrain_pos,
                                    Vector3 & normal) const
{
  unsigned i11, j11, i22, j22;
  unsigned i12, j12, i21, j21;
  
  const Position pos_min = ground_vertex_array->get_pos(0, 0);
  const Position pos_min1 = ground_vertex_array->get_pos(1, 1);
  const Position delta = pos_min1-pos_min;
  
  float x0 = pos[0];
  float y0 = pos[1];
  
  get_index(x0, y0,
            i11, j11,
            pos_min, delta);
  
  i12 = i11;
  i21 = i11 + 1;
  i22 = i11 + 1;
  
  j21 = j11;
  j12 = j11 + 1;
  j22 = j11 + 1;
  
  assert1( ((int) i11 < nx) &&
           ((int) i21 < nx) &&
           ((int) i12 < nx) &&
           ((int) i22 < nx) &&
           ((int) j11 < ny) &&
           ((int) j21 < ny) &&
           ((int) j12 < ny) &&
           ((int) j22 < ny) );
  
  const Position pos11 = ground_vertex_array->get_pos(i11, j11);
  const Position pos12 = ground_vertex_array->get_pos(i12, j12);
  const Position pos21 = ground_vertex_array->get_pos(i21, j21);
  const Position pos22 = ground_vertex_array->get_pos(i22, j22);
  
  bool upward_slope;
  if ( (i11 + j11) % 2)
  {
    // odd, so this quad is divided by a downward slope
    upward_slope = false;
  }
  else
    upward_slope = true;
  
  float interp_z = interpolate_for_z(pos, 
                                     pos11,
                                     pos12, 
                                     pos21,
                                     pos22,
                                     upward_slope);
  
  // We've calculated the terrain point, at least for now. We could do
  // better by recalculating this using the surface normal.
  terrain_pos = pos;
//    terrain_pos[2] = 
//      (pos11[2]*(1-frac_i) + pos21[2]*frac_i) * (1-frac_j) +
//      (pos12[2]*(1-frac_i) + pos22[2]*frac_i) * frac_j;
  terrain_pos[2] = interp_z;
  
  normal = cross( (pos22-pos11), (pos12 - pos21) ).normalise();
}


float Environment::get_z(float x0, float y0) const
{
  unsigned i11, j11, i22, j22;
  unsigned i12, j12, i21, j21;
  
  Position pos_min = ground_vertex_array->get_pos(0, 0);
  Position pos_min1 = ground_vertex_array->get_pos(1, 1);
  Position delta = pos_min1-pos_min;
  
  get_index(x0, y0,
            i11, j11,
            pos_min, delta);
  
  i12 = i11;
  i21 = i11 + 1;
  i22 = i11 + 1;
  
  j21 = j11;
  j12 = j11 + 1;
  j22 = j11 + 1;
  
  assert1( ((int) i11 < nx) &&
           ((int) i21 < nx) &&
           ((int) i12 < nx) &&
           ((int) i22 < nx) &&
           ((int) j11 < ny) &&
           ((int) j21 < ny) &&
           ((int) j12 < ny) &&
           ((int) j22 < ny) );
  
  const Position pos11 = ground_vertex_array->get_pos(i11, j11);
  const Position pos12 = ground_vertex_array->get_pos(i12, j12);
  const Position pos21 = ground_vertex_array->get_pos(i21, j21);
  const Position pos22 = ground_vertex_array->get_pos(i22, j22);
  
  bool upward_slope;
  if ( (i11 + j11) % 2)
  {
    // odd, so this quad is divided by a downward slope
    upward_slope = false;
  }
  else
    upward_slope = true;
  return interpolate_for_z(Position(x0, y0, 0), 
                           pos11,
                           pos12, 
                           pos21,
                           pos22,
                           upward_slope);
}

void Environment::set_z(Position & pos) const
{
  pos[2] = get_z(pos[0], pos[1]);
}

/// Helper to generate a suitable cloud texture
Grey_texture * generate_cloud_texture(int size,
                                      float noise_r,
                                      float bump_frac,
                                      Vector3 light_dir)
{
  const unsigned nx = size;
  const unsigned ny = nx;
  Array_2D<float> image(ny, nx);
  calculate_midpoint_displacement(image, noise_r, 0, 1);
  image.set_wrap(true);
  image.gaussian_filter(2, 4);
//      image.multiply(image);
  
  Array_2D<float> alpha(image);
  alpha.set_wrap(true);
  alpha.pow(3.0f);
  alpha.set_range(0, 1);
  
  // store the original before we mess with it
  Array_2D<float> cloud(image);

  image.pow(0.1f);

  // do the bump map bit?
//   if (bump_frac < 0.0001f)
//   {
//     return new Grey_texture(image, alpha, Grey_texture::REPEAT);
//   }
  
  // try bump-mapping the cloud, and blending with the original
//      Array_2D<float> image2(image);
  Array_2D<float> image_x(image);
  image_x.set_wrap(true);
  image_x.gradient_x();
  image_x *= nx;
  Array_2D<float> image_y(image);
  image_y.set_wrap(true);
  image_y.gradient_y();
  image_y *= ny;
  
  Array_2D<float> bump_map(nx, ny);
  bump_map.set_wrap(true);
  unsigned i, j;

  light_dir.normalise();
  
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float vz = 1.0f;
      float vx = vz * image_x(i, j);
      float vy = vz * image_y(i, j);
      Vector3 surf(vx, vy, vz);
      surf.normalise();
      
      bump_map(i, j) = dot(light_dir, surf);
    }
  }
  
  cloud *= -1.0f;
  cloud.set_range(0.85f, 1.0f);

  cloud.set_wrap(true);
  cloud *= (1.0f - bump_frac);
  bump_map *= bump_frac;
  
  cloud.add(bump_map);
  
  return new Grey_texture(cloud, alpha, Grey_texture::REPEAT);
}

Image_texture * generate_sun_texture(const int nx, 
                                     const float sun_size,
                                     const float decay)
{
  const int ny = nx;
  Array_2D<float> red(nx, ny);
  Array_2D<float> green(nx, ny);
  Array_2D<float> blue(nx, ny);
  Array_2D<float> alpha(nx, ny);

//   const float xmid = nx/2;
//   const float ymid = ny/2;

  Vector3 sun_colour(1.0f, 1.0f, 1.0f);

  for (int i = 0 ; i < nx ; ++i)
  {
    for (int j = 0 ; j < ny ; ++j)
    {
      float fi = (float) i / (nx - 1);
      float fj = (float) j / (ny - 1);
      
      float dist = hypot(fi - 0.5f, fj - 0.5f);
      dist -= sun_size;
      if (dist < 0.0f)
      {
        // in the sun
        red(i, j) = sun_colour[0];
        green(i, j) = sun_colour[1];
        blue(i, j) = sun_colour[2];
        alpha(i, j) = 1.0f;
      }
      else
      {
        float frac = exp(-dist / decay );
        red(i, j) = sun_colour[0];
        green(i, j) = sun_colour[1];
        blue(i, j) = sun_colour[2];
        alpha(i, j) = frac;
      }
    }
  }
  return new Image_texture(red, green, blue, alpha, Image_texture::CLAMP);
}


// use this within a display list
void draw_sky_dome(float cloud_height,
                   float radius,
                   bool do_colour,
                   bool do_sun_tex_coords,
                   Vector3 sun_dir)
{
  Vector3 col_top(0.1f, 0.3f, 0.55f);
  Vector3 col_bot(0.55f, 0.7f, 0.85f);

  // setup the vectors for the sun
  Vector3 sun_up; // 
  Vector3 sun_right;
  if (do_sun_tex_coords)
  {
    sun_dir.normalise();
    // hope the sun isn't directly overhead.
    sun_up = Vector3(0, 0, 1); // initial guess...
    sun_right = cross(sun_dir, sun_up);
    sun_right.normalise();
    sun_up = cross(sun_right, sun_dir);
  }

  const int nr = 16;
  const int ntheta = 16;

  for (unsigned i = 1 ; i < nr*2 ; ++i)
  {
    float r0 = pow((i - 1.0)/(nr - 1), 2) * radius;
    float r1 = pow(i/(nr - 1.0f), 2) * radius;
    float z0 = cloud_height * (1.0 - sqrt(r0 / radius));
    float z1 = cloud_height * (1.0 - sqrt(r1 / radius));
        
    float f0 = atan_deg(z0/r0) / 90.0f;
    float f1 = atan_deg(z1/r1) / 90.0f;
        
//        TRACE("r0 = %f, r1 = %f, z0 = %f, f0 = %f\n", r0, r1, z0, f0);
        
    glBegin(GL_QUAD_STRIP);
    for (int j = 0 ; j < ntheta ; ++j)
    {
      float t0 = j * 360.0f / (ntheta - 1);
          
      float x0 = r0 * sin_deg(t0);
      float y0 = r0 * cos_deg(t0);
      float x1 = r1 * sin_deg(t0);
      float y1 = r1 * cos_deg(t0);
          
      Vector3 col0 = f0 * col_top + (1.0f - f0) * col_bot;
      Vector3 col1 = f1 * col_top + (1.0f - f1) * col_bot;
          
      if (do_colour)
        glColor3fv(col0.get_data());
      if (do_sun_tex_coords)
      {
        Vector3 v(x0, y0, z0);
        v.normalise();
        // fraction of distance to sun
        float x = dot(sun_right, v);
        float y = dot(sun_up, v);

        x += 0.5;
        y += 0.5;

        if (x > 1.0f) x = 1.0f;
        if (x < 0.0f) x = 0.0f;
        if (y > 1.0f) y = 1.0f;
        if (y < 0.0f) y = 0.0f;

        glTexCoord2f(x, y);
      }
      glVertex3f(x0, y0, z0);

      if (do_colour)
        glColor3fv(col1.get_data());
      if (do_sun_tex_coords)
      {
        Vector3 v(x1, y1, z1);
        v.normalise();
        float x = dot(sun_right, v);
        float y = dot(sun_up, v);

        x += 0.5;
        y += 0.5;

        if (x > 1.0f) x = 1.0f;
        if (x < 0.0f) x = 0.0f;
        if (y > 1.0f) y = 1.0f;
        if (y < 0.0f) y = 0.0f;

        glTexCoord2f(x, y);
      }
      glVertex3f(x1, y1, z1);
    }
    glEnd();
  }
 
  
}


//=====================================================+
//               
// Method/Name : draw_sky
//               
// Description : 
//               
//=====================================================+
void Environment::draw_sky()
{
  // Currently three choices - panorama, skybox, or dynamic clouds.
//=====================================================+
// Dynamic clouds
//=====================================================+
  if (Sss::instance()->config().sky_mode == Config::SKY_DYNAMIC)
  {
    static GLuint list_sky_colour = glGenLists(1);
    static GLuint list_sky_plain = glGenLists(1);

    float xmid = Sss::instance()->eye().get_eye()[0];
    float ymid = Sss::instance()->eye().get_eye()[1];
    
    const float horizon_offset = 0.0f;
    
    static vector<Grey_texture *> cloud_textures;
    static Image_texture * sun_texture = 0;

    static bool initialised = false;
    if (initialised == false)
    {
      initialised = true;

      // make sure the textures are random
      srand((int) time( NULL ));
    
      glNewList(list_sky_colour, GL_COMPILE);
      draw_sky_dome(2000.0f - horizon_offset,
                    Sss::instance()->config().clip_far * 0.95f,
                    true,
                    true, 
                    Vector3(light_pos[0], light_pos[1], light_pos[2]));
      glEndList();      
      
      glNewList(list_sky_plain, GL_COMPILE);
      draw_sky_dome(2000.0f - horizon_offset,
                    Sss::instance()->config().clip_far * 0.95f,
                    false,
                    false, 
                    Vector3(light_pos[0], light_pos[1], light_pos[2]));
      glEndList();      
      
      // number of cloud textures are checked each frame - they can
      // increase
      sun_texture = generate_sun_texture(256, 0.02f, 0.05f);
    }

    int num_to_do = Sss::instance()->config().dynamic_cloud_layers -
      cloud_textures.size();
    for (int tex_i = 0 ; 
         tex_i < num_to_do ;
         ++tex_i)
    {
      // now prepare the cloud textures
      Grey_texture * texture = generate_cloud_texture(
        128,
        0.6f, // noise
        0.2f, // bump frac
        Vector3(0, 1, 1));
      
      cloud_textures.push_back(texture);
    }

    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    
    if (Sss::instance()->config().fog == true)
      glEnable(GL_FOG);
    else
      glDisable(GL_FOG);

    glDisable(GL_LIGHTING);

    glTranslatef(xmid, ymid, horizon_offset);
    
    // we call everything in the right order - avoid nasty artifacts
    // by disabling the depth buffer
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glPolygonMode(GL_FRONT, GL_FILL);

    // sun texture
    if (Sss::instance()->config().texture_level > 2)
    {
      glEnable(GL_TEXTURE_2D);
      
      glBindTexture(GL_TEXTURE_2D, sun_texture->get_high_texture());
      
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
      
      glDisable(GL_TEXTURE_GEN_S);
      glDisable(GL_TEXTURE_GEN_T);
    }

    // The basic sky dome
    glCallList(list_sky_colour);

//     glPolygonMode(GL_FRONT, GL_LINE);
//     glCallList(list_sky_plain);
    

    // cloud textures
    if (Sss::instance()->config().texture_level > 3)
    {
    
      // the different layers move at different speeds/directions.
      Velocity basic_wind_vel = get_ambient_wind(Position(0, 0, 500));
      float wind_dir_min = 0;
      float wind_dir_max = 40;
      float wind_scale_min = 8.0f;
      float wind_scale_max = 4.0f;
    
      // size of each cloud tile
      const float tex_size = 3000.0f;
      const float inv_size = 1.0f/tex_size;

      const int num_layers = Sss::instance()->config().dynamic_cloud_layers;

      glEnable(GL_TEXTURE_2D);
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      
      for (int cloud_i = 0 ; 
           cloud_i < num_layers ; 
           ++cloud_i)
      {
        assert1(cloud_i < (int) cloud_textures.size());

        float wind_dir = wind_dir_min + 
          (wind_dir_max - wind_dir_min) * cloud_i / num_layers;
        float wind_scale = wind_scale_min + 
          (wind_scale_max - wind_scale_min) * cloud_i / num_layers;

        Velocity wind_vel = wind_scale * gamma(wind_dir) * basic_wind_vel;
        const float time = Sss::instance()->get_seconds();
        const float dx = wind_vel[0] * time * inv_size;
        const float dy = wind_vel[1] * time * inv_size;
      
        glBindTexture(GL_TEXTURE_2D,
                      cloud_textures[cloud_i]->get_high_texture());
      
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        
        GLfloat plane_x[] = {inv_size, 0, 0, -dx};
        GLfloat plane_y[] = {0, inv_size, 0, -dy};
        glTexGenfv(GL_S, GL_OBJECT_PLANE, plane_x);
        glTexGenfv(GL_T, GL_OBJECT_PLANE, plane_y);
        glCallList(list_sky_plain);
    
      }
      glDisable(GL_BLEND);
    }

    if (Sss::instance()->config().texture_level > 2)
    {
      glDisable(GL_TEXTURE_2D);
      glDisable(GL_TEXTURE_GEN_S);
      glDisable(GL_TEXTURE_GEN_T);
    }
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // we will want to store the position, so that we can calculate
    // the lens flare...
    const float sun_range = Sss::instance()->config().clip_far;
    float R = sqrt(light_pos[0]*light_pos[0] +
                   light_pos[1]*light_pos[1] +
                   light_pos[2]*light_pos[2]);
    
    float sun_x = light_pos[0] * sun_range/R;
    float sun_y = light_pos[1] * sun_range/R;
    float sun_z = light_pos[2] * sun_range/R;
    GLint viewport[4];
    GLdouble model_matrix[16], proj_matrix[16];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, model_matrix);
    glGetDoublev(GL_PROJECTION_MATRIX, proj_matrix);
    if ( (gluProject(sun_x,
                     sun_y, 
                     sun_z,
                     model_matrix,
                     proj_matrix,
                     viewport,
                     &sun_win_x, &sun_win_y, &sun_win_z)) != GL_TRUE)
    {
      TRACE("gluProject error!\n");
    }

    glPopMatrix();
    glPopAttrib();
  }
  else if (Sss::instance()->config().sky_mode == Config::SKY_PANORAMA)
  {
    float xmid = Sss::instance()->eye().get_eye()[0];
    float ymid = Sss::instance()->eye().get_eye()[1];
    
    // set up the display lists
    static GLuint list_num_roof, list_num_sides;
    static bool initialised = false;
    if (initialised == false)
    {
      initialised = true;
      
      const float radius = Sss::instance()->config().clip_far * 0.5f;
      const float height = 0.4 * radius;
      list_num_roof = glGenLists(1);
      list_num_sides = glGenLists(1);
      
      // roof
      glNewList(list_num_roof, GL_COMPILE);
      glBegin(GL_QUADS);
      const float crop = 0.083f;
      glTexCoord2f(1-crop,1-crop); 
      glVertex3f(-radius, -radius, height);
      glTexCoord2f(crop, 1-crop);
      glVertex3f(radius, -radius,  height);
      glTexCoord2f(crop, crop);
      glVertex3f(radius, radius, height); 
      glTexCoord2f(1-crop, crop);
      glVertex3f(-radius, radius, height);
      glEnd();
      glEndList(); // roof
      
      // sides
      glNewList(list_num_sides, GL_COMPILE);
      glBegin(GL_QUAD_STRIP);
      int n, nsides = 64;
      float angleoffset = 180.0f;
      for (n = 0; n < nsides; n++) 
      {
        float frac = ((float) n)/((float) nsides -1);
        float angle = frac * 360.0f + angleoffset;
        float sar = radius * sin_deg(angle);
        float car = radius * cos_deg(angle);
        glTexCoord2f(1-frac, 0.0f);
        glVertex3f(sar, car, 0);
        glTexCoord2f(1-frac, 1.0f);
        glVertex3f(sar, car, 1.0f * height);
      }
      glEnd();
      glEndList(); // sides
    }
    
    // get the textures if necessary
    if (panorama_top_texture == 0)
      panorama_top_texture = new Rgba_file_texture(
        Sss::instance()->config().panorama_texture + "_roof.png",
        Rgba_file_texture::CLAMP);
    if (panorama_sides_texture == 0)
      panorama_sides_texture = new Rgba_file_texture(
        Sss::instance()->config().panorama_texture + "_panorama.png",
        Rgba_file_texture::CLAMP);
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_2D);
    // make sure that we don't get nasty depth artifacts - we
    // draw the sky first anyway, and everything else should
    // be in front of it.
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glPushMatrix();
    {
      // Move to centre of the box 
      glTranslatef(xmid, ymid, 0);
      
      // do the roof
      glBindTexture(GL_TEXTURE_2D, panorama_top_texture->get_high_texture());
      glCallList(list_num_roof);
      
      // do the sides
      glBindTexture(GL_TEXTURE_2D, panorama_sides_texture->get_high_texture());
      glCallList(list_num_sides);
    }
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    return;
  }
  else if (Sss::instance()->config().sky_mode == Config::SKY_SKYBOX)
  {
    const Position& viewpos = Sss::instance()->eye().get_eye();
    
    // set up the display lists
    static GLuint list_skybox = glGenLists(6);
    
    static Vector3 view_dirs[6] = 
      {
        Vector3(0, 1, 0), // left
        Vector3(-1, 0, 0),// back
        Vector3(0, -1, 0),// right
        Vector3(1, 0, 0), // fwd
        Vector3(0, 0, 1), // up
        Vector3(0, 0, -1) // down
      };
    static Vector3 view_ups[6] = 
      {
        Vector3(0, 0, 1),
        Vector3(0, 0, 1),
        Vector3(0, 0, 1),
        Vector3(0, 0, 1),
        Vector3(0, -1, 0),
        Vector3(0, 1, 0)
      };

    static bool initialised = false;
    if (initialised == false)
    {
      initialised = true;
      
      // get the textures if necessary
      sky_textures[0] = new Rgba_file_texture(
        Sss::instance()->config().skybox_texture + "_front.png",
        Rgba_file_texture::CLAMP_TO_EDGE, Rgba_file_texture::RGB, Rgba_file_texture::GENERATE_HIGH);
      sky_textures[1] = new Rgba_file_texture(
        Sss::instance()->config().skybox_texture + "_left.png",
        Rgba_file_texture::CLAMP_TO_EDGE, Rgba_file_texture::RGB, Rgba_file_texture::GENERATE_HIGH);
      sky_textures[2] = new Rgba_file_texture(
        Sss::instance()->config().skybox_texture + "_back.png",
        Rgba_file_texture::CLAMP_TO_EDGE, Rgba_file_texture::RGB, Rgba_file_texture::GENERATE_HIGH);
      sky_textures[3] = new Rgba_file_texture(
        Sss::instance()->config().skybox_texture + "_right.png",
        Rgba_file_texture::CLAMP_TO_EDGE, Rgba_file_texture::RGB, Rgba_file_texture::GENERATE_HIGH);
      sky_textures[4] = new Rgba_file_texture(
        Sss::instance()->config().skybox_texture + "_up.png",
        Rgba_file_texture::CLAMP_TO_EDGE, Rgba_file_texture::RGB, Rgba_file_texture::GENERATE_HIGH);
      sky_textures[5] = new Rgba_file_texture(
        Sss::instance()->config().skybox_texture + "_down.png",
        Rgba_file_texture::CLAMP_TO_EDGE, Rgba_file_texture::RGB, Rgba_file_texture::GENERATE_HIGH);

      for (unsigned i = 0 ; i < 6 ; ++i)
      {
        glNewList(list_skybox + i, GL_COMPILE);
        {
          float d = 10.0f;
          Position p = d * view_dirs[i]; // the middle of the quad
          Vector3 left = d * cross(view_ups[i], view_dirs[i]);
          const Vector3 up = d * view_ups[i];
          Position p0 = p + left + up;
          Position p1 = p + left - up;
          Position p2 = p - left - up;
          Position p3 = p - left + up;

          glBegin(GL_QUADS);
          glTexCoord2f(0.0f, 1.0f);
          glVertex3fv(p0.get_data());
          glTexCoord2f(0.0f, 0.0f);
          glVertex3fv(p1.get_data());
          glTexCoord2f(1.0f, 0.0f);
          glVertex3fv(p2.get_data());
          glTexCoord2f(1.0f, 1.0f);
          glVertex3fv(p3.get_data());
          glEnd();
          glEndList();
        }
        
      } // loop over sides
    } // initialised

    // Clear to skybox
    glClearDepth(1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glPushMatrix();
    {
      // Move to centre of the box 
      bool skipTest = false;
      glTranslatef(viewpos[0], viewpos[1], viewpos[2]);

      // if drawing terrain and skybox rotate skybox so it doesn't look crap - need to 
      // hack the light dir too!
      static float orig_light_pos[2] = {light_pos[0], light_pos[1]};
      light_pos[0] = orig_light_pos[0];
      light_pos[1] = orig_light_pos[1];
      if (Sss::instance()->config().use_terragen_terrain && 
          (Object*) (&Sss::instance()->eye()) == (Object*) (&Sss::instance()->glider()))
      {
        glRotatef(180.0f, 0, 0, 1);
        skipTest = true;
        light_pos[0] = -orig_light_pos[0];
        light_pos[1] = -orig_light_pos[1];
      }
      for (unsigned i = 0 ; i < 6 ; ++i)
      {
        Vector3 eye_dir = Sss::instance()->eye().get_eye_target() - Sss::instance()->eye().get_eye();
        if (skipTest || dot(view_dirs[i], eye_dir) > -0.7f)
        {
          glBindTexture(GL_TEXTURE_2D, sky_textures[i]->get_high_texture());
          glCallList(list_skybox + i);
        }
      }
    }
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_MATERIAL);
    return;
  }
  else if (Sss::instance()->config().sky_mode == Config::SKY_NONE)
  {
  }
  else
  {
    TRACE("Unhandled sky mode %d\n", Sss::instance()->config().sky_mode);
  } // end of big if
}

void Environment::draw_lens_flare()
{
  if (Sss::instance()->config().sky_mode != Config::SKY_DYNAMIC)
    return;
  
  if (Sss::instance()->config().texture_level > 3)
  {
    if (flare_texture == 0)
      flare_texture = new Flare_texture();
    
    int win_width = glutGet(GLUT_WINDOW_WIDTH);
    int win_height = glutGet(GLUT_WINDOW_HEIGHT);
    int win_mid_x = win_width/2;
    int win_mid_y = win_height/2;
    
    if ( (sun_win_x > 0) &&
         (sun_win_y > 0) &&
         (sun_win_x < win_width) &&
         (sun_win_y < win_height) &&
         (sun_win_z < 1.0f) )
    {
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      
      glLoadIdentity();
      gluOrtho2D(0, win_width, 0, win_height);
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      
      glLoadIdentity();
      
      glPushAttrib(GL_ALL_ATTRIB_BITS);
      
      // output text
      glDisable(GL_FOG);
      glDisable(GL_LIGHTING);
      glDisable(GL_DEPTH_TEST);
      
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      
      glEnable(GL_TEXTURE_2D);
      
//      // draw the sun
      
      glBindTexture(GL_TEXTURE_2D, flare_texture->get_texture());
      
      glDisable(GL_TEXTURE_GEN_S);
      glDisable(GL_TEXTURE_GEN_T);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      // loop over elements
      
//        const float flare_pos[] = {
//          -0.9f, -0.7f, -0.3f, 0.5f, 1.3f, 2.9f
//        };
//        const float flare_rad[] = {
//          .7f, .9f, 1.2f, 1.5f, 1.9f, 2.7f
//        };
      const float flare_pos[] = {
        -0.7f, -0.5f, -0.3f, -0.1f, 0.2f, 0.4f, 0.5f, 0.65f, 0.9f, 1.3f, 2.9f
      };
      const float flare_rad[] = {
        .2f,    .1f,   0.3f,  0.3f, 0.2f, 0.2f, 0.4f, 0.35f, 0.2f, 0.4f, 0.8f
      };
      
      const float red[] = {
        .8f,    1,     1,   .7f,  .6f,   .6f,  1,    .7f,    .8f,   .6f,   1
      };
      const float green[] = {
        1,    .7f,    .8f,    .6f,  .5f,   .7f,  .6f,    1,    .7f,   .6f,  .7f
      };
      const float blue[] = {
        1,    .8f,    .6f,    .8f,  .9f,   1,  .5f,    .6f,    .7f,   .8f,   1
      };
      
      float dx = sun_win_x - win_mid_x;
      float dy = sun_win_y - win_mid_y;
      float dist = sss_max(fabs(dx)/win_mid_x, fabs(dy)/win_mid_y);
      float alpha = 0.25f*(1-dist);
      
      unsigned int i;
      for (i = 0 ; i < sizeof(flare_pos)/sizeof(flare_pos[0]) ; ++i)
      {
        //float radius_x = flare_pos[i] * win_width/10.0f;
        float radius_x = flare_rad[i] * 0.7f * win_width/10.0f;
        float radius_y = radius_x;
        
        glColor4f(red[i], green[i], blue[i], alpha);
        
        draw_lens_flare_element(win_mid_x - dx*flare_pos[i],
                                win_mid_y - dy*flare_pos[i],
                                radius_x, radius_y);
      }
      
      glDisable(GL_TEXTURE_2D);
      
      // Finally, add a fog to represent the overall sun glare
      glBegin(GL_QUADS);
      glColor4f(1,1,1,(1-dist)*(1-dist));
      glVertex2f(0,         0         );
      glVertex2f(win_width, 0         );
      glVertex2f(win_width, win_height);
      glVertex2f(0,         win_height);
      glEnd();
      
      glDisable(GL_BLEND);
      
      // undo
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      glPopAttrib();
    }
  }
}

void Environment::draw_lens_flare_element(float x, float y,
                                          float radius_x, float radius_y)
{
  glBegin(GL_QUADS);
  glTexCoord2f(0, 0);
  glVertex2f(x-radius_x, y+radius_y);
  glTexCoord2f(1, 0);
  glVertex2f(x+radius_x, y+radius_y);
  glTexCoord2f(1, 1);
  glVertex2f(x+radius_x, y-radius_y);
  glTexCoord2f(0, 1);
  glVertex2f(x-radius_x, y-radius_y);
  glEnd();
}

//==============================================================
// draw_terragen_ground
//==============================================================
void Environment::draw_terragen_ground(Object* blocker)
{
  // make sure terrain clips/does not clip as appropriate
  if (Sss::instance()->config().clip == true)
  {
    ground_vertex_array->set_clipping(
//      dynamic_cast<const Object &>(Sss::instance()->body()),
      Sss::instance()->eye(),
      Sss::instance()->config().fov,
      (float) Sss::instance()->config().window_x/ /* aspect ratio */ 
      Sss::instance()->config().window_y,
      Sss::instance()->config().clip_near,
      Sss::instance()->config().clip_far);
  }
  else
  {
    ground_vertex_array->set_clipping(false);
  }

//  sea_altitude = -1000;
  glDisable(GL_LIGHTING);
  glFrontFace(GL_CW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  if (Sss::instance()->config().texture_level < 3)
  {
    // for debugging
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glDisable(GL_TEXTURE_2D);
    glColor4f(1, 1, 1, 1);
  }
  else if (Sss::instance()->config().shadow == false)
  {
    glDisable(GL_TEXTURE_2D);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  }
  else
  {
    const Position & pos = blocker->get_pos();
    // the shadow
    assert1(shadow_texture != 0);
  
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, shadow_texture->get_texture());
    
    glColor4f(1, 1, 1, 1);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
  
    // work out stuff for automatic texture coords
    Vector3 towards = Vector3(-light_pos[0], -light_pos[1], -light_pos[2]);
    // assume that "up" is not parallel to towards
    // normal for texture "x" coord
    Vector3 Nx = cross(towards, Vector3(0,0,1)).normalise();
    // normal for texture "y" coord
    Vector3 Ny = cross(Nx, towards).normalise();
  
    // hmmm why the factor of two? (needed to get the shadow size
    // right)
    float inv_range = 0.5f/blocker->get_graphical_bounding_radius();
  
    float Dx = -(Nx[0]*pos[0] + Nx[1]*pos[1] + Nx[2]*pos[2]);
    GLfloat planes_x[]
      = { Nx[0]*inv_range, Nx[1]*inv_range, Nx[2]*inv_range, 0.5f+Dx*inv_range }; 
  
    float Dy = -(Ny[0]*pos[0] + Ny[1]*pos[1] + Ny[2]*pos[2]);
    GLfloat planes_y[] 
      = { Ny[0]*inv_range, Ny[1]*inv_range, Ny[2]*inv_range, 0.5f+Dy*inv_range }; 
  
    //          GLfloat planes_x[] = {1/range, 0, 0, 0.5f-pos[0]/range}; 
    //          GLfloat planes_y[] = {0, 1/range, 0, 0.5f-pos[1]/range};
    glTexGenfv(GL_S, GL_OBJECT_PLANE, planes_x);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, planes_y);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
  }

  ground_vertex_array->mesh_refine(Sss::instance()->eye().get_eye(),
                                   Sss::instance()->config().lod,
                                   false,
                                   false);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  glDisable(GL_TEXTURE_2D);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glDisable(GL_BLEND);
  glFrontFace(GL_CCW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}


//==============================================================
// draw_ground
//==============================================================
void Environment::draw_ground(Object * blocker)
{
  if (Sss::instance()->config().use_terragen_terrain && 
      (Object*) (&Sss::instance()->eye()) != (Object*) (&Sss::instance()->glider()))
  {
    draw_terragen_ground(blocker);
    return;
  }
  
  static GLuint list_num_sea;
  
  // make sure terrain clips/does not clip as appropriate
  if (Sss::instance()->config().clip == true)
  {
    ground_vertex_array->set_clipping(
//      dynamic_cast<const Object &>(Sss::instance()->body()),
      Sss::instance()->eye(),
      Sss::instance()->config().fov,
      (float) Sss::instance()->config().window_x/ /* aspect ratio */ 
      Sss::instance()->config().window_y,
      Sss::instance()->config().clip_near,
      Sss::instance()->config().clip_far);
  }
  else
  {
    ground_vertex_array->set_clipping(false);
  }
  
  static bool init = false;
  if (init == false)
  {
    init = true;
    
    // prepare the sea
    list_num_sea = glGenLists(1);
    glNewList(list_num_sea, GL_COMPILE);
    gluDisk(Renderer::instance()->quadric(),
            0, Sss::instance()->config().clip_far, 8, 8);
    glEndList();
    
  }
  
  glPushMatrix();
  glFrontFace(GL_CW);
  
  glDisable(GL_LIGHTING);
  
  // in case texturing and light is off
  glColor4f(1, 1, 1, 1);
  
  glEnable(GL_DEPTH_TEST);
  
  if (Sss::instance()->config().fog == true)
    glEnable(GL_FOG);
  else
    glDisable(GL_FOG);
  
  if (Sss::instance()->config().texture_level > 2)
  {
    // main terrain map scaling
    /// @todo take these from file
    float ax = 0.005f;
    float dx = 0;
    float by = 0.005f;
    float dy = 0;
    
    if (false == Sss::instance()->config().tile_ground_texture)
    {
      // need to set the mapping so that the generated coordinate
      // goes from 0 to 1 across the range of x and y
      // i.e. ax*xmin + dx = 0
      //      ax*xmax + dx = 1
      int n = ground_vertex_array->get_nx();
      Position pos_min = ground_vertex_array->get_pos(0, 0);
      Position pos_max = ground_vertex_array->get_pos(n-1, n-1);
      float xmax = pos_max[0];
      float ymax = pos_max[1];
      float xmin = pos_min[0];
      float ymin = pos_min[1];
      ax = 1.0f/(xmax - xmin);
      dx = - ax * xmin;
      by = 1.0f/(ymax - ymin);
      dy = - by * ymin;
    }
    
    if (ground_texture == 0)
    {
      ground_texture = new Rgba_file_texture(Sss::instance()->config().ground_texture_file,
                                             Rgba_file_texture::REPEAT);
    }
    
#ifdef WITH_GL_EXT
    // multitexturing, or normal?
    if ((Sss::instance()->config().texture_level >= 5) && 
        (multitextureSupported == true) )
    {
      // the background
      glActiveTextureARB(GL_TEXTURE0_ARB);
      glEnable(GL_TEXTURE_2D);
      
      glBindTexture(GL_TEXTURE_2D, ground_texture->get_high_texture());
      
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      
      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
      
      GLfloat plane_x[] = {ax, 0, 0, dx}; // smaller -> bigger blocks
      GLfloat plane_y[] = {0, by, 0, dy};
      glTexGenfv(GL_S, GL_OBJECT_PLANE, plane_x);
      glTexGenfv(GL_T, GL_OBJECT_PLANE, plane_y);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      
      if (Sss::instance()->config().shadow == false)
      {
        //  The detail
        if (ground_texture_detail == 0)
//           ground_texture_detail = new Rgba_file_texture(
//             Sss::instance()->config().ground_detail_texture_file,
//             Rgba_file_texture::REPEAT,
//             Rgba_file_texture::LUM);
          ground_texture_detail = new Detail_texture(
            256, 256,
            0.5f,
            0.7f,
            1.0f,
            Vector3(light_pos[0], light_pos[1], light_pos[2]));
        
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glEnable(GL_TEXTURE_2D);
        
        glBindTexture(GL_TEXTURE_2D, ground_texture_detail->get_high_texture());
        
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        GLfloat planed_x[] = {0.15f, 0, 0, 0}; // smaller -> bigger blocks
        GLfloat planed_y[] = {0, 0.15f, 0, 0};
        glTexGenfv(GL_S, GL_OBJECT_PLANE, planed_x);
        glTexGenfv(GL_T, GL_OBJECT_PLANE, planed_y);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
      }
      else
      {
        const Position & pos = blocker->get_pos();
        // the shadow
        assert1(shadow_texture != 0);
        
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glEnable(GL_TEXTURE_2D);
        
        glBindTexture(GL_TEXTURE_2D, shadow_texture->get_texture());
        
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
        
        // work out stuff for automatic texture coords
        Vector3 towards = Vector3(-light_pos[0], -light_pos[1], -light_pos[2]);
        // assume that "up" is not parallel to towards
        // normal for texture "x" coord
        Vector3 Nx = cross(towards, Vector3(0,0,1)).normalise();
        // normal for texture "y" coord
        Vector3 Ny = cross(Nx, towards).normalise();
        
        // hmmm why the factor of two? (needed to get the shadow size
        // right)
        float inv_range = 0.5f/blocker->get_graphical_bounding_radius();
        
        float Dx = -(Nx[0]*pos[0] + Nx[1]*pos[1] + Nx[2]*pos[2]);
        GLfloat planes_x[]
          = { Nx[0]*inv_range, Nx[1]*inv_range, Nx[2]*inv_range, 0.5f+Dx*inv_range }; 
        
        float Dy = -(Ny[0]*pos[0] + Ny[1]*pos[1] + Ny[2]*pos[2]);
        GLfloat planes_y[] 
          = { Ny[0]*inv_range, Ny[1]*inv_range, Ny[2]*inv_range, 0.5f+Dy*inv_range }; 
        
//          GLfloat planes_x[] = {1/range, 0, 0, 0.5f-pos[0]/range}; 
//          GLfloat planes_y[] = {0, 1/range, 0, 0.5f-pos[1]/range};
        glTexGenfv(GL_S, GL_OBJECT_PLANE, planes_x);
        glTexGenfv(GL_T, GL_OBJECT_PLANE, planes_y);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
      }
      
    }
    else
#endif
    { // single texturing
      
      glEnable(GL_TEXTURE_2D);
      
      if (Sss::instance()->config().texture_level == 3)
        glBindTexture(GL_TEXTURE_2D, ground_texture->get_low_texture());
      else // must be 4
        glBindTexture(GL_TEXTURE_2D, ground_texture->get_high_texture());
      
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      
      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
      GLfloat plane_x[] = {ax, 0, 0, dx}; // smaller -> bigger blocks
      GLfloat plane_y[] = {0, by, 0, dy};
      glTexGenfv(GL_S, GL_OBJECT_PLANE, plane_x);
      glTexGenfv(GL_T, GL_OBJECT_PLANE, plane_y);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      
    }
  }
  
  // Having set up the textures, draw the terrain
  
  if (Sss::instance()->config().texture_level == 0)
  {
    glColor4f(1, 1, 1, 1);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    ground_vertex_array->mesh_refine(Sss::instance()->eye().get_eye(),
                                     Sss::instance()->config().lod);
    ground_vertex_array->plot_plain();
    
    
  }
  else if (Sss::instance()->config().texture_level == 1)
  {
    GLenum orig_shading;
    glGetIntegerv(GL_SHADE_MODEL, (GLint *) &orig_shading);
    glShadeModel(GL_FLAT);
    
    // do the lines first
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glColor4f(0, 0, 0, 1);
    // multi-pass - first as normal... but compile into a "display" list 
    // ready for the second pass
    ground_vertex_array->clear_saved();
    ground_vertex_array->mesh_refine(Sss::instance()->eye().get_eye(),
                                     Sss::instance()->config().lod, 
                                     false, // colour
                                     true); // record the points
    ground_vertex_array->plot_plain(true);
    
    // no the filled triangles
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glColor4f(0.5f, 0.4f, 0.3f, 1);
    
    ground_vertex_array->plot_saved();
    
    glDisable(GL_POLYGON_OFFSET_FILL);
    glShadeModel(orig_shading);
  }
  else if (Sss::instance()->config().texture_level == 2)
  {
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    ground_vertex_array->mesh_refine(Sss::instance()->eye().get_eye(),
                                     Sss::instance()->config().lod,
                                     true); // do colouring
    ground_vertex_array->plot_plain();
    
  }
  else if (Sss::instance()->config().texture_level >= 6)
  {
    
    // multi-pass - first as normal... but compile into a "display" list 
    // ready for the second pass
    ground_vertex_array->clear_saved();
    ground_vertex_array->mesh_refine(Sss::instance()->eye().get_eye(),
                                     Sss::instance()->config().lod, 
                                     false, // colour
                                     true); // record the points
    ground_vertex_array->plot_plain(true);
    
    // Multi-texturing and mult-pass. The first pass we either did 
    // basic+shadow or basic+detail. The second pass we use a lightmap for 
    // the terrain, plus either the original detail or some other detail...
    
    if (terrain_lightmap_texture == 0)
      calculate_terrain_lightmap();
    
#ifdef WITH_GL_EXT
    glActiveTextureARB(GL_TEXTURE0_ARB);
#endif
    glEnable(GL_TEXTURE_2D);
    
    glBindTexture(GL_TEXTURE_2D, terrain_lightmap_texture->get_texture());
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    // need to set the mapping so that the generated coordinate
    // goes from 0 to 1 across the range of x and y
    // i.e. ax*xmin + dx = 0
    //      ax*xmax + dx = 1
    int n = ground_vertex_array->get_nx();
    Position pos_min = ground_vertex_array->get_pos(0, 0);
    Position pos_max = ground_vertex_array->get_pos(n-1, n-1);
    float xmax = pos_max[0];
    float ymax = pos_max[1];
    float xmin = pos_min[0];
    float ymin = pos_min[1];
    float ax = 1.0f/(xmax - xmin);
    float dx = - ax * xmin;
    float by = 1.0f/(ymax - ymin);
    float dy = - by * ymin;
    GLfloat plane_x[] = {ax, 0, 0, dx}; // smaller -> bigger blocks
    GLfloat plane_y[] = {0, by, 0, dy};
    glTexGenfv(GL_S, GL_OBJECT_PLANE, plane_x);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, plane_y);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    
#ifdef WITH_GL_EXT
    // we have another texture unit to play with. If we're doing the
    // glider shadow we can use the detail texture. Otherwise we use
    // the detail again, but at a much higher resolution.
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glEnable(GL_TEXTURE_2D);
    //  The detail
    if (ground_texture_detail == 0)
//       ground_texture_detail = new Rgba_file_texture(
//                Sss::instance()->config().ground_detail_texture_file,
//                Rgba_file_texture::REPEAT,
//                Rgba_file_texture::LUM);
      ground_texture_detail = new Detail_texture(
        256, 256,
        0.5f,
        0.7f,
        1.0f,
        Vector3(light_pos[0], light_pos[1], light_pos[2]));
    
    glBindTexture(GL_TEXTURE_2D, ground_texture_detail->get_high_texture());
    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    
    // This time we're doing the ultra-fine detail
    if (Sss::instance()->config().shadow == false)
    {
      GLfloat planed_x[] = {0.015f, 0, 0, 0}; // smaller -> bigger blocks
      GLfloat planed_y[] = {0, 0.015f, 0, 0};
      glTexGenfv(GL_S, GL_OBJECT_PLANE, planed_x);
      glTexGenfv(GL_T, GL_OBJECT_PLANE, planed_y);
    }
    else
    {
      GLfloat planed_x[] = {0.15f, 0, 0, 0}; // smaller -> bigger blocks
      GLfloat planed_y[] = {0, 0.15f, 0, 0};
      glTexGenfv(GL_S, GL_OBJECT_PLANE, planed_x);
      glTexGenfv(GL_T, GL_OBJECT_PLANE, planed_y);
    }
    
    // Finally, if we have the texture units, and I can think of
    // something to put in them(!) - do the very fine detail (which
    // won't have been done yet if we drew the shadow in the first
    // pass).
    if ( (Sss::instance()->config().texture_level == 7) &&
         (Sss::instance()->config().shadow == true) && 
         (maxTexelUnits >= 3) )
    {
      glActiveTextureARB(GL_TEXTURE2_ARB);
      glEnable(GL_TEXTURE_2D);
      //  The detail
      if (ground_texture_detail == 0)
//         ground_texture_detail = new Rgba_file_texture(
//           Sss::instance()->config().ground_detail_texture_file,
//           Rgba_file_texture::REPEAT,
//           Rgba_file_texture::LUM);
        ground_texture_detail = new Detail_texture(
          256, 256,
          0.5f,
          0.7f,
          1.0f,
          Vector3(light_pos[0], light_pos[1], light_pos[2]));
      
      glBindTexture(GL_TEXTURE_2D, ground_texture_detail->get_high_texture());
      
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      
      glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
      glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
      glEnable(GL_TEXTURE_GEN_S);
      glEnable(GL_TEXTURE_GEN_T);
      GLfloat planed_x[] = {0.11f, 0, 0, 0}; // smaller -> bigger blocks
      GLfloat planed_y[] = {0, 0.11f, 0, 0};
      glTexGenfv(GL_S, GL_OBJECT_PLANE, planed_x);
      glTexGenfv(GL_T, GL_OBJECT_PLANE, planed_y);
    }
#endif // multitexturing available?
    
    glDepthFunc(GL_EQUAL);
    glEnable(GL_BLEND);
//    glBlendFunc(GL_ONE_MINUS_SRC_COLOR, GL_ONE);
    glBlendFunc(GL_DST_COLOR, GL_ZERO);
    
    // plot the saved points
    ground_vertex_array->plot_saved();
//    ground_vertex_array->mesh_refine(Sss::instance()->eye().get_eye(),
//                                     Sss::instance()->config().lod);
//    ground_vertex_array->plot_plain();
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
  }
  else
  {
    ground_vertex_array->mesh_refine(Sss::instance()->eye().get_eye(),
                                     Sss::instance()->config().lod);
    ground_vertex_array->plot_plain();
  }
  
  //=================================================================
  // The plain
  //=================================================================
  
  
  // get rid of multi-texturing
  
#ifdef WITH_GL_EXT
  if ((Sss::instance()->config().texture_level >= 5) && 
      (multitextureSupported == true) )
  {
    if ( (Sss::instance()->config().texture_level == 7) &&
         (maxTexelUnits >= 3) )
    {
      glActiveTextureARB(GL_TEXTURE2_ARB);
      glDisable(GL_TEXTURE_2D);
    }
    glActiveTextureARB(GL_TEXTURE1_ARB);
    glDisable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0_ARB);
    glDisable(GL_TEXTURE_2D);
  }
  else 
#endif
    if (Sss::instance()->config().texture_level > 2)
    {
      glDisable(GL_TEXTURE_2D);
    }
  
  //=================================================================
  //        Now the beach
  //=================================================================
  
  if (Sss::instance()->config().texture_level > 2)
  {
    glEnable(GL_TEXTURE_2D);
    
    // texture for the beach
    if (sand_texture == 0)
    {
      sand_texture = new Rgba_file_texture(Sss::instance()->config().sand_texture_file,
                                           Rgba_file_texture::REPEAT);
    }
    
    if (Sss::instance()->config().texture_level == 3)
      glBindTexture(GL_TEXTURE_2D, sand_texture->get_low_texture());
    else // must be 2
      glBindTexture(GL_TEXTURE_2D, sand_texture->get_high_texture());
    
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    GLfloat plane_x[] = {0.005f, 0, 0, 0}; // smaller -> bigger blocks
    GLfloat plane_y[] = {0, 0.005f, 0, 0};
    glTexGenfv(GL_S, GL_OBJECT_PLANE, plane_x);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, plane_y);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    
  }
  
  ground_vertex_array->plot_beach(50);
  
  if (Sss::instance()->config().texture_level > 2)
  {
    glDisable(GL_TEXTURE_2D);
  }
  
  // Done plotting the terrain
  
  glFrontFace(GL_CCW);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glPopMatrix();
  
  //=================================================================
  //                        now the sea
  //=================================================================
  
  glPushMatrix();
  
  if (Sss::instance()->config().texture_level > 2)
  {
    if (sea_texture == 0)
      sea_texture = new Rgba_file_texture(Sss::instance()->config().sea_texture_file,
                                          Rgba_file_texture::REPEAT);
    
    glEnable(GL_TEXTURE_2D);
    
    if (Sss::instance()->config().texture_level == 3)
      glBindTexture(GL_TEXTURE_2D, sea_texture->get_low_texture());
    else // must be 2
      glBindTexture(GL_TEXTURE_2D, sea_texture->get_high_texture());
    
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    float eye_x = Sss::instance()->eye().get_eye()[0];
    float eye_y = Sss::instance()->eye().get_eye()[1];
    
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    float sea_scale = 0.002f;
    GLfloat plane_x[] = {sea_scale, 0, 0, eye_x * sea_scale}; // smaller -> bigger blocks
    GLfloat plane_y[] = {0, sea_scale, 0, eye_y * sea_scale};
    glTexGenfv(GL_S, GL_OBJECT_PLANE, plane_x);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, plane_y);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    
    if (Sss::instance()->config().translucent_sea == true)
    {
      const float sea_alt_max = sea_altitude + 0.5f;
      const float sea_alt_min = sea_altitude - 0.5f;
      const float sea_alt_num = 4;
      
      float sea_alt_begin, sea_alt_end;
      
      if (Sss::instance()->eye().get_eye()[2] > get_sea_altitude())
      {
        sea_alt_begin = sea_alt_min;
        sea_alt_end = sea_alt_max;
      }
      else
      {
        sea_alt_begin = sea_alt_max;
        sea_alt_end = sea_alt_min;
      }
      float sea_alt_dz = (sea_alt_end - sea_alt_begin)/sea_alt_num;
      
      // Make the center where the eye is - the horizon is most important.
      glTranslatef(eye_x,
                   eye_y,
                   sea_alt_begin);
      
//      glTranslatef(ground_vertex_array->xmid,
//                   ground_vertex_array->ymid,
//                   sea_alt_begin);
      
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      
      glColor4f(1, 1, 1, 2.0f/sea_alt_num);
      
      for (int i = 0; i < sea_alt_num; ++i)
      {
        glCallList(list_num_sea);
        glTranslatef(0, 0, sea_alt_dz);
        sea_alt_begin += sea_alt_dz;
      }
      
      glDisable(GL_BLEND);
    }
    else
    {
      // Make the center where the eye is - the horizon is most important.
      glTranslatef(eye_x,
                   eye_y,
                   sea_altitude);
      
//     glTranslatef(ground_vertex_array->xmid,
//                   ground_vertex_array->ymid,
//                   sea_altitude);
      
      glCallList(list_num_sea);
    }
    glDisable(GL_TEXTURE_2D);
    
  }
  else if (Sss::instance()->config().texture_level > 0)
  {
    glColor3f(.2f, .5f, .8f);
    glTranslatef(ground_vertex_array->xmid,
                 ground_vertex_array->ymid,
                 sea_altitude);
    glCallList(list_num_sea);
  }
  
  glPopMatrix();
}

void Environment::calculate_terrain_lightmap()
{
  TRACE_METHOD_ONLY(3);
  
  if (terrain_lightmap_texture)
    return;
  
  // @todo get this from config
  const unsigned size = 256;
  const float ambient = 0.2; // ambient lighting
  
  float * image = new float[size*size];
  
  Vector sun_dir = Vector(light_pos[0], 
                          light_pos[1],
                          light_pos[2]).normalise();
  
  int n = ground_vertex_array->get_nx();
  Position pos00 = ground_vertex_array->get_pos(0, 0);
  Position posnn = ground_vertex_array->get_pos(n-1, n-1);
  Position pos11 = ground_vertex_array->get_pos(1, 1);
  
  float xmin = pos00[0];
  float ymin = pos00[1];
  float xmax = posnn[0];
  float ymax = posnn[1];
  float dx = pos11[0] - pos00[0];
  float dy = pos11[1] - pos00[1];
  
  dx *= ((float) size / n);
  dy *= ((float) size / n);
  
  unsigned int i, j;
  for (i = 1 ; i < size-1 ; ++i)
  {
    for (j = 1 ; j < size-1 ; ++j)
    {
      Vector normal;
      Position pos; // junked
      float x = xmin + i * (xmax-xmin) / (size-1);
      float y = ymin + j * (ymax-ymin) / (size-1);
      Position pos_w(x - dx, y, 0);
      Position pos_e(x + dx, y, 0);
      Position pos_s(x, y - dy, 0);
      Position pos_n(x, y + dy, 0);
      set_z(pos_w);
      set_z(pos_e);
      set_z(pos_s);
      set_z(pos_n);
      normal = cross(pos_e - pos_w, pos_n - pos_s).normalise();
      // lightmap is the dot product of the normal and the sun dir
      float val = dot(sun_dir, normal);
      val = ambient + (1-ambient)*val;
      // bit of a hack here... if we use val directly then everything 
      // looks rather dark. So, to maintain contrast, and brightness,
      // we square-root it
      val = pow((float) val, 0.5f);
      image[Lightmap_texture::calc_index(i, j, size, size)] = val;
    }
  }
  
  // zero the edges, to prevent "bleeding" into the plain
  for (i = 0 ; i < size ; ++i)
  {
    image[Lightmap_texture::calc_index(i, 0, size, size)] = 1;
    image[Lightmap_texture::calc_index(i, size-1, size, size)] = 1;
    image[Lightmap_texture::calc_index(0, i, size, size)] = 1;
    image[Lightmap_texture::calc_index(size-1, i, size, size)] = 1;
  }
  terrain_lightmap_texture = new Lightmap_texture(size, size, image);
  delete [] image;
}

// finds the float index for looking up x, y or z.
// very slow...
inline float Environment::find_index(float * ix, int inx, float p) const
{
  if (p >= ix[inx-1])
    return inx-1;
  if (p <= ix[0])
    return 0;
  
// binary search - guaranteed not to lie beyond the range
  int l = 0;
  int r = inx-1;
  while (r >= l)
  {
    int m = (l+r)/2;
    if ( (ix[m] <= p) && (ix[m+1] >= p) )
    {
//        float ret = m + (p-ix[m])/(ix[m+1] - ix[m]);
//        assert1(ret < inx);
//        return ret;
      return (m + (p-ix[m])/(ix[m+1] - ix[m]));
    }
    if (p < ix[m])
      r = m;
    else 
      l = m+1;
    //    assert1(l < inx);
  }
  
  assert1(!"shouldn't get here!");
  return l;
  
/*
// sequential search below
int i;
for (i = 0 ; i < inx ; i++)
{
if (ix[i] > p)
break;
}

if (i == 0)
return 0;
else if (i == nx)
return (inx-1.0f);

float & ix1 = ix[i-1];

float frac = (p-ix1)/(ix[i] - ix1);

return (i-1)+frac;
*/
  }


float Environment::interp(float * u_array, float i, float j, float k) const
{
  const int ii = (int) i;
  int ij = (int) j;
  int ik = (int) k;
  
  const float di = i-ii;
  const float dj = j-ij;
  const float dk = k-ik;
  
  const float dip = 1-di; // for convenience : di' = 1-di
  const float djp = 1-dj;
  const float dkp = 1-dk;
  
  const float & u000 = u_array[calc_index(ii  , ij  , ik  )];
  const float & u100 = u_array[calc_index(ii+1, ij  , ik  )];
  const float & u010 = u_array[calc_index(ii  , ij+1, ik  )];
  const float & u110 = u_array[calc_index(ii+1, ij+1, ik++)]; // inc ik
  const float & u001 = u_array[calc_index(ii  , ij  , ik  )];
  const float & u101 = u_array[calc_index(ii+1, ij++, ik  )]; // ink ij
  const float & u011 = u_array[calc_index(ii  , ij  , ik  )];
  const float & u111 = u_array[calc_index(ii+1, ij  , ik  )];
  
  return
    u000*dip*djp*dkp + 
    u100*di *djp*dkp + 
    u010*dip*dj *dkp + 
    u110*di *dj *dkp + 
    u001*dip*djp*dk  + 
    u101*di *djp*dk  + 
    u011*dip*dj *dk  + 
    u111*di *dj *dk   ;
}

// in this version, i and j are already calculated, and passed to us
Velocity Environment::get_background_wind(const Position & pos, 
                                          float i,
                                          float j, 
                                          bool thermal) const
{
  float altitude = pos[2]-get_z(pos[0], pos[1]);
  float k = find_index(wind_z, wind_nz, altitude);
  
  int ii = (int) i;
  int ij = (int) j;
  int ik = (int) k;
  
  float u, v, w;
  
  if ( (ii < (wind_nx-1)) && (ij < (wind_ny-1)) && (ik < (wind_nz-1)) )
  {
    // can safely interpolate
    u = interp(wind_u, i, j, k);
    v = interp(wind_v, i, j, k);
    w = interp(wind_w, i, j, k);
  }
  else
  {
    // for now... doesn't really matter what happens at the edges
    u = wind_u[calc_index(ii, ij, ik)];
    v = wind_v[calc_index(ii, ij, ik)];
    w = wind_w[calc_index(ii, ij, ik)];
  }
  
  //    TRACE("wind at (%5.2f, %5.2f, %5.2f) = (%5.2f, %5.2f, %5.2f)\n", i, j, k, u, v, w);
  //    TRACE("wind_x = %f, glider_x = %f\n", wind_x[ii], glider->get_pos()[0]);
  if (thermal)
  {
    return (Velocity(u,v,w) * Sss::instance()->config().wind_scale +
            m_thermal_manager->get_wind(pos));
  }
  else
  {
    return (Velocity(u,v,w) * Sss::instance()->config().wind_scale);
  }
}

// returns background wind and i + j for future use
inline Velocity Environment::get_background_wind_and_ij(const Position & pos, float & i, float & j) const
{
  i = find_index(wind_x, wind_nx, pos[0]);
  j = find_index(wind_y, wind_ny, pos[1]);
  return get_background_wind(pos, i, j, true);
}

// returns background wind
inline Velocity Environment::get_background_wind(const Position & pos) const
{
  float i, j;
  if (wind_u)
  {
    return get_background_wind_and_ij(pos, i, j);
  }
  else
  {
    return get_background_wind_from_terrain(pos);
  }
}

// returns ambient wind (no thermals)
Velocity Environment::get_ambient_wind(const Position & pos) const
{
  if (wind_u)
  {
    float i=0;
    float j=0;
    return get_background_wind(pos, i, j, false);
  }
  else
  {
    return get_background_wind_from_terrain(pos, false);
  }
}

// returns wind calculated from the terrain, including thermals
Velocity Environment::get_background_wind_from_terrain(const Position & pos,
                                                       bool thermal) const
{
  assert1(wind_u == 0);
  float x0, y0, z0, x1, y1, z1, x2, y2, z2;
  
  // wind direction in deg from north (so 0 is a northerly wind)
  float wind_dir = Sss::instance()->config().wind_dir; 
  
  float sin_wind_dir = sin_deg(wind_dir);
  float cos_wind_dir = cos_deg(wind_dir);

  x1 = pos[0];
  y1 = pos[1];
  z1 = get_z(x1, y1);
  
  float alt = pos[2] - z1;

  // shift the pattern upwind
  if (fabsf(Sss::instance()->config().wind_wave_upwind_shift) > 0.0001f)
  {
    float fwd_dist = alt * Sss::instance()->config().wind_wave_upwind_shift;
    x1 -= fwd_dist * sin_wind_dir;
    y1 -= fwd_dist * cos_wind_dir;
    z1 = get_z(x1, y1) + alt;
  }

  float dist = 3.0f + alt * Sss::instance()->config().wind_slope_smoothing;
  

  x0 = x1 + dist * sin_wind_dir;
  y0 = y1 + dist * cos_wind_dir;
  
  // x2, y2 are opposite x0, y0
  x2 = 2 * x1 - x0;
  y2 = 2 * y1 - y0;
  
  z0 = get_z(x0, y0);
  z2 = get_z(x2, y2);
  
  float slope = (z2 - z0) / (2.0f * dist);
  
  float wind_speed = 5.0f;
  
  const float height_scale_top = boundary_layer_depth;
  const float height_scale_bot = 0.4f; // effective height of ground (for scaling)
  
  float f = 1 - exp(-(height_scale_bot + alt)/height_scale_top);
  
  // scale the wind slope with height...
  slope *= exp(-alt/Sss::instance()->config().wind_slope_influence_height);
  
  float wind_speed1 = wind_speed * f;
  
  float w = wind_speed1 * slope;
  
  float u = -wind_speed1 * sin_wind_dir;
  float v = -wind_speed1 * cos_wind_dir;
  
  if (thermal)
  {
    return (Velocity(u,v,w) * Sss::instance()->config().wind_scale +
            m_thermal_manager->get_wind(pos));
  }
  else
  {
    return (Velocity(u,v,w) * Sss::instance()->config().wind_scale);
  }
}

// fast and inaccurate sin function for the turbulence calculation
static inline float fast_sin(float rad)
{
#define TABLE_SIZE 32
  static const float d_rad = 2*PI/TABLE_SIZE;
  static float table[TABLE_SIZE+1];
  
  int i;
  static bool init = false;
  
  if (init == false)
  {
    init = true;
    for (i = 0 ; i <= TABLE_SIZE ; ++i)
    {
      table[i] = sin(i * d_rad);
    }
  }
  
  i = (int) (rad/d_rad);
  float d_i = rad/d_rad - i;
  
  if (i < 0)
  {
    // the -= accounts for the fact that i is negative
    i -= TABLE_SIZE * (-1 + (int) (i/TABLE_SIZE));
  }
  
  i = i % TABLE_SIZE;
  
  assert1 (i >= 0);
  assert1 (i < TABLE_SIZE);
  return table[i] + d_i * (table[i+1] - table[i]);
}

// Returns just the turbulent wind component
Velocity Environment::get_turbulent_wind(const Position & pos, 
                                         const Velocity & orig_vel,
                                         bool use_i_and_j,
                                         float i, float j,
                                         float lambda,
                                         int rand_offset) const
{
  static float rand_nums[3 * 3 * NUM_TURB_SAMPLES];
  unsigned int index;
  static bool init = false;
  if (init == false)
  {
    init = true;
    for (index = 0 ; index < sizeof(rand_nums)/sizeof(rand_nums[0]) ; ++index)
    {
      rand_nums[index] = 2 * PI * rand()/RAND_MAX;
    }
  }
  
  // the eddies on the scale of 0.5f* lambda will bring the other components in
  Velocity other_vel;
  if (use_i_and_j)
    other_vel = get_background_wind(pos + Position(0, 0, 0.5f * lambda), i, j);
  else
    other_vel = get_background_wind(pos + Position(0, 0, 0.5f * lambda));

  // assume isotropic turbulence, so u and v contribute... (and w nreally)
  const float u_var = Sss::instance()->config().turbulence_shear_offset * 0.5f * lambda + 
    hypot(other_vel[0] - orig_vel[0], 
          other_vel[1] - orig_vel[1] );
  const float v_var = u_var;
  const float w_var = u_var/2.0f;
  
  float last_update_time = Sss::instance()->last_update_time();
  index = rand_offset;
  
  const float k = 2 * PI / lambda;
  
  // advection -> sin(kx - wt) -> speed c = w/k = wind speed
  float kx = pos[0]/k;
  float ky = pos[1]/k;
  float kz = pos[2]/k;
  
  float wtx = last_update_time * orig_vel[0] * k;
  float wty = last_update_time * orig_vel[1] * k;
  float wtz = last_update_time * orig_vel[2] * k;
  
  float u = u_var * fast_sin(rand_nums[index++] + kx - wtx) * 
    fast_sin(rand_nums[index++] + ky - wty) * 
    fast_sin(rand_nums[index++] + kz - wtz) ;
  float v = v_var * fast_sin(rand_nums[index++] + kx - wtx) * 
    fast_sin(rand_nums[index++] + ky - wty) * 
    fast_sin(rand_nums[index++] + kz - wtz) ;
  float w = w_var * fast_sin(rand_nums[index++] + kx - wtx) * 
    fast_sin(rand_nums[index++] + ky - wty) * 
    fast_sin(rand_nums[index++] + kz - wtz) ;
  assert1(index <= (3 * 3 * NUM_TURB_SAMPLES));
  float altitude = pos[2] - get_z(pos[0], pos[1]);
  // scale the result to reduce the effect when within lambda/2 of the ground
  float scale_z = 1 - exp(-0.2f * altitude/lambda);
  // also scale as E ~= u^2 is proportional to k^-5/3
  // but limit the effect of large lambda
  float effective_k = sss_max(k, (float) TWO_PI/5.0f);
  float scale = pow((double) effective_k, -5.0/6.0);
  
  return Velocity(u, v, w * scale_z) * scale;
}

// returns the environment wind, including turbulence
Velocity Environment::get_wind(const Position & pos, 
                               const Object * object) const
{
  Velocity wind;
  if (Sss::instance()->config().turbulence == false)
  {
    wind = get_background_wind(pos);
  }
  else
  {
    float lambda_min = object->get_graphical_bounding_radius();
    float lambda_max = MAX_TURB_SCALE * lambda_min;
    float delta_lambda = (lambda_max - lambda_min) / NUM_TURB_SAMPLES;

    if (wind_u != 0) // i.e. pre-calculated
    {
      float i, j;
      Velocity background_wind = get_background_wind_and_ij(pos, i, j);
      wind = background_wind;
      int octave;

      for (octave = 0 ; octave < NUM_TURB_SAMPLES ; ++octave)
      {
        float lambda = lambda_min + delta_lambda * octave;
        float k_below;
        if (lambda - 0.5f * delta_lambda > lambda_min)
          k_below = TWO_PI / (lambda - 0.5f * delta_lambda);
        else
          k_below = TWO_PI / lambda_min;
        float k_above = TWO_PI / (lambda + 0.5f * delta_lambda);
        float delta_k = k_below - k_above;
        wind = wind + Sss::instance()->config().turbulence_scale *
          delta_k * get_turbulent_wind(pos, 
                                       background_wind, 
                                       true,
                                       i, j,
                                       lambda, 
                                       octave);
      }
    }
    else
    {
      Velocity background_wind = get_background_wind(pos);
      wind = background_wind;

      int octave;
      for (octave = 0 ; octave < NUM_TURB_SAMPLES ; ++octave)
      {
        float lambda = lambda_min + delta_lambda * octave;
        float k_below;
        if (lambda - 0.5f * delta_lambda > lambda_min)
          k_below = TWO_PI / (lambda - 0.5f * delta_lambda);
        else
          k_below = TWO_PI / lambda_min;
        float k_above = TWO_PI / (lambda + 0.5f * delta_lambda);
        float delta_k = k_below - k_above;
        wind = wind + Sss::instance()->config().turbulence_scale *
          delta_k * get_turbulent_wind(pos, 
                                       background_wind, 
                                       false,
                                       0.0f, 0.0f,
                                       lambda, 
                                       octave);
      }
    }
  }
  
  // if we are under water scale everything (arbitrarily) - it's nice
  // to have a slight current!
  if (pos[2] < sea_altitude)
    wind *= 0.1;
  
  for (Wind_modifiers::const_iterator it = m_wind_modifiers.begin() ;
       it != m_wind_modifiers.end() ;
       ++it)
  {
    (*it)->get_wind(pos, wind);
  }
  return wind;
}

Velocity Environment::get_non_turbulent_wind(const Position & pos) const
{
  return get_background_wind(pos);
}

//! \todo remove hard-coded up
void Environment::calculate_shadow(Object * shadow_maker)
{
  static bool init = false;
  if (init == false)
  {
    init = true;
    
    if (shadow_texture == 0)
      shadow_texture = 
        new Shadow_texture(Sss::instance()->config().shadow_size, 
                           Sss::instance()->config().shadow_size, 
                           shadow_maker->get_graphical_bounding_radius());
    
  }
  
  if ((Sss::instance()->config().texture_level >= 5) &&
      (multitextureSupported == true) )
  {
    if (Sss::instance()->config().shadow == true)
    {
      int tx, ty, tw, th;
      Gui::instance()->get_viewport_area(&tx, &ty, &tw, &th);
      glViewport(tx, ty, 
                 shadow_texture->get_w(), 
                 shadow_texture->get_h());
      
      // set up projection
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      
      glLoadIdentity();
      const float dist = shadow_maker->get_graphical_bounding_radius();
      glOrtho(-dist, dist, -dist, dist, -5, 5);
      
      glMatrixMode(GL_MODELVIEW);
      glPushMatrix();
      
      glPushAttrib(GL_ALL_ATTRIB_BITS);
      
      // set up view 
      const Vector3 & to = shadow_maker->get_pos();
      const Vector3 from = 
        to + Vector3(light_pos[0], light_pos[1], light_pos[2]);
      // hard-wire "up" = hope the sun isn't overhead!
      glLoadIdentity();
      gluLookAt(from[0], from[1], from[2],
                to[0],   to[1],   to[2],
                0,       0,       1 );
      
      // draw stuff
      glDisable(GL_FOG);
      glDisable(GL_LIGHTING);
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_BLEND);
      glDisable(GL_TEXTURE_2D);
      
      glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
      glClear(GL_COLOR_BUFFER_BIT);
      
      glColor3f(0.5f, 0.5f, 0.5f);
      shadow_maker->draw(Object::SHADOW);
      
      // copy int texture
      glBindTexture(GL_TEXTURE_2D, shadow_texture->get_texture());
      
      glCopyTexImage2D(GL_TEXTURE_2D, 0,
                       GL_RGB,
                       0, 0,
                       shadow_texture->get_w(), 
                       shadow_texture->get_h(),
                       0);
// annoyingly, this seems to mess up when in full-screen
//        glCopyTexSubImage2D(GL_TEXTURE_2D, 0,
//                            1, 1,
//                            1, 1,
//                            shadow_texture->get_w()-2, 
//                            shadow_texture->get_h()-2);
      
      // undo changes
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glPopMatrix();
      glPopAttrib();
      glViewport(tx, ty, tw, th);
    }
  }
}

void Environment::setup_lighting()
{
  //  static GLfloat mat_specular[]={0.1f,0.1f,0.1f,1.0f};
  static GLfloat white_light[]=  {1.0f,1.0f,1.0f,1.0f};
  static GLfloat ambient_light[]={0.1f,0.1f,0.1f,1.0f};
  
  // set the light position (especially) after setting the viewpoint,
  // so that it is fixed
  glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
  glLightfv(GL_LIGHT0,GL_DIFFUSE,white_light);
  //  glLightfv(GL_LIGHT0,GL_SPECULAR,white_light);
  glLightfv(GL_LIGHT0,GL_AMBIENT,ambient_light);
  glEnable(GL_LIGHT0);
}

/*!
  
Estimates a reasonable standing position for the pilot. This is
based (partly) on the vertical velocity in one of the layers -
trying to get a compromise between:

(1) standing just downwind of high vertical velocity,

(2) just down-wind of a patch of generally-high vertical velocity, and 

(3) somewhere in the middle (or maybe a bit downwind) of the region.

The chances of getting this right are fairly remote...?!

*/
  Position Environment::estimate_best_start_position()
{
  TRACE("Calculating start position\n");
  
  int i, j;
  
  Array_2D<float> ground(nx, ny);
  
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      ground(i, j) = ground_vertex_array->get_pos(i, j)[2];
    }
  }
  
  // store the original ground terrain
  const Array_2D<float> ground_orig(ground);
  
//  ground.dump("ground0.dat");
  
  ground.gaussian_filter(4, 8);
  
//  ground.dump("ground1.dat");
  
  // make some copies of the filtered ground
  Array_2D<float> ground_copy1(ground); // for d/dy
  Array_2D<float> ground_copy2(ground); // for z
  
  // calculate d/dx
  ground.gradient_x();
  
//  ground.dump("ground2.dat");
  
  // we don't want to be on the side of a hill, so encourage regions
  // where d/dy = 0;
  
  ground_copy1.gradient_y();
  ground_copy1 *= 0.2f;
  
  ground_copy1.abs();
  
//  ground_copy.dump("ground3.dat");
  
  ground.subtract(ground_copy1);
  
//  ground.dump("ground4.dat");
  
  // we don't want to be on a steep slope that is low down!
  float min = ground_copy2.get_min();
  float max = ground_copy2.get_max();
  
  ground_copy2 -= min;
  ground_copy2 *= 1/(max-min);
  
  ground.multiply(ground_copy2);
  
  // now weight the values according to a desire to be a bit downwind of center.
  
  int i_mid = 2*nx/3;
  int j_mid = ny/2;
  float radius = nx/2.7f; // Rather arbitrary...
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      float d2 = ( (i-i_mid) * (i-i_mid) + (j-j_mid) * (j-j_mid) );
      ground(i, j) *= exp(-d2/(radius*radius));
    }
  }
  
//  ground.dump("ground5.dat");
  
  // now find the highest value...
  int i_max = 0, j_max = 0;
  
  for (i = 0 ; i < nx ; ++i)
  {
    for (j = 0 ; j < ny ; ++j)
    {
      if (ground(i, j) > ground(i_max, j_max))
      {
        i_max = i;
        j_max = j;
      }
    }
  }
  
  // finally walk downwind, stopping when the slope falls below a critical value.
  float crit_slope = 0.15f; // move downwind until slope falls below this
  float slope;
  i = i_max;
  j = j_max;
  
  float dx = ground_vertex_array->get_pos(1, 0)[0] - 
    ground_vertex_array->get_pos(0, 0)[0];
  while ( 
    (i < nx-1) && 
    ( (slope = (ground_orig(i+1, j) - ground_orig(i, j))/dx) > crit_slope) 
    )
  {
    ++i;
  }
  
  Position pos = ground_vertex_array->get_pos(i, j);
  pos[2] = 0;
  
  TRACE("Starting position is (%f, %f)\n", pos[0], pos[1]);
  
  return pos;
  
}

float Environment::get_air_density(const Position & pos)
{
  // We have to be careful - having a sharp boundary between 
  // air and water can make life very hard for the physics
  // code, so we provide a gradual change.
  if (pos[2] > sea_altitude)
    return m_air_density;
  
  float dz = 2; // depth of the transition layer
  float depth = sea_altitude - pos[2];
  
  if (depth > dz)
    return m_water_density;
  else
    return (m_air_density + (m_water_density - m_air_density) * depth/dz);
}

void Environment::update_environment(float dt)
{
  //! \todo do some calculation for the turbulence here?
  m_thermal_manager->update_thermals(dt);
}

void Environment::draw_thermals() const
{
  m_thermal_manager->draw_thermals();
}

void Environment::register_wind_modifier(const Wind_modifier * modifier)
{
  m_wind_modifiers.insert(modifier);
}

void Environment::deregister_wind_modifier(const Wind_modifier * modifier)
{
  m_wind_modifiers.erase(modifier);
}

unsigned Environment::get_terrain_triangles() const
{ 
  return ground_vertex_array->get_triangles();
}

Position Environment::get_ground_intersection(
  const Position & pos0,
  const Position & pos1) const
{
  
  float dx = 0.5 * ( ground_vertex_array->get_pos(1, 0)[0] - 
                     ground_vertex_array->get_pos(0, 0)[0] );
  
  Vector dir = (pos1 - pos0);
  int num = (int) (dir.mag()/dx);
  dir.normalise();
  Vector step = dir * dx;
  
  for (int i = 0 ; i < num ; ++i)
  {
    Position pos = pos0 + i * step;
//     pos.show("pos");
//     TRACE("ground z = %f\n", get_z(pos[0], pos[1]));
    if (get_z(pos[0], pos[1]) > pos[2])
    {
      pos -= 0.5 * step;
      return pos;
    }
  }
  
  return pos0;
}




Wind_modifier::Wind_modifier()
{
//  Environment::instance()->register_wind_modifier(this);
}

Wind_modifier::~Wind_modifier()
{
  Environment::instance()->deregister_wind_modifier(this);
}

