#pragma once

#include <iostream>

#ifdef HORST_DEBUG
  #define DBG(x) { std::cerr << " [DBG] " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << "(): " << x << std::endl << std::flush; }
//#define DBG_JACK(x) { jack_info ("%s:%s:%s: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, x); }
  #define DBG_JACK DBG
  #define DBG_ITEM(x) { std::cerr << " [DBG] " << x ; }
#else
  #define DBG(x) { }
  #define DBG_JACK(x) { }
  #define DBG_ITEM(x) { }
#endif

#define INFO(x) { std::cerr << "[INFO] " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << "(): " << x << std::endl << std::flush; }

#define DBG_ENTER DBG("...")
#define DBG_EXIT DBG(".")
#define DBG_ENTER_EXIT { DBG_ENTER DBG_EXIT }
