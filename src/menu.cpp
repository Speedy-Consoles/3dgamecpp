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

Menu::Menu(GraphicsConf *conf) : conf(conf) {
	renderDistanceBuf = conf->render_distance;
	aaBuf = conf->aa;
}

bool Menu::navigateUp() {
	cursor = (cursor + getEntryCount() - 1) % getEntryCount();
	return false;
}

bool Menu::navigateDown() {
	cursor = (cursor + 1) % getEntryCount();
	return false;
}

bool Menu::navigateRight() {
	switch (cursor) {
	case RENDER_BACKEND:
		return false;

	case FULLSCREEN:
		conf->fullscreen ^= true;
		return true;

	case ANTI_ALIASING:
		switch (aaBuf) {
		case AntiAliasing::NONE:    aaBuf = AntiAliasing::MSAA_2; break;
		case AntiAliasing::MSAA_2:  aaBuf = AntiAliasing::MSAA_4; break;
		case AntiAliasing::MSAA_4:  aaBuf = AntiAliasing::MSAA_8; break;
		case AntiAliasing::MSAA_8:  aaBuf = AntiAliasing::MSAA_16; break;
		case AntiAliasing::MSAA_16: aaBuf = AntiAliasing::NONE; break;
		}
		return false;

	case FOG:
		switch (conf->fog) {
		case Fog::NONE:   conf->fog = Fog::FAST; break;
		case Fog::FAST:   conf->fog = Fog::FANCY; break;
		case Fog::FANCY:  conf->fog = Fog::NONE; break;
		}
		return true;

	case RENDER_DISTANCE:
		++renderDistanceBuf;
		return false;
	}

	return false;
}

bool Menu::navigateLeft() {
	switch (cursor) {
	case RENDER_BACKEND:
		return false;

	case FULLSCREEN:
		conf->fullscreen ^= true;
		return true;

	case ANTI_ALIASING:
		switch (aaBuf) {
		case AntiAliasing::NONE:    aaBuf = AntiAliasing::MSAA_16; break;
		case AntiAliasing::MSAA_2:  aaBuf = AntiAliasing::NONE; break;
		case AntiAliasing::MSAA_4:  aaBuf = AntiAliasing::MSAA_2; break;
		case AntiAliasing::MSAA_8:  aaBuf = AntiAliasing::MSAA_4; break;
		case AntiAliasing::MSAA_16: aaBuf = AntiAliasing::MSAA_8; break;
		}
		return false;

	case FOG:
		switch (conf->fog) {
		case Fog::NONE:   conf->fog = Fog::FANCY; break;
		case Fog::FAST:   conf->fog = Fog::NONE; break;
		case Fog::FANCY:  conf->fog = Fog::FAST; break;
		}
		return true;

	case RENDER_DISTANCE:
		if (renderDistanceBuf > 0)
			--renderDistanceBuf;
		return false;
	}

	return false;
}

bool Menu::finish() {
	bool changed = false;
	if (aaBuf != conf->aa) {
		conf->aa = aaBuf;
		changed = true;
	}

	if (renderDistanceBuf != conf->render_distance) {
		conf->render_distance = renderDistanceBuf;
		changed = true;
	}

	return changed;
}

uint Menu::getEntryCount() {
	return NUM_ENTRIES;
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
		case Fog::FAST: ss << "Fast"; break;
		case Fog::FANCY: ss << "Fancy"; break;
	}
	return ss;
}

std::string Menu::getEntryValue(uint i) {
	stringstream ss;

	switch (i) {
		case RENDER_BACKEND: ss << conf->render_backend; break;
		case FULLSCREEN: ss << (conf->fullscreen ? "On" : "Off"); break;

		//case WINDOWED_RES:
		//case FULLSCREEN_RES:

		case ANTI_ALIASING: ss << aaBuf; break;
		case FOG: ss << conf->fog; break;
		case RENDER_DISTANCE: ss << renderDistanceBuf; break;
	}

	return ss.str();
}
