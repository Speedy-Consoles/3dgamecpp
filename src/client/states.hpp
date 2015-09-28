#ifndef STATES_HPP_
#define STATES_HPP_

#include <memory>

class Client;

class ConnectingState;
class LocalPlayingState;
class RemotePlayingState;
class MenuState;
class SystemInitState;
class TextInputState;

class States {
public:
	States(Client *client);
	~States();
	
	ConnectingState *getConnecting() { return connecting.get(); }
	LocalPlayingState *getLocalPlaying() { return localPlaying.get(); }
	MenuState *getMenu() { return menu.get(); }
	RemotePlayingState *getRemotePlaying() { return remotePlaying.get(); }
	SystemInitState *getSystemInit() { return systemInit.get(); }
	TextInputState *getTextInput() { return textInput.get(); }

private:
	std::unique_ptr<ConnectingState> connecting;
	std::unique_ptr<LocalPlayingState> localPlaying;
	std::unique_ptr<MenuState> menu;
	std::unique_ptr<RemotePlayingState> remotePlaying;
	std::unique_ptr<SystemInitState> systemInit;
	std::unique_ptr<TextInputState> textInput;
};

#endif // STATES_HPP_
