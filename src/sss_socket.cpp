/*
  Sss - a slope soaring simulater.
  Copyright (C) 2002 Danny Chapman - flight@rowlhouse.freeserve.co.uk

  \file socket.cpp
*/

#include "sss_socket.h"

#include <errno.h>
#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#ifdef __APPLE__
// POSIX says we shouldn't need this! Should be in sys/socket.h.
//typedef int socklen_t
#endif
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <unistd.h>
#define closesocket close
#else
#include <winsock2.h>
#endif
#include <sys/types.h>
#include "sss_assert.h"

#include <sstream>
using namespace std;

Sss_socket::Sss_socket(bool & success)
  :
  m_fd(-1), m_local_port("unset"), m_remote_ip("unset"), m_remote_port("unset")
{
  // let's be optimistic
  success = true;

#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
#else
  WORD wVersionRequested;
  WSADATA wsaData;

  wVersionRequested = MAKEWORD( 2, 2 );
 
  int err = WSAStartup( wVersionRequested, &wsaData );
  if ( err != 0 ) 
  {
    TRACE("Unable to start Windows socket\n");
    m_fd = -1;
    success = false;
    return;
  }
#endif 

  if ( (m_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    TRACE("Error creating socket: %s\n", strerror(errno));
    m_fd = -1;
    success = false;
    return;
  }
  
  // set some options.

  // disable Nagle
  int flag = 1;
  if (setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, (const char *) &flag, sizeof(flag)) < 0)
  {
    TRACE("Error setting TCP_NODELAY: %s\n", strerror(errno));
    closesocket(m_fd);
    m_fd = -1;
    success = false;
    return;
  }

  flag = 1;
  if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&flag, sizeof(flag)) < 0)
  {
    TRACE("Error setting SO_REUSEADDR: %s\n", strerror(errno));
    closesocket(m_fd);
    m_fd = -1;
    success = false;
    return;
  }
}

Sss_socket::~Sss_socket()
{
  if (m_fd != -1)
    do_close();
}

int Sss_socket::do_bind(string & port)
{
  istringstream is(port);
  unsigned short port_no;
  is >> port_no;

  // set up the server address
  struct sockaddr_in server_addr;
//  bzero(&server_addr, sizeof(server_addr));
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(port_no);

  if ( bind(m_fd, 
            (const sockaddr*) &server_addr, 
            sizeof(server_addr) ) < 0 )
  {
    TRACE("Unable to bind to server socket: %s\n", strerror(errno));
    do_close();
    return -1;
  }

  // set the local port string
  m_local_port = port;
  return 0;
}

int Sss_socket::do_listen()
{
  if ( listen(m_fd, 5) < 0 )
  {
    TRACE("Unable to listen on server socket: %s\n", strerror(errno));
    do_close();
    return -1;
  }
  return 0;
}

Sss_socket * Sss_socket::do_accept()
{
  struct sockaddr_in client_addr;
#ifdef unix
  socklen_t len = sizeof(client_addr);
#else
  int len = sizeof(client_addr);
#endif
  int new_fd = accept(m_fd, (sockaddr*) &client_addr, &len);
  if (new_fd < 0)
  {
    TRACE("server socket accept failure: %s\n", strerror(errno));
    return 0;
  }

  string remote_ip(inet_ntoa(client_addr.sin_addr));
  ostringstream os;
  os << ntohs(client_addr.sin_port);
  string remote_port=string(os.str());

  TRACE_FILE_IF(2)
    TRACE("Detected connection from %s:%s\n", 
          remote_ip.c_str(),
          remote_port.c_str());

  // Note that we inherit the options from the original socket

  return (new Sss_socket(new_fd, remote_ip, remote_port));
}

int Sss_socket::do_connect(string & ip_address, string & port)
{
  struct sockaddr_in server_addr;
  unsigned short port_no;
  istringstream is(port);
  is >> port_no;
  
//  bzero(&server_addr, sizeof(server_addr));
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
#ifdef unix
  if (inet_pton(AF_INET, 
                ip_address.c_str(), 
                &server_addr.sin_addr) <= 0)
  {
    TRACE("error dealing with %s\n", ip_address.c_str());
    do_close();
    return -1;
  }
#else
  //! \todo do this under win32
  server_addr.sin_addr.s_addr = inet_addr(ip_address.c_str());
#endif
  server_addr.sin_port = htons(port_no);
  
  if (connect(m_fd, 
              (const sockaddr*) &server_addr, 
              sizeof(server_addr)) < 0)
  {
#if defined(__APPLE__) || defined(MACOSX) || defined(unix)
    if (errno == ECONNREFUSED)
#else
      if (errno == WSAECONNREFUSED)
#endif
      {
        TRACE("Connection to %s:%s refused\n", ip_address.c_str(), port.c_str());
        return -1;
      }
      else
      {
        TRACE("Error connecting: %s\n", strerror(errno));
        return -1;
      }
  }

  m_remote_ip = ip_address;
  m_remote_port = port;
  TRACE("Connected to %s:%s\n", m_remote_ip.c_str(), m_remote_port.c_str());
  return 0;
}

void Sss_socket::do_shutdown()
{
  if (m_fd == -1)
  {
    TRACE("socket fd already closed\n");
  }

  if (shutdown(m_fd, 2) < 0)
  {
    TRACE("Error shutting down fd %d: %s\n", m_fd, strerror(errno));
    show();
  }
}


void Sss_socket::do_close()
{
  TRACE_FILE_IF(1)
    TRACE("Closing fd %d\n", m_fd);
  
  if (m_fd == -1)
  {
    TRACE("socket fd already closed\n");
  }
  else if (closesocket(m_fd) < 0)
  {
    TRACE("Error closing fd %d: %s\n", m_fd, strerror(errno));
    show();
  }

  m_fd = -1;
  m_local_port = "unset";
  m_remote_ip = "unset";
  m_remote_port = "unset";
}

void Sss_socket::show() const
{
  TRACE("Sss_socket fd: %d, local port %s, remote = %s:%s\n",
        m_fd,
        m_local_port.c_str(),
        m_remote_ip.c_str(),
        m_remote_port.c_str());
}

int Sss_socket::send_data(const char * msg, int len, Blocking block)
{
  struct 
  {
    unsigned short len;
    char data[512];
  } hdr_msg;
  assert1(len < 512);
  
  hdr_msg.len = len;
  memcpy(&hdr_msg.data[0], msg, len);
  const char * buf_ptr = (char *) &hdr_msg;

  unsigned flag;

  if (block == BLOCK)
  {
    flag = 0;
  }
  else
  {
#ifdef unix
    flag = MSG_DONTWAIT;
#else
    //! \todo how to do this under win32?
    flag = 0;
#endif
  }

  // socket is naturally blocking
  int bytes_sent = 0;
  int bytes_to_send = len + sizeof(unsigned short);

  // The blocking/non-blocking behaviour is determined only by the
  // flag passed to send.
  
  while (true)
  {
    int rv = send(m_fd, &buf_ptr[bytes_sent], bytes_to_send, flag);
    if (rv == bytes_to_send)
    {
//      cout << "send_data: Wrote " << rv << " bytes as required.\n";
      return 0;
    }
    else if (rv == 0)
    {
      TRACE("send_data: Write returned 0!\n");
      return -1;
    }
    else if (rv < 0)
    {
      TRACE("send_data: Write returned %d: %s\n", rv, strerror(errno));
      if (errno != EINTR)
      {
        return -1;
      }
      else
      {
        // if errno == EINTR we try again
        TRACE("send_data: Sss_socket got EINTR\n");
      }
    }
    else
    {
      bytes_sent += rv;
      bytes_to_send -= rv;
//        cout << "send_data: Wrote " << rv << " bytes, " 
//             << bytes_to_send << " remaining\n";
    }
  }
  TRACE("send_data: Shouldn't get here on write!\n");
  return -1;
}

//! Note that recv_data will generally be called in a different thread
//! to send_data. So is closing the socket here thread safe? Probably
//! not - so we wait until a send fails, and close it in the sending
//! thread.
int Sss_socket::recv_data(char * buf, Blocking block)
{
  if (block != BLOCK)
  {
    TRACE("non-blocking read not supported!\n");
    return -1;
  }
  unsigned short bytes_to_read;
  
  int rv = recv(m_fd, (char *) &bytes_to_read, sizeof(bytes_to_read), 0);
  if (rv != 2)
  {
    if (rv == 0)
    {
      TRACE("End of file read - cxn broken\n");
      return -1;
    }
    else if (rv == 1)
    {
      TRACE("Only 1 byte received!\n");
      return -1;
    }
    else if (rv < 0)
    {
      TRACE("recv returned %d: %s\n", rv, strerror(errno));
      return -1;
    }
  }
  
//  cout << "recv_data: Expecting to read " << bytes_to_read << " bytes\n";

  // now the main message
  int bytes_read = 0;
  while (true)
  {
    int rv = recv(m_fd, &buf[bytes_read], bytes_to_read, 0);
    if (rv == bytes_to_read)
    {
//      cout << "recv_data: Read " << bytes_to_read << " bytes as expected\n";
      return 0;
    }
    else if (rv == 0)
    {
      TRACE("recv_data: End of file read - cxn broken\n");
      return -1;
    }
    else if (rv < 0)
    {
      TRACE("recv_data: Read returned %d: %s\n", rv, strerror(errno));
      if (errno != EINTR)
      {
        TRACE("recv_data: Read error\n");
        return -1;
      }
      else
      {
        // if errno == EINTR we try again
        TRACE("recv_data: Sss_socket got EINTR\n");
      }
    }
    else
    {
      bytes_read += rv;
      bytes_to_read -= rv;
//        cout << "recv_data: Read " << rv << " bytes, " 
//             << bytes_to_read << " remaining.\n";
    }
  }

  TRACE("Shouldn't get here on read!\n");
  return -1;
}

 
