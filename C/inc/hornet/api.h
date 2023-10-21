#ifndef __HORNET_API_H__
#define __HORNET_API_H__

#include <stdio.h>
#include <hornet/type.h>

#ifdef HORNET_WINDOWS
  #define HORNET_API __declspec(dllexport)
#else
  #define HORNET_API
#endif

#ifdef __cplusplus
  #define HORNET_API_BEGIN extern "C" {
  #define HORNET_API_END   }
#else
  #define HORNET_API_BEGIN
  #define HORNET_API_END
#endif

#ifndef null
  #define null NULL
#endif

#ifndef memzero
  #define memzero(_addr, _size) memset(_addr, 0, _size)
#endif

#endif