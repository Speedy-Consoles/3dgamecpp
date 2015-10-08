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

	sample = Mix_LoadWAV("sounds/grass2.ogg");
	if (!sample) {
		LOG_ERROR(logger) << "Music could not be loaded: ";
		LOG_ERROR(logger) << Mix_GetError();
	}

	effects.resize(num_channels_actual);
	for (int channel = num_channels_actual - 1; channel >= 0; --channel) {
		free_channels.push(channel);
	}
}

Sounds::~Sounds() {
	LOG_DEBUG(logger) << "Destroying Sounds";
	Mix_FreeChunk(sample);
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

void Sounds::play(vec3i64 pos) {
	play(pos, SFX_LOCALIZED);
}

void Sounds::playDirectional(vec3i64 dir) {
	play(dir, SFX_DIRECTIONAL);
}

void Sounds::play() {
	play(vec3i64(0, 0, 0), SFX_OMNIPRESENT);
}

void Sounds::play(vec3i64 v, EffectState state) {
	if (free_channels.size() <= 0) {
		LOG_DEBUG(logger) << "No free channels for sound effect";
		return;
	}

	int channel = free_channels.top();
	free_channels.pop();

	auto &effect = effects[channel];

	effect.state = state;
	effect.v = v;

	Character &character = client->getLocalCharacter();
	if (state == SFX_OMNIPRESENT) {
		Mix_SetPosition(channel, 0, 0);
	} else {
		updateChannelPosition(channel, character.getPos(), character.getYaw());
	}

	if (Mix_PlayChannel(channel, sample, 0) == -1) {
		LOG_ERROR(logger) << "Could not play sample: ";
		LOG_ERROR(logger) << Mix_GetError();
	}
}

void Sounds::updateChannelPosition(int channel, vec3i64 player_pos, int player_yaw) {
	if (effects[channel].state == SFX_OMNIPRESENT || effects[channel].state == SFX_NOT_PLAYING)
		return;

	vec3i64 rel;
	int distance;
	if (effects[channel].state == SFX_DIRECTIONAL) {
		rel = effects[channel].v;
		distance = 0;
	} else {
		rel = effects[channel].v - player_pos;
		distance = (int)(clamp(rel.norm() / (RESOLUTION * 100.0), 0.0, 1.0) * 255.0);
	}

	int angle;
	double angle_world = atan2(rel[1], rel[0]) * (36000.0 / TAU);
	double angle_rel = angle_world - player_yaw;
	angle_rel = cycle(angle_rel, 36000.0);
	angle = (int)floor(angle_rel) / 100;

	Mix_SetPosition(channel, 360 - angle, distance);
}
