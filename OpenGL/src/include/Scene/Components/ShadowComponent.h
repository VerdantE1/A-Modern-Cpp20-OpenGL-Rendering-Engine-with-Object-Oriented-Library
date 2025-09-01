#pragma once
#include "Component.h"
#include "Shader.h"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>

// 前向声明
class LightComponent;
class Renderer;
class Entity;

class ShadowComponent : public Component {
public:
    enum class ShadowType {
        NONE,           // 无阴影
        SHADOW_MAP      // 标准阴影映射 (支持所有光源类型)
    };

    enum class Quality {
        LOW,
        MEDIUM,
        HIGH,
        Ultra
    };

    enum class ShadowQuality {
        HARD_SHADOW,    // 硬阴影（禁用PCF）
        SOFT_LOW,       // 软阴影低质量（2x2）
        SOFT_MEDIUM,    // 软阴影中等质量（3x3）
        SOFT_HIGH,      // 软阴影高质量（4x4）
        SOFT_ULTRA      // 软阴影超高质量（5x5）
    };

    ShadowComponent(ShadowType type = ShadowType::SHADOW_MAP);
    ~ShadowComponent();

    // 阴影映射配置
    void SetShadowMapSize(int width, int height);
    void SetShadowMapQuality(Quality quality);
    void SetShadowType(ShadowType type) { m_shadowType = type; }
    void SetBias(float bias) { m_shadowBias = bias; }
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled && m_shadowType != ShadowType::NONE; }
    void SetPCFEnabled(bool enabled) { m_enablePCF = enabled; }
    void SetPCFSamples(int samples) { m_pcfSamples = samples; }
    void SetPCFRadius(float radius) { m_pcfRadius = radius; }

    //  确保这些 getter 方法存在
    float GetBias() const { return m_shadowBias; }
    bool IsPCFEnabled() const { return m_enablePCF; }
    int GetPCFSamples() const { return m_pcfSamples; }
    float GetPCFRadius() const { return m_pcfRadius; }
    GLuint GetShadowMapTexture() const { return m_shadowMapTexture; }
    glm::mat4 GetLightSpaceMatrix() const { return m_lightSpaceMatrix; }
    
    // 渲染流程
    void BeginShadowPass();
    void EndShadowPass();

    
    // 组件生命周期
    void Update(float deltaTime) override;

    // 渲染物体
    void RenderShadowCasters(const Renderer& renderer, const std::vector<std::unique_ptr<Entity>>& entities);

	void SetShadowQuality(ShadowQuality quality);

    // 阴影Shader
    std::shared_ptr<Shader> GetShadowShader();

    // 绑定阴影深度图到固定槽位
    void BindDepthTextureToFixedSlot() const;
    // 设置采样器 uniform 为固定槽位（sampler 名需与你的着色器一致）
    void ApplyShadowUniforms(Shader& shader, const char* samplerUniform = "shadowMap") const;
    // 统一从组件写入本组件所需的所有 uniform（渲染阶段调用）
    void ApplyToShader(Shader& shader) override;

private:
    void InitializeShadowMap();
    void CalculateLightSpaceMatrix();
    std::shared_ptr<Shader> CreateShadowShader();
    
    // 获取关联的光源组件
    LightComponent* GetAssociatedLight() const;
    
    ShadowType m_shadowType = ShadowType::SHADOW_MAP;
    bool m_enabled = true;
    
    // 阴影映射资源
    GLuint m_shadowFramebuffer = 0;
    GLuint m_shadowMapTexture = 0;
    int m_shadowMapWidth = 1024;
    int m_shadowMapHeight = 1024;
    
    // 阴影参数
    float m_shadowBias = 0.005f;
    float m_nearPlane = 1.0f;
    float m_farPlane = 25.0f;
    
    // PCF 控制参数
    bool m_enablePCF = true;        // 是否启用PCF
    int m_pcfSamples = 9;          // PCF采样数量（4, 9, 16, 25）
    float m_pcfRadius = 1.0f;      // PCF采样半径

    // 光源空间变换矩阵
    glm::mat4 m_lightSpaceMatrix = glm::mat4(1.0f);

        // 阴影着色器
    std::shared_ptr<Shader> m_shadowShader;

public:
    // 🆕 添加近远平面设置方法
    void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
    void SetFarPlane(float farPlane) { m_farPlane = farPlane; }
    float GetNearPlane() const { return m_nearPlane; }
    float GetFarPlane() const { return m_farPlane; }
};