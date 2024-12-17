#pragma once
#include "Object3D.h"
#include "Animation.h"
#include <glm/glm.hpp>
#include <cmath>

class SinBouncingAnimation : public Animation {
private:
    float m_xRotationRange;
    float m_elapsed;

    void applyAnimation(float dt) override {
        m_elapsed += dt;
        float pi = 3.14151f;
        float sineValue = std::sin(2 * pi * m_elapsed);

        //find new X rotation offset
        float xRotationOffset = glm::radians(sineValue * m_xRotationRange);
        glm::vec3 currentRotation = object().getOrientation();
        currentRotation.x = xRotationOffset;
        object().setOrientation(currentRotation);
    }

public:
    SinBouncingAnimation(Object3D& object, float duration, float xRotationRange)
        : Animation(object, duration),
        m_xRotationRange(xRotationRange),
        m_elapsed(0.0f) {}
};