/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk
*/

#ifndef SSS_SOCKET_H
#define SSS_SOCKET_H

#include <string>

/*!

  This encapsulates a socket offering the kind of functionality needed
  by the SSS flight simulator.

 */
class Sss_socket
{
public:
  enum Blocking {BLOCK, NO_BLOCK};
  Sss_socket(bool & success);
  ~Sss_socket(); // closes the socket if necessary
  
  // when acting as a server. These are all blocking
  int do_bind(std::string & port);
  int do_listen();
  Sss_socket * do_accept(); //!< returns new socket - caller needs to delete
  
  // when acting as a client. This is a potentially blocking call.
  int do_connect(std::string & ip_address, std::string & port);
  
  //! Closes the socket
  void do_close();

  //! Shuts the socket down (for reading and writing)
  void do_shutdown();

  //! Sending data. Returns number of bytes sent (possibly 0?), or -1
  //! in case of socket failure. If block = BLOCK, this will only
  //! return once all data has been sent (unless there was a socket
  //! error).
  int send_data(const char * msg, int len, Blocking block);
  
  //! Reading data. Returns number of bytes read (possibly 0?), or -1
  //! in case of socket failure. If block is BLOCK, this will only 
  //! return once all data has been received (unless there was a socket
  //! error).
  int recv_data(char * buf, Blocking block);
  
  //! Returns the file descriptor - e.g. for use in select
  int get_fd() const {return m_fd;}

  //! Displays the vital statistics
  void show() const;

private:
  Sss_socket(int fd, std::string remote_ip, std::string remote_port)
    : m_fd(fd), m_local_port("unset"), 
    m_remote_ip(remote_ip), m_remote_port(remote_port) {}
  
  int m_fd;
  std::string m_local_port, m_remote_ip, m_remote_port;
};


#endif
