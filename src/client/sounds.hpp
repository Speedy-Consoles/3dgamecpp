#ifndef SOUNDS_HPP_
#define SOUNDS_HPP_

#include <vector>
#include <stack>
#include <unordered_map>
#include <atomic>

#include "shared/engine/vmath.hpp"
#include "shared/engine/random.hpp"

class Client;

struct Mix_Chunk;

class Sounds {
	Client *client = nullptr;

	// track what is playing on which channel
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

	// catalogue all the samples we have
	enum SampleType {
		SAMPLE_NONE,
		SAMPLE_STANDARD,
		SAMPLE_RANDOMIZED,
	};
	struct Sample {
		SampleType type;
		std::string name;
		std::string path;
		union {
			Mix_Chunk *chunk;
			std::vector<int> *sample_set;
		};

		void free();
	};
	std::vector<Sample> samples;
	std::unordered_map<std::string, int> sample_map;

	// for selecting random sound effects
	std::minstd_rand rng;

public:
	Sounds(Client *client);
	~Sounds();

	void tick();
	
	void play(int i, vec3i64 pos);
	void play(int i);
	
	int load(const char *name, const char *path);
	int createRandomized(const char *name);
	void addToRandomized(int randomized_sample, int other_sample);
	void addToRandomized(const char *randomized_sample, const char *other_sample);
	int get(const char *name);

private:
	void play(int i, vec3i64 v, EffectState state);
	void updateChannelPosition(int channel, vec3i64 player_pos, int player_yaw);
};

#endif // SOUNDS_HPP_
