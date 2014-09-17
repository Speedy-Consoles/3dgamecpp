#include "config.hpp"

RenderBackend DEFAULT_RENDER_BACKEND  = RenderBackend::OGL_2;
bool          DEFAULT_FULLSCREEN   = false;
vec2i         DEFAULT_WINDOWED_RES    = vec2i{1600, 900};
vec2i         DEFAULT_FULLSCREEN_RES  = vec2i{0, 0};
AntiAliasing  DEFAULT_ANTI_ALIASING   = AntiAliasing::NONE;
Fog           DEFAULT_FOG             = Fog::FANCY;
uint          DEFAULT_RENDER_DISTANCE = 8;

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/info_parser.hpp>

#include "logging.hpp"

using namespace std;
using namespace boost;

namespace boost {
namespace property_tree {

// RenderBackend Translator
template<typename Ch, typename Traits, typename Alloc>
struct translator_between<basic_string<Ch, Traits, Alloc>, RenderBackend> {
	typedef struct RenderBackendTranslator {
		typedef basic_string<Ch, Traits, Alloc> internal_type;
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
template<typename Ch, typename Traits, typename Alloc>
struct translator_between<basic_string<Ch, Traits, Alloc>, AntiAliasing> {
	typedef struct AntiAliasingTranslator {
		typedef basic_string<Ch, Traits, Alloc> internal_type;
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
template<typename Ch, typename Traits, typename Alloc>
struct translator_between<basic_string<Ch, Traits, Alloc>, Fog> {
	typedef struct FogTranslator {
		typedef basic_string<Ch, Traits, Alloc> internal_type;
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

} // namespace property_tree
} // namespace boost

void store(const char *filename, const GraphicsConf &conf) {
	property_tree::ptree pt;

	pt.put("graphics.render_backend", conf.render_backend);
	pt.put("graphics.windowed_res.w", conf.windowed_res[0]);
	pt.put("graphics.windowed_res.h", conf.windowed_res[1]);
	pt.put("graphics.fullscreen_res.w", conf.fullscreen_res[0]);
	pt.put("graphics.fullscreen_res.h", conf.fullscreen_res[1]);
	pt.put("graphics.aa", conf.aa);
	pt.put("graphics.fog", conf.fog);
	pt.put("graphics.render_distance", conf.render_distance);

	write_info(filename, pt);
}

void load(const char *filename, GraphicsConf *conf) {
	property_tree::ptree pt;
	try {
		read_info(filename, pt);
	} catch (...) {
		LOG(warning) << "'" << filename << "' could not be opened";
	}

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
}
