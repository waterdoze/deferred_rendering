#pragma  once
#ifndef CAMERA_H
#define CAMERA_H

#include <memory>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class MatrixStack;

class Camera
{
public:
	enum {
		ROTATE = 0,
		TRANSLATE,
		SCALE
	};
	
	Camera();
	virtual ~Camera();
	void setInitDistance(float z) { translations.z = -std::abs(z); }
	void setAspect(float a) { aspect = a; };
	void setRotationFactor(float f) { rfactor = f; };
	void setTranslationFactor(float f) { tfactor = f; };
	void setScaleFactor(float f) { sfactor = f; };
	void mouseClicked(float x, float y, bool shift, bool ctrl, bool alt);
	void mouseMoved(float x, float y);
	void wasd(char key);
    void changeFOVY(bool zoomIn);
	void applyProjectionMatrix(std::shared_ptr<MatrixStack> P) const;
	void applyViewMatrix(std::shared_ptr<MatrixStack> MV) const;
    glm::mat4 getViewMatrix() const;
	glm::vec3 getPosition();
    float getFOVY();

private:
	float aspect;
	float fovy;
	float znear;
	float zfar;
	glm::vec2 rotations;
	glm::vec2 mousePrev;

	glm::vec3 translations;
	glm::vec3 position;
	float yaw;
	float pitch;

	int state;
	float rfactor;
	float tfactor;
	float sfactor;
};

#endif
