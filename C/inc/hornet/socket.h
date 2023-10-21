#ifndef __HORNET_SOCKET_H__
#define __HORNET_SOCKET_H__

#include <hornet/api.h>

#ifdef HORNET_WINDOWS
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #define Socket SOCKET
#else
  #define Socket int32
  #define INVALID_SOCKET -1
  #define SOCKET_ERROR -1
  #define closesocket
#endif

typedef union {
  struct sockaddr     base;
  struct sockaddr_in  ipv4;
  struct sockaddr_in6 ipv6;
} HornetSockaddr;

typedef struct {
  char stringBytes[64];
  int32 socketType;
  int32 sockaddrLength;
  HornetSockaddr sockaddr;
} HornetSocketAddress;

typedef struct {
  Socket socket;
  HornetSocketAddress address;
} HornetSocket;

HORNET_API_BEGIN
  HORNET_API HornetSocketAddress* hornet_socket_get_address(HornetSocket* socket);
  HORNET_API string hornet_socket_address_to_string(HornetSocketAddress* address);
  HORNET_API HornetSocket* hornet_socket_resolve_address_peer(string host, string port);
  HORNET_API HornetSocket* hornet_socket_resolve_address_bind(string socketType, string host, string port);
  HORNET_API HornetSocket* hornet_socket_resolve_address_connect(string socketType, string host, string port);
  HORNET_API HornetSocket* hornet_socket_connect_async(HornetSocket* connectedSocket);
  HORNET_API HornetSocket* hornet_socket_accept(HornetSocket* bindSocket);
  HORNET_API int32 hornet_socket_read(HornetSocket* socket, void* buffer, int32 length);
  HORNET_API int32 hornet_socket_write(HornetSocket* socket, void* buffer, int32 length);
  HORNET_API void hornet_socket_close(HornetSocket* socket);
HORNET_API_END

#endif