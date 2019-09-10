#pragma once
#include "glm/vec2.hpp"
#include "glm/mat3x3.hpp"

class Transform
{
public:
	Transform();
	~Transform();

	glm::vec2 getLocalPosition();
	void setLocalPosition(glm::vec2 value);

	glm::vec2 getLocalScale();
	void setLocalScale(glm::vec2 value);

	float getLocalAngle();
	void setLocalAngle(float value);

	Transform* getParent();
	void setParent(Transform* value);

	glm::mat3x3 getLocalMatrix();
	glm::mat3x3 getGlobalMatrix();

private:
	glm::vec2 _position;
	glm::vec2 _scale;
	float _angle;
	Transform* _parent;
};