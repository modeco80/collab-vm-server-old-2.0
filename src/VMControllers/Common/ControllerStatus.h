#include <Common.h>

namespace CollabVM {

	enum class ControllerStatus : byte {
		Stopped,
		Starting,
		Started,
		Resetting
	};

}