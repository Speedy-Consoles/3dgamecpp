#include "remote_server_interface.hpp"

#include <thread>

#include "io/logging.hpp"

RemoteServerInterface::RemoteServerInterface(GraphicsConf &conf)
		: conf(conf) {

}


RemoteServerInterface::~RemoteServerInterface() {

}

void RemoteServerInterface::togglePlayerFly() {

}

void RemoteServerInterface::setPlayerMoveInput(int moveInput) {

}

void RemoteServerInterface::setPlayerOrientation(double yaw, double pitch) {

}

void RemoteServerInterface::setBlock(uint8 block) {

}

void RemoteServerInterface::edit(vec3i64 bc, uint8 type) {

}

void RemoteServerInterface::receive(uint64 timeLimit) {

}

void RemoteServerInterface::sendInput() {

}

void RemoteServerInterface::setConf(const GraphicsConf &conf) {
	GraphicsConf old_conf = this->conf;
	this->conf = conf;

	if (conf.render_distance != old_conf.render_distance) {
		// TODO
	}
}

int RemoteServerInterface::getLocalClientID() {

}

void RemoteServerInterface::stop() {

}

