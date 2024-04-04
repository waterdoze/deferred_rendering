#define _USE_MATH_DEFINES
#include <cmath> 
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "MatrixStack.h"

Camera::Camera() :
	aspect(1.0f),
	fovy((float)(45.0*M_PI/180.0)),
	znear(0.1f),
	zfar(1000.0f),
	rotations(0.0, 0.0),
	translations(0.0f, 0.0f, 0.0f),
	rfactor(0.1f),
	tfactor(0.1f),
	sfactor(0.005f),
    position(0, 1, 0),
    yaw(0),
    pitch(0)
{
}

Camera::~Camera()
{
}

void Camera::mouseClicked(float x, float y, bool shift, bool ctrl, bool alt)
{
	mousePrev.x = x;
	mousePrev.y = y;
    if(ctrl) {
		state = Camera::SCALE;
	} else {
		state = Camera::ROTATE;
	}
}

void Camera::mouseMoved(float x, float y)
{
	glm::vec2 mouseCurr(x, y);
	glm::vec2 dv = mouseCurr - mousePrev;
	switch(state) {
		case Camera::ROTATE:
			yaw -= rfactor * dv.x;
            pitch += rfactor * dv.y;
			break;
		case Camera::SCALE:
			position.z *= (1.0f - sfactor * dv.y);
			break;
	}
	mousePrev = mouseCurr;
}
void Camera::wasd(char key) {
    glm::vec3 forward = glm::vec3(
        cos(glm::radians(yaw)),
        0,
        sin(glm::radians(yaw))
    );
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

    switch(key) {
        case 'w':
			position += forward * tfactor;
            break;
        case 'a':
			position -= right * tfactor;
            break;
        case 's':
			position -= forward * tfactor;
            break;
        case 'd':
			position += right * tfactor;
            break;

    }

}
void Camera::applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const
{
	// Modify provided MatrixStack
	P->multMatrix(glm::perspective(fovy, aspect, znear, zfar));
}

void Camera::applyViewMatrix(std::shared_ptr<MatrixStack> MV) const
{
    glm::mat4 viewMatrix = getViewMatrix();
    MV->multMatrix(viewMatrix);

}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 forward = glm::vec3(
        cos(glm::radians(yaw)) * cos(glm::radians(pitch)),
        sin(glm::radians(pitch)),
        sin(glm::radians(yaw)) * cos(glm::radians(pitch))
    );

    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    glm::vec3 target = position + forward;

    return glm::lookAt(position, target, up);
}

void Camera::changeFOVY(bool zoomIn) {
    fovy += zoomIn ? -0.1 : 0.1;
    fovy = std::min(std::max(glm::radians(4.0), (double) fovy), glm::radians(114.0));

}

glm::vec3 Camera::getPosition() {
    return position;
}

float Camera::getFOVY() {
    return fovy;
}