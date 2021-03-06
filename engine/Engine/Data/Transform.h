#pragma once
#include "glm/vec2.hpp"
#include "glm/mat3x3.hpp"
#include <nlohmann/json.hpp>
#include "Engine/Utilities/glmJson.h"

struct Transform
{
public:
	Transform();
	~Transform();

	glm::vec2 getLocalPosition();
	void setLocalPosition(glm::vec2 value);

	glm::vec2 getPosition();

	glm::vec2 getLocalScale();
	void setLocalScale(glm::vec2 value);

	float getLocalAngle();
	void setLocalAngle(float value);

	Transform* getParent();
	void setParent(Transform* value);

	//void addChild(Transform* child);
	//void removeChild(Transform* child);

	void translate(glm::vec2 value);

	glm::mat4x4 getLocalMatrix();
	glm::mat4x4 getGlobalMatrix();
	float depth;

	nlohmann::json get_json() {
		nlohmann::json j;
		j["position"] = _position;
		j["scale"] = _scale;
		j["angle"] = _angle;
		//TODO Parent somehow
		return j;
	}

	void from_json(const nlohmann::json& j) {
		setLocalPosition(j["position"]);
		setLocalScale(j["scale"]);
		setLocalAngle(j["angle"]);
	}


private:
	glm::vec2 _position;
	glm::vec2 _scale;
	
	float _angle;
	Transform* _parent;
};