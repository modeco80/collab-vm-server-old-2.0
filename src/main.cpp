#include <iostream>
#include "Common.h"
#include "CollabVMServer.h"
#include "Logger.h"

#include <boost/program_options.hpp>

using namespace CollabVM;

namespace po = boost::program_options;

template<typename Fun>
inline void Worker(Fun function) {
	// TODO try/catch guards
	function();
}

int main(int argc, char** argv) {
	uint16 port;

	po::variables_map vm;
	po::options_description desc("CollabVM Server command line options");	
	desc.add_options()
		("help", "Print this help message")
		("verbose", "Enable verbose debug logging")
		("version", "Output version of CollabVM Server")
		("port", po::value<decltype(port)>(), "Server port (default 6004)");


	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if(vm.count("help")) {
		std::cout << desc;
		return 0;
	}

	if(vm.count("version")) {
		std::stringstream builder;
		builder << "CollabVM 2.0 Server\n";
		builder << "(C) 2020 Lily\n\n";
		builder << "Third Party Libraries:\n";
		builder << "Boost C++ Version " << (BOOST_VERSION / 100000) << '.' <<  (BOOST_VERSION / 100 % 1000) << '.' << (BOOST_VERSION % 100) << '\n';
		std::cout << builder.str();
		return 0;
	}

	if(vm.count("port")) {
		try {
			port = vm["port"].as<uint16>();
		} catch (...) {
			std::cout << "Error: Port specified is invalid\n";
			return 1;
		}
	}

	if(vm.count("verbose")) {
		Logger::AllowVerbose = true;
	}


	//net::io_context ioc;

	//std::thread thread([&]() {
	//	Worker([&ioc]() {
	//		ioc.run();
	//	});
	//});
	

	//thread.join();
	return 0;
}