#pragma once
#include "Common.h"
#include "WebsocketServer.h"
#include "Logger.h"

namespace CollabVM {
	
	enum class ActionType {
		AddConnection,
		RemoveConnection,
		IpDataTimeout
	};

	struct IAction {
		ActionType type;

		IAction(ActionType t)
			: type(t) {
		
		}
	};

	struct AddConnectionAction : public IAction {
		
		AddConnectionAction() 
			: IAction(ActionType::AddConnection) {
		
		}
	};

	struct Server : public WebsocketServer {
		typedef WebsocketServer base;

		~Server();

		void Start(net::io_service& ioc, uint16 port);

		void Stop();

		bool OnWebsocketValidate(base::handle_type userHdl);

		void OnWebsocketOpen(base::handle_type userHdl);

		void OnWebsocketMessage(base::handle_type userHdl, base::message_type message);

		void OnWebsocketClose(base::handle_type userHdl);

	private:
		void ProcessActions();

		// processing thread
		std::thread processingThread;

		// mutex locking work
		// if locked by thread, thread
		// can modify to it's own will
		std::mutex workLock;

		bool stopProcessing = false;

		// deque of work
		// locked by workLock
		std::deque<IAction*> work;		


		Logger logger = Logger::GetLogger("CollabVMServer");
	};

}