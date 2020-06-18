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

		std::error_code ec;
		auto conPtr = server->get_con_from_hdl(userHdl, ec);

		if(ec)
			return false;

		auto protocols = conPtr->get_requested_subprotocols();

		for(auto& protocol : protocols) {
			if (protocol == "cvm2") {
				conPtr->select_subprotocol(protocol);

				auto endpoint = conPtr->get_raw_socket().remote_endpoint();
				auto addr = endpoint.address();

				if(FindIPData(addr) == nullptr) {
					CreateIPData(addr);
				}
				
				FindIPData(addr)->connection_count++;
				return true;
			}
		}

		return false;
	}

	void Server::OnWebsocketOpen(base::handle_type userHdl) {
		std::error_code ec;
		auto conPtr = server->get_con_from_hdl(userHdl, ec);

		if(ec)
			return;

		AddWork(new AddConnectionAction(conPtr));
	}

	void Server::OnWebsocketMessage(base::handle_type userHdl, base::message_type message) {
		std::error_code ec;
		auto conPtr = server->get_con_from_hdl(userHdl, ec);

		if(ec)
			return;

		// exclude text messages from work,
		// we don't use those 
		if(message->get_opcode() == websocketpp::frame::opcode::binary)
			AddWork(new MessageAction(conPtr, message));
	}

	void Server::OnWebsocketClose(base::handle_type userHdl) {
		std::error_code ec;
		auto conPtr = server->get_con_from_hdl(userHdl, ec);

		if(ec)
			return;

		
		AddWork(new RemoveConnectionAction(conPtr));
	}

	IPData* Server::FindIPData(net::ip::address& address) {
		std::lock_guard<std::mutex> lock(ipDataLock);

		if(address.is_v4()) {
			auto v4addr = address.to_v4();
			auto it = ipv4data.find(v4addr.to_uint());
			if (it == ipv4data.end())
				return nullptr;
			return it->second;
		} else if(address.is_v6()) {
			auto v6addr = address.to_v6();
			if(v6addr.is_v4_mapped()) {
				auto v4addr = address.to_v6().to_v4();
				auto it = ipv4data.find(v4addr.to_uint());
				if (it == ipv4data.end())
					return nullptr;
				return it->second;
			} else {
				auto it = ipv6data.find(v6addr.to_bytes());
				if (it == ipv6data.end())
					return nullptr;
				return it->second;
			}
		}
		return nullptr;
	}

	void Server::CreateIPData(net::ip::address& address) {
		std::lock_guard<std::mutex> lock(ipDataLock);

		if(address.is_v4()) {
			auto v4addr = address.to_v4();
			uint32 ip = v4addr.to_uint();
			ipv4data[ip] = new IPData(address);
		} else if(address.is_v6()) {
			auto v6addr = address.to_v6();
			if(v6addr.is_v4_mapped()) {
				auto v4addr = address.to_v6().to_v4();
				uint32 ip = v4addr.to_uint();
				ipv4data[ip] = new IPData(address);
			} else {
				auto ip = v6addr.to_bytes();
				ipv6data[ip] = new IPData(address);
			}
		}
	}

	void Server::CleanupIPData() {
		std::lock_guard<std::mutex> lock(ipDataLock);

		for(auto& ip : ipv4data) {
			IPData* ipData = ip.second;

			if(ipData) {
				if(ipData->SafeToDelete()) {
					auto addr = ipData->address;
					delete ipData;
					ipv4data.erase(ipv4data.find(addr.to_v4().to_uint()));
				}
			} else {
				continue;
			}
		}

		for(auto& ip : ipv6data) {
			IPData* ipData = ip.second;

			if(ipData) {
				if(ipData->SafeToDelete()) {
					auto addr = ipData->address;
					delete ipData;
					ipv6data.erase(ipv6data.find(addr.to_v6().to_bytes()));
				}
			} else {
				continue;
			}
		}
	}

	void Server::ProcessActions() {
		logger.verbose("Processing thread started");

		while(!stopProcessing) {
			std::unique_lock<std::mutex> lock(workLock);

			while(work.empty())
				workReady.wait(lock);

			IAction* action = work.front();
			work.pop_front();

			switch(action->type) {
				case ActionType::AddConnection: {
					std::lock_guard<std::mutex> lock(usersLock);
					AddConnectionAction* add = (AddConnectionAction*)action;

					IPData* data = FindIPData(add->conPtr->get_raw_socket().remote_endpoint().address());
					users[add->conPtr] = new User(add->conPtr, data);
					logger.info("User Connected (IP: ", data->str(), ")");
				} break;

				case ActionType::RemoveConnection: {
					std::lock_guard<std::mutex> lock(usersLock);
					RemoveConnectionAction* add = (RemoveConnectionAction*)action;

					auto it = users.find(add->conPtr);
					
					IPData* data = FindIPData(it->second->ipData->address);

					// decrement connection count
					data->connection_count--;

					logger.info("User Disconnect (IP: ", it->second->ipData->str(), ")");

					if(it->second)
						delete it->second;

					users.erase(it);
				} break;

				case ActionType::Message: {
					// TODO
				} break;

				default:
					logger.verbose("Action type ", (int)action->type, " not implemented");
					break;
			}

			delete action;
		}
	}


}