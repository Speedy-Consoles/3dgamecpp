#ifndef LABEL_HPP_
#define LABEL_HPP_

#include "widget.hpp"

#include <string>

namespace gui {

class Label : public Widget {
public:
	virtual ~Label() = default;
	Label(float x, float y, float w, float h, const std::string &text = "");

	std::string &text() { return _text; }
	const std::string &text() const { return _text; }

private:
	std::string _text;
};

}

#endif // LABEL_HPP_
