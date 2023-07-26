/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#include "particle_source.h"
#include "log_trace.h"
#include "object.h"
#include "texture.h"
#include "sss_assert.h"

#include "sss_glut.h"

Smoke_texture * smoke_texture = 0;

//========================================================
// the particle functions
//========================================================

Particle::Particle(Particle_type type,
                   const Position & init_pos, 
                   const Velocity & init_vel,
                   float init_size,
                   float max_size,
                   float lifetime,
                   float alpha_scale)
  :
  m_type(type),
  m_pos(init_pos),
  m_vel(init_vel),
  m_size(init_size),
  m_init_size(init_size),
  m_max_size(max_size),
  m_time_left(lifetime),
  m_lifetime(lifetime),
  m_init_alpha(0.4 * alpha_scale),
  m_rotation(ranged_random(0.0f, 360.0f)),
  m_rotation_rate(ranged_random(-40.0f, 40.0f))
{
  TRACE_METHOD_ONLY(4);
  m_size = m_init_size;

  switch (m_type)
  {
  case PARTICLE_SMOKE_LIGHT:
    m_colour[0] = 1.0f;
    m_colour[1] = 1.0f;
    m_colour[2] = 1.0f;
    break;
  case PARTICLE_SMOKE_MEDIUM:
    m_colour[0] = 0.5f;
    m_colour[1] = 0.5f;
    m_colour[2] = 0.5f;
    break;
  case PARTICLE_SMOKE_DARK:
    m_colour[0] = 0.0f;
    m_colour[1] = 0.0f;
    m_colour[2] = 0.0f;
    break;
  case PARTICLE_FLAME:
    m_colour[0] = 0.6f + ranged_random(-0.2f, 0.2f);
    m_colour[1] = 0.3f + ranged_random(-0.3f, 0.3f);
    m_colour[2] = 0.0f;
    if (m_colour[1] > m_colour[0])
      m_colour[1] = m_colour[0];
    break;
  default:
    TRACE("Impossible particle type %d\n", m_type);
  }      
}
  
void Particle::update(float dt)
{
  m_pos += dt * m_vel;
  m_time_left -= dt;
  m_size = m_max_size - (m_max_size - m_init_size) *
    (m_time_left/m_lifetime);
  m_rotation += m_rotation_rate * dt;
}

void Particle::draw(const Object * eye,
                    const float * rot_matrix)
{
  glPushMatrix();

  glTranslatef(m_pos[0], m_pos[1], m_pos[2]);
  glMultMatrixf(&rot_matrix[0]);
  glRotatef(m_rotation, 1.0f, 0.0f, 0.0f);
  const float end_alpha  = 0.0f;

  // frac is the fraction through the life...
  float density_frac = (m_size-m_init_size)/(m_max_size - m_init_size);
  density_frac = sqrt(density_frac);
  float alpha = m_init_alpha + (end_alpha - m_init_alpha) * density_frac;

  // bigger values of alpha -> darker

  //alpha = 0.5;
  //glColor4f(alpha, alpha, alpha, alpha);
  m_colour[3] = alpha;
  glColor4fv(m_colour);

  float s2 = m_size*0.5f;
  glBegin(GL_QUADS);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(0, s2, -s2);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(0, -s2, -s2);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(0, -s2, s2);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(0, s2, s2);
  glEnd();

  glPopMatrix();
}

//========================================================
// the source functions
//========================================================

Particle_source::Particle_source(Particle_type type, 
                                 int max_num,
                                 const Position & pos,
                                 const Velocity & vel,
                                 float alpha_scale,
                                 float init_size,
                                 float max_size,
                                 float lifetime,
                                 float rate) 
  : 
  m_type(type),
  m_max_num(max_num),
  m_num(0),
  m_suicidal(false),
  m_dead(false),
  m_num_to_add_remainder(0.0f)
{
  TRACE_METHOD_ONLY(2);

  // make sure the static textures are there
  static bool init = false;
  if (init == false)
  {
    init = true;
    smoke_texture = new Smoke_texture(128, 128);
  }

  m_pos = pos;
  m_vel = vel;
  m_rate = rate;

  m_alpha_scale = alpha_scale;
  m_init_size = init_size;
  m_max_size = max_size;
  m_lifetime = lifetime;
}

void Particle_source::update_source(float dt,
                                    const Position & new_pos,
                                    const Velocity & new_vel,
                                    float vel_jitter_mag)
{
  if (m_dead || m_suicidal)
    return;
  
  float num_to_add_f = m_num_to_add_remainder + (m_rate * dt);
  int num_to_add = (int) (num_to_add_f);

  // clamp to the max number of particles
  if (num_to_add > (m_max_num - m_num))
    num_to_add = (m_max_num - m_num);

  m_num_to_add_remainder = num_to_add_f - num_to_add;

  if (num_to_add > 0)
  {
    Vector pos_diff = new_pos - m_pos;
    Vector vel_diff = new_vel - m_vel;
    for (int i = 0 ; i < num_to_add ; ++i)
    {
      // add in a gradual sense... i.e. spread the particles out
      float frac = (1.0f + i) / (num_to_add+1);
      particles.push_back(
        Particle(m_type,
                 m_pos + frac * pos_diff,
                 m_vel +  
                 Vector(frac * vel_diff[0] + ranged_random(-vel_jitter_mag, vel_jitter_mag),
                        frac * vel_diff[1] + ranged_random(-vel_jitter_mag, vel_jitter_mag),
                        frac * vel_diff[2] + ranged_random(-vel_jitter_mag, vel_jitter_mag)),
                 m_init_size,
                 m_max_size,
                 m_lifetime * ranged_random(0.5f, 2.0f),
                 m_alpha_scale));
      --num_to_add;
      ++m_num;
    }
  }
  // just update the source pos etc
  m_pos = new_pos;
  m_vel = new_vel;
}

void Particle_source::draw_particles(const Object * eye)
{
  if (particles.empty())
    return;
  
  // Calculate the rotation matrix used for all particles based 
  // on the eye orientation. This is an approximation... but it's 
  // one that's worth doing if we want to support many thousands 
  // particles. To do it accurately, each billboard should face the
  // eye, rather than just be orientation in the same direction.
  
  // need to rotate
  Vector3 vec_i = eye->get_eye_target() - eye->get_eye();
  // for up, just use the k' direction
  Vector3 vec_j = cross(Vector(0, 0, 1), vec_i);
  Vector3 vec_k = cross(vec_i, vec_j);

  Orientation orient(vec_i.normalise(),
                     vec_j.normalise(),
                     vec_k.normalise());

  // orient.show("particle");
  // see Object::basic_draw()
  float matrix[] = 
    {
      orient(0, 0), // 1st column
      orient(1, 0),
      orient(2, 0),
      0,
      orient(0, 1), // 2nd column
      orient(1, 1),
      orient(2, 1),
      0,
      orient(0, 2), // 3rd column
      orient(1, 2),
      orient(2, 2),
      0,
      0, 0, 0, 1        // 4th column
    };

  glPushAttrib(GL_ALL_ATTRIB_BITS);

  // Blend by multiplying  - basically the smoke just makes stuff 
  // look black
  //  glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glDisable(GL_TEXTURE_GEN_S);
  glDisable(GL_TEXTURE_GEN_T);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, smoke_texture->get_texture());

  for (Particle_list::iterator it = particles.begin();
       it != particles.end();
       ++it)
  {
    it->draw(eye, matrix);
  }
  glPopAttrib();
}

void Particle_source::move_particles(float dt)
{
  for (Particle_list::iterator it = particles.begin();
       it != particles.end();
    )
  {
    it->update(dt);
    if (it->get_time_left() < 0.0f)
    {
      particles.erase(it++);
      --m_num;
    }
    else
      ++it;
  }
  if (m_suicidal && particles.empty())
    m_dead = true;
}

