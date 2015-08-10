#include "label.hpp"

using namespace gui;
using namespace std;

Label::Label(float x, float y, float w, float h, const std::string &text) :
	Widget(x, y, w, h), _text(text)
{
	// nothing;
}
