#pragma once
#include "Common.h"

namespace CollabVM {

	// Per-IP address data
	struct IPData {
		net::ip::address address;

		uint64 connection_count;

		inline bool SafeToDelete() {
			return connection_count == 0;
		}
	};


	struct User {
	
		// IPData of the user.
		IPData* ipData;
	};

}