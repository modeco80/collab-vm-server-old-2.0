#include <iostream>
#include "Common.h"
#include "Server.h"
#include "Logger.h"

#include <boost/program_options.hpp>

using namespace CollabVM;

namespace po = boost::program_options;

std::string laddr = "0.0.0.0";
uint16 port = 6004;

net::ip::address address;

net::io_service ioc;
net::io_service::work* work;
Server* server;

Logger mainlogger = Logger::GetLogger("Main");

void StopServer() {
	delete work;
	server->Stop();
	delete server;
}

template<typename F>
inline void Worker(F function) {
	try {
		function();
	} catch(std::exception& ex) {
		mainlogger.error("Got exception: ", ex.what());
	}
}

int main(int argc, char** argv) {

	po::variables_map vm;
	po::options_description desc("CollabVM Server command line options");	
	desc.add_options()
		("help", "Print this help message")
		("verbose", "Enable verbose debug logging")
		("version", "Output version of CollabVM Server")
		("listen", po::value<std::string>(),  "Listen address (default 0.0.0.0)")
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
		builder << "(C) 2020 Lily (Computernewb Development Team)\n\n";
		builder << "Versions of Third Party Libraries:\n";
		builder << "Boost C++ Version " << (BOOST_VERSION / 100000) << '.' <<  (BOOST_VERSION / 100 % 1000) << '.' << (BOOST_VERSION % 100) << '\n';
		builder << "ASIO (Boost) Version " << (BOOST_ASIO_VERSION / 100000) << '.' <<  (BOOST_ASIO_VERSION / 100 % 1000) << '.' << (BOOST_ASIO_VERSION % 100) << '\n';
		std::cout << builder.str();
		return 0;
	}

	if(vm.count("listen")) {
		try {
			laddr = vm["listen"].as<std::string>();
		} catch(...) {
			std::cout << "Error: invalid listen address\n";
			return 1;
		}
	}

	address = net::ip::make_address(laddr);

	if(vm.count("port")) {
		try {
			port = vm["port"].as<uint16>();
		} catch (...) {
			std::cout << "Error: Port specified is invalid\n";
			return 1;
		}
	}

	// allow verbose messages on all "channels"
	if(vm.count("verbose"))
		Logger::AllowVerbose = true;

	work = new net::io_service::work(ioc);

	server = new Server(ioc);

	std::thread thread([]() {
		Worker([]() {
			ioc.run();
		});
	});

	Worker([]() {
		tcp::endpoint ep{address, port};
		server->Start(ep);
	});

	thread.join();
	delete server;
	return 0;
}