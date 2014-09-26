#ifndef REMOTE_SERVER_INTERFACE_HPP
#define REMOTE_SERVER_INTERFACE_HPP

#include <future>
#include <string>

#include "server_interface.hpp"
#include "time.hpp"
#include "net/net.hpp"
#include "net/socket.hpp"
#include "net/buffer.hpp"

class RemoteServerInterface : public ServerInterface {
private:
	GraphicsConf conf;
	uint8 localPlayerId;

	my::time::time_t timeout = my::time::seconds(10); // 10 seconds

	my::net::ios_t ios;
	my::net::ios_t::work *w;
	std::future<void> f;
	my::net::Socket socket;

	std::future<void> connectFuture;
	std::atomic<Status> status;

	Buffer inBuf;
	Buffer outBuf;

public:
	RemoteServerInterface(const char *address, const GraphicsConf &conf);

	~RemoteServerInterface();

	Status getStatus() override;

	void togglePlayerFly() override;

	void setPlayerMoveInput(int moveInput) override;

	void setPlayerOrientation(double yaw, double pitch) override;
	void setBlock(uint8 block) override;

	void edit(vec3i64 bc, uint8 type) override;

	void receiveChunks(uint64 timeLimit) override;

	void sendInput() override;

	void setConf(const GraphicsConf &) override;

	int getLocalClientID() override;

	void stop() override;

private:
	void asyncConnect(std::string address);
};

#endif // REMOTE_SERVER_INTERFACE_HPP

