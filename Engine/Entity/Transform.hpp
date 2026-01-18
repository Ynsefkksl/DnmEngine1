#pragma once

#include "Entity/Entity.hpp"
#include "Utility/EngineDefines.hpp"
#include "Entity/Component.hpp"
#include <glm/glm.hpp>

#include <glm/gtc/matrix_transform.hpp>

class ENGINE_API Transform {
public:
	Transform() : position(0), scale(1), rotation(0) {};
		
	const glm::vec3& GetPosition() const { return position; }
	const glm::vec3& GetScale() const { return scale; }
	const glm::vec3& GetRotation() const { return rotation; }

	void SetPosition(const glm::vec3& value) { position = value; }
	void SetScale(const glm::vec3& value) { scale = value; }
	void SetRotation(const glm::vec3& value) { rotation = value; }

	void AddPosition(const glm::vec3& value) { position += value; }
	void AddScale(const glm::vec3& value) { scale += value; }
	void AddRotation(const glm::vec3& value) { rotation += value; }

	glm::mat4 CalcModelMatrix() const noexcept;
	glm::mat4 CalcViewMatrix() const noexcept;
	glm::vec3 CalcDirection() const noexcept;

	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;
};

inline glm::mat4 Transform::CalcModelMatrix() const noexcept {
	auto modelMtx = glm::mat4(1);
	modelMtx = glm::translate(modelMtx, position);
	modelMtx = glm::scale(modelMtx, scale);
	modelMtx = glm::rotate(modelMtx, glm::radians(rotation.x), glm::vec3(1, 0, 0));
	modelMtx = glm::rotate(modelMtx, glm::radians(rotation.y), glm::vec3(0, 1, 0));
	modelMtx = glm::rotate(modelMtx, glm::radians(rotation.z), glm::vec3(0, 0, 1));
	return modelMtx;
}

inline glm::mat4 Transform::CalcViewMatrix() const noexcept {
	return glm::lookAt(position, position + CalcDirection(), glm::vec3(0.0f, 1.0f, 0.0f));
}

inline glm::vec3 Transform::CalcDirection() const noexcept {
	auto direction = glm::vec3(cos(glm::radians(rotation.y)) * cos(glm::radians(rotation.x)),
        sin(glm::radians(rotation.x)),
        sin(glm::radians(rotation.y)) * cos(glm::radians(rotation.x)));

    return direction;
}