/*!
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
  
  \file remote_sss_iface.cpp
*/

#include "remote_sss_iface.h"

#include "remote_sss_queue.h"
#include "sss_socket.h"

// for the application objects
#include "glider.h"
#include "missile.h"
#include "joystick.h"
#include "config_file.h"
#include "sss.h"
#include "config.h"

#include "sss_glut.h"

#include <string.h>
#include <errno.h>

#include <fcntl.h>
#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
#include <signal.h>
#include <unistd.h>
#else
#include <process.h>
#endif

void sleep_seconds(float sec)
{
#ifdef _WIN32
  Sleep((unsigned) (sec * 1000));
#else
  usleep((unsigned) (sec * 1000000));
#endif
}

#include <sstream>
#include <algorithm>
using namespace std;

Remote_sss_iface * Remote_sss_iface::m_instance = 0;

void Remote_sss_iface::create_instance(
  string server_port, // the local port to listen on
  string client_addr,
  string client_port)
{
  if (m_instance == 0)
  {
    // ignore pipe signals
#ifdef unix
    signal(SIGPIPE, SIG_IGN);
#endif
    m_instance = new Remote_sss_iface(server_port,
                                      client_addr,
                                      client_port);
  }
  else
  {
    TRACE("Remote_sss_iface already created!\n");
  }
}

/*!
  
There are two main phases:

1. Connecting as a client to any remote simulation that is
ready.

2. Listening as a server until all remote simulations have been
contacted. This only needs to be done if step 1 failed.

*/

Remote_sss_iface::Remote_sss_iface(
  string server_port, // the local port to listen on
  string client_addr,
  string client_port)
  :
  m_client_socket(0),
  m_server_socket(0),
  m_queue(new Remote_sss_queue),
  m_is_server(false)
{
  TRACE_METHOD_ONLY(1);
  // start a timer
  glutTimerFunc(
    100, // can't access config via sss yet
    Remote_sss_iface::_timer_func, 0);
  
  bool success;
  TRACE_FILE_IF(2)
    TRACE("Creating client socket\n");
  m_client_socket = new Sss_socket(success);
  if (success == false)
  {
    TRACE("Unable to even create client socket!\n");
    delete m_client_socket;
    m_client_socket = 0;
    return;
  }
  
  TRACE_FILE_IF(2) TRACE("Connecting client socket to %s:%s\n", 
                         client_addr.c_str(),
                         client_port.c_str());
  if (0 == m_client_socket->do_connect(client_addr, client_port))
  {
    TRACE_FILE_IF(2)
      TRACE("Connected as client to %s:%s\n",
            client_addr.c_str(),
            client_port.c_str());
    
    // The server on the remote end will send us a config request.
    recv_config_req();
    
    // that's all folks - don't bother with a server for now
    start_socket_reader();
    return;
  }
  
  // Scrap the useless socket we created
  delete m_client_socket;
  m_client_socket = 0;
  
  TRACE_FILE_IF(2)
    TRACE("Creating server socket\n");
  
  m_server_socket = new Sss_socket(success);
  if (success == false)
  {
    TRACE("Unable to even create server socket!\n");
    delete m_server_socket;
    m_server_socket = 0;
    return;
  }
  
  TRACE_FILE_IF(2)
    TRACE("Binding to %s\n", server_port.c_str());
  
  if (0 != m_server_socket->do_bind(server_port))
  {
    TRACE("Unable to bind to port %s\n", server_port.c_str());
    delete m_server_socket;
    m_server_socket = 0;
    return;
  }
  
  if (0 != m_server_socket->do_listen())
  {
    TRACE("Unable to listen on server port\n");
    delete m_server_socket;
    m_server_socket = 0;
    return;
  }
  
  m_client_socket = m_server_socket->do_accept();
  if (m_client_socket == 0)
  {
    TRACE("Server failed to accept a connection\n");
    return;
  }
  
  TRACE_FILE_IF(2)
    TRACE("Accepted a connection:\n");
  m_client_socket->show();
  
  // remember that we are a server
  m_is_server = true;
  
  // get rid of the server socket.
  delete m_server_socket;
  m_server_socket = 0;
  
  // send the config request message
  send_config_req();
  
  // start thread to read from the client connection
  start_socket_reader();
}

void Remote_sss_iface::start_socket_reader()
{
  // start thread to read from the client connection
  static Sss_socket_reader_data socket_reader_data;
  socket_reader_data.socket_ptr = m_client_socket;
  socket_reader_data.queue_ptr = m_queue;
  
#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
  pthread_t thread_id;
  int rv = pthread_create(&thread_id,
                          NULL,
                          socket_reader_fn,
                          &socket_reader_data);
#else
  _beginthread(socket_reader_fn, 0, &socket_reader_data);
  //  AfxBeginThread(socket_reader_fn,
//                 &socket_reader_data);
  int rv = 0;
#endif
  if (rv != 0)
  {
    TRACE("Unable to create a client reading thread!\n");
  }
}


#ifdef unix
void * Remote_sss_iface::socket_reader_fn(void * arg)
#define RET_VAL 0
#else
  void Remote_sss_iface::socket_reader_fn(void * arg)
#define RET_VAL
#endif
{
  while (!Sss::is_instance_available())
    sleep_seconds(0.1f);

  Sss_socket_reader_data * data = (Sss_socket_reader_data *) arg;
  
  Sss_socket * socket_ptr = data->socket_ptr;
  Remote_sss_queue * queue_ptr = data->queue_ptr;
  
  if (socket_ptr == 0)
  {
    TRACE("socket_ptr = 0\n");
    return RET_VAL;
  }
  if (queue_ptr == 0)
  {
    TRACE("queue_ptr = 0\n");
    return RET_VAL;
  }
  TRACE_FILE_IF(1)
    TRACE("socket_reader_fn:\n");
  socket_ptr->show();
  
  while (true)
  {
    Remote_sss_msg * msg = new Remote_sss_msg;
    if (socket_ptr->recv_data((char *) msg, Sss_socket::BLOCK) < 0)
    {
      TRACE("Error receving data from socket!\n");
      socket_ptr->do_shutdown();
      return RET_VAL;
    }
//    cout << "Received a message\n";
    queue_ptr->push(msg);
  }
  return RET_VAL;
#undef RET_VAL
}


void Remote_sss_iface::_timer_func(int val)
{
  m_instance->timer_func(val);
  glutTimerFunc(
    (unsigned) (1000.0/Sss::instance()->config().remote_update_freq),
    Remote_sss_iface::_timer_func, 0);
}

void Remote_sss_iface::timer_func(int val)
{
  // Do inter-simulation sending
  for (map<Object *, Remote_sss_msg::Object_handle>::iterator it = 
         local_objects.begin() ;
       it != local_objects.end() ;
       ++it)
  {
    it->first->send_remote_update();
  }
}

void Remote_sss_iface::recv_msgs()
{
  vector<void *> msgs = m_queue->pop();
  for (vector<void *>::iterator it = msgs.begin();
       it != msgs.end();
       ++it)
  {
    Remote_sss_msg * sss_msg = (Remote_sss_msg *) (*it);
    handle_received_msg(*sss_msg);
    delete sss_msg;
  }
}


void Remote_sss_iface::handle_received_msg(Remote_sss_msg & msg)
{
//  cout << "received a message!\n";
  switch (msg.msg_type)
  {
  case Remote_sss_msg::CREATE:
    handle_create_msg(msg);
    return;
  case Remote_sss_msg::DESTROY:
    handle_destroy_msg(msg);
    return;
  case Remote_sss_msg::UPDATE:
    handle_update_msg(msg);
    return;
  case Remote_sss_msg::TEXT:
    Sss::instance()->recv_text_msg(msg.msg.text.message);
    return;
  default:
    TRACE("Impossible msg type %d\n", msg.msg_type);
    assert1(!"Error!");
  }
  
  TRACE("Impossible msg type! %d\n", msg.msg_type);
}

void Remote_sss_iface::handle_update_msg(Remote_sss_msg & msg)
{
//  cout << "received an update message!\n";
  
  Remote_sss_msg::Object_handle handle = msg.object_handle;
  
  // work out how much this msg was delayed by
  float dt = 0;
  if (Sss::instance()->config().jitter_correct == true)
    dt = (
      (Sss::instance()->get_milliseconds() - m_local_t0) -
      (msg.basic_info.dispatch_time - m_remote_t0)
      ) * 0.001; // in seconds
  if (Sss::instance()->config().lag_correct == true)
    dt += (m_lag_time * 0.001);
  
  static int counter = 0;
  if (counter++ == 100)
  {
    TRACE_FILE_IF(2)
      TRACE("dt correction = %d\n", dt);
    counter = 0;
  }
  
  switch (msg.msg.update.object_type)
  {
  case Remote_sss_update_msg::GLIDER:
  case Remote_sss_update_msg::MISSILE:
  {
    Object * object = remote_objects[handle];
    if (object == 0)
    {
      // hack hack hack alert. maybe this is a glider missile hit
      // update - in which case it's coming from the ghost, and the
      // handle is the object
      object = (Object *) handle;
    }
    assert1(object);
    object->recv_remote_update(msg, dt);
    return;
  }
  default:
    TRACE("Impossible msg type %d\n", msg.msg.update.object_type);
    assert1(!"Error!");
  }
//  cerr << "Impossible update type! " 
//       << msg.msg.remote_sss_update_msg.object_type << endl;
}

void Remote_sss_iface::handle_destroy_msg(Remote_sss_msg & msg)
{
  TRACE_FILE_IF(2)
    TRACE("received a destroy message!\n");
  
  Remote_sss_msg::Object_handle handle = msg.object_handle;
  
  switch (msg.msg.destroy.object_type)
  {
  case Remote_sss_destroy_msg::GLIDER:
  {
    Object * glider = remote_objects[handle];
    assert1(glider);
    TRACE_FILE_IF(2)
      TRACE("Deleting glider %p\n", glider);
    Sss::instance()->remove_object(glider);
    delete glider;
    remote_objects.erase(handle);
    return;
  }
  case Remote_sss_destroy_msg::MISSILE:
  {
    Object * missile = remote_objects[handle];
    assert1(missile);
    TRACE_FILE_IF(2)
      TRACE("Deleting missile %p\n", missile);
    Sss::instance()->remove_object(missile);
    delete missile;
    remote_objects.erase(handle);
    return;
  }
  default:
    TRACE("Impossible msg type %d\n", msg.msg.destroy.object_type);
    assert1(!"Error!");
  }
//  cerr << "Impossible update type! " 
//       << msg.msg.update.object_type << endl;
}


void Remote_sss_iface::handle_create_msg(Remote_sss_msg & msg)
{
  TRACE_FILE_IF(2)
    TRACE("received a create message!\n");
  
  Position pos;
  Velocity vel;
  Orientation orient;
  Rotation rot;
  msg.get_basic_info(pos, vel, orient, rot);
  
  Remote_sss_msg::Object_handle handle = msg.object_handle;
  
  switch (msg.msg.create.object_type)
  {
  case Remote_sss_create_msg::GLIDER:
  {
    // need to create an object and store it together with the
    // handle for future reference.
    string glider_file(msg.msg.create.msg.
                       glider.glider_file);
    bool success;
    Config_file glider_config_file(glider_file,
                                   success);
    assert2(success, "Unable to open glider file");
    // \todo leak the joystick for now
    Joystick * joystick = new Joystick(10);
    TRACE_FILE_IF(2)
      TRACE("Creating remote glider\n");
    Glider * glider = new Glider(glider_config_file,
                                 pos, 0, 
                                 joystick,
                                 false);
    glider->set_pos(pos);
    glider->set_vel(vel);
    glider->set_orient(orient);
    glider->set_rot(rot);
    Sss::instance()->add_object(glider);
    
    remote_objects[handle] = glider;
    
    return;
  }
  case Remote_sss_create_msg::MISSILE:
  {
    // need to create an object and store it together with the
    // handle for future reference.
    
    TRACE_FILE_IF(2)
      TRACE("Creating remote missile\n");
    
    float mass = msg.msg.create.msg.
      missile.mass;
    float length = msg.msg.create.msg.
      missile.length;
    
    Missile * missile = new Missile(pos,
                                    vel,
                                    orient,
                                    rot,
                                    length,
                                    mass,
                                    0, // parent
                                    false);
    Sss::instance()->add_object(missile);
    
    remote_objects[handle] = missile;
    return;
  }
  default:
    TRACE("Impossible msg type %d\n", msg.msg.create.object_type);
    assert1(!"Error!");
  }
  
  TRACE("Impossible create type! %d\n", msg.msg.create.object_type);
}




void Remote_sss_iface::send_create_ind(Object * object,
                                       Remote_sss_msg & msg)
{
  // need to assign a unique (across all simulations) handle to the
  // object pointer and store it.
  // \todo make the handle unique!
  msg.object_handle = (Remote_sss_msg::Object_handle) object;
  
  local_objects[object] = msg.object_handle;
  
  if (m_client_socket == 0)
  {
    TRACE("No client connection to send update on\n");
    return;
  }
  
  if (m_client_socket->send_data((char *) &msg,
                                 msg.get_size(), 
                                 Sss_socket::BLOCK) < 0)
  {
    TRACE("Error sending - closing cxn\n");
    handle_lost_client_socket();
  }
  else
  {
    TRACE_FILE_IF(2)
      TRACE("create msg sent OK\n");
  }
}
void Remote_sss_iface::send_destroy_ind(Object * object,
                                        Remote_sss_msg & msg)
{
  // get the handle
  msg.object_handle = local_objects[object];
  // erase the handle
  local_objects.erase(object);
  
  if (m_client_socket == 0)
  {
    TRACE("No client connection to send destroy on\n");
    return;
  }
  
  if (m_client_socket->send_data((char *) &msg, 
                                 msg.get_size(),
                                 Sss_socket::BLOCK) < 0)
  {
    TRACE("Error sending - closing cxn\n");
    handle_lost_client_socket();
  }
  else
  {
    TRACE_FILE_IF(2)
      TRACE("destroy msg sent OK\n");
  }
  
}

void Remote_sss_iface::send_update_ind_remote(const Object * object,
                                              Remote_sss_msg & msg)
{
  if (m_client_socket == 0)
  {
//    cerr << "No client connection to send update on\n";
    return;
  }
  
  msg.object_handle = 0;
  for (Remote_objects_it it = remote_objects.begin() ; 
       it != remote_objects.end() ; 
       ++it)
  {
    if (it->second == object)
    {
      msg.object_handle = it->first;
    }
  }

  assert1(msg.object_handle);
  
  msg.basic_info.dispatch_time = Sss::instance()->get_milliseconds();
  
  if (m_client_socket->send_data((const char *) &msg, 
                                 msg.get_size(),
                                 Sss_socket::BLOCK) < 0)
  {
    TRACE("Error sending - closing cxn\n");
    handle_lost_client_socket();
  }
  else
  {
//    cout << "update msg sent OK\n";
  }
}



void Remote_sss_iface::send_update_ind(const Object * object,
                                       Remote_sss_msg & msg)
{
  if (m_client_socket == 0)
  {
//    cerr << "No client connection to send update on\n";
    return;
  }
  
  msg.object_handle = local_objects[(Object *)object];
  assert1(msg.object_handle);
  
  msg.basic_info.dispatch_time = Sss::instance()->get_milliseconds();
  
  if (m_client_socket->send_data((const char *) &msg, 
                                 msg.get_size(),
                                 Sss_socket::BLOCK) < 0)
  {
    TRACE("Error sending - closing cxn\n");
    handle_lost_client_socket();
  }
  else
  {
//    cout << "update msg sent OK\n";
  }
}
void Remote_sss_iface::send_msg(Remote_sss_msg & msg)
{
  if (m_client_socket == 0)
  {
//    cerr << "No client connection to send update on\n";
    return;
  }
  
  if (m_client_socket->send_data((const char *) &msg, 
                                 msg.get_size(),
                                 Sss_socket::BLOCK) < 0)
  {
    TRACE("Error sending - closing cxn\n");
    handle_lost_client_socket();
  }
  else
  {
//    cout << "update msg sent OK\n";
  }
}

/*!  Note that we keep our record of local objects (in case the
  reconnection gets coded...) */
void Remote_sss_iface::handle_lost_client_socket()
{
  if (m_client_socket == 0)
  {
    TRACE("Client socket already gone!\n");
    return;
  }
  
  m_client_socket->do_close();
  delete m_client_socket;
  m_client_socket = 0;
  
  for (Remote_objects_it it = remote_objects.begin() ;
       it != remote_objects.end();
       ++it)
  {
    // handle glider differently for now...
    Glider * glider;
    if (0 != (glider = dynamic_cast<Glider *> (it->second)))
    {
      Sss::instance()->remove_object(glider);
      delete glider;
    }
    else
    {
      delete it->second;
    }
  }
}

//! Sends a configuration msg request and waits synchronously for the
//! response. Called by a server.
void Remote_sss_iface::send_config_req()
{
  
  Remote_sss_msg req_msg;
  req_msg.msg_type = Remote_sss_msg::CONFIG_REQ;
  m_local_t0 = Sss::instance()->get_milliseconds();
  req_msg.msg.config_req.server_time_zero = m_local_t0;
  
  // send msg
  if (m_client_socket->send_data((char *) &req_msg,
                                 req_msg.get_size(),
                                 Sss_socket::BLOCK) < 0)
  {
    TRACE("Error sending config req message - closing cxn\n");
    handle_lost_client_socket();
    return;
  }
  
  // read the response - NOTE that we must not have started the reader
  // thread at this point!
  
  Remote_sss_msg resp_msg;
  if (m_client_socket->recv_data((char *) &resp_msg, Sss_socket::BLOCK) < 0)
  {
    TRACE("Error receiving config resp message - closing cxn\n");
    handle_lost_client_socket();
    return;
  }
  
  if (resp_msg.msg_type != Remote_sss_msg::CONFIG_RESP)
  {
    TRACE("Config response is not a CONFIG_RESP!\n");
    handle_lost_client_socket();
    return;
  }
  
  m_remote_t0 = resp_msg.msg.config_resp.client_time_zero;
  
  TRACE_FILE_IF(2)
    TRACE("Local t0 = %d, remote t0 = %d\n",
          m_local_t0,
          m_remote_t0);
  
  
  // Send a ping
  req_msg.msg_type = Remote_sss_msg::PING_REQ;
  int t1 = Sss::instance()->get_milliseconds();
  m_client_socket->send_data((char *) &req_msg,
                             req_msg.get_size(),
                             Sss_socket::BLOCK);
  
// simulate lag
//    struct timespec tv = 
//    {
//      0, 750000000
//    };
//    nanosleep(&tv, 0);
  
  // recv response
  m_client_socket->recv_data((char *) &resp_msg, Sss_socket::BLOCK);
  int t2 = Sss::instance()->get_milliseconds();
  
  // send a response response
  resp_msg.msg_type = Remote_sss_msg::PING_RESP;
  m_client_socket->send_data((char *) &resp_msg,
                             resp_msg.get_size(),
                             Sss_socket::BLOCK);
  
  m_lag_time = (t2 - t1)/2;
  TRACE_FILE_IF(2)
    TRACE("One-way lag is %d ms\n", m_lag_time);
}

//! Waits for a config request and sends a response. Called by a
//! client
void Remote_sss_iface::recv_config_req()
{
  
  // read the request - NOTE that we must not have started the reader
  // thread at this point!
  
  Remote_sss_msg req_msg;
  if (m_client_socket->recv_data((char *) &req_msg, Sss_socket::BLOCK) < 0)
  {
    TRACE("Error receiving config req message - closing cxn\n");
    handle_lost_client_socket();
    return;
  }
  
  if (req_msg.msg_type != Remote_sss_msg::CONFIG_REQ)
  {
    TRACE("Config request is not a CONFIG_REQ!\n");
    handle_lost_client_socket();
    return;
  }
  
  m_remote_t0 = req_msg.msg.config_req.server_time_zero;
  
  // now send response
  
  Remote_sss_msg resp_msg;
  resp_msg.msg_type = Remote_sss_msg::CONFIG_RESP;
  m_local_t0 = Sss::instance()->get_milliseconds();
  resp_msg.msg.config_resp.client_time_zero = m_local_t0;
  
  // send msg
  if (m_client_socket->send_data((char *) &resp_msg,
                                 resp_msg.get_size(),
                                 Sss_socket::BLOCK) < 0)
  {
    TRACE("Error sending config resp message - closing cxn\n");
    handle_lost_client_socket();
    return;
  }
  
  TRACE_FILE_IF(2)
    TRACE("Local t0 = %d remote t0 = %d\n",
          m_local_t0,
          m_remote_t0);
  
  // recv ping request
  m_client_socket->recv_data((char *) &req_msg, Sss_socket::BLOCK);
  
  int t1 = Sss::instance()->get_milliseconds();
  
  // Send a ping response
  resp_msg.msg_type = Remote_sss_msg::PING_RESP;
  m_client_socket->send_data((char *) &resp_msg,
                             resp_msg.get_size(),
                             Sss_socket::BLOCK);
  
  // recv response response
  m_client_socket->recv_data((char *) &resp_msg, Sss_socket::BLOCK);
  
  int t2 = Sss::instance()->get_milliseconds();
  
  m_lag_time = (t2 - t1)/2;
  TRACE_FILE_IF(2)
    TRACE("One-way lag is %d ms\n", m_lag_time);
}
