#include "chunk_loader.hpp"

#include "game/world.hpp"
#include "game/chunk.hpp"
#include "world_generator.hpp"

using namespace std;

ChunkLoader::ChunkLoader(World *world, uint64 seed, uint localPlayer) :
	isLoaded(0, vec3i64HashFunc), queue(1000),
	chunkArchive("./region/")
{
	this->gen = new WorldGenerator(seed);
	this->world = world;
	this->updateFaces = true;
	this->localPlayer = localPlayer;
}

ChunkLoader::~ChunkLoader() {
	wait();
	storeChunksOnDisk();

	Chunk *chunk = nullptr;
	while ((chunk = getNextLoadedChunk()) != nullptr) {
		deallocateChunk(chunk);
	}

	delete this->gen;

	clearChunkPool();
}

void ChunkLoader::dispatch() {
	shouldHalt = false;
	fut = async(launch::async, [this]() { run(); });
}

void ChunkLoader::requestTermination() {
	 shouldHalt.store(true, memory_order_relaxed);
}

void ChunkLoader::wait() {
	requestTermination();
	if (fut.valid())
		fut.get();
}

Chunk *ChunkLoader::getNextLoadedChunk() {
	Chunk *chunk;
	bool result = queue.pop(chunk);
	return result ? chunk : nullptr;
}

ProducerStack<vec3i64>::Node *ChunkLoader::getUnloadQueries() {
	return unloadQueries.consumeAll();
}

void ChunkLoader::setRenderDistance(uint dist) {
	newRenderDistance = dist;
}
