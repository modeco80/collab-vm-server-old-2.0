#pragma once
#include "Common.h"
#include "WebsocketServer.h"
#include <collabvm_generated.h> // For UserType

namespace CollabVM {

	// Forward decl
	struct VMController;

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
			return "Error";
		}
	};

	// User data structure
	struct User {

		User(WebsocketServer::handle_type handle, std::shared_ptr<IPData> ipdata)
			: handle(handle), ipData(ipdata) {
			type = UserType::Guest;
		}

		~User() {
			// release ownership of session handle
			handle.reset();
		}

		// handle to session that this user connected with
		WebsocketServer::handle_type handle;

		// IPData of the user.
		std::shared_ptr<IPData> ipData;

		// Username of the user
		std::string username;

		// pointer to VMController user is on,
		// nullptr if they are not connected
		std::shared_ptr<VMController> vm;

		UserType type;


		// Generates a guest name, if the user didn't join with one
		static inline std::string GenerateGuestName() {

			// Guest name prefix. Change to your liking
			constexpr static char NamePrefix[] = "guest";

			// Max ID.
			constexpr uint64 MaxID = 99999;
			
			// made static so that this won't happen every single call
			// (since that would be slow) and instead on the first call to GenerateGuestName
			static std::uniform_int_distribution<int> dist(0, MaxID);
			static std::mt19937 mt(std::chrono::high_resolution_clock::now().time_since_epoch().count());

			auto str = std::to_string(dist(mt));

			std::stringstream ss;
			ss << NamePrefix << str;
			
			// TODO: There's probably a far better way of doing this, 
			// but it's the most relatively cheapest way and it works,
			// so whatever.

			// Pad names too small (under or at 3 characters) with additional zeroes 
			// example `guest202` to `guest20200`
			if(str.size() <= 3) {
				const auto pad_count = 5 - str.size();
				for(int i = 0; i < pad_count; ++i)
					ss << '0';
			}

			return ss.str();
		}
	};

}