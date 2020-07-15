#pragma once
// Common header.
// NOTE: This is PCH'd, so be careful what you put here

#include <ctime>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <map>
#include <deque>
#include <vector>
#include <mutex>
#include <thread>
#include <functional>
#include <filesystem>
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

	// Call a function object
	// if the function object is valid
	template<typename Function, class ...Args>
	inline void CheckedFunctionCall(const Function& func, Args... args) {
		if(func)
			func(std::forward(args)...);
	}

}


// Include sdkddkver.h
// to make Boost.Asio not spew warnings about it
// every single time it's referenced
#ifdef _WIN32
#if __has_include(<sdkddkver.h>)
	#include <sdkddkver.h>
#endif
#endif

#ifdef _WIN32
#define COLLABVM_WINDOWS
#elif defined(__linux__)
#define COLLABVM_LINUX
#else
#endif


// Boost
// TODO: move this to places that need it
#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace fs = std::filesystem;

namespace net = boost::asio;
namespace beast = boost::beast;

namespace http = beast::http;
namespace ws = beast::websocket;

using tcp = net::ip::tcp;