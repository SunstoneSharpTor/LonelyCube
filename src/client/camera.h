/*
  Copyright (C) 2024 Bertie Cartwright

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "constants.h"

namespace client {

struct Plane {
	//unit vector
	glm::vec3 normal = { 0.f, 1.f, 0.f };
	//distance from origin to the nearest point in the plane
	float distance = 0.0f;

	Plane(const glm::vec3& point, const glm::vec3& norm) {
		normal = glm::normalize(norm);
		distance = glm::dot(normal, point);
	}

	Plane() {};
	
	float getSignedDistanceToPlane(const glm::vec3& point) const {
		return glm::dot(normal, point) - distance;
	}
};

struct Frustum {
	Plane topFace;
	Plane bottomFace;
	
	Plane rightFace;
	Plane leftFace;
	
	Plane farFace;
	Plane nearFace;
};

struct AABB {
	glm::vec3 centre{ 0.f, 0.f, 0.f };
	glm::vec3 extents{ 0.f, 0.f, 0.f };

	AABB(const glm::vec3& min, const glm::vec3& max) {
		centre = (max + min) * 0.5f;
		extents = { max.x - centre.x, max.y - centre.y, max.z - centre.z };
	}

	AABB(const glm::vec3& inCenter, float iI, float iJ, float iK) {
		centre = inCenter;
		extents = { iI, iJ, iK };
	}

	bool isOnOrForwardPlane(const Plane& plane) const {
		//Compute the projection interval radius of b onto L(t) = b.c + t * p.n
		const float r = extents.x * std::abs(plane.normal.x) +
			extents.y * std::abs(plane.normal.y) + extents.z * std::abs(plane.normal.z);

		return -r <= plane.getSignedDistanceToPlane(centre);
	}

	bool isOnFrustum(const Frustum& camFrustum) const {
		return (isOnOrForwardPlane(camFrustum.leftFace) &&
			isOnOrForwardPlane(camFrustum.rightFace) &&
			isOnOrForwardPlane(camFrustum.topFace) &&
			isOnOrForwardPlane(camFrustum.bottomFace) &&
			isOnOrForwardPlane(camFrustum.nearFace));
			//isOnOrForwardPlane(camFrustum.farFace));
	};
};

class Camera {
public:
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;
	glm::vec3 position;

	Camera(glm::vec3 Position = glm::vec3(0.0f, 0.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f, glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f));
	
	void updateRotationVectors(float yaw, float pitch);

	void getViewMatrix(glm::mat4* viewMatrix);

	void getPosition(float* position);

	Frustum createViewFrustum(float aspect, float fovY, float zNear, float zFar);
};

}  // namespace client