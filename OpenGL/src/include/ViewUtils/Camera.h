#pragma once
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

struct FrustumRect {
    float left, right, bottom, top, z;
};

inline std::ostream& operator<<(std::ostream& os, const FrustumRect& rect) {
    os << "FrustumRect: ["
        << "left=" << rect.left << ", right=" << rect.right
        << ", bottom=" << rect.bottom << ", top=" << rect.top
        << ", z=" << rect.z << "]";
    return os;
}

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 target;
    glm::vec3 up;
    float fov, aspect, nearPlane, farPlane;

    Camera(const glm::vec3& pos, const glm::vec3& tgt, const glm::vec3& upVec,
           float fovAngle, float aspectRatio, float nearPlaneDist, float farPlaneDist)
        : position(pos), target(tgt), up(upVec),
          fov(fovAngle), aspect(aspectRatio), nearPlane(nearPlaneDist), farPlane(farPlaneDist),
          m_originalPosition(pos), m_originalTarget(tgt), m_originalUp(upVec) {}

    glm::mat4 GetViewMatrix() const {
        return glm::lookAt(position, target, up);
    }

    glm::mat4 GetProjectionMatrix() const {
        return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    }

    // 🆕 相机控制功能
    void StartMouseLook(double mouseX, double mouseY) {
        m_isMouseLookActive = true;
        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
        // 🔧 记录初始状态
        m_initialRadius = glm::length(position - target);
    }

    void UpdateMouseLook(double mouseX, double mouseY) {
        if (!m_isMouseLookActive) return;

        // 🔧 添加有效性检查
        if (mouseX == m_lastMouseX && mouseY == m_lastMouseY) return;

        double deltaX = mouseX - m_lastMouseX;
        double deltaY = mouseY - m_lastMouseY;

        // 🔧 降低灵敏度，减少计算负担
        float sensitivity = 0.003f;

        glm::vec3 direction = position - target;
        float radius = m_initialRadius;  // 🔧 使用固定半径

        // 🔧 简化球坐标计算
        float theta = atan2(direction.x, direction.z) + deltaX * sensitivity;
        float phi = asin(glm::clamp(direction.y / radius, -0.99f, 0.99f)) + deltaY * sensitivity;

        // 限制角度
        phi = glm::clamp(phi, -1.4f, 1.4f);

        // 🔧 优化：直接计算新位置
        position = target + radius * glm::vec3(
            sin(theta) * cos(phi),
            sin(phi),
            cos(theta) * cos(phi)
        );

        m_lastMouseX = mouseX;
        m_lastMouseY = mouseY;
    }

    void StopMouseLook() {
        m_isMouseLookActive = false;
    }

    void ResetToOriginal() {
        position = m_originalPosition;
        target = m_originalTarget;
        up = m_originalUp;
        m_isMouseLookActive = false;
    }

    bool IsMouseLookActive() const {
        return m_isMouseLookActive;
    }

    // 现有功能保持不变
    FrustumRect GetFrustumRectAtZ(float z) const {
        float half_fovy = glm::radians(fov) * 0.5f;
        float tan_half_fovy = std::tan(half_fovy);
        float abs_z = std::abs(z);

        float h = tan_half_fovy * abs_z;
        float w = h * aspect;
        return FrustumRect{ -w, +w, -h, +h, z };
    }

    FrustumRect GetNearFrustumRect() const {
        return GetFrustumRectAtZ(-nearPlane);
    }

    FrustumRect GetFarFrustumRect() const {
        return GetFrustumRectAtZ(-farPlane);
    }

private:
    // 🆕 相机控制状态
    bool m_isMouseLookActive = false;
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    float m_initialRadius = 0.0f;  // 🔧 缓存初始半径
    
    // 🆕 原始状态
    glm::vec3 m_originalPosition;
    glm::vec3 m_originalTarget;
    glm::vec3 m_originalUp;
};
