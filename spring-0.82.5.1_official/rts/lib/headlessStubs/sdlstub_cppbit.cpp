/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/*
 * This wraps a couple of useful boost c++ time functions,
 * for use in sdlstub
 * (which is written in C, and can not easily access C++ stuff directly)
 */

#include "boost/thread.hpp"

#include "boost/version.hpp"
#if BOOST_VERSION < 105000
#include "boost/thread/xtime.hpp"
namespace boost {
	enum xtime_compat {
		TIME_UTC_=TIME_UTC
	};
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

int stub_sdl_getSystemMilliSeconds() {

	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC_);
	const int milliSeconds = t.sec * 1000 + (t.nsec / 1000000);   
	return milliSeconds;
}

void stub_sdl_sleepMilliSeconds(int milliSeconds) {

	boost::xtime t;
	boost::xtime_get(&t, boost::TIME_UTC_);
	t.nsec += 1000000 * milliSeconds;
	boost::thread::sleep(t);
}

#ifdef __cplusplus
} // extern "C"
#endif

