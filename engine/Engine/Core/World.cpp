#include "World.h"

#include "Engine/Nodes/Body.h"
#include "Engine/Utilities/DebugMemory.h"
#include "Engine/Utilities/ObjectFactory.h"
#include "Engine/Utilities/Debug.h"
#include "Engine/Core/Renderer.h"
#include "Engine/Nodes/Camera.h"

World::World()
{
	_root = DBG_NEW Node2D();
	_nodes = std::vector<Node*>();
}

World::~World()
{
	for (size_t i = 0; i < _nodes.size(); i++)
	{
		if (_nodes[i] != nullptr) {
			delete(_nodes[i]);
		}
	}
	if (_root != nullptr) {
		delete(_root);
	}
}

void World::tick(float delta)
{
	//Input

	//Logic
}

void World::render()
{
	for (size_t i = 0; i < _nodes.size(); i++)
	{
		_nodes[i]->render();
	}
}

Node* World::create(std::string nodeType)
{
	Node* node = nullptr;
	node = me::util::ObjectFactory::createObject(nodeType);
	if (node != nullptr) {
		node->setName(nodeType + std::to_string(_nodes.size()));
		_nodes.push_back(node);
		return node;
	}
	else {
		return nullptr;
	}
}

bool World::eraseNode(Node* node, int index)
{
	if (Renderer::getCamera() != nullptr) {
		if (node->getName() == Renderer::getCamera()->getName()) {

			Renderer::setCamera(nullptr);
		}
	}

	delete(node);
	_nodes.erase(_nodes.begin() + index);
	return true;
}

bool World::destroy(std::string name)
{
	for (size_t i = 0; i < _nodes.size(); i++)
	{
		if (_nodes[i]->getName() == name) {
			return eraseNode(_nodes[i], i);;
		}
	}
	return false;
}

bool World::destroy(Node* node)
{
	for (size_t i = 0; i < _nodes.size(); i++)
	{
		if (node == _nodes[i]) {
			return eraseNode(_nodes[i], i);
		}
	}
	return false;
}

nlohmann::json World::to_json()
{
	nlohmann::json retval;
	retval["name"] = name;
	for (size_t i = 0; i < _nodes.size(); i++)
	{
		retval["nodes"][i] = (_nodes[i]->to_json());
	}
	return retval;
}

void World::from_json(const nlohmann::json& j)
{
	name = j.at("name");
	Node* node;
	for (auto& [key, value] : j["nodes"].items()) {
		node = me::util::ObjectFactory::createObject(value["type"]);
		if (node == nullptr) {
			Debug::Log("Failed to create. Type not found! " + value["type"]);
		}
		else {
			node->from_json(value);
			_nodes.push_back(node);
		}
	}
}
