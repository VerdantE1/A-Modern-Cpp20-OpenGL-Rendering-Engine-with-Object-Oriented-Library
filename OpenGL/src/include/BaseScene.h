#pragma once
#include "Scene.h"
#include "GlobalLight.h"
#include "Renderer.h"
#include "LightComponent.h"
#include <unordered_map>
#include <functional>
#include "Globals.h"
#include "RenderComponent.h"

class BaseScene : public Scene {
public:
    using EnityInitializer = std::function<void(Scene*) >;

    void Initialize() override;
    void Update(float delataTime) override;
    void Render(const Renderer& renderer) override;
    void Cleanup() override;
    
    // 设置实体初始化器
    void SetEntityInitializer(EnityInitializer initializer);
    
    Renderer& GetRenderer();

    //提供访问实体的方法（供Engine使用）
    const std::vector<std::unique_ptr<Entity>>& GetAllEntities() const { return m_Entities; }

   
    void RenderSkybox();

private:
    CameraConfig cameraConfig;
    EnityInitializer enityInitializer;
    GlobalLight m_globalLight;
    Renderer m_renderer;
    std::unordered_map<std::string, LightComponent*> m_lightIndex; // 局部光源索引

    void RenderAllEntities(const Renderer& renderer, const glm::mat4& view, const glm::mat4& projection);
    void BuildLightIndex();
    void ApplyIndexedLightsToShader(Shader& shader, const glm::mat4& view);
    void SetGlobalLight();
    void ApplyGlobalLightToAllShaders();
    void InlitializeEntities(EnityInitializer initializer);
    void UpdateAllEntity(float deltaTime);
    void UpdateDynamicLights(float deltaTime);
    void ApplyGlobalLightToShader(RenderComponent& renderComp);

    // 阴影系统支持
    void InitializeShadowSystem();    // 初始化阶段调用
    void RenderShadowPass(const Renderer& renderer);  // 渲染阶段调用

    // 天空盒
	std::unique_ptr<Texture> m_skyboxTexture = nullptr;
};