#pragma once
#include "Object3D.h"
#include "Animation.h"
#include <glm/glm.hpp>

class BezierCurveAnimation : public Animation {
private:
    glm::vec3 m_p0;
    glm::vec3 m_p1;
    glm::vec3 m_p2;
    glm::vec3 m_p3;
    float m_elapsed;
    float m_duration;

    void applyAnimation(float dt) override {
        m_elapsed += dt;
        float t = glm::clamp(m_elapsed / m_duration, 0.0f, 1.0f);
        float u = 1 - t;
        glm::vec3 newPosition = u * u * u * m_p0 + 3 * u * u * t * m_p1 + 3 * u * t * t * m_p2 + t * t * t * m_p3;

        object().setPosition(newPosition);
    }

public:
    BezierCurveAnimation(Object3D& object, float duration,
        const glm::vec3& p0, const glm::vec3& p1,
        const glm::vec3& p2, const glm::vec3& p3)
        : Animation(object, duration), m_p0(p0), m_p1(p1), m_p2(p2), m_p3(p3), m_elapsed(0.0f), m_duration(duration) {}
};
