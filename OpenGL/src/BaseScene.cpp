#include "BaseScene.h"
#include "Logger.h"
#include "Entity.h"
#include "LightComponent.h"
#include "RenderComponent.h"
#include "ShadowComponent.h"
#include "Camera.h"
#include "MaterialComponent.h"
#include "Globals.h"
#include <GLFW/glfw3.h>

void BaseScene::Initialize() {
    LOG_INFO("BaseScene: Initializing scene...");

    // 1. Configure renderer first
    m_renderer.SetPolygonMode(false).SetDepthTest(true);

    // 2. 创建摄像机
    SetCamera(std::make_unique<Camera>(cameraConfig.CreateCamera()));

    // 3. 设置全局光源
    SetGlobalLight();

    // 4. 初始化实体
    InlitializeEntities(enityInitializer);

    // 5. 建立局部光源索引
    BuildLightIndex();

	// 6. 阴影贴图相关
    InitializeShadowSystem();

    LOG_INFO("\tEntities initialized: Count={}", m_Entities.size());
    LOG_INFO("\tLights indexed: Count={}", m_lightIndex.size());
}

void BaseScene::Update(float delataTime) {
    //1.通用更新
    UpdateAllEntity(delataTime);        // 只调用 Component::Update

    //2.光照更新
    UpdateDynamicLights(delataTime);    // 更新动态光源位置等
}

void BaseScene::Render(const Renderer& renderer) {
    LOG_TRACE("BaseScene: Starting render");
    // Step1: Shadow Pass
    RenderShadowPass(renderer);

    // Step2: Main Pass
    renderer.Clear();     // 使用传入的 renderer
    glm::mat4 view = GetCamera().GetViewMatrix();
    glm::mat4 projection = GetCamera().GetProjectionMatrix();

    // 使用传入的 renderer
    RenderAllEntities(renderer, view, projection);

    LOG_TRACE("BaseScene: Render complete");
}

void BaseScene::SetEntityInitializer(EnityInitializer initializer) {
    enityInitializer = initializer;
}

void BaseScene::Cleanup() {
    m_Entities.clear();
    m_NamedEntities.clear();
}

Renderer& BaseScene::GetRenderer() { 
    return m_renderer; 
}

void BaseScene::ApplyIndexedLightsToShader(Shader& shader, const glm::mat4& view) {
    if (m_lightIndex.empty()) {
        LOG_WARNING("No indexed lights found");
        return;
    }

    auto lightIt = m_lightIndex.begin();
    auto* lightComp = lightIt->second;

    if (lightComp && lightComp->enabled) {
        auto* lightEntity = lightComp->GetOwner();

        // ========== 只应用光源数据 ==========
        if (auto transform = lightEntity->GetTransform()) {
            glm::vec3 worldPos = transform->GetPosition();
            glm::vec3 viewPos = glm::vec3(view * glm::vec4(worldPos, 1.0f));
            shader.SetUniform3f("light.position", viewPos.x, viewPos.y, viewPos.z);
        }

        // 应用光源属性
        shader.SetUniform4f("light.ambient",
            lightComp->ambient.r * lightComp->intensity,
            lightComp->ambient.g * lightComp->intensity,
            lightComp->ambient.b * lightComp->intensity,
            lightComp->ambient.a);
        shader.SetUniform4f("light.diffuse",
            lightComp->diffuse.r * lightComp->intensity,
            lightComp->diffuse.g * lightComp->intensity,
            lightComp->diffuse.b * lightComp->intensity,
            lightComp->diffuse.a);
        shader.SetUniform4f("light.specular",
            lightComp->specular.r * lightComp->intensity,
            lightComp->specular.g * lightComp->intensity,
            lightComp->specular.b * lightComp->intensity,
            lightComp->specular.a);

        // ========== 应用阴影数据 ==========
        auto shadowComp = lightEntity->GetComponent<ShadowComponent>();
        if (shadowComp && shadowComp->IsEnabled()) {
            LOG_INFO("\t\t\tApplying shadow data from light '{}'", lightEntity->GetName());
            
            // 绑定阴影贴图到纹理单元1
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowComp->GetShadowMapTexture());

            // 传递阴影相关 uniform 数据
            shader.SetUniform1i("shadowMap", 1);
            shader.SetUniformMat4fv("lightSpaceMatrix", shadowComp->GetLightSpaceMatrix());
            shader.SetUniform1f("shadowBias", shadowComp->GetBias());

            // 🆕 传递 PCF 控制参数
            shader.SetUniform1i("enablePCF", shadowComp->IsPCFEnabled() ? 1 : 0);
            shader.SetUniform1i("pcfSamples", shadowComp->GetPCFSamples());
            shader.SetUniform1f("pcfRadius", shadowComp->GetPCFRadius());

            LOG_INFO("\t\t\tShadow uniforms applied - Texture:{}, PCF:{}, Samples:{}, Radius:{}, Bias:{}",
                shadowComp->GetShadowMapTexture(), shadowComp->IsPCFEnabled(), 
                shadowComp->GetPCFSamples(), shadowComp->GetPCFRadius(), shadowComp->GetBias());
        }
        else {
            LOG_WARNING("\t\t\tNo shadow component found on light '{}'", lightEntity->GetName());
            
            // 🆕 如果没有阴影，传递默认值避免着色器错误
            shader.SetUniform1i("enablePCF", 0);
            shader.SetUniform1f("shadowBias", 0.0f);
        }
    }
}

void BaseScene::RenderAllEntities(const Renderer& renderer, const glm::mat4& view, const glm::mat4& projection) {
    LOG_DEBUG("\tRendering {} entities", m_Entities.size());

    for (auto& entity : m_Entities) {
        auto renderComp = entity->GetComponent<RenderComponent>();
        auto transform = entity->GetTransform();

        if (!renderComp || !transform) continue;

        auto shader = renderComp->GetShader();
        auto geometry = renderComp->GetGeometry();

        if (!shader || !geometry || shader->GetID() == 0) {
            LOG_ERROR("\t\tEntity '{}' has invalid render components", entity->GetName());
            continue;
        }

        shader->Bind();
        LOG_DEBUG("\t\tRendering entity '{}' with shader ID {}", entity->GetName(), shader->GetID());

        // 检查是否为光源实体，设置Unlit标志
        bool isLightSource = entity->GetComponent<LightComponent>() != nullptr;
   
        // 应用全局光照
        ApplyGlobalLightToShader(*renderComp);

        // 只有非光源实体才应用光源数据
        if (!isLightSource) {
            ApplyIndexedLightsToShader(*shader, view);
        }

        shader->SetUniform1i("isLightSource", isLightSource ? 1 : 0);
        LOG_INFO("Setting isLightSource={} for entity '{}'", isLightSource, entity->GetName());
        // 执行渲染
        renderComp->Render(renderer, projection, view, transform->GetMatrix());
        LOG_DEBUG("\t\tEntity '{}' rendered successfully", entity->GetName());
    }
    LOG_DEBUG("\tAll entities rendered");
}

void BaseScene::BuildLightIndex() {
    m_lightIndex.clear();
    for (auto& entity : m_Entities) {
        auto lightComp = entity->GetComponent<LightComponent>();
        if (lightComp) {
            m_lightIndex[entity->GetName()] = lightComp;
            LOG_DEBUG("Indexed light: {}", entity->GetName());
        }
    }
}

void BaseScene::SetGlobalLight() {
    // 设置全局环境光
    m_globalLight.SetAmbient(glm::vec3(0.2f, 0.2f, 0.2f));
}

void BaseScene::ApplyGlobalLightToAllShaders() {
    for (auto& entity : m_Entities) {
        auto renderComp = entity->GetComponent<RenderComponent>();
        if (renderComp) {
            auto shader = renderComp->GetShader();
            if (shader) {
                m_globalLight.ApplyToShader(*shader);
            }
        }
    }
}

void BaseScene::InlitializeEntities(EnityInitializer initializer) {
    enityInitializer = initializer;
    enityInitializer(this);
}

void BaseScene::UpdateAllEntity(float deltaTime) {
    for (auto& entity : m_Entities) {
        entity->UpdateAllComponent(deltaTime);
    }
}

void BaseScene::UpdateDynamicLights(float deltaTime) {
    //float currentTime = static_cast<float>(glfwGetTime());
    //currentLightPos = glm::vec3(
    //    initialLightLoc.x + sin(currentTime * 0.8f) * 3.0f,
    //    initialLightLoc.y + cos(currentTime * 0.6f) * 2.0f,
    //    initialLightLoc.z
    //);
    //// 更新所有光源组件的位置
    //for (auto& entity : m_Entities) {
    //    auto lightComp = entity->GetComponent<LightComponent>();
    //    if (lightComp) {
    //        entity->GetTransform()->SetPosition(currentLightPos);
    //    }
    //}
}

void BaseScene::ApplyGlobalLightToShader(RenderComponent& renderComp) {
    auto shader = renderComp.GetShader();
    if(!shader) {
        LOG_ERROR("RenderComponent has no shader to apply global light.");
        return;
    }
    m_globalLight.ApplyToShader(*shader);
}


void BaseScene::InitializeShadowSystem() {
    LOG_INFO("BaseScene: Initializing shadow system...");

    int shadowLightCount = 0;
    for (auto& entity : m_Entities) {
		// ShadowComponent 默认应只在光源实体里面即一个Enitity要么同时有LightComponent和ShadowComponent，要么仅有LightComponent
        auto shadowComp = entity->GetComponent<ShadowComponent>();
        if (shadowComp && shadowComp->IsEnabled()) {
            LOG_INFO("\tFound shadow light: {}", entity->GetName());
            shadowLightCount++;
        }
        else {
			LOG_WARNING("\tEntity '{}' has no shadow component or is disabled", entity->GetName());
        }
    }

    LOG_INFO("\tShadow system initialized with {} shadow lights", shadowLightCount);
}

void BaseScene::RenderShadowPass(const Renderer& renderer) {
    // 寻找启用阴影的光源
    for (auto& entity : m_Entities) {
        auto shadowComp = entity->GetComponent<ShadowComponent>();
        if (!shadowComp || !shadowComp->IsEnabled()) {
            continue; // 跳过没有阴影组件或未启用的实体
        }

        // 检查是否有关联的光源组件
        auto lightComp = entity->GetComponent<LightComponent>();
        if (!lightComp) {
            LOG_WARNING("ShadowComponent found on entity '{}' without LightComponent. Skipping.", entity->GetName());
            continue;
        }

        LOG_DEBUG("BaseScene: Rendering shadow pass for light '{}'", entity->GetName());

        // 1. 开始阴影Pass
        shadowComp->BeginShadowPass();

        // 2. ShadowComponent 渲染阴影贴图
        shadowComp->RenderShadowCasters(renderer, m_Entities);

        // 3. 结束阴影Pass
        shadowComp->EndShadowPass();

        LOG_DEBUG("BaseScene: Shadow pass complete for light '{}'", entity->GetName());
        break; // 目前只处理一个阴影光源
    }
}
