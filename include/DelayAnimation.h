#pragma once
#include "Animation.h"

class DelayAnimation : public Animation {
public:
    DelayAnimation(Object3D& object, float duration) : Animation(object, duration) {}

private:
    void startAnimation() override {
    }

    void applyAnimation(float dt) override {
    }
};
