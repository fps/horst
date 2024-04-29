#pragma once

#include <iostream>
#include <ctime>

namespace lv2_horst
{
  std::string timestamp ()
  {
    timespec tp;
    clock_gettime (CLOCK_REALTIME, &tp);
    
    char buffer[64];
    
    sprintf(buffer, "%010ld.%03ld", tp.tv_sec, tp.tv_nsec / 1000000);
    return buffer;
  }
}

#ifdef HORST_DEBUG
  #define DBG(x) { std::cerr << " [DBG] " << timestamp() << " " << __builtin_frame_address(0) << " " << __FILE_NAME__ << ":" << __LINE__ << " " << __FUNCTION__ << "(): " << x << std::endl << std::flush; }
#else
  #define DBG(x) { }
#endif

#define INFO(x) { std::cout << "[INFO] " << timestamp() << " " << __FILE_NAME__ << ":" << __LINE__ << " " << __FUNCTION__ << "(): " << x << std::endl << std::flush; }

#define DBG_ENTER DBG("...")
#define DBG_EXIT DBG(".")
#define DBG_ENTER_EXIT { DBG_ENTER DBG_EXIT }
