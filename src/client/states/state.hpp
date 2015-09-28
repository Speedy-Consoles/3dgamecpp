#ifndef STATE_HPP_
#define STATE_HPP_

class Client;
struct Event;

class State {
public:
	State(Client *client) : client(client) {}
	virtual ~State() = default;

	/// gets called when this state is first pushed onto the stack
	virtual void onPush(State *);
	/// gets called when the state is removed from the stack
	virtual void onPop();

	/// gets called when another state is pushed on top of this one
	virtual void onObscure(State *);
	/// gets called when this state becomes the top state again
	virtual void onUnobscure();

	/// gets called once per frame on the top most state on the stack
	virtual void update();
	/// gets called for any events on the top most state on the stack
	virtual void handle(const Event &);
	
	/// checks if this state is currently anywhere ont he stack
	bool isActive() const { return is_active; }
	/// checks if this state is currently at the top of the stack
	bool isTop() const { return is_top; }

protected:
	/// reference to the client for inhereting classes to use
	Client *client = nullptr;
	/// store the state right under this one
	State *parent = nullptr;

private:
	/// whether the state is anywhere on the stack
	bool is_active = false;
	/// whether the state is at the top of the stack specifically
	bool is_top = false;
};

#endif // STATE_HPP_
