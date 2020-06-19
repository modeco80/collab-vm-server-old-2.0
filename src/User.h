#pragma once
#include "Common.h"
#include "WebsocketServer.h"

namespace CollabVM {

	enum class UserType : byte {
		Guest,
		Registered,
		Admin
	};

	// Per-IP address data
	struct IPData {
		// The IP address.
		net::ip::address address;

		// Amount of connections
		uint64 connection_count = 0;

		IPData(net::ip::address& addr)
			: address(addr) {
		
		}

		// returns true if the IPData
		// is safe to delete
		inline bool SafeToDelete() {
			return connection_count == 0;
		}

		inline std::string str() {
			if (address.is_v4())
				return address.to_v4().to_string();
			else if (address.is_v6()) {
				if(address.to_v6().is_v4_mapped())
					return address.to_v4().to_string();
				return address.to_v6().to_string();
			}

			// Just to satisify the compiler honestly
			return "Unknown IP";
		}
	};


	struct User {

		User(WebsocketServer::connection_type con, IPData* ipdata)
			: conptr(con), ipData(ipdata) {
			type = UserType::Guest;
		}

		// Connection pointer
		WebsocketServer::connection_type conptr;


		// IPData of the user.
		IPData* ipData;

		UserType type;
	};

}