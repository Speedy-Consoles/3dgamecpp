#include "sounds.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "client.hpp"
#include "shared/game/character.hpp"
#include "shared/engine/math.hpp"

#include "shared/engine/logging.hpp"

static logging::Logger logger("sfx");

Sounds::Sounds(Client *client) : client(client) {
	LOG_DEBUG(logger) << "Constructing Sounds";

	if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
		LOG_ERROR(logger) << "Could not initialize sound subsystem: ";
		LOG_ERROR(logger) << Mix_GetError();
	}

	int mix_init_flags = MIX_INIT_OGG | MIX_INIT_FLAC | MIX_INIT_MP3;
	int mix_init_result = Mix_Init(mix_init_flags);
	if ((mix_init_flags & MIX_INIT_OGG) != 0 && (mix_init_result & MIX_INIT_OGG) == 0) {
		LOG_ERROR(logger) << "SDL_Mixer OGG Plugin could not be initialized: ";
		LOG_ERROR(logger) << Mix_GetError();
	}
	if ((mix_init_flags & MIX_INIT_FLAC) != 0 && (mix_init_result & MIX_INIT_FLAC) == 0) {
		LOG_ERROR(logger) << "SDL_Mixer FLAC Plugin could not be initialized: ";
		LOG_ERROR(logger) << Mix_GetError();
	}
	if ((mix_init_flags & MIX_INIT_MP3) != 0 && (mix_init_result & MIX_INIT_MP3) == 0) {
		LOG_ERROR(logger) << "SDL_Mixer MP3 Plugin could not be initialized: ";
		LOG_ERROR(logger) << Mix_GetError();
	}

	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == -1) {
		LOG_ERROR(logger) << "Could not open audio: ";
		LOG_ERROR(logger) << Mix_GetError();
	}

	const int num_channels = 16;
	int num_channels_actual = Mix_AllocateChannels(num_channels);
	if (num_channels_actual != num_channels) {
		LOG_ERROR(logger) << "Could not allocate " << num_channels << " channels";
		LOG_ERROR(logger) << Mix_GetError();
	}

	effects.resize(num_channels_actual);
	for (int channel = num_channels_actual - 1; channel >= 0; --channel) {
		free_channels.push(channel);
	}
}

Sounds::~Sounds() {
	LOG_DEBUG(logger) << "Destroying Sounds";
	Mix_HaltMusic();
	Mix_HaltChannel(-1);
	Mix_CloseAudio();
	Mix_Quit();
}

void Sounds::tick() {
	Character &character = client->getLocalCharacter();
	for (int channel = 0; channel < effects.size(); ++channel) {
		auto &effect = effects[channel];
		if (effect.state == SFX_NOT_PLAYING)
			continue;
		if (!Mix_Playing(channel)) {
			effect.state = SFX_NOT_PLAYING;
			free_channels.push(channel);
			continue;
		}
		updateChannelPosition(channel, character.getPos(), character.getYaw());
	}
}

void Sounds::play(int i, vec3i64 pos) {
	play(i, pos, SFX_LOCALIZED);
}

void Sounds::play(int i) {
	play(i, vec3i64(0, 0, 0), SFX_OMNIPRESENT);
}

int Sounds::load(const char *name, const char *path) {
	Mix_Chunk *chunk = Mix_LoadWAV(path);
	if (!chunk) {
		LOG_ERROR(logger) << "File '" << path << "' could not be loaded: ";
		LOG_ERROR(logger) << Mix_GetError();
		return -1;
	}
	int index;
	int i = get(name);
	if (i < 0) {
		index = (int) samples.size();
		samples.push_back(Sample{ SAMPLE_STANDARD, name, path });
		samples[index].chunk = chunk;
		sample_map.insert({name, index});
	} else {
		LOG_WARNING(logger) << "Sample '" << samples[i].name << "' is overwritten";
		index = i;
		samples[i].free();
		samples[i] = Sample{ SAMPLE_STANDARD, name, path };
		samples[index].chunk = chunk;
	}
	return index;
}

int Sounds::createRandomized(const char *name) {
	int index;
	int i = get(name);
	if (i < 0) {
		index = (int) samples.size();
		samples.push_back(Sample{ SAMPLE_RANDOMIZED, name, "" });
		sample_map.insert({name, index});
	} else {
		LOG_WARNING(logger) << "Sample '" << samples[i].name << "' is overwritten";
		index = i;
		samples[i].free();
		samples[i] = Sample{ SAMPLE_RANDOMIZED, name, "" };
	}
	samples[index].sample_set = new std::vector<int>;
	return index;
}

void Sounds::addToRandomized(int randomized_sample, int other_sample) {
	if (randomized_sample < 0 || other_sample < 0)
		return;
	Sample &sample = samples[randomized_sample];
	if (sample.type != SAMPLE_RANDOMIZED) {
		LOG_WARNING(logger) << "Tried to add to " << sample.name
			<< ", which is not a randomized sample";
		return;
	}
	sample.sample_set->push_back(other_sample);

}

int Sounds::get(const char *name) {
	auto iter = sample_map.find(name);
	if (iter == sample_map.end()) {
		return -1;
	} else {
		return iter->second;
	}
}

void Sounds::addToRandomized(const char *rand_sample, const char *other_sample) {
	int rand_index = get(rand_sample);
	int other_index = get(other_sample);
	addToRandomized(rand_index, other_index);
}

int Sounds::addToRandomized(const char *rand_sample, const char *other_sample, const char *path) {
	int rand_index = get(rand_sample);
	int other_index = load(other_sample, path);
	addToRandomized(rand_index, other_index);
	return other_index;
}

void Sounds::play(const char *name, vec3i64 pos) {
	play(get(name), pos);
}

void Sounds::play(const char *name) {
	play(get(name));
}

void Sounds::Sample::free() {
	switch (type) {
	default:
	case SAMPLE_NONE:
		break;
	case SAMPLE_STANDARD:
		if (chunk) {
			Mix_FreeChunk(chunk);
			chunk = nullptr;
		}
		break;
	case SAMPLE_RANDOMIZED:
		if (sample_set) {
			delete sample_set;
			sample_set = nullptr;
		}
		break;
	}
}

void Sounds::play(int i, vec3i64 v, EffectState state) {
	if (free_channels.size() <= 0) {
		LOG_DEBUG(logger) << "No free channels for sound effect";
		return;
	}
	if (i < 0) {
		LOG_DEBUG(logger) << "Sound effect does not exist";
		return;
	}

	const Sample *sample = &samples[i];

	while (sample->type != SAMPLE_STANDARD) {
		switch (sample->type) {
		default:
			LOG_DEBUG(logger) << "Problem while resolving sample";
			return;
		case SAMPLE_RANDOMIZED:
			{
				int num = (int) sample->sample_set->size();
				uniform_int_distribution<int> distr(0, num - 1);
				int i = distr(rng);
				sample = &samples[(*sample->sample_set)[i]];
			}
		}
	}
	Mix_Chunk *chunk = sample->chunk;

	if (!chunk) {
		LOG_DEBUG(logger) << "Sound effect does not exist";
		return;
	}

	int channel = free_channels.top();
	free_channels.pop();
	auto &effect = effects[channel];

	effect.state = state;
	effect.v = v;

	if (state == SFX_OMNIPRESENT) {
		Mix_SetPosition(channel, 0, 0);
	} else {
		Character &character = client->getLocalCharacter();
		updateChannelPosition(channel, character.getPos(), character.getYaw());
	}

	if (Mix_PlayChannel(channel, chunk, 0) == -1) {
		LOG_ERROR(logger) << "Could not play sample " << sample->name << ": ";
		LOG_ERROR(logger) << Mix_GetError();
	}
}

void Sounds::updateChannelPosition(int channel, vec3i64 player_pos, int player_yaw) {
	if (effects[channel].state == SFX_OMNIPRESENT || effects[channel].state == SFX_NOT_PLAYING)
		return;

	// where is the sound relative to us in world coordinates?
	vec3d rel_pos_world = (effects[channel].v - player_pos).cast<double>();

	// where is the sound relative to us in our own local coordinates? (+x in front of us)
	vec3d rel_pos_player;
	{
		double phi = player_yaw * (TAU / 36000.0);
		rel_pos_player[0] = + cos(phi) * rel_pos_world[0] + sin(phi) * rel_pos_world[1];
		rel_pos_player[1] = - sin(phi) * rel_pos_world[0] + cos(phi) * rel_pos_world[1];
		rel_pos_player[2] = rel_pos_world[2];
	}

	// next we use spherical coordinates, but with the main axis going along y, so theta will
	// indicate which ear hears what and phi indicates whether something is in front or behind us
	double rho, phi, theta;
	rho = rel_pos_world.norm();
	phi = atan2(rel_pos_player[2], rel_pos_player[0]);
	// center theta around equator
	theta = acos(rel_pos_player[1] / rho) - TAU / 4;

	int distance = (int)(clamp(rho / (RESOLUTION * 40.0), 0.0, 1.0) * 255.0);
	int angle = (int)floor(theta  * (36000.0 / TAU)) / 100;
	angle = cycle(angle, 360);

	Mix_SetPosition(channel, angle, distance);
}
