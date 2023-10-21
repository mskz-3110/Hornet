#ifndef __HORNET_TIME_H__
#define __HORNET_TIME_H__

#include <hornet/api.h>

HORNET_API_BEGIN
  HORNET_API double hornet_time_now();
  HORNET_API void hornet_time_sleep_msec(uint32 msec);
HORNET_API_END

#endif