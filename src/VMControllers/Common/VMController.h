#include <Common.h>
#include <Server.h>
#include "ControllerStatus.h"
#include <Protocol.h>
#include <UserList.h>

namespace CollabVM {
	
	// Base interface for VM controllers to implement.
	struct VMController : public std::enable_shared_from_this<VMController> {
	
		// Constructor.
		inline VMController(std::shared_ptr<Server> server)
			: server(server) {
			// This constructor only serves the purpose
			// of receving a handle to the server.
			// Nothing more, nothing less
		}

		virtual ~VMController() {
			// remove ownership explicitly when destructing
			server.reset();
		}

		virtual bool Start() = 0;

		virtual void OnStateChange() = 0;

		// Join a user to the VM controller.
		inline void Join(std::shared_ptr<User> user) {
			userlist.AddUser(user);
			user->vm = shared_from_this();
		}

		inline void Leave(std::shared_ptr<User> user) {
			userlist.RemoveUser(user);
			user->vm.reset();
		}

		// Implementation-defined value
		// to detect VM controller type.
		const byte Type;

	private:

		// Pointer to server
		std::shared_ptr<Server> server;

		UserList userlist;

		ControllerStatus status;
	};

}