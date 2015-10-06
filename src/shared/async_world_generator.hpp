#ifndef ASYNC_WORLD_GENERATOR_HPP
#define ASYNC_WORLD_GENERATOR_HPP

#include <memory>

#include "engine/thread.hpp"

#include "engine/queue.hpp"
#include "game/world_generator.hpp"

class AsyncWorldGenerator : public Thread {
	WorldGenerator *worldGenerator;

	ProducerQueue<Chunk *> loadedQueue;
	ProducerQueue<Chunk *> toLoadQueue;

public:
	AsyncWorldGenerator(WorldGenerator *worldGenerator);
	~AsyncWorldGenerator();

	// networking
	void doWork() override;
	
	// chunks
	bool generateChunk(Chunk *chunk);
	Chunk *getNextChunk();
};

#endif // ASYNC_WORLD_GENERATOR_HPP

