#pragma once
#include "Common.h"

namespace CollabVM {

	// Simple logger class
	// Shamelessly plagarized from let's play

	struct Logger {

		// get a logger with a group name
		static Logger GetLogger(std::string group_name) {
			Logger l;
			l.group_name = group_name;
			return l;
		}

		// Allow verbose messages
		// NOTE: This applies to all groups made with Logger::GetLogger()
		static bool AllowVerbose;

		// Logging functions

		template<typename T, typename ...Args>
		inline void info(const T value, Args... args) {
			std::lock_guard<std::mutex> lock(mutex);
			std::ostringstream ss;
			ss << TimestampString() << "[" << group_name << "/INFO] " << Stringify(value, args...) << '\n';
			std::cout << ss.str();
			ss.clear();
		}

		template<typename T, typename ...Args>
		inline void warn(const T value, Args... args) {
			std::lock_guard<std::mutex> lock(mutex);
			std::ostringstream ss;
			ss << TimestampString() << "[" << group_name << "/WARNING] " << Stringify(value, args...) << '\n';
			std::cout << ss.str();
			ss.clear();
		}

		template<typename T, typename ...Args>
		inline void error(const T value, Args... args) {
			std::lock_guard<std::mutex> lock(mutex);
			std::ostringstream ss;
			ss << TimestampString() << "[" << group_name << "/ERROR] " << Stringify(value, args...) << '\n';
			std::cout << ss.str();
			ss.clear();
		}


		template<typename T, typename ...Args>
		inline void verbose(const T value, Args... args) {
			if(!Logger::AllowVerbose)
				return;
			std::lock_guard<std::mutex> lock(mutex);
			std::ostringstream ss;
			ss << TimestampString() << "[" << group_name << "/VERBOSE] " << Stringify(value, args...) << '\n';
			std::cout << ss.str();
			ss.clear();
		}


	private:

		inline Logger() {
		}

		inline Logger(Logger&& c) {
			this->group_name = c.group_name;
		}

		static std::string TimestampString() {
			std::chrono::time_point<std::chrono::system_clock> p = std::chrono::system_clock::now();

			std::time_t t = std::chrono::system_clock::to_time_t(p);
			std::string ts = std::string(std::ctime(&t));

			ts.back() = ']';
			ts.push_back(' ');
			ts.insert(ts.begin(), '[');

			return ts;
		}

		template<typename Head, typename... Tail>
		static std::string Stringify(Head h, Tail... t) {
			std::string s;
			StringifyImpl(s, h, t...);
			return s;
		}

		template<typename Head, typename...Tail>
		static void StringifyImpl(std::string& string, Head h, Tail... t) {
			StringifyImpl(string, h);
			StringifyImpl(string, t...);
		}

		template<typename T>
		static void StringifyImpl(std::string& string, T item) {
			std::ostringstream ss;
			ss << item;
			const std::string str = ss.str();

			string += str;
		}

		std::string group_name;
		std::mutex mutex;
	};

}