#pragma once

#include <iostream>

#ifdef HORST_DEBUG
  #define DBG(x) { std::cerr << "  " << __FILE__ << ":" << __LINE__ << " " << __FUNCTION__ << "(): " << x << std::endl << std::flush; }
//#define DBG_JACK(x) { jack_info ("%s:%s:%s: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, x); }
  #define DBG_JACK DBG
  #define DBG_ITEM(x) { std::cerr << x ; }
#else
  #define DBG(x) { }
  #define DBG_JACK(x) { }
  #define DBG_ITEM(x) { }
#endif

#define DBG_ENTER DBG("<- enter...")
#define DBG_EXIT DBG("-> done.")
#define DBG_ENTER_EXIT DBG("<- enter ... -> done")
