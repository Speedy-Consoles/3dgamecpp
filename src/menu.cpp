/*
 * menu.cpp
 *
 *  Created on: 16.09.2014
 *      Author: lars
 */

#include "menu.hpp"

#include "graphics.hpp"

#include <sstream>

using namespace std;

enum Entry {
	RENDER_BACKEND,
	FULLSCREEN,

	//WINDOWED_RES,
	//FULLSCREEN_RES,

	ANTI_ALIASING,
	FOG,
	RENDER_DISTANCE,

	NUM_ENTRIES,
};

Menu::Menu(Graphics *graphics) : graphics(graphics) {
	// nothing
}

void Menu::navigateUp() {
	cursor = (cursor + getEntryCount() - 1) % getEntryCount();
}

void Menu::navigateDown() {
	cursor = (cursor + 1) % getEntryCount();
}

void Menu::navigateRight() {
	switch (cursor) {
	case RENDER_BACKEND:
		break;

	case FULLSCREEN:
		conf.fullscreen ^= true;
		break;

	case ANTI_ALIASING:
		switch (conf.aa) {
		case AntiAliasing::NONE:    conf.aa = AntiAliasing::MSAA_2; break;
		case AntiAliasing::MSAA_2:  conf.aa = AntiAliasing::MSAA_4; break;
		case AntiAliasing::MSAA_4:  conf.aa = AntiAliasing::MSAA_8; break;
		case AntiAliasing::MSAA_8:  conf.aa = AntiAliasing::MSAA_16; break;
		case AntiAliasing::MSAA_16: conf.aa = AntiAliasing::NONE; break;
		}
		break;

	case FOG:
		switch (conf.fog) {
		case Fog::NONE:   conf.fog = Fog::SIMPLE; break;
		case Fog::SIMPLE: conf.fog = Fog::NONE; break;
		}
		break;

	case RENDER_DISTANCE:
		++conf.render_distance;
		break;
	}
}

void Menu::navigateLeft() {
	switch (cursor) {
	case RENDER_BACKEND:
		break;

	case FULLSCREEN:
		conf.fullscreen ^= true;
		break;

	case ANTI_ALIASING:
		switch (conf.aa) {
		case AntiAliasing::NONE:    conf.aa = AntiAliasing::MSAA_16; break;
		case AntiAliasing::MSAA_2:  conf.aa = AntiAliasing::NONE; break;
		case AntiAliasing::MSAA_4:  conf.aa = AntiAliasing::MSAA_2; break;
		case AntiAliasing::MSAA_8:  conf.aa = AntiAliasing::MSAA_4; break;
		case AntiAliasing::MSAA_16: conf.aa = AntiAliasing::MSAA_8; break;
		}
		break;

	case FOG:
		switch (conf.fog) {
		case Fog::NONE:   conf.fog = Fog::SIMPLE; break;
		case Fog::SIMPLE: conf.fog = Fog::NONE; break;
		}
		break;

	case RENDER_DISTANCE:
		--conf.render_distance;
		break;
	}
}

uint Menu::getEntryCount() {
	return NUM_ENTRIES;
}

void Menu::reload() {
	conf = graphics->getConf();
}

void Menu::flush() {
	graphics->setConf(conf);
}

std::string Menu::getEntryName(uint i) {
	switch (i) {
		case RENDER_BACKEND: return "Render backend";
		case FULLSCREEN: return "Fullscreen";

		//case WINDOWED_RES:
		//case FULLSCREEN_RES:

		case ANTI_ALIASING: return "Anti-aliasing";
		case FOG: return "Fog";
		case RENDER_DISTANCE: return "Renderdistance";

		default: return "???";
	}
}

stringstream &operator << (stringstream &ss, RenderBackend backend) {
	switch (backend) {
		case RenderBackend::UNSUPPORTED: ss << "Unsupported"; break;
		case RenderBackend::OGL_2: ss << "OpenGL 2.x"; break;
		case RenderBackend::OGL_3: ss << "OpenGL 3.x"; break;
	}
	return ss;
}

stringstream &operator << (stringstream &ss, AntiAliasing aa) {
	switch (aa) {
		case AntiAliasing::NONE: ss << "None"; break;
		case AntiAliasing::MSAA_2: ss << "MSAA x2"; break;
		case AntiAliasing::MSAA_4: ss << "MSAA x4"; break;
		case AntiAliasing::MSAA_8: ss << "MSAA x8"; break;
		case AntiAliasing::MSAA_16: ss << "MSAA x16"; break;
	}
	return ss;
}

stringstream &operator << (stringstream &ss, Fog fog) {
	switch (fog) {
		case Fog::NONE: ss << "Off"; break;
		case Fog::SIMPLE: ss << "On"; break;
	}
	return ss;
}

std::string Menu::getEntryValue(uint i) {
	stringstream ss;

	switch (i) {
		case RENDER_BACKEND: ss << conf.render_backend; break;
		case FULLSCREEN: ss << (conf.fullscreen ? "On" : "Off"); break;

		//case WINDOWED_RES:
		//case FULLSCREEN_RES:

		case ANTI_ALIASING: ss << conf.aa; break;
		case FOG: ss << conf.fog; break;
		case RENDER_DISTANCE: ss << conf.render_distance; break;
	}

	return ss.str();
}
