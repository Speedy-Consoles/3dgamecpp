#include "resource_loader.hpp"

#include <fstream>
#include <string>
#include <stack>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "client/client.hpp"
#include "client/sounds.hpp"
#include "shared/engine/logging.hpp"
#include "shared/block_loader.hpp"

logging::Logger logger("res");

void ResourceLoader::load(const char *file) {
	LOG_INFO(logger) << "Reading resource file '" << file << "'";
	std::fstream is;
	is.open(file, std::ios_base::in);
	if (!is.good()) {
		LOG_WARNING(logger) << "Could not open file '" << file << "'";
		return;
	}
	YAML::Node node = YAML::Load(is);
	if (node.IsNull()) {
		LOG_WARNING(logger) << "Could not parse file '" << file << "'";
		return;
	}
	loadNode(node);
}

void ResourceLoader::eval(const char *expr) {
	LOG_INFO(logger) << "Parsing custom resource string";
	YAML::Node node = YAML::Load(expr);
	if (node.IsNull()) {
		LOG_WARNING(logger) << "Could not parse custom resource string";
		return;
	}
	loadNode(node);
}

void ResourceLoader::loadNode(const YAML::Node &node) {
	switch (node.Type()) {
	case YAML::NodeType::Map:
		loadMap(node);
		break;
	case YAML::NodeType::Sequence:
		loadSeq(node);
		break;
	default:
		break;
	}
}

void ResourceLoader::loadMap(const YAML::Node &node) {
	for (auto iter = node.begin(); iter != node.end(); ++iter) {
		std::string key = iter->first.as<std::string>();
		if (key == "sounds") {
			loadSounds(iter->second);
		} else 
		if (key == "blocks") {
			loadBlocks(iter->second);
		}
	}
}

void ResourceLoader::loadSeq(const YAML::Node &node) {
	for (auto iter = node.begin(); iter != node.end(); ++iter) {
		YAML::Node element = *iter;
		LOG_TRACE(logger) << "seq";
	}
}

void ResourceLoader::loadSounds(const YAML::Node &node) {
	if (!node.IsSequence())
		return;

	for (auto iter = node.begin(); iter != node.end(); ++iter) {
		loadSound(*iter);
	}
}

void ResourceLoader::loadSound(const YAML::Node &node) {
	if (!node.IsMap())
		return;

	auto name_node = node["name"];
	if (!name_node)
		return;
	std::string name = name_node.as<std::string>();

	auto path_node = node["path"];
	if (path_node) {
		std::string path = path_node.as<std::string>();
		client->getSounds()->load(name.c_str(), path.c_str());
	} else {
		client->getSounds()->createRandomized(name.c_str());
		auto list_node = node["list"];
		if (list_node && list_node.IsSequence()) {
			for (auto iter = list_node.begin(); iter != list_node.end(); ++iter) {
				std::string entry = iter->as<std::string>();
				client->getSounds()->addToRandomized(name.c_str(), entry.c_str());
			}
		}
	}
}

void ResourceLoader::loadBlocks(const YAML::Node &node) {
	if (!node.IsMap())
		return;

	BlockLoader bl(client);

	struct TreeTraversalState {
		YAML::Node node;
		YAML::Node::const_iterator iter;
	};
	std::stack<TreeTraversalState> stack;
	std::vector<std::string> names;
	std::string prefix = "";
	stack.push(TreeTraversalState{ node, node.begin() });

	while (stack.size() > 0) {
		if (stack.top().iter != stack.top().node.end()) {
			if (stack.empty()) break;
			const YAML::Node &key_node = stack.top().iter->first;
			const YAML::Node &value_node = stack.top().iter->second;
			++stack.top().iter;
			std::string key = key_node.as<std::string>();
			if (value_node.IsScalar()) {
				int value = value_node.as<int>();
				bl.add(prefix + key, value);
			} else if (value_node.IsMap()) {
				stack.push(TreeTraversalState{ value_node, value_node.begin() });
				names.push_back(key);
				prefix = "";
				for (auto str : names)
					prefix += str + ".";
			}
		} else if (stack.size() == 1) {
			break;
		} else {
			stack.pop();
			names.pop_back();
			prefix = "";
			for (auto str : names)
				prefix += str + ".";
		}
	}
}
