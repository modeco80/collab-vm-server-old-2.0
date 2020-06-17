#pragma once
#include "Common.h"

namespace CollabVM {

	struct Webserver {

		typedef websocket::stream<tcp::socket> socket_type;
		
		typedef beast::flat_buffer buffer_type;

		void Start(tcp::endpoint addr) {} // no op for right now

		void Stop() {} // no op

		virtual void OnOpen() = 0;

	protected:
		int todo;
	};

}