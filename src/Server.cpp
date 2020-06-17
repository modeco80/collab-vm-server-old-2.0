#include "Server.h"
#include "User.h"

namespace CollabVM {

	Server::~Server() {
	}

	void Server::Start(net::io_service& ioc, uint16 port) {
		logger.verbose("Starting processing thread before WebSockets");

		processingThread = std::thread(&Server::ProcessActions, this);

		base::Start(ioc, port);

		server->run();

		processingThread.join();
	}

	void Server::Stop() {
		stopProcessing = true;
		base::Stop();
	}

	
	bool Server::OnWebsocketValidate(base::handle_type userHdl) {
		// TODO: Implement connection limit & soft-bans
		// (shouldn't be hard)
		logger.info("OnWebsocketValidate() called");
		return true;
	}

	void Server::OnWebsocketOpen(base::handle_type userHdl) {
		logger.info("OnWebsocketOpen() called");
	}

	void Server::OnWebsocketMessage(base::handle_type userHdl, base::message_type message) {
		logger.info("OnWebsocketMessage() called");
	}

	void Server::OnWebsocketClose(base::handle_type userHdl) {
		logger.info("OnWebsocketClose() called");
	}


	void Server::ProcessActions() {
		logger.verbose("Processing thread started");

		while(!stopProcessing) {
			std::lock_guard<std::mutex> lock(workLock);

			// don't bother continuing on
			if (work.size() == 0)
				continue;

			IAction* action = work.front();
			work.pop_front();

			switch(action->type) {
				case ActionType::AddConnection: {
					// TODO
				} break;
			}

			delete action;
		}
	}


}