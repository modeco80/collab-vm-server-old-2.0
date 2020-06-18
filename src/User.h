#pragma once
#include "Common.h"
#include "WebsocketServer.h"

namespace CollabVM {

	// Per-IP address data
	struct IPData {
		net::ip::address address;

		uint64 connection_count;

		IPData(net::ip::address& addr)
			: address(addr) {
		
		}

		inline bool SafeToDelete() {
			return connection_count == 0;
		}

		inline std::string str() {
			return address.to_string();
		}
	};


	struct User {

		User(WebsocketServer::connection_type con, IPData* ipdata)
			: conptr(con), ipData(ipdata) {
		
		}

		// Connection pointer
		WebsocketServer::connection_type conptr;


		// IPData of the user.
		IPData* ipData;
	};

}