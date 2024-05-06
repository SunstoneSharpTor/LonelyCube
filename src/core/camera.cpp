#include "camera.h"

camera::camera(glm::vec3 Position, float yaw, float pitch, glm::vec3 WorldUp) {
	position = Position;
    worldUp = WorldUp;
    updateRotationVectors(yaw, pitch);
}

void camera::updateRotationVectors(float yaw, float pitch) {
    glm::vec3 cameraDirection;
    cameraDirection.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraDirection.y = sin(glm::radians(pitch));
    cameraDirection.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(cameraDirection);
    right = glm::normalize(glm::cross(front, worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up = glm::normalize(glm::cross(right, front));
}

void camera::getViewMatrix(glm::mat4* viewMatrix) {
    *viewMatrix = glm::lookAt(position, position + front, up);
}

void camera::getPosition(float* floatPosition) {
    floatPosition[0] = position.x;
    floatPosition[1] = position.y;
    floatPosition[2] = position.z;
}

frustum camera::createViewFrustum(float aspect, float fovY, float zNear, float zFar) {
    frustum viewFrustum;
    const float halfVSide = zFar * tanf(glm::radians(fovY) * 0.5f);
    const float halfHSide = halfVSide * aspect;
    const glm::vec3 frontMultFar = zFar * front;

    viewFrustum.nearFace = { position + zNear * front, front };
    viewFrustum.farFace = { position + frontMultFar, -front };
    viewFrustum.rightFace = { position, glm::cross(frontMultFar - right * halfHSide, up) };
    viewFrustum.leftFace = { position, glm::cross(up,frontMultFar + right * halfHSide) };
    viewFrustum.topFace = { position, glm::cross(right, frontMultFar - up * halfVSide) };
    viewFrustum.bottomFace = { position, glm::cross(frontMultFar + up * halfVSide, right) };

    return viewFrustum;
}