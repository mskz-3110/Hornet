#include <hornet/time.h>
#include <time.h>
#ifdef HORNET_WINDOWS
  #include <windows.h>
#endif

double hornet_time_now(){
#ifdef HORNET_WINDOWS
  struct timespec ts;
  timespec_get(&ts, TIME_UTC);
  return (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000);
#else
  struct timeval tv;
  gettimeofday(&tv, null);
  return (double)tv.tv_sec + ((double)tv.tv_usec / 1000000);
#endif
}

void hornet_time_sleep_msec(uint32 msec){
#ifdef HORNET_WINDOWS
  Sleep(msec);
#else
  struct timespec ts;
  ts.tv_sec = msec / 1000;
  ts.tv_nsec = (msec % 1000) * 1000000;
  nanosleep(&ts, null);
#endif
}