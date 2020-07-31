#include <iostream>
#include "Common.h"
#include "Server.h"
#include "Logger.h"

#ifdef COLLABVM_LINUX
	#define BOOST_STACKTRACE_USE_BACKTRACE
#endif

#include <boost/stacktrace.hpp>
#include <boost/program_options.hpp>

using namespace CollabVM;
namespace po = boost::program_options;

std::string laddr = "0.0.0.0";
std::string webroot = "http";
uint16 port = 6004;

net::ip::address address;
net::io_service ioc;

std::shared_ptr<net::io_service::work> work;
std::shared_ptr<Server> server;

constexpr static char dumpfile[] = "./Server_Stacktrace.dmp";

Logger mainlogger = Logger::GetLogger("Main");

void StopServer() {
	ioc.stop();
	work.reset();
	server->Stop();
	server.reset();
}

void SignalHandler(boost::system::error_code ec, int sig) {
	if(sig == SIGSEGV || sig == SIGABRT) {
		// We can't really print to the console,
		// so we dump the stack trace to a file.
		boost::stacktrace::safe_dump_to(dumpfile);
		raise(SIGABRT);
		return;
	}

	StopServer();
}

template<typename F>
inline void Worker(F function) {
	try {
		function();
	} catch(std::exception& ex) {
		// Get the stack trace as early as possible so we have a "pristine" one
		auto stacktrace = boost::stacktrace::stacktrace();
		mainlogger.error("Got exception: ", ex.what());
		mainlogger.error("Stack trace:", stacktrace);
		StopServer();
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
		("port", po::value<uint16>(), "Server port (default 6004)");

	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
	} catch(po::error_with_option_name& er) {
		std::cout << "Invalid option " << er.get_option_name() << ".\n";
		return 1;
	}

	po::notify(vm);

	if(vm.count("help")) {
		std::cout << desc;
		return 0;
	}

	if(vm.count("version")) {
		std::stringstream builder;
		builder << "CollabVM 2.0 Server\n";
		builder << "(C) 2020 Lily (Computernewb Development Team)\n\n";
		builder << "Third Party Libraries:\n";
		builder << "Boost C++ Version " << (BOOST_VERSION / 100000) << '.' <<  (BOOST_VERSION / 100 % 1000) << '.' << (BOOST_VERSION % 100) << '\n';
		builder << "ASIO (Boost) Version " << (BOOST_ASIO_VERSION / 100000) << '.' <<  (BOOST_ASIO_VERSION / 100 % 1000) << '.' << (BOOST_ASIO_VERSION % 100) << '\n';
		std::cout << builder.str();
		return 0;
	}

	if(vm.count("listen")) {
		try {
			laddr = vm["listen"].as<std::string>();
			address = net::ip::make_address(laddr);
		} catch(...) {
			std::cout << "Invalid listen address\n";
			return 1;
		}
	}


	if(vm.count("port")) {
		try {
			port = vm["port"].as<uint16>();
		} catch (...) {
			std::cout << "Invalid port specified\n";
			return 1;
		}
	}

	// allow verbose messages on all channels
	if(vm.count("verbose"))
		Logger::AllowVerbose = true;

	work = std::make_shared<net::io_service::work>(ioc);
	server = std::make_shared<Server>(ioc);

	net::signal_set signal(ioc, SIGINT, SIGABRT, SIGSEGV);
	signal.async_wait(SignalHandler);

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

	return 0;
}