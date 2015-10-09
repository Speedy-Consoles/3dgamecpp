#ifndef SOUNDS_HPP_
#define SOUNDS_HPP_

#include <vector>
#include <stack>
#include <atomic>

#include "shared/engine/vmath.hpp"

class Client;

struct Mix_Chunk;

class Sounds {
	Client *client = nullptr;

	Mix_Chunk *sample = nullptr;

	enum EffectState {
		SFX_NOT_PLAYING,
		SFX_LOCALIZED,
		SFX_OMNIPRESENT,
	};
	struct Effect {
		EffectState state;
		vec3i64 v;
	};

	std::vector<Effect> effects;
	std::stack<int> free_channels;

public:
	Sounds(Client *client);
	~Sounds();

	void tick();
	
	void play(vec3i64 pos);
	void play();

private:
	void play(vec3i64 v, EffectState state);
	void updateChannelPosition(int channel, vec3i64 player_pos, int player_yaw);
};

#endif // SOUNDS_HPP_
