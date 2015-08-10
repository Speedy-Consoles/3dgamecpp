#ifndef BUTTON_HPP_
#define BUTTON_HPP_

#include "label.hpp"

#include <functional>

namespace gui {

class Button : public Label {
public:
	virtual ~Button() = default;
	Button(float x, float y, float w, float h, const std::string &text = "");

	void setOnClick(std::function<void()>);

	virtual void handleMouseClick(float x, float y) override;

private:
	std::function<void()> _on_click;
};

} // namespace gui

#endif // BUTTON_HPP_
