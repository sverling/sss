/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file glider_aero_crrcsim.cpp
*/
#include "glider_aero_crrcsim.h"
#include "config_file.h"
#include "environment.h"
#include "glider.h"
#include "joystick.h"
#include "log_trace.h"

#include <math.h>

using namespace std;

/*!  Needs to add to the vector of aerofoils based on what is in the
  config file.  */
Glider_aero_crrcsim::Glider_aero_crrcsim(Config_file & aero_config,
                                         Glider & glider)
  :
  m_glider(glider)
{
  TRACE_METHOD_ONLY(1);
  TRACE("Using CRRCSIM aerodynamics\n");
  
  C_ref = 0 ; aero_config.get_value("C_ref", C_ref);
  B_ref = 0 ; aero_config.get_value("B_ref", B_ref);
  S_ref = 0 ; aero_config.get_value("S_ref", S_ref);
  U_ref = 0 ; aero_config.get_value("U_ref", U_ref);
  Alpha_0 = 0 ; aero_config.get_value("Alpha_0", Alpha_0);
  Cm_0 = 0 ; aero_config.get_value("Cm_0", Cm_0 );
  CL_0 = 0 ; aero_config.get_value("CL_0", CL_0 );
  CL_max = 0 ; aero_config.get_value("CL_max", CL_max );
  CL_min = 0 ; aero_config.get_value("CL_min", CL_min );
  CD_prof = 0 ; aero_config.get_value("CD_prof", CD_prof);
  Uexp_CD = 0 ; aero_config.get_value("Uexp_CD", Uexp_CD);
  CL_a = 0 ; aero_config.get_value("CL_a", CL_a);
  Cm_a = 0 ; aero_config.get_value("Cm_a", Cm_a);
  CY_b = 0 ; aero_config.get_value("CY_b", CY_b);
  Cl_b = 0 ; aero_config.get_value("Cl_b", Cl_b);
  Cn_b = 0 ; aero_config.get_value("Cn_b", Cn_b);
  CL_q = 0 ; aero_config.get_value("CL_q", CL_q);
  Cm_q = 0 ; aero_config.get_value("Cm_q", Cm_q);
  CY_p = 0 ; aero_config.get_value("CY_p", CY_p);
  Cl_p = 0 ; aero_config.get_value("Cl_p", Cl_p);
  Cn_p = 0 ; aero_config.get_value("Cn_p", Cn_p);
  CY_r = 0 ; aero_config.get_value("CY_r", CY_r);
  Cl_r = 0 ; aero_config.get_value("Cl_r", Cl_r);
  Cn_r = 0 ; aero_config.get_value("Cn_r", Cn_r);
  CL_de = 0 ; aero_config.get_value("CL_de", CL_de);
  Cm_de = 0 ; aero_config.get_value("Cm_de", Cm_de);
  CY_dr = 0 ; aero_config.get_value("CY_dr", CY_dr);
  Cl_dr = 0 ; aero_config.get_value("Cl_dr", Cl_dr);
  Cn_dr = 0 ; aero_config.get_value("Cn_dr", Cn_dr);
  CY_da = 0 ; aero_config.get_value("CY_da", CY_da);
  Cl_da = 0 ; aero_config.get_value("Cl_da", Cl_da);
  Cn_da = 0 ; aero_config.get_value("Cn_da", Cn_da);
  eta_loc = 0 ; aero_config.get_value("eta_loc", eta_loc);
  CG_arm = 0 ; aero_config.get_value("CG_arm", CG_arm);
  CL_drop = 0 ; aero_config.get_value("CL_drop", CL_drop);
  CD_stall = 0 ; aero_config.get_value("CD_stall", CD_stall);
  span_eff = 1.0f ; aero_config.get_value("span_eff", span_eff);
  CL_CD0 = 0 ; aero_config.get_value("CL_CD0", CL_CD0);
  CD_CLsq = 0 ; aero_config.get_value("CD_CLsq", CD_CLsq);
  CD_AIsq = 0 ; aero_config.get_value("CD_AIsq", CD_AIsq);
  CD_ELsq = 0 ; aero_config.get_value("CD_ELsq", CD_ELsq);

  // now some others...
  float mass, I_x, I_y, I_z;
  aero_config.get_value_assert("Mass", mass);
  aero_config.get_value_assert("I_xx", I_x);
  aero_config.get_value_assert("I_yy", I_y);
  aero_config.get_value_assert("I_zz", I_z);
  
  // convert to metric
  mass *= 14.47f; // slugs to kg ( = g in f.p.s)
  I_x *= 14.47f / (3.28f * 3.28f); // slugs ft2 to kg m2
  I_y *= 14.47f / (3.28f * 3.28f);
  I_z *= 14.47f / (3.28f * 3.28f);
  

  m_glider.set_physics_params(mass, I_x, I_y, I_z);
}

/*!
  Accumulate all the forces from the constituent aerofoils.
*/
void Glider_aero_crrcsim::get_force_and_moment(Vector3 & force,
                                               Vector3 & moment)
{
/* new CD model       MD 8 June 01
   non-unity span efficiency
   profile CD is quadratic on CL and aileron deflection
   CDi is increased by load deformation due to aileron and roll rate  */

  float Phat, Qhat, Rhat;  // dimensionless rotation rates
  float CL_left, CL_cent, CL_right; // CL values across the span
  float dCL_left,dCL_cent,dCL_right; // stall-induced CL changes
  float dCD_left,dCD_cent,dCD_right; // stall-induced CD changes
  float dCl,dCn,dCl_stall,dCn_stall;  // stall-induced roll,yaw moments
  float dCm_stall;  // Stall induced pitching moment.
  float CL_wing, CD_all, CD_scaled, Cl_w;
  float Cl_r_mod,Cn_p_mod;
  float CL,CD,Cx,Cy,Cz,Cl,Cm,Cn;
  float QS;
  float P_atmo,Q_atmo,R_atmo;
  int stalling;
  
  // controls
  float aileron = m_glider.get_real_joystick().get_value(1);
  float elevator = -m_glider.get_real_joystick().get_value(2);
  float rudder = -m_glider.get_real_joystick().get_value(4);

  // emulate crrcsim
  aileron *= 0.5f;
  elevator *= 0.5f;
  rudder *= 0.5f;

  // Wind velocity in the glider reference frame due to the glider's
  // motion.
  const Velocity wind_static_rel(
    -dot(m_glider.get_vel(), m_glider.get_vec_i_d()),
    -dot(m_glider.get_vel(), m_glider.get_vec_j_d()),
    -dot(m_glider.get_vel(), m_glider.get_vec_k_d()) );
  // Wind velocity due to the wind in global frame
  const Velocity ambient_wind = Environment::instance()->get_wind(
    m_glider.get_pos(), 
    &m_glider);
  // Wind velocity due to the wind in glider's frame
  const Velocity ambient_wind_rel(
    dot(ambient_wind, m_glider.get_vec_i_d()),
    dot(ambient_wind, m_glider.get_vec_j_d()),
    dot(ambient_wind, m_glider.get_vec_k_d()) );
  
  // Total wind velocity relative to the glider in the glider's frame.
  // need to convert from metric to imperial units  
  const Velocity wind_rel = 3.28f * (ambient_wind_rel + wind_static_rel);

//  wind_rel.show("wind_rel");

  /* Wind-relative velocities in body axis  */
  float U_body = -wind_rel[0];
  float V_body = wind_rel[1];
  float W_body = wind_rel[2];

  TRACE_FILE_IF(6)
    TRACE("U_body = %f, V_body = %f, W_body = %f\n",
          U_body, V_body, W_body);
  
#define SEA_LEVEL_DENSITY 0.00237f
  float Density =  SEA_LEVEL_DENSITY;  // in silly units - slugs/ft3
//  float Density = Environment::instance()->get_air_density(m_glider.get_pos()) * 
//                  SEA_LEVEL_DENSITY / 1.0f;

  // just the air-relative speed?
  float V_rel_wind = wind_rel.mag();

  // To calculate the airmass rotation we take derivatives on the
  // appropriate scale
  const float dist_scale = m_glider.get_structural_bounding_radius();
  const Velocity vel1 = 3.28f * Environment::instance()->get_wind(
    m_glider.get_pos() + dist_scale * m_glider.get_vec_i_d(), 
    &m_glider);
  const Velocity vel2 = 3.28f * Environment::instance()->get_wind(
    m_glider.get_pos() - dist_scale * m_glider.get_vec_i_d(), 
    &m_glider);
  const Velocity vel3 = 3.28f * Environment::instance()->get_wind(
    m_glider.get_pos() + dist_scale * m_glider.get_vec_j_d(), 
    &m_glider);
  const Velocity vel4 = 3.28f * Environment::instance()->get_wind(
    m_glider.get_pos() - dist_scale * m_glider.get_vec_j_d(), 
    &m_glider);
//    const Velocity vel5 = 3.28f * Environment::instance()->get_wind(
//      m_glider.get_pos() + dist_scale * m_glider.get_vec_k_d(), 
//      &m_glider);
//    const Velocity vel6 = 3.28f * Environment::instance()->get_wind(
//      m_glider.get_pos() - dist_scale * m_glider.get_vec_k_d(), 
//      &m_glider);

  // we need a few components of these velocities in the glider
  // reference frame to set the airmass rotation rates in the glider's
  // frame.
  
  // also adjust the signs for the flipped Q and R values

  float w3 = dot(vel3, m_glider.get_vec_k_d());
  float w4 = dot(vel4, m_glider.get_vec_k_d());
  P_atmo = (w3 - w4) / (2 * dist_scale);
  
  float w1 = dot(vel1, m_glider.get_vec_k_d());
  float w2 = dot(vel2, m_glider.get_vec_k_d());
  Q_atmo = -(w2 - w1) / (2 * dist_scale);
  
  // use both directions here
  float v1 = dot(vel1, m_glider.get_vec_j_d());
  float v2 = dot(vel2, m_glider.get_vec_j_d());
  float u3 = dot(vel3, m_glider.get_vec_i_d());
  float u4 = dot(vel4, m_glider.get_vec_i_d());
  R_atmo = -0.5f * ( (v1 - v2) + (u4 - u3) ) / (2 * dist_scale);
  
  /* set net effective dimensionless rotation rates */
  if (V_rel_wind != 0)
  {
    Vector body_rot = transpose(m_glider.get_orient())*m_glider.get_rot();
    Phat = body_rot[0] - P_atmo;
    Qhat = -body_rot[1] - Q_atmo;
    Rhat = -body_rot[2] - R_atmo;
    
    Phat *= B_ref / (2.0f * V_rel_wind);
    Qhat *= C_ref / (2.0f * V_rel_wind);
    Rhat *= B_ref / (2.0f * V_rel_wind);
  }
  else
  {
    Phat=0;
    Qhat=0;
    Rhat=0;
  }
  
  TRACE_FILE_IF(6)
    TRACE("Phat = %f, Qhat = %f, Rhat = %f\n",
          Phat, Qhat, Rhat);

  // angle of attack
  float Alpha, Beta;
  if ( (U_body == 0) && (W_body == 0) )
    Alpha = 0;
  else
    Alpha = atan2( W_body, U_body );
  if (V_rel_wind == 0)
    Beta = 0;
  else
    Beta = asin( V_body/ V_rel_wind );

  float Cos_alpha = cos(Alpha);
  float Sin_alpha = sin(Alpha);
  float Cos_beta  = cos(Beta);

//  TRACE("Alpha = %f, Cos_alpha = %f, Sin_alpha = %f, Beta = %f\n",
//         Alpha, Cos_alpha, Sin_alpha, Beta);

  if (V_rel_wind > 0.1f)
  {
    CD_scaled=CD_prof*pow(((double)V_rel_wind/(double)U_ref),(double)Uexp_CD);
  }
  else
  {
    CD_scaled=CD_prof;
  }
  
  /* compute local CL at three spanwise locations */
  CL_left  = CL_0 + CL_a*(Alpha - Alpha_0 - Phat*eta_loc);
  CL_cent  = CL_0 + CL_a*(Alpha - Alpha_0               );
  CL_right = CL_0 + CL_a*(Alpha - Alpha_0 + Phat*eta_loc);
  
//  TRACE("CL_left: %f CL_cent: %f CL_right: %f\n",CL_left,CL_cent,CL_right);

  /* set CL-limit changes */
  dCL_left  = 0.;
  dCL_cent  = 0.;
  dCL_right = 0.;
  
  stalling=0;
  if (CL_left  > CL_max)
  {
    dCL_left  = CL_max-CL_left -CL_drop; 
    stalling=1;
  }
  
  if (CL_cent  > CL_max)
  {
    dCL_cent  = CL_max-CL_cent -CL_drop;
    stalling=1;
  }
  
  if (CL_right > CL_max)
  {
    dCL_right = CL_max-CL_right -CL_drop;
    stalling=1;      
  }
  
  if (CL_left  < CL_min)
  {
    dCL_left  = CL_min-CL_left -CL_drop;
    stalling=1;
  }
  
  if (CL_cent  < CL_min)
  {
    dCL_cent  = CL_min-CL_cent -CL_drop; 
    stalling=1;
  }
  
  if (CL_right < CL_min)
  {
    dCL_right = CL_min-CL_right -CL_drop;
    stalling=1;
  }
//  TRACE("Stalling = %d\n", stalling);
  
  /* set average wing CL */
  CL_wing = CL_0 + CL_a*(Alpha-Alpha_0)
    + 0.25f*dCL_left + 0.5f*dCL_cent + 0.25f*dCL_right;
  
//  TRACE("CL_wing: %f\n",CL_wing);
  
  /* correct profile CD for CL dependence and aileron dependence */
  CD_all = CD_prof
    + CD_CLsq*(CL_wing-CL_CD0)*(CL_wing-CL_CD0)
    + CD_AIsq*aileron*aileron
    + CD_ELsq*elevator*elevator;
  
  /* scale profile CD with Reynolds number via simple power law */
  if (V_rel_wind > 0.1f)
  {
    CD_scaled = CD_all*pow(((double)V_rel_wind/(double)U_ref),(double)Uexp_CD);
  }
  else
  {
    CD_scaled=CD_all;
  }

  /* Scale lateral cross-coupling derivatives with wing CL */
  Cl_r_mod = Cl_r*CL_wing/CL_0;
  Cn_p_mod = Cn_p*CL_wing/CL_0;
  
//  TRACE("Cl_r_mod: %f Cn_p_mod: %f\n",Cl_r_mod,Cn_p_mod);
  
  /* total average CD with induced and stall contributions */
  dCD_left  = CD_stall*dCL_left *dCL_left ;
  dCD_cent  = CD_stall*dCL_cent *dCL_cent ;
  dCD_right = CD_stall*dCL_right*dCL_right;
  
  /* total CL, with pitch rate and elevator contributions */
  CL = (CL_wing + CL_q*Qhat + CL_de*elevator)*Cos_alpha;
  
//  TRACE("CL:%f\n",CL);
  
  
//  TRACE("CL: %f CD:%f\n",CL,CD);
  
  /* assymetric stall will cause roll and yaw moments */
  dCl =  0.45f*-1*(dCL_right-dCL_left)*0.5f*eta_loc;
  dCn =  0.45f*(dCD_right-dCD_left)*0.5f*eta_loc;
  dCm_stall = (0.25f*dCL_left + 0.5f*dCL_cent + 0.25f*dCL_right)*CG_arm;
  
//  TRACE("dCl: %f dCn:%f\n",dCl,dCn);
  
  /* stall-caused moments in body axes */
  dCl_stall = dCl*Cos_alpha - dCn*Sin_alpha;
  dCn_stall = dCl*Sin_alpha + dCn*Cos_alpha;
  
  /* total CD, with induced and stall contributions */
  
  Cl_w = Cl_b*Beta  + Cl_p*Phat + Cl_r_mod*Rhat
    + dCl_stall  + Cl_da*aileron;
  CD = CD_scaled
    + (CL*CL + 32.0f*Cl_w*Cl_w)*S_ref/(B_ref*B_ref*3.14159f*span_eff)
    + 0.25f*dCD_left + 0.5f*dCD_cent + 0.25f*dCD_right;

  /* total forces in body axes */
  Cx = -CD*Cos_alpha + CL*Sin_alpha*Cos_beta*Cos_beta;
  Cz = -CD*Sin_alpha - CL*Cos_alpha*Cos_beta*Cos_beta;
  Cy = CY_b*Beta  + CY_p*Phat + CY_r*Rhat + CY_dr*rudder;
  
  /* total moments in body axes */
  Cl = Cl_b*Beta  + Cl_p*Phat + Cl_r_mod*Rhat + Cl_dr*rudder
    + dCl_stall + Cl_da*aileron;
  Cn = Cn_b*Beta  + Cn_p_mod*Phat + Cn_r*Rhat + Cn_dr*rudder
    + dCn_stall     + Cn_da*aileron;
  Cm = Cm_0 + Cm_a*(Alpha-Alpha_0) +dCm_stall
    + Cm_q*Qhat                 + Cm_de*elevator;
  
  /* set dimensional forces and moments */
  QS = 0.5f*Density*V_rel_wind*V_rel_wind * S_ref;
  
  float F_X_aero = Cx * QS;
  float F_Y_aero = Cy * QS;
  float F_Z_aero = Cz * QS;
  TRACE_FILE_IF(5)
    TRACE("F_x = %f, F_y = %f, F_z = %f\n", F_X_aero, F_Y_aero, F_Z_aero);
  
  float M_l_aero = Cl * QS * B_ref;
  float M_m_aero = Cm * QS * C_ref;
  float M_n_aero = Cn * QS * B_ref;
  TRACE_FILE_IF(5)
    TRACE("M_l = %f, M_m = %f, M_n = %f\n", M_l_aero, M_m_aero, M_n_aero);
  
//    Vector3 l_force(0); // forces in local frame
//    Vector3 l_moment(0); // moments in local frame

  Vector3 l_force(F_X_aero, -F_Y_aero, -F_Z_aero);
  Vector3 l_moment(M_l_aero, -M_m_aero, -M_n_aero);

  // for force we convert "pounds" (ugh!) into Newtons -

  l_force *= 4.45f; 
  l_moment *= 4.45f / 3.28f; 

  // convert the local vectors to world coords
//    l_force.show("force");
//    l_moment.show("moment");
  moment = m_glider.get_orient() * l_moment;
  force  = m_glider.get_orient() * l_force;
  
}


void Glider_aero_crrcsim::show()
{
  TRACE("Glider_aero_crrcsim\n");
  
}
