#include <hornet/socket.h>

typedef struct {
  Socket socket;
  struct addrinfo* addrinfo;
} ResolveSocket;

static Socket opensocket(int32 addressFamily, int32 socketType, int32 protocol){
  return socket(addressFamily, socketType, protocol);
}

static bool setsocketoption(Socket socket, int32 optionName, int32 value){
  return setsockopt(socket, SOL_SOCKET, optionName, (string)&value, sizeof(value)) != SOCKET_ERROR;
}

static bool setsocketreuseaddr(Socket socket){
  return setsocketoption(socket, SO_REUSEADDR, 1);
}

static bool setsocketnonblock(Socket socket, bool value){
  long ioctlValue = value;
  return ioctlsocket(socket, FIONBIO, &ioctlValue) != SOCKET_ERROR;
}

static bool isnonblockerror(int32 error){
#ifdef HORNET_WINDOWS
  return error == WSAEWOULDBLOCK;
#else
  return error == EINPROGRESS;
#endif
}

static Socket socket_bind(int32 addressFamily, int32 socketType, int32 protocol, struct sockaddr* sockaddr, int32 sockaddrLength){
  Socket socket = opensocket(addressFamily, socketType, protocol);
  if (socket == INVALID_SOCKET) return INVALID_SOCKET;

  if (!setsocketreuseaddr(socket)){
    closesocket(socket);
    return INVALID_SOCKET;
  }

  if (bind(socket, sockaddr, sockaddrLength) == SOCKET_ERROR){
    closesocket(socket);
    return INVALID_SOCKET;
  }
  return socket;
}

static Socket socket_connect_async(int32 addressFamily, int32 socketType, int32 protocol, struct sockaddr* sockaddr, int32 sockaddrLength){
  Socket socket = opensocket(addressFamily, socketType, protocol);
  if (socket == INVALID_SOCKET) return INVALID_SOCKET;

  if (!setsocketnonblock(socket, true)){
    closesocket(socket);
    return INVALID_SOCKET;
  }

  if (connect(socket, sockaddr, sockaddrLength) == SOCKET_ERROR){
    if (!isnonblockerror(WSAGetLastError())){
      closesocket(socket);
      return INVALID_SOCKET;
    }
  }
  return socket;
}

static int32 socket_read(HornetSocket* socket, void* buffer, int32 length, int32 flags){
  switch (socket->address.socketType){
    case SOCK_STREAM:{
      return recv(socket->socket, buffer, length, flags);
    }break;

    case SOCK_DGRAM:{
      socket->address.sockaddrLength = sizeof(socket->address.sockaddr);
      return recvfrom(socket->socket, buffer, length, flags, &(socket->address.sockaddr.base), &(socket->address.sockaddrLength));
    }break;
  }
  return SOCKET_ERROR;
}

static int32 socket_write(HornetSocket* socket, void* buffer, int32 length, int32 flags){
  switch (socket->address.socketType){
    case SOCK_STREAM:{
      return send(socket->socket, buffer, length, flags);
    }break;

    case SOCK_DGRAM:{
      return sendto(socket->socket, buffer, length, flags, &(socket->address.sockaddr.base), socket->address.sockaddrLength);
    }break;
  }
  return SOCKET_ERROR;
}

static int32 socket_type_from_string(string socketType){
  if (strcmp(socketType, "udp") == 0) return SOCK_DGRAM;
  if (strcmp(socketType, "tcp") == 0) return SOCK_STREAM;
  return 0;
}

static void socket_address_set(HornetSocketAddress* address, struct addrinfo* addrinfo){
  int32 sockaddrLength = (int32)(addrinfo->ai_addrlen);
  address->socketType = addrinfo->ai_socktype;
  address->sockaddrLength = sockaddrLength;
  memcpy(&(address->sockaddr.base), addrinfo->ai_addr, sockaddrLength);
}

static HornetSocket* socket_accept_peer_new(Socket socket, int32 socketType, HornetSockaddr* sockaddr, int32 sockaddrLength){
  HornetSocket* peerSocket = malloc(sizeof(HornetSocket));
  if (peerSocket != null){
    peerSocket->socket = socket;
    peerSocket->address.socketType = socketType;
    peerSocket->address.sockaddrLength = sockaddrLength;
    memcpy(&(peerSocket->address.sockaddr.base), &(sockaddr->base), sockaddrLength);
  }
  return peerSocket;
}

typedef HornetSocket* (*ResolveAddressEvent)(struct addrinfo* root);

static HornetSocket* resolve_address(string socketType, string host, string port, ResolveAddressEvent onResolveAddress){
  HornetSocket* socket = null;
  struct addrinfo hints;
  struct addrinfo* root = null;
  memzero(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = socket_type_from_string(socketType);
  if (getaddrinfo(host, port, &hints, &root) == 0) socket = onResolveAddress(root);
  if (root != null) freeaddrinfo(root);
  return socket;
}

static HornetSocket* on_resolve_address_peer(struct addrinfo* root){
  HornetSocket* peerSocket = malloc(sizeof(HornetSocket));
  if (peerSocket == null) return null;
  for (struct addrinfo* addrinfo = root; addrinfo != null; addrinfo = addrinfo->ai_next){
    Socket socket = opensocket(addrinfo->ai_family, addrinfo->ai_socktype, 0);
    if (socket == INVALID_SOCKET) continue;

    peerSocket->socket = socket;
    socket_address_set(&(peerSocket->address), addrinfo);
    return peerSocket;
  }
  free(peerSocket);
  return null;
}

static HornetSocket* on_resolve_address_bind(struct addrinfo* root){
  HornetSocket* bindSocket = malloc(sizeof(HornetSocket));
  if (bindSocket == null) return null;
  for (struct addrinfo* addrinfo = root; addrinfo != null; addrinfo = addrinfo->ai_next){
    Socket socket = socket_bind(addrinfo->ai_family, addrinfo->ai_socktype, 0, addrinfo->ai_addr, (int32)(addrinfo->ai_addrlen));
    if (socket == INVALID_SOCKET) continue;

    bindSocket->socket = socket;
    socket_address_set(&(bindSocket->address), addrinfo);
    if (bindSocket->address.socketType != SOCK_STREAM) return bindSocket;

    if (listen(bindSocket->socket, SOMAXCONN) != SOCKET_ERROR) return bindSocket;
    closesocket(socket);
    break;
  }
  free(bindSocket);
  return null;
}

static HornetSocket* on_resolve_address_connect(struct addrinfo* root){
  HornetSocket* connectSocket = malloc(sizeof(HornetSocket));
  if (connectSocket == null) return null;

  int32 count = 0;
  for (struct addrinfo* addrinfo = root; addrinfo != null; addrinfo = addrinfo->ai_next){
    ++count;
  }
  ResolveSocket* resolveSockets = (0 < count) ? malloc(sizeof(ResolveSocket) * count) : null;
  if (resolveSockets == null){
    free(connectSocket);
    return null;
  }

  fd_set readFds;
  FD_ZERO(&readFds);
  int32 i = 0;
  for (struct addrinfo* addrinfo = root; addrinfo != null; addrinfo = addrinfo->ai_next, ++i){
    Socket socket = socket_connect_async(addrinfo->ai_family, addrinfo->ai_socktype, 0, addrinfo->ai_addr, (int32)(addrinfo->ai_addrlen));
    if (socket != INVALID_SOCKET){
      FD_SET(socket, &readFds);
    }
    resolveSockets[i].socket = socket;
    resolveSockets[i].addrinfo = addrinfo;
  }

  connectSocket->socket = INVALID_SOCKET;
  struct timeval timeout = {3, 0};
  if (0 < select(0, null, &readFds, null, &timeout)){
    for (i = 0; i < count; ++i){
      if (resolveSockets[i].socket == INVALID_SOCKET) continue;

      if (FD_ISSET(resolveSockets[i].socket, &readFds)){
        connectSocket->socket = resolveSockets[i].socket;
        socket_address_set(&(connectSocket->address), resolveSockets[i].addrinfo);
        break;
      }
    }
  }
  for (i = 0; i < count; ++i){
    if ((resolveSockets[i].socket != INVALID_SOCKET) && (resolveSockets[i].socket != connectSocket->socket)){
      closesocket(resolveSockets[i].socket);
    }
  }
  free(resolveSockets);
  if (connectSocket->socket != INVALID_SOCKET) return connectSocket;
  free(connectSocket);
  return null;
}

HornetSocketAddress* hornet_socket_get_address(HornetSocket* socket){
  return &(socket->address);
}

string hornet_socket_address_to_string(HornetSocketAddress* address){
  size_t length;
  switch (address->socketType){
    case SOCK_STREAM:{
      length = snprintf(address->stringBytes, sizeof(address->stringBytes), "tcp@");
    }break;

    case SOCK_DGRAM:{
      length = snprintf(address->stringBytes, sizeof(address->stringBytes), "udp@");
    }break;

    default:{
      length = snprintf(address->stringBytes, sizeof(address->stringBytes), "%d@", address->socketType);
    }break;
  }
  void* ipAddr = null;
  uint16 port = 0;
  switch (address->sockaddr.base.sa_family){
    case AF_INET:{
      ipAddr = &(address->sockaddr.ipv4.sin_addr);
      port = ntohs(address->sockaddr.ipv4.sin_port);
    }break;

    case AF_INET6:{
      ipAddr = &(address->sockaddr.ipv6.sin6_addr);
      port = ntohs(address->sockaddr.ipv6.sin6_port);
    }break;
  }
  if (ipAddr != null){
    inet_ntop(address->sockaddr.base.sa_family, ipAddr, &(address->stringBytes[length]), sizeof(address->stringBytes) - length);
    length = strlen(address->stringBytes);
    snprintf(&(address->stringBytes[length]), sizeof(address->stringBytes) - length, ":%d", port);
  }
  return address->stringBytes;
}

HornetSocket* hornet_socket_resolve_address_peer(string host, string port){
  return resolve_address("udp", host, port, on_resolve_address_peer);
}

HornetSocket* hornet_socket_resolve_address_bind(string socketType, string host, string port){
  return resolve_address(socketType, host, port, on_resolve_address_bind);
}

HornetSocket* hornet_socket_resolve_address_connect(string socketType, string host, string port){
  return resolve_address(socketType, host, port, on_resolve_address_connect);
}

HornetSocket* hornet_socket_connect_async(HornetSocket* connectedSocket){
  HornetSocket* connectingSocket = malloc(sizeof(HornetSocket));
  if (connectedSocket != null){
    connectingSocket->socket = socket_connect_async(connectedSocket->address.sockaddr.base.sa_family, connectedSocket->address.socketType, 0, &(connectedSocket->address.sockaddr.base), connectedSocket->address.sockaddrLength);
    if (connectingSocket->socket != INVALID_SOCKET){
      connectingSocket->address = connectedSocket->address;
      return connectingSocket;
    }
  }
  free(connectingSocket);
  return null;
}

HornetSocket* hornet_socket_accept(HornetSocket* bindSocket){
  HornetSockaddr sockaddr;
  int32 sockaddrLength = sizeof(sockaddr);
  Socket socket = accept(bindSocket->socket, &(sockaddr.base), &(sockaddrLength));
  if (socket != INVALID_SOCKET){
    return socket_accept_peer_new(socket, bindSocket->address.socketType, &(sockaddr), sockaddrLength);
  }
  return null;
}

int32 hornet_socket_read(HornetSocket* socket, void* buffer, int32 length){
  return socket_read(socket, buffer, length, 0);
}

int32 hornet_socket_write(HornetSocket* socket, void* buffer, int32 length){
  return socket_write(socket, buffer, length, 0);
}

void hornet_socket_close(HornetSocket* socket){
  if (socket->socket != INVALID_SOCKET) closesocket(socket->socket);
  free(socket);
}