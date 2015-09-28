#include "state_machine.hpp"

#include "states/state.hpp"
#include "client/events.hpp"

StateMachine::~StateMachine() {
	while (size() > 0)
		pop();
}

void StateMachine::push(State *new_top) {
	State *old_top = nullptr;
	if (!stack.empty()) {
		old_top = stack.back();
		old_top->onObscure(new_top);
	}
	new_top->onPush(old_top);
	stack.push_back(new_top);
}

void StateMachine::pop() {
	State *old_top = stack.back();
	stack.pop_back();
	old_top->onPop();
	if (!stack.empty()) {
		State *new_top = stack.back();
		new_top->onUnobscure();
	}
}

State *StateMachine::get(int i) {
	return stack[i];
}

State *StateMachine::top() {
	return stack.back();
}

int StateMachine::size() {
	return stack.size();
}

void StateMachine::update() {
	Event e;
	while (e.next())
		stack.back()->handle(e);
	stack.back()->update();
}