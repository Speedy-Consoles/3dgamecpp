#ifndef MENU_STATE_HPP_
#define MENU_STATE_HPP_

#include "state.hpp"

class Client;
class Menu;

class MenuState : public State {
public:
	MenuState(Client *client) : State(client) {};

	void onPush(State *) override;
	void onPop() override;

	void onUnobscure() override;

	void handle(const Event &) override;

protected:
	Menu *menu;
};

#endif // MENU_STATE_HPP_
