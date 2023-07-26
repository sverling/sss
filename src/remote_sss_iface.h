/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
/*!

  This class acts as the interface between a local simulation and a
  group of remote simulations. It hides any knowledge that there may
  be multiple remote simulations. Thus this class is a singleton (that
  may not exist).

  This class has two roles:

  1. All out-going traffic goes through this class. Each class that
  gets involved in remote simulations is responsible for sending
  data. So each class should, once every frame (or less), use this
  object to send a protocol-specific update.

  2. All incoming traffic comes through this class.

  When this class is created no Object classes must exist. It
  will first (a) act as a client and get passed a list of remote
  addresses which it attempts to connect to, and then (b) act as a
  server and listen on a port, only completing its creation when it
  has made connections to the complete number of remote
  simulations. Thereafter no further simulations may connect, but
  some/all may drop out.

  Subsequently, whenever a Object is created/destroyd, it will
  place a pointer to itself on a queue in this class. At the end of
  the frame, this class will then:

  1. Send a destroy message to all remote simulations for any object
     that has been destroyd (apart from objects created and destroyd in
     the same frame).

  2. For each object that has been created, this class will ask it to
     construct an application-specific create message. This class will
     assign a unique handle to the object (maybe a combination of the
     pointer and information gathered during the connection stage?)
     and send a suitable message to each of the other simulations.

  Every time an object wants to send a message it passes its this
  pointer to this class. This lets us insert the appropriate handle
  into the message before sending it to the other simulations.

  There are three kinds of incoming traffic that this class has to
  deal with:

  1. Normal update messages. These will contain a handle of a real
     object, which can be converted to a pointer to a ghost
     Object object, so the ghost object can be updated
     directly.

  2. Create requests containing a handle of the real object. These get
     forwarded to the Remote_object_manager, which knows about such
     application specific things. The manager creates the ghost object
     and returns a pointer to it.

  3. Destroy requests. Again these get forwarded to the
     Remote_object_manager.

  The implementation will have to use non-blocking sockets.

*/
#ifndef REMOTE_SSS_IFACE_H
#define REMOTE_SSS_IFACE_H

//#include "stdafx.h"

#include "remote_sss_msg.h"
#include "object.h"

#include <vector>
#include <string>
#include <map>

class Sss_socket;
class Remote_sss_queue;

class Remote_sss_iface
{
public:
  //! Returns the singleton
  static Remote_sss_iface * instance() {return m_instance;}

  //! creates an instance that initialises as a client first, then as a
  //! server
  static void create_instance(
    std::string server_port, // the local port to listen on
    std::string client_addr,
    std::string client_port);

  /// used by real/local objects
  void send_update_ind(const Object * object,
                       Remote_sss_msg & msg);

  /// Used by ghost/remote objects
  void send_update_ind_remote(const Object * object,
                              Remote_sss_msg & msg);

  void send_create_ind(Object * object,
                       Remote_sss_msg & msg);

  void send_destroy_ind(Object * object,
                       Remote_sss_msg & msg);
  
  void recv_msgs();

  //! Any old message
  void send_msg(Remote_sss_msg & msg);
  
  bool is_server() const {return m_is_server;}

private:
  Remote_sss_iface(
    std::string server_port, // the local port to listen on
    std::string client_addr,
    std::string client_port);

  //! Cannot delete me
  ~Remote_sss_iface();

  void handle_received_msg(Remote_sss_msg & msg);
  void handle_create_msg(Remote_sss_msg & msg);
  void handle_destroy_msg(Remote_sss_msg & msg);
  void handle_update_msg(Remote_sss_msg & msg);
  
  void send_config_req();
  void recv_config_req();

  //! Timer is used to prompt the application to send data at a
  //! certain rate, and to check for incoming data at a possibly
  //! different rate.
  static void _timer_func(int val);
  void timer_func(int val);

  //! arg to socket_reader_fn is one of these
  struct Sss_socket_reader_data
  {
    Sss_socket * socket_ptr;
    Remote_sss_queue * queue_ptr;
  };
  //! fn to read from a socket and write to a queue
#ifdef unix
  static void * socket_reader_fn(void * arg);
#else
  static void socket_reader_fn(void * arg);
#endif
  void start_socket_reader();

  //! handles the failure to send a message on the client connection
  //! by closing it, deleting it, and deleting all the remote objects.
  void handle_lost_client_socket();
  
  Sss_socket * m_client_socket;
  Sss_socket * m_server_socket; //!< not currently used after init.

  std::map<Object *, Remote_sss_msg::Object_handle> local_objects;
  typedef std::map<Object *, 
    Remote_sss_msg::Object_handle>::iterator Local_objects_it;

  std::map<Remote_sss_msg::Object_handle, Object *> remote_objects;
  typedef std::map<Remote_sss_msg::Object_handle, 
    Object *>::iterator Remote_objects_it;
  
  Remote_sss_queue * m_queue;

  //! A reference time (in milliseconds) exchanged when the connection
  //! is set up. Used to time-adjust update messages.
  int m_local_t0;
  int m_remote_t0;

  //! The estimated one-way lag in milliseconds. This is estimated
  //! when the connection is first made by exchanging some ping
  //! messages. It should only change when there is a complete
  //! resynchronisation of the clocks.
  int m_lag_time;
  
  static Remote_sss_iface * m_instance;

  ///  set to true if we were the server during the initial handshake,
  /// false if not
  bool m_is_server;
};

#endif
