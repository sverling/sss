/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef PILOT_MANAGER_H
#define PILOT_MANAGER_H

#include <set>
using namespace std;

class Pilot;

/*!

  This keeps a record of all the pilots, whether they are
  human-controlled, robots, or remote pilots.

 */
class Pilot_manager
{
public:
  static Pilot_manager * instance();
  
  void register_pilot(Pilot * pilot);
  bool deregister_pilot(Pilot * pilot);

  set<Pilot *> get_pilots() {return pilots;}

  //! calls update on all the pilots
  void update_pilots(float dt);

private:
  Pilot_manager() {};
  Pilot_manager(const Pilot_manager &);
  ~Pilot_manager();
  static Pilot_manager * m_instance;
  set<Pilot *> pilots;
};


#endif // file included

