#include <hornet.h>

static void on_tcp(string socketType, string host, string port, char* writeBuffer, char* readBuffer){
  HornetSocket* connectSocket = hornet_socket_resolve_address_connect(socketType, host, port);
  printf("connectSocket=%p\n", connectSocket);
  if (connectSocket != null){
    printf("peerAddress=%s\n", hornet_socket_address_to_string(hornet_socket_get_address(connectSocket)));
    int32 writeLength = hornet_socket_write(connectSocket, writeBuffer, (int32)strlen(writeBuffer));
    hornet_time_sleep_msec(100);
    memzero(readBuffer, sizeof(readBuffer));
    int32 readLength = hornet_socket_read(connectSocket, readBuffer, sizeof(readBuffer));
    printf("writeLength=%d readLength=%d readBuffer=%s\n", writeLength, readLength, readBuffer);
    hornet_socket_close(connectSocket);
  }
}

static void on_udp(string host, string port, char* writeBuffer, char* readBuffer){
  HornetSocket* peerSocket = hornet_socket_resolve_address_peer(host, port);
  printf("peerSocket=%p\n", peerSocket);
  if (peerSocket != null){
    printf("peerAddress=%s\n", hornet_socket_address_to_string(hornet_socket_get_address(peerSocket)));
    int32 writeLength = hornet_socket_write(peerSocket, writeBuffer, (int32)strlen(writeBuffer));
    hornet_time_sleep_msec(100);
    memzero(readBuffer, sizeof(readBuffer));
    int32 readLength = hornet_socket_read(peerSocket, readBuffer, sizeof(readBuffer));
    printf("writeLength=%d readLength=%d readBuffer=%s\n", writeLength, readLength, readBuffer);
    hornet_socket_close(peerSocket);
  }
}

int main(int argc, string argv[]){
  if (!hornet_init()) return -1;

  string socketType = "tcp";
  if (2 <= argc) socketType = argv[1];
  string host = "127.0.0.1";
  if (3 <= argc) host = argv[2];
  string port = "12345";
  if (4 <= argc) port = argv[3];
  char writeBuffer[65536] = "default";
  char readBuffer[65536];
  if (5 <= argc) snprintf(writeBuffer, sizeof(writeBuffer), "[%s]", argv[4]);

  printf("socketType=%s host=%s port=%s writeBuffer=%s\n", socketType, host, port, writeBuffer);
  if (strcmp(socketType, "tcp") == 0){
    on_tcp(socketType, host, port, writeBuffer, readBuffer);
  }else if (strcmp(socketType, "udp") == 0){
    on_udp(host, port, writeBuffer, readBuffer);
  }

  hornet_exit();
  return 0;
}