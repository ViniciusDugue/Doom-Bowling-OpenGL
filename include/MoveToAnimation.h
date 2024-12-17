#pragma once
#include "Object3D.h"
#include "Animation.h"


class MoveToAnimation : public Animation {
private:
    glm::vec3 m_startPosition;

    glm::vec3 m_targetPosition;

    float m_elapsedTime = 0.0f;

    void applyAnimation(float dt) override {
        m_elapsedTime += dt;
        float t = glm::clamp(m_elapsedTime / duration(), 0.0f, 1.0f);
        glm::vec3 newPosition = glm::mix(m_startPosition, m_targetPosition, t);
        object().setPosition(newPosition);
    }

public:

    MoveToAnimation(Object3D& object, float duration, const glm::vec3& targetPosition) :
        Animation(object, duration),
        m_startPosition(object.getPosition()),
        m_targetPosition(targetPosition) {}
};
