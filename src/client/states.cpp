#include "states.hpp"

#include "states/connecting_state.hpp"
#include "states/local_playing_state.hpp"
#include "states/menu_state.hpp"
#include "states/remote_playing_state.hpp"
#include "states/system_init_state.hpp"
#include "states/text_input_state.hpp"

// this can't be declared default, because the destructors of the states are not
// visible to the compiler that way
States::~States() {
	// nothing
}

States::States(Client *client) {
	connecting = std::unique_ptr<ConnectingState>(new ConnectingState(client));
	localPlaying = std::unique_ptr<LocalPlayingState>(new LocalPlayingState(client));
	menu = std::unique_ptr<MenuState>(new MenuState(client));
	remotePlaying = std::unique_ptr<RemotePlayingState>(new RemotePlayingState(client));
	systemInit = std::unique_ptr<SystemInitState>(new SystemInitState(client));
	textInput = std::unique_ptr<TextInputState>(new TextInputState(client));
}