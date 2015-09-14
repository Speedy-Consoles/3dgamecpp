#ifndef MENU_STATE_HPP_
#define MENU_STATE_HPP_

#include "state.hpp"

class Client;
class Menu;

class MenuState : public State {
public:
	MenuState(State *parent, Client *client);
	~MenuState();

	void handle(const Event &) override;

protected:
	Menu *menu;
};

#endif // MENU_STATE_HPP_
