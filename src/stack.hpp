#ifndef STACK_HPP
#define STACK_HPP

#include <atomic>

template<typename T>
class ProducerStack
{
public:
	struct Node
	{
		Node(const T& data) : data(data), next(nullptr) {}
		T data;
		Node* next;
	};

	ProducerStack() : head(nullptr) {}

	~ProducerStack() {
		auto p = consumeAll();
		while (p)
		{
			auto tmp = p->next;
			delete p;
			p = tmp;
		}
	}

	void push(const T &data)
	{
		Node* new_node = new Node(data);
		new_node->next = head.load(std::memory_order_relaxed);
		while(!std::atomic_compare_exchange_weak_explicit(
			&head,
			&new_node->next,
			new_node,
			std::memory_order_release,
			std::memory_order_relaxed)) {}
	}

	Node* consumeAll()
	{
		return head.exchange(nullptr, std::memory_order_acquire);
	}
private:
	std::atomic<Node*> head;
};

#endif
