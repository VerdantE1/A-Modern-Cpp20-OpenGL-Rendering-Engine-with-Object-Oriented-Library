#pragma once
#include "Component.h"
#include "Shaper.h"
#include "Shader.h"
#include "Renderer.h"
#include "MaterialComponent.h"
#include "ShaderDataComponent.h"
#include <memory>
#include <iostream>
#include "Logger.h"

class RenderComponent : public Component {
public:
    RenderComponent() = default;
    
    RenderComponent(std::shared_ptr<Shaper> geometry, 
                   std::shared_ptr<Shader> shader)
        : m_Geometry(std::move(geometry)), m_Shader(std::move(shader)) {}

    void Render(const Renderer& renderer, 
               const glm::mat4& projectionMatrix, 
               const glm::mat4& viewMatrix, 
               const glm::mat4& modelMatrix) {
        auto* owner = GetOwner();
        const std::string ownerName = owner ? owner->GetName() : "Unknown";

        if (!m_Geometry) {
            LOG_FAILED("RenderComponent('{}'): No geometry bound, cannot render.", ownerName);
            return;
        }
        if (!m_Shader) {
            LOG_FAILED("RenderComponent('{}'): No shader bound, cannot render.", ownerName);
            return;
        }
        
        LOG_CRITICAL("RenderComponent: Starting render process for Entity '{}'", ownerName);
        
        // 先绑定 Shader
        LOG_INFO("\t\tRenderComponent('{}'): Binding shader ID {}", ownerName, m_Shader->GetID());
        m_Shader->Bind();
        LOG_INFO("\t\tRenderComponent('{}'): Shader ID {} bound successfully.", ownerName, m_Shader->GetID());
        
        // 1. 基础矩阵
        const glm::mat4 mv = viewMatrix * modelMatrix;
        m_Shader->SetUniformMat4fv("proj_matrix", projectionMatrix);
        m_Shader->SetUniformMat4fv("mv_matrix", mv);
        m_Shader->SetUniformMat4fv("model", modelMatrix);

        if (m_NeedsNormalMatrix) {
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(mv));
            m_Shader->SetUniformMat4fv("norm_matrix", normalMatrix);
            LOG_INFO("\t\tRenderComponent('{}'): Normal matrix set.", ownerName);
        } else {
            LOG_WARNING("RenderComponent('{}'): Normal matrix not needed, skipping.", ownerName);
        }

        // 2. 应用所有组件（TextureComponent/MaterialComponent/ShadowComponent等）
        ApplyAllComponentsToShader(*m_Shader);
        LOG_INFO("RenderComponent('{}'): Finished all component uniforms to shader.", ownerName);

        // 3. 执行渲染（纹理绑定由 TextureComponent 完成）
        renderer.Draw(*m_Geometry, *m_Shader);
        LOG_INFO("RenderComponent('{}'): Finished rendering.", ownerName);
    }

    // 其他成员保持不变...
    void SetGeometry(std::shared_ptr<Shaper> geometry) { m_Geometry = std::move(geometry); }
    void SetShader(std::shared_ptr<Shader> shader) { m_Shader = std::move(shader); }
    
    std::shared_ptr<Shaper> GetGeometry() const { return m_Geometry; }
    std::shared_ptr<Shader> GetShader() const { return m_Shader; }
    
    void SetNeedsNormalMatrix(bool needs) { m_NeedsNormalMatrix = needs; }
    bool GetNeedsNormalMatrix() const { return m_NeedsNormalMatrix; }

private:
    std::shared_ptr<Shaper> m_Geometry;
    std::shared_ptr<Shader> m_Shader;  // 共享资源
    bool m_NeedsNormalMatrix = true;

private:
    // 应用实体所有组件的 uniform 数据
    void ApplyAllComponentsToShader(Shader& shader) {
        auto owner = GetOwner();
        if (!owner) {
            LOG_ERROR("RenderComponent: No owner entity found, cannot apply components to shader.");
            return;
        }
        for (auto& [type, component] : owner->GetAllComponents()) {
            component->ApplyToShader(shader);
        }
    }
};