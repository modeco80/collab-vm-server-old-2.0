#pragma once

#include <ctime>
#include <iostream>
#include <cstdint>
#include <vector>
#include <mutex>
#include <thread>
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>

namespace CollabVM {
	// Prefer these typedefs over
	// standard library ones

	typedef std::uint8_t byte;
	typedef std::int8_t sbyte;
	
	typedef std::uint16_t uint16;
	typedef std::int16_t int16;

	typedef std::uint32_t uint32;
	typedef std::int32_t int32;

	typedef std::uint64_t uint64;
	typedef std::int64_t int64;

}

// Include sdkddkver.h
// to make Boost.Asio not spew warnings about it
// every single time it's referenced
#ifdef _WIN32
#ifdef _MSC_VER
	#include <sdkddkver.h>
#endif
#endif

#include <boost/asio.hpp>

namespace net = boost::asio;
using tcp = net::ip::tcp;
