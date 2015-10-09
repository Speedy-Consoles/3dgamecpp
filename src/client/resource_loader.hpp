#ifndef RESOURCE_LOADER_HPP_
#define RESOURCE_LOADER_HPP_

class Client;

namespace YAML {
	class Node;
}

class ResourceLoader {
	Client *client = nullptr;
public:
	ResourceLoader(Client *client) : client(client) {}
	~ResourceLoader() = default;
	
	void load(const char *file);
	void eval(const char *expr);

private:
	void loadNode(const YAML::Node &node);

	void loadMap(const YAML::Node &node);
	void loadSeq(const YAML::Node &node);
	
	void loadSounds(const YAML::Node &node);
	void loadSound(const YAML::Node &node);
};

#endif // RESOURCE_LOADER_HPP_
