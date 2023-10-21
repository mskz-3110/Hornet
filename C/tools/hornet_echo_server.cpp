#include <hornet.h>

static void on_tcp(HornetSocket* bindSocket){
  HornetSocket* peerSocket = hornet_socket_accept(bindSocket);
  printf("peerSocket=%p\n", peerSocket);
  if (peerSocket != null){
    char buffer[65536];
    while (true){
      memzero(buffer, sizeof(buffer));
      int32 readLength = hornet_socket_read(peerSocket, buffer, sizeof(buffer));
      if (readLength <= 0) break;

      int32 writeLength = hornet_socket_write(peerSocket, buffer, readLength);
      printf("peerAddress=%s buffer=%s readLength=%d writeLength=%d\n",
        hornet_socket_address_to_string(hornet_socket_get_address(peerSocket)), buffer, readLength, writeLength);
      break;
    }
    hornet_socket_close(peerSocket);
  }
}

static void on_udp(HornetSocket* bindSocket){
  char buffer[65536];
  memzero(buffer, sizeof(buffer));
  int32 readLength = hornet_socket_read(bindSocket, buffer, sizeof(buffer));
  int32 writeLength = hornet_socket_write(bindSocket, buffer, readLength);
  printf("peerAddress=%s buffer=%s readLength=%d writeLength=%d\n",
    hornet_socket_address_to_string(hornet_socket_get_address(bindSocket)), buffer, readLength, writeLength);
}

int main(int argc, string argv[]){
  if (!hornet_init()) return -1;

  string socketType = "tcp";
  if (2 <= argc) socketType = argv[1];
  string host = "127.0.0.1";
  if (3 <= argc) host = argv[1];
  string port = "12345";
  if (4 <= argc) port = argv[1];

  printf("socketType=%s host=%s port=%s\n", socketType, host, port);
  HornetSocket* bindSocket = hornet_socket_resolve_address_bind(socketType, host, port);
  printf("bindSocket=%p\n", bindSocket);
  if (bindSocket != null){
    printf("bindAddress=%s\n", hornet_socket_address_to_string(hornet_socket_get_address(bindSocket)));
    if (strcmp(socketType, "tcp") == 0){
      on_tcp(bindSocket);
    }else if (strcmp(socketType, "udp") == 0){
      on_udp(bindSocket);
    }
    hornet_socket_close(bindSocket);
  }

  hornet_exit();
  return 0;
}