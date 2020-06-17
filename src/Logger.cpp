#include "Common.h"
#include "Logger.h"

namespace CollabVM {

	bool Logger::AllowVerbose = false;

	// meh, I don't like this

	Logger mainLogger = Logger::GetLogger("Main");

}