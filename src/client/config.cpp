#include "config.hpp"

#include <yaml-cpp/yaml.h>

#include <fstream>

#include "shared/engine/logging.hpp"

using namespace std;

static logging::Logger logger("conf");

// Default values for all customizable options
std::string   DEFAULT_LAST_WORLD_ID   = "New World";
RenderBackend DEFAULT_RENDER_BACKEND  = RenderBackend::OGL_3;
bool          DEFAULT_FULLSCREEN      = false;
vec2i         DEFAULT_WINDOWED_RES    = vec2i{1600, 900};
vec2i         DEFAULT_FULLSCREEN_RES  = vec2i{0, 0};
AntiAliasing  DEFAULT_ANTI_ALIASING   = AntiAliasing::MSAA_4;
Fog           DEFAULT_FOG             = Fog::FANCY;
uint          DEFAULT_RENDER_DISTANCE = 8;
float         DEFAULT_FOV             = 120;
uint          DEFAULT_TEX_MIPMAPPING  = 1000;
TexFiltering  DEFAULT_TEX_FILTERING   = TexFiltering::LINEAR;
bool          DEFAULT_TEX_ATLAS       = false;
string        DEFAULT_TEXTURES_FILE   = "textures_lbpr_128.txt";

namespace YAML {

template <>
struct convert<RenderBackend> {
	static Node encode(RenderBackend rhs) {
		const char *str;
		switch (rhs) {
			case RenderBackend::OGL_2:       str = "opengl2";     break;
			case RenderBackend::OGL_3:       str = "opengl3";     break;
		}
		Node node;
		node = str;
		return node;
	}

	static bool decode(const Node& node, RenderBackend& rhs) {
		std::string str = node.as<std::string>();
		if (str == "opengl2") {
			rhs = RenderBackend::OGL_2;
			return true;
		} else if (str == "opengl3") {
			rhs = RenderBackend::OGL_3;
			return true;
		} else {
			rhs = DEFAULT_RENDER_BACKEND;
			return true;
		}
	}
};

template <>
struct convert<AntiAliasing> {
	static Node encode(AntiAliasing rhs) {
		const char *str;
		switch (rhs) {
			case AntiAliasing::NONE:    str = "none";   break;
			case AntiAliasing::MSAA_2:  str = "msaa2";  break;
			case AntiAliasing::MSAA_4:  str = "msaa4";  break;
			case AntiAliasing::MSAA_8:  str = "msaa8";  break;
			case AntiAliasing::MSAA_16: str = "msaa16"; break;
		}
		Node node;
		node = str;
		return node;
	}

	static bool decode(const Node& node, AntiAliasing& rhs) {
		std::string str = node.as<std::string>();
		if (str == "none") {
			rhs = AntiAliasing::NONE;
			return true;
		} else if (str == "msaa2") {
			rhs = AntiAliasing::MSAA_2;
			return true;
		} else if (str == "msaa4") {
			rhs = AntiAliasing::MSAA_4;
			return true;
		} else if (str == "msaa8") {
			rhs = AntiAliasing::MSAA_8;
			return true;
		} else if (str == "msaa16") {
			rhs = AntiAliasing::MSAA_16;
			return true;
		} else {
			rhs = DEFAULT_ANTI_ALIASING;
			return true;
		}
	}
};

template <>
struct convert<Fog> {
	static Node encode(Fog rhs) {
		const char *str;
		switch (rhs) {
			case Fog::NONE:    str = "none";   break;
			case Fog::FAST:    str = "fast";   break;
			case Fog::FANCY:   str = "fancy";  break;
		}
		Node node;
		node = str;
		return node;
	}

	static bool decode(const Node& node, Fog& rhs) {
		std::string str = node.as<std::string>();
		if (str == "none") {
			rhs = Fog::NONE;
			return true;
		} else if (str == "fast") {
			rhs = Fog::FAST;
			return true;
		} else if (str == "fancy") {
			rhs = Fog::FANCY;
			return true;
		} else {
			rhs = DEFAULT_FOG;
			return true;
		}
	}
};

template <>
struct convert<TexFiltering> {
	static Node encode(TexFiltering rhs) {
		const char *str;
		switch (rhs) {
			case TexFiltering::NEAREST: str = "nearest"; break;
			case TexFiltering::LINEAR:  str = "linear";  break;
		}
		Node node;
		node = str;
		return node;
	}

	static bool decode(const Node& node, TexFiltering& rhs) {
		std::string str = node.as<std::string>();
		if (str == "nearest") {
			rhs = TexFiltering::NEAREST;
			return true;
		} else if (str == "linear") {
			rhs = TexFiltering::LINEAR;
			return true;
		} else {
			rhs = DEFAULT_TEX_FILTERING;
			return true;
		}
	}
};

template <typename T>
struct convert<vec2<T>> {
	static Node encode(const vec2<T> &rhs) {
		Node node;
		node.push_back(rhs[0]);
		node.push_back(rhs[1]);
		return node;
	}

	static bool decode(const Node& node, vec2<T>& rhs) {
		if (!node || !node.IsSequence() || node.size() != 2)
			return false;
		
		rhs[0] = node[0].as<T>();
		rhs[1] = node[1].as<T>();
		return true;
	}
};

template <typename T>
struct convert<vec3<T>> {
	static Node encode(const vec3<T> &rhs) {
		Node node;
		node.push_back(rhs[0]);
		node.push_back(rhs[1]);
		node.push_back(rhs[2]);
		return node;
	}

	static bool decode(const Node& node, vec3<T>& rhs) {
		if (!node || !node.IsSequence() || node.size() != 3)
			return false;
		
		rhs[0] = node[0].as<T>();
		rhs[1] = node[1].as<T>();
		rhs[2] = node[2].as<T>();
		return true;
	}
};

} // namespace YAML

template <typename T>
YAML::Emitter& operator << (YAML::Emitter& out, const vec2<T>& v) {
	out << YAML::Flow;
	out << YAML::BeginSeq << v[0] << v[1] << YAML::EndSeq;
	return out;
}

template <typename T>
YAML::Emitter& operator << (YAML::Emitter& out, const vec3<T>& v) {
	out << YAML::Flow;
	out << YAML::BeginSeq << v[0] << v[1] << v[2] << YAML::EndSeq;
	return out;
}

template <typename T>
YAML::Emitter& operator << (YAML::Emitter& out, const T& t) {
	YAML::Node node;
	node = t;
	out << node;
	return out;
}

void store(const char *filename, const GraphicsConf &conf) {
	YAML::Emitter out;

	out << YAML::BeginMap;
	out << YAML::Key << "config" << YAML::Value;
	out << YAML::BeginMap;
	out << YAML::Key << "last_world_id" << YAML::Value << conf.last_world_id;
	out << YAML::Key << "graphics" << YAML::Value;
	out << YAML::BeginMap;
	out << YAML::Key << "render_backend" << YAML::Value << conf.render_backend;
	out << YAML::Key << "fullscreen" << YAML::Value << conf.fullscreen;
	out << YAML::Key << "windowed_res" << YAML::Value << conf.windowed_res;
	out << YAML::Key << "fullscreen_res" << YAML::Value << conf.fullscreen_res;
	out << YAML::Key << "aa" << YAML::Value << conf.aa;
	out << YAML::Key << "fog" << YAML::Value << conf.fog;
	out << YAML::Key << "render_distance" << YAML::Value << conf.render_distance;
	out << YAML::Key << "fov" << YAML::Value << conf.fov;
	out << YAML::Key << "textures" << YAML::Value;
	out << YAML::BeginMap;
	out << YAML::Key << "mipmapping" << YAML::Value << conf.tex_mipmapping;
	out << YAML::Key << "filtering" << YAML::Value << conf.tex_filtering;
	out << YAML::Key << "use_atlas" << YAML::Value << conf.tex_atlas;
	out << YAML::EndMap;
	out << YAML::Key << "textures_file" << YAML::Value << conf.textures_file;
	out << YAML::EndMap;
	out << YAML::EndMap;
	out << YAML::EndMap;

	std::fstream fs;
	fs.open(filename, std::ios_base::out);
	fs << out.c_str();
	fs.close();
}

void load(const char *filename, GraphicsConf *conf) {
	YAML::Node root, node;
	try {
		root = YAML::LoadFile(filename);
	} catch (...) {
		root.reset();
	}

	node = root["config"]["last_world_id"];
	conf->last_world_id = node ? node.as<std::string>() : DEFAULT_LAST_WORLD_ID;

	node = root["config"]["graphics"]["render_backend"];
	conf->render_backend = node ? node.as<RenderBackend>() : DEFAULT_RENDER_BACKEND;
	
	node = root["config"]["graphics"]["fullscreen"];
	conf->fullscreen = node ? node.as<bool>() : DEFAULT_FULLSCREEN;

	node = root["config"]["graphics"]["windowed_res"];
	conf->windowed_res = node ? node.as<vec2i>() : DEFAULT_WINDOWED_RES;
	
	node = root["config"]["graphics"]["fullscreen_res"];
	conf->fullscreen_res = node ? node.as<vec2i>() : DEFAULT_FULLSCREEN_RES;

	node = root["config"]["graphics"]["aa"];
	conf->aa = node ? node.as<AntiAliasing>() : DEFAULT_ANTI_ALIASING;

	node = root["config"]["graphics"]["fog"];
	conf->fog = node ? node.as<Fog>() : DEFAULT_FOG;

	node = root["config"]["graphics"]["render_distance"];
	conf->render_distance = node ? node.as<uint>() : DEFAULT_RENDER_DISTANCE;

	node = root["config"]["graphics"]["fov"];
	conf->fov = node ? node.as<float>() : DEFAULT_FOV;

	node = root["config"]["graphics"]["mipmapping"];
	conf->tex_mipmapping = node ? node.as<uint>() : DEFAULT_TEX_MIPMAPPING;

	node = root["config"]["graphics"]["textures"]["filtering"];
	conf->tex_filtering = node ? node.as<TexFiltering>() : DEFAULT_TEX_FILTERING;

	node = root["config"]["graphics"]["textures"]["use_atlas"];
	conf->tex_atlas = node ? node.as<bool>() : DEFAULT_TEX_ATLAS;

	node = root["config"]["graphics"]["textures"]["textures_file"];
	conf->textures_file = node ? node.as<std::string>() : DEFAULT_TEXTURES_FILE;
}
