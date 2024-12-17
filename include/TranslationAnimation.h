#pragma once
#include "Object3D.h"
#include "Animation.h"

class TranslationAnimation : public Animation {
private:

    glm::vec3 m_perSecond;

    void applyAnimation(float dt) override {
        object().move(m_perSecond * dt);
    }

public:
    TranslationAnimation(Object3D& object, float duration, const glm::vec3& totalTranslation) :
        Animation(object, duration), m_perSecond(totalTranslation / duration) {}
};
