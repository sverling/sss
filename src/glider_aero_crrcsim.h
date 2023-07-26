/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef GLIDER_AERO_CRRCSIM_H
#define GLIDER_AERO_CRRCSIM_H

#include "glider_aero.h"

class Config_file;

/// Implements aerodynamics based on the Crrcsim simulator
class Glider_aero_crrcsim : public Glider_aero
{
public:
  //! constructor
  Glider_aero_crrcsim(Config_file & aero_config, Glider & glider);
  
  //! calculates net force and moment from all aerofoils
  void get_force_and_moment(Vector3 & force,
                            Vector3 & moment);
  
  virtual void show();
private:
  Glider & m_glider;

  // aero stuff
  float C_ref;  //  reference chord (ft)
  float B_ref;  //  reference span  (ft)
  float S_ref;  //  reference area (ft^2)
  float U_ref;  //  reference speed for Re-scaling of CD_prof  (ft/s)
  float Alpha_0;  //  baseline alpha (rad)
  float Cm_0   ;  //  baseline Cm at Alpha_0
  float CL_0   ;  //  baseline CL at Alpha_0
  float CL_max ;  //  positive stall limit
  float CL_min ;  //  negative stall limit
  float CD_prof;  //  profile CD at U_ref
  float Uexp_CD;  //  for Re-scaling of CD_prof  ~ (U/U_ref)^Uexp_CD
  float CL_a;  // lift-force   / alpha    ~  2 pi / (1 + 2/AR)
  float Cm_a;  // pitch-moment / alpha    (pitch stability)
  float CY_b;  // side-force  / sideslip
  float Cl_b;  // roll-moment / sideslip (crucial for rudder-only turns)
  float Cn_b;  //  yaw-moment  / sideslip (yaw stability)
  float CL_q;  //  lift-force   / pitch-rate
  float Cm_q;  //  pitch-moment / pitch-rate  (pitch damping)
  float CY_p;  //  side-force  / roll-rate
  float Cl_p;  //  roll-moment / roll-rate   (roll damping)
  float Cn_p;  //  yaw-moment  / roll-rate   (yaw-roll coupling)
  float CY_r;  //  side-force  / yaw-rate
  float Cl_r;  //  roll-moment / yaw-rate
  float Cn_r;  //  yaw-moment  / yaw-rate  (yaw damping)
  float CL_de;  //  lift-force   / elevator
  float Cm_de;  //  pitch-moment / elevator
  float CY_dr; // side-force  / rudder
  float Cl_dr; // roll-moment / rudder
  float Cn_dr; // yaw-moment  / rudder
  float CY_da;  // side-force  / aileron 
  float Cl_da;  // roll-moment / aileron
  float Cn_da;  // yaw-moment  / aileron
  float eta_loc;
  float CG_arm;
  float CL_drop;
  float CD_stall;
/* new CD model       MD 8 June 01
   non-unity span efficiency
   profile CD is quadratic on CL and aileron deflection
   CDi is increased by load deformation due to aileron and roll rate  */
  float span_eff; //  span efficiency
  float CL_CD0;   //  CL at minimum profile CD
  float CD_CLsq;  //  d(CD)/d(CL^2),  curvature of parabolic profile polar
  float CD_AIsq;  //  d(CD)/d(aileron^2) , curvature of ail. CD influence
  float CD_ELsq;  //  d(CD)/d(elevator^2), curvature of ele. CD influence
};


#endif
