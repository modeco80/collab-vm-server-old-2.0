#pragma once
#include "Common.h"
#include "WebsocketServer.h"
#include <collabvm_generated.h> // For UserType

namespace CollabVM {

	// Per-IP address data
	struct IPData {
		// The IP address.
		net::ip::address address;

		// Amount of connections from this IP address.
		uint64 connection_count = 0;

		// more fields here as they're needed

		IPData(net::ip::address& addr)
			: address(addr) {
		
		}

		// returns true if the IPData
		// is safe to be cleaned up.
		inline bool SafeToDelete() {
			return connection_count == 0;
		}

		inline std::string str() {
			if (address.is_v4()) {
				return address.to_v4().to_string();
			} else if (address.is_v6()) {
				if(address.to_v6().is_v4_mapped())
					return address.to_v4().to_string();

				return address.to_v6().to_string();
			}

			// Just to satisify the compiler honestly
			return "Unknown IP";
		}
	};

	// User data structure
	struct User {

		User(WebsocketServer::handle_type handle, IPData* ipdata)
			: handle(handle), ipData(ipdata) {
			type = UserType::Guest;
		}

		~User() {
			// release ownership of session handle
			handle.reset();
		}

		// handle to session
		WebsocketServer::handle_type handle;

		// IPData of the user.
		IPData* ipData;

		UserType type;
	};

}