#include "config.hpp"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "shared/engine/logging.hpp"

using namespace std;
using namespace boost;
using namespace boost::property_tree;

static logging::Logger logger("conf");

// Default values for all customizable options
std::string   DEFAULT_LAST_WORLD_ID   = "New World";
RenderBackend DEFAULT_RENDER_BACKEND  = RenderBackend::OGL_2;
bool          DEFAULT_FULLSCREEN      = false;
vec2i         DEFAULT_WINDOWED_RES    = vec2i{1600, 900};
vec2i         DEFAULT_FULLSCREEN_RES  = vec2i{0, 0};
AntiAliasing  DEFAULT_ANTI_ALIASING   = AntiAliasing::NONE;
Fog           DEFAULT_FOG             = Fog::FANCY;
uint          DEFAULT_RENDER_DISTANCE = 8;
float         DEFAULT_FOV             = 120;
uint          DEFAULT_TEX_MIPMAPPING  = 1000;
TexFiltering  DEFAULT_TEX_FILTERING   = TexFiltering::LINEAR;
bool          DEFAULT_TEX_ATLAS       = false;
string        DEFAULT_TEXTURES_FILE   = "block_textures.txt";

namespace boost {
namespace property_tree {

// RenderBackend Translator
template <>
struct translator_between<string, RenderBackend> {
	typedef struct RenderBackendTranslator {
		typedef string internal_type;
		typedef RenderBackend external_type;

		optional<external_type> get_value(const internal_type &i) {
			external_type e;
			if      (i == "opengl2") e = RenderBackend::OGL_2;
			else if (i == "opengl3") e = RenderBackend::OGL_3;
			else                     e = DEFAULT_RENDER_BACKEND;
			return optional<external_type>(e);
		}

		optional<internal_type> put_value(const external_type &e) {
			internal_type i;
			switch (e) {
				case RenderBackend::OGL_2: i = "opengl2"; break;
				case RenderBackend::OGL_3: i = "opengl3"; break;
				default:                   i = "opengl2"; break;
			}
			return optional<internal_type>(i);
		}
	} type;
};

// AntiAliasing Translator
template <>
struct translator_between<string, AntiAliasing> {
	typedef struct AntiAliasingTranslator {
		typedef string internal_type;
		typedef AntiAliasing external_type;

		optional<external_type> get_value(const internal_type &i) {
			external_type e;
			if      (i == "none")    e = AntiAliasing::NONE;
			else if (i == "msaa 2")  e = AntiAliasing::MSAA_2;
			else if (i == "msaa 4")  e = AntiAliasing::MSAA_4;
			else if (i == "msaa 8")  e = AntiAliasing::MSAA_8;
			else if (i == "msaa 16") e = AntiAliasing::MSAA_16;
			else                     e = DEFAULT_ANTI_ALIASING;
			return optional<external_type>(e);
		}

		optional<internal_type> put_value(const external_type &e) {
			internal_type i;
			switch (e) {
				case AntiAliasing::NONE:    i = "none"; break;
				case AntiAliasing::MSAA_2:  i = "msaa 2"; break;
				case AntiAliasing::MSAA_4:  i = "msaa 4"; break;
				case AntiAliasing::MSAA_8:  i = "msaa 8"; break;
				case AntiAliasing::MSAA_16: i = "msaa 16"; break;
				default:                    i = "none"; break;
			}
			return optional<internal_type>(i);
		}
	} type;
};

// Fog Translator
template <>
struct translator_between<string, Fog> {
	typedef struct FogTranslator {
		typedef string internal_type;
		typedef Fog external_type;

		optional<external_type> get_value(const internal_type &i) {
			external_type e;
			if      (i == "none")   e = Fog::NONE;
			else if (i == "fast")   e = Fog::FAST;
			else if (i == "fancy")  e = Fog::FANCY;
			else                    e = DEFAULT_FOG;
			return optional<external_type>(e);
		}

		optional<internal_type> put_value(const external_type &e) {
			internal_type i;
			switch (e) {
				case Fog::NONE:   i = "none"; break;
				case Fog::FAST:   i = "fast"; break;
				case Fog::FANCY:  i = "fancy"; break;
				default:          i = "none"; break;
			}
			return optional<internal_type>(i);
		}
	} type;
};

// Fog Translator
template <>
struct translator_between<string, TexFiltering> {
	typedef struct TexFilteringTranslator {
		typedef string internal_type;
		typedef TexFiltering external_type;

		optional<external_type> get_value(const internal_type &i) {
			external_type e;
			if      (i == "nearest") e = TexFiltering::NEAREST;
			else if (i == "linear")  e = TexFiltering::LINEAR;
			else                     e = DEFAULT_TEX_FILTERING;
			return optional<external_type>(e);
		}

		optional<internal_type> put_value(const external_type &e) {
			internal_type i;
			switch (e) {
				case TexFiltering::NEAREST: i = "nearest"; break;
				case TexFiltering::LINEAR:  i = "linear"; break;
				default:                    i = "linear"; break;
			}
			return optional<internal_type>(i);
		}
	} type;
};

} // namespace property_tree
} // namespace boost

void store(const char *filename, const GraphicsConf &conf) {
	ptree pt;

	pt.put("last_world_id", conf.last_world_id);
	pt.put("graphics.render_backend", conf.render_backend);
	pt.put("graphics.windowed_res.w", conf.windowed_res[0]);
	pt.put("graphics.windowed_res.h", conf.windowed_res[1]);
	pt.put("graphics.fullscreen_res.w", conf.fullscreen_res[0]);
	pt.put("graphics.fullscreen_res.h", conf.fullscreen_res[1]);
	pt.put("graphics.aa", conf.aa);
	pt.put("graphics.fog", conf.fog);
	pt.put("graphics.render_distance", conf.render_distance);
	pt.put("graphics.fov", conf.fov);
	pt.put("graphics.textures.mipmapping", conf.tex_mipmapping);
	pt.put("graphics.textures.filtering", conf.tex_filtering);
	pt.put("graphics.textures.use_atlas", conf.tex_atlas);
	pt.put("graphics.textures_file", conf.textures_file);

	write_info(filename, pt);
}

void load(const char *filename, GraphicsConf *conf) {
	ptree pt;
	try {
		read_info(filename, pt);
	} catch (...) {
		LOG_WARNING(logger) << "'" << filename << "' could not be opened";
	}

	conf->last_world_id = pt.get("last_world_id",
			DEFAULT_LAST_WORLD_ID);
	conf->render_backend = pt.get("graphics.render_backend",
			DEFAULT_RENDER_BACKEND);
	conf->fullscreen = pt.get("graphics.is_fullscreen",
			DEFAULT_FULLSCREEN);
	conf->windowed_res[0] = pt.get("graphics.windowed_res.w",
			DEFAULT_WINDOWED_RES[0]);
	conf->windowed_res[1] = pt.get("graphics.windowed_res.h",
			DEFAULT_WINDOWED_RES[1]);
	conf->fullscreen_res[0] = pt.get("graphics.fullscreen_res.w",
			DEFAULT_FULLSCREEN_RES[0]);
	conf->fullscreen_res[1] = pt.get("graphics.fullscreen_res.h",
			DEFAULT_FULLSCREEN_RES[1]);
	conf->aa = pt.get("graphics.aa",
			DEFAULT_ANTI_ALIASING);
	conf->fog = pt.get("graphics.fog",
			DEFAULT_FOG);
	conf->render_distance = pt.get("graphics.render_distance",
			DEFAULT_RENDER_DISTANCE);
	conf->fov = pt.get("graphics.fov",
			DEFAULT_FOV);
	conf->tex_mipmapping = pt.get("graphics.textures.mipmapping",
			DEFAULT_TEX_MIPMAPPING);
	conf->tex_filtering = pt.get("graphics.textures.filtering",
			DEFAULT_TEX_FILTERING);
	conf->tex_atlas = pt.get("graphics.textures.use_atlas",
			DEFAULT_TEX_ATLAS);
	conf->textures_file = pt.get("graphics.textures_file",
			DEFAULT_TEXTURES_FILE);
}
