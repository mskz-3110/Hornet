#include <hornet.h>

bool hornet_init(){
#if HORNET_WINDOWS
  WSADATA wsadata;
  if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) return false;
#endif
  return true;
}

void hornet_exit(){
#if HORNET_WINDOWS
  WSACleanup();
#endif
}