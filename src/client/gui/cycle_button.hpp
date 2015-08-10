#ifndef CYCLE_BUTTON_HPP_
#define CYCLE_BUTTON_HPP_

#include "button.hpp"

#include <string>
#include <vector>

#include "shared/engine/std_types.hpp"

namespace gui {

template <typename T>
class CycleButton : public Button {
public:
	virtual ~CycleButton() = default;
	CycleButton(float x, float y, float w, float h);

	void add(T val, std::string desc);

	void setIndex(int index);
	void set(T val);

	T get(int index) const;
	T get() const;

	void setOnDataChange(std::function<void(T)>);
	virtual void handleMouseClick(float x, float y) override;
private:
	struct Entry {
		T val;
		std::string desc;
	};

	size_t _current = 0;
	std::vector<Entry> _entries;
	std::function<void(T)> _on_data_change;

	size_t getIndex(T t);
};

template <typename T>
CycleButton<T>::CycleButton(float x, float y, float w, float h) :
	Button(x, y, w, h)
{
	// nothing
}

template <typename T>
void CycleButton<T>::add(T val, std::string desc) {
	_entries.push_back({val, desc});
}

template <typename T>
void CycleButton<T>::setIndex(int index) {
	_current = index;
	text() = _entries[_current].desc;
}

template <typename T>
void CycleButton<T>::set(T val) {
	_current = getIndex(val);
	text() = _entries[_current].desc;
}

template <typename T>
T CycleButton<T>::get(int index) const {
	return _entries[index].val;
}

template <typename T>
T CycleButton<T>::get() const {
	return _entries[_current].val;
}

template <typename T>
void CycleButton<T>::setOnDataChange(std::function<void(T)> cb) {
	_on_data_change = cb;
}

template <typename T>
void CycleButton<T>::handleMouseClick(float x, float y) {
	Button::handleMouseClick(x, y);
	if (isInside(x, y)) {
		setIndex((_current + 1) % _entries.size());
		if (_on_data_change)
			_on_data_change(get());
	}
}

template <typename T>
size_t CycleButton<T>::getIndex(T t) {
	for (size_t i = 0; i < _entries.size(); ++i) {
		if (_entries[i].val == t)
			return i;
	}
	return -1;
}

} // namespace gui

#endif // CYCLE_BUTTON_HPP_
