#ifndef STATE_HPP_
#define STATE_HPP_

class Client;
struct Event;

class State {
public:
	State(State *parent, Client *client) : parent(parent), client(client) {}
	virtual ~State() {};

	virtual void hide() {};
	virtual void unhide() {};

	virtual void update() {};
	virtual void handle(const Event &);

protected:
	State *parent;
	Client *client;
};

#endif // STATE_HPP_
