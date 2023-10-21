#ifndef __HORNET_H__
#define __HORNET_H__

#include <hornet/socket.h>
#include <hornet/time.h>

HORNET_API_BEGIN
  HORNET_API bool hornet_init();
  HORNET_API void hornet_exit();
HORNET_API_END

#endif