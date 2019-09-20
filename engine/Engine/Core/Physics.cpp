#include "Engine/Core/Physics.h"
#include "Engine/Nodes/Collider.h"
#include "Engine/Utilities/Debug.h"
#include "glm/vec2.hpp"

#define _CRTDBG_MAP_ALLOC
#include <cstdlib>
#include <crtdbg.h>
#ifdef _DEBUG
#define DBG_NEW new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
// Replace _NORMAL_BLOCK with _CLIENT_BLOCK if you want the
// allocations to be of _CLIENT_BLOCK type
#else
#define DBG_NEW new
#endif


Physics* Physics::_instance = nullptr;
glm::vec2 Physics::_gravity = { 0.0f, -9.81f };
std::vector<Collider*> Physics::_colliders;

void Physics::init()
{
	if (_instance == nullptr) {
		_instance = DBG_NEW Physics();
	}
}

Physics::Physics()
{

}

Physics::~Physics()
{

}

void Physics::quit()
{
	delete(_instance);
}

void Physics::tick()
{
	for (size_t a = 0; a < _colliders.size(); a++){
		if (_colliders[a] != nullptr) {
			for (size_t b = 0; b < _colliders.size(); b++){
				if (_colliders[a] != _colliders[b]) {
					Collision col = _colliders[a]->doesCollide(_colliders[b]);
					if (col.penetration > 0) {
						if (!_colliders[b]->isTrigger && !_colliders[a]->isTrigger) {
							if (_colliders[b]->isStatic && !_colliders[a]->isStatic) {
								_colliders[a]->transform.translate(col.penetration * col.collisionNormal);
							}
							if (!_colliders[b]->isStatic && !_colliders[a]->isStatic) {
								_colliders[a]->transform.translate(col.penetration / 2 * col.collisionNormal);
								_colliders[b]->transform.translate(-col.penetration / 2 * col.collisionNormal);
							}
						}
						_colliders[a]->addColliding(col);
						col.other = _colliders[a];
						_colliders[b]->addColliding(col);
					}
				}
			}
		}
	}
	for (size_t i = 0; i < _colliders.size(); i++) {
		_colliders[i]->solveColliding();
	}

}

void Physics::setGravity(glm::vec2& value)
{
	_gravity = value;
}

glm::vec2 Physics::getGravity()
{
	return glm::vec2();
}

bool Physics::addCollider(Collider* col)
{
	size_t placement = -1;
	for (size_t i = 0; i < _colliders.size(); i++)
	{
		if (_colliders[i] == col) {
			return false;
		}
		if (_colliders[i] == nullptr) {
			placement = i;
		}
	}

	if (placement != -1) {
		_colliders[placement] = col;
	}
	else {
		_colliders.push_back(col);
	}
	return true;
}

bool Physics::removeCollider(Collider* col)
{
	for (size_t i = 0; i < _colliders.size(); i++)
	{
		if (_colliders[i] == col) {
			_colliders[i] = nullptr;
			return true;
		}
	}

	return false;
}

void Physics::debugDrawColliders() {
	for (size_t i = 0; i < _colliders.size(); i++)
	{
		if (_colliders[i] != nullptr) {
			Debug::DrawCollider(_colliders[i]);
		}
	}
}
