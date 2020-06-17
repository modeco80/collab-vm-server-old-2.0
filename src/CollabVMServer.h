#pragma once
#include "Common.h"
#include "Webserver.h"
#include "Logger.h"

namespace CollabVM {



	// TODO
	struct Server : public Webserver {
	

	private:
		bool stop = false;

		Logger logger = Logger::GetLogger("Server");
	};

}