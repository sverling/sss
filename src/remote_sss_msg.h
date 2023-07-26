/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/
#ifndef REMOTE_SSS_MSG_H
#define REMOTE_SSS_MSG_H

#include "remote_sss_glider_msg.h"
#include "remote_sss_body_msg.h"
#include "remote_sss_missile_msg.h"
#include "remote_sss_explosion_msg.h"

//! Message describing creation indications
struct Remote_sss_create_msg
{
  size_t get_size() const
    {
      size_t this_size = sizeof(object_type);
      switch (object_type)
      {
      case GLIDER:    return this_size + sizeof(msg.glider);
      case BODY:      return this_size + sizeof(msg.body);
      case MISSILE:   return this_size + sizeof(msg.missile);
      case EXPLOSION: return this_size + sizeof(msg.explosion);
      }
      return sizeof(*this);
    }
  enum Object_type
  {
    GLIDER,
    BODY,
    MISSILE,
    EXPLOSION
  } object_type;
  
  union Object_specific_msg
  {
    Remote_sss_create_glider_msg    glider;
    Remote_sss_create_body_msg      body;
    Remote_sss_create_missile_msg   missile;
    Remote_sss_create_explosion_msg explosion;
  } msg;
};

//! Message describing destroy indications
struct Remote_sss_destroy_msg
{
  size_t get_size() const
    { return sizeof(*this); }

  enum Object_type
  {
    GLIDER,
    BODY,
    MISSILE,
    EXPLOSION
  } object_type;
};

//! Message describing the update indications
struct Remote_sss_update_msg
{
  size_t get_size() const
    {
      size_t this_size = sizeof(object_type);
      switch (object_type)
      {
      case GLIDER:    return this_size + sizeof(msg.glider);
      case BODY:      return this_size + sizeof(msg.body);
      case MISSILE:   return this_size + sizeof(msg.missile);
      case EXPLOSION: return this_size + sizeof(msg.explosion);
      }
      return sizeof(*this);
    }

  enum Object_type
  {
    GLIDER,
    BODY,
    MISSILE,
    EXPLOSION
  } object_type;  

  // the structures here contain object-specific information
  union Object_specific_msg
  {
    Remote_sss_update_glider_msg    glider;
    Remote_sss_update_body_msg      body;
    Remote_sss_update_missile_msg   missile;
    Remote_sss_update_explosion_msg explosion;
  } msg;
};

// This is sent by the server
struct Remote_config_req_msg
{
  size_t get_size() const {return sizeof(*this);}

  int server_time_zero; // stored by sender and receiver
  
};

// This is sent by the client
struct Remote_config_resp_msg
{
  size_t get_size() const {return sizeof(*this);}

  int client_time_zero; // stored by sender and receiver
  
};

struct Remote_sss_ping_req_msg
{
  size_t get_size() const {return sizeof(*this);}
};

struct Remote_sss_ping_resp_msg
{
  size_t get_size() const {return sizeof(*this);}
};

// This is sent by the server
struct Remote_sss_text_msg
{
  size_t get_size() const {return sizeof(*this);}

  char message[80];
  
};

//! This is the top-level remote-flight message
struct Remote_sss_msg
{
  size_t get_size() const
    {
      size_t this_size = ( sizeof(object_handle) + 
                           sizeof(msg_type) + 
                           sizeof(basic_info) );
      switch (msg_type)
      {
      case CONFIG_REQ:  return this_size + msg.config_req.get_size();
      case CONFIG_RESP:  return this_size + msg.config_resp.get_size();
      case PING_REQ: return this_size + msg.ping_req.get_size();
      case PING_RESP: return this_size + msg.ping_resp.get_size();
      case TEXT: return this_size + msg.text.get_size();
      case CREATE:  return this_size + msg.create.get_size();
      case DESTROY: return this_size + msg.destroy.get_size();
      case UPDATE:  return this_size + msg.update.get_size();
      }
      return sizeof(*this);
    }
  

  // This handle may need to become more complex. It is assigned by
  // the infrastucture
  typedef size_t Object_handle;
  Object_handle object_handle;

  struct Basic_info
  {
    float pos[3];
    float vel[3];
    float orient[9];
    float rot[3];
    int dispatch_time; // time at which the message was sent
  } basic_info;

  // The following should be set by the application
  enum Msg_type
  {
    CONFIG_REQ,
    CONFIG_RESP,
    PING_REQ,
    PING_RESP,
    TEXT,
    CREATE,
    DESTROY,
    UPDATE
  } msg_type;

  union 
  {
    Remote_config_req_msg config_req;
    Remote_config_resp_msg config_resp;
    Remote_sss_ping_req_msg ping_req;
    Remote_sss_ping_resp_msg ping_resp;
    Remote_sss_text_msg text;
    Remote_sss_create_msg create;
    Remote_sss_destroy_msg destroy;
    Remote_sss_update_msg update;
  } msg;

  // all objects will want to transmit the following parameters
  // (except for in destroy)
  inline void set_basic_info(const Position & pos,
                             const Velocity & vel,
                             const Orientation & orient,
                             const Rotation & rot);
  inline void get_basic_info(Position & pos,
                             Velocity & vel,
                             Orientation & orient,
                             Rotation & rot);
  
};

void Remote_sss_msg::set_basic_info(const Position & pos,
                                       const Velocity & vel,
                                       const Orientation & orient,
                                       const Rotation & rot)
{
  memcpy(basic_info.pos, pos.get_data(), 3*sizeof(pos[0]));
  memcpy(basic_info.vel, vel.get_data(), 3*sizeof(vel[0]));
  memcpy(basic_info.orient, orient.get_data(), 9*sizeof(orient(0,0)));
  memcpy(basic_info.rot, rot.get_data(), 3*sizeof(rot[0]));
}

void Remote_sss_msg::get_basic_info(Position & pos,
                                       Velocity & vel,
                                       Orientation & orient,
                                       Rotation & rot)
{
  pos.set_data(basic_info.pos);
  vel.set_data(basic_info.vel);
  orient.set_data(basic_info.orient);
  rot.set_data(basic_info.rot);
}

#endif
