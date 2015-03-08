/*
 * frame.hpp
 *
 *  Created on: 21.09.2014
 *      Author: lars
 */

#ifndef FRAME_HPP_
#define FRAME_HPP_

#include "widget.hpp"

#include <list>

namespace gui {

class Label;

class Frame : public Widget {
public:
	virtual ~Frame();
	Frame(float, float, float, float);

	void add(Widget *);

	const std::list<Widget *> &widgets() const { return _widgets; };

	virtual void updateMousePosition(float x, float y) override;
	virtual void handleMouseClick(float x, float y) override;

private:
	std::list<Widget *> _widgets;
};

} // namespace gui

#endif // FRAME_HPP_
