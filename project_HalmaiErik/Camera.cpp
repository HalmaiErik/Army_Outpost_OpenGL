#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 cameraUp) {
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = cameraUp;

        //TODO - Update the rest of camera parameters
        this->cameraFrontDirection = glm::normalize(cameraTarget - cameraPosition);
        this->cameraRightDirection = glm::normalize(glm::cross(cameraFrontDirection, cameraUp));
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(cameraPosition, cameraTarget, cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        switch (direction) {
        case MOVE_FORWARD:
            this->cameraPosition += cameraFrontDirection * speed;
            break;

        case MOVE_BACKWARD:
            this->cameraPosition -= cameraFrontDirection * speed;
            break;

        case MOVE_RIGHT:
            this->cameraPosition += cameraRightDirection * speed;
            break;

        case MOVE_LEFT:
            this->cameraPosition -= cameraRightDirection * speed;
            break;
        }
        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 front;
        front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        front.y = sin(glm::radians(pitch));
        front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->cameraFrontDirection = glm::normalize(front);

        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraFrontDirection, glm::vec3(0.0f, 1.0f, 0.0f)));

        this->cameraUpDirection = glm::normalize(glm::cross(this->cameraRightDirection, this->cameraFrontDirection));

        cameraTarget = cameraPosition + cameraFrontDirection;
    }

    glm::vec3 Camera::getCameraTarget() {
        return this->cameraTarget;
    }

    glm::vec3 Camera::getCameraPosition() {
        return this->cameraPosition;
    }
}