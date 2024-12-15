#ifndef HELPER_FUNCTIONS_H
#define HELPER_FUNCTIONS_H

#include "AssimpImport.h"  // For assimpLoad
#include <glm/glm.hpp>
#include <vector>
#include "Object3D.h"

// Function to create and configure an Object3D
inline Object3D createObject(const std::string& path, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation = glm::vec3(0)) {
    auto object = assimpLoad(path, true);
    object.move(position);
    object.grow(scale);
    object.rotate(rotation);
    return object;
}

// Function to add pins dynamically
inline void addPins(Scene& scene, const glm::vec3& basePosition, const glm::vec3& scale, float spacing) {
    std::vector<glm::vec3> pinOffsets = {
        {0, 0, 0},                     // Row 1
        {-spacing / 2, 0, spacing},    // Row 2
        {spacing / 2, 0, spacing},
        {-spacing, 0, 2 * spacing},    // Row 3
        {0, 0, 2 * spacing},
        {spacing, 0, 2 * spacing},
        {-1.5f * spacing, 0, 3 * spacing}, // Row 4
        {-0.5f * spacing, 0, 3 * spacing},
        {0.5f * spacing, 0, 3 * spacing},
        {1.5f * spacing, 0, 3 * spacing},
    };

    for (const auto& offset : pinOffsets) {
        glm::vec3 pinPosition = basePosition + offset;
        scene.objects.push_back(createObject("models/bowlingpin/Pin.obj", pinPosition, scale));
    }
}

// Function to add a rotation animation
inline void addRotationAnimation(Scene& scene, int objectIndex, float duration, const glm::vec3& axis) {
    Animator animator;
    animator.addAnimation(std::make_unique<RotationAnimation>(scene.objects[objectIndex], duration, axis));
    scene.animators.push_back(std::move(animator));
}

#endif // SCENE_HELPERS_H