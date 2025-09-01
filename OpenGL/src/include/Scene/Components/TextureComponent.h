#pragma once
#include <memory>
#include "Component.h"
#include "Texture.h"
#include "Shader.h"
#include "Globals.h"

class TextureComponent : public Component {
public:
    // 贴图设置
    TextureComponent& SetAlbedoTexture(std::shared_ptr<Texture> tex) {
        m_Albedo = std::move(tex);
        m_UseAlbedo = (m_Albedo != nullptr);
        return *this;
    }
    TextureComponent& SetNormalTexture(std::shared_ptr<Texture> tex) {
        m_Normal = std::move(tex);
        m_UseNormal = (m_Normal != nullptr);
        return *this;
    }
    TextureComponent& SetEnvCubeTexture(std::shared_ptr<Texture> tex) {
        m_EnvCube = std::move(tex);
        return *this;
    }

    // 额外控制
    TextureComponent& SetNormalScale(float s) { m_NormalScale = s; return *this; }
    TextureComponent& EnableAlbedo(bool enabled) { m_UseAlbedo = enabled && (m_Albedo != nullptr); return *this; }
    TextureComponent& EnableNormal(bool enabled) { m_UseNormal = enabled && (m_Normal != nullptr); return *this; }
    // 新增：由几何/VAO 配置完成后，告知是否已提供切线属性(layout=3)
    TextureComponent& SetHasTangents(bool has) { m_HasTangents = has; return *this; }

    // 渲染阶段统一接口
    void ApplyToShader(Shader& shader) override {
        // Albedo
        if (m_Albedo) {
            m_Albedo->BindToUnit(static_cast<unsigned>(TextureSlots::Albedo));
            shader.SetUniform1i("albedoMap", static_cast<int>(TextureSlots::Albedo));
            shader.SetUniform1i("useAlbedoMap", m_UseAlbedo ? 1 : 0);
        } else {
            shader.SetUniform1i("useAlbedoMap", 0);
        }

        // Normal（必须具备切线）
        const bool effectiveNormal = m_UseNormal && (m_Normal != nullptr) && m_HasTangents;
        if (m_Normal) {
            m_Normal->BindToUnit(static_cast<unsigned>(TextureSlots::Normal));
            shader.SetUniform1i("normalMap", static_cast<int>(TextureSlots::Normal));
            shader.SetUniform1i("useNormalMap", effectiveNormal ? 1 : 0);
            shader.SetUniform1f("normalScale", m_NormalScale);
        } else {
            shader.SetUniform1i("useNormalMap", 0);
            shader.SetUniform1f("normalScale", m_NormalScale);
        }

        // Env cube（环境贴图着色器使用 EnvironmentTex 或 u_EnvCube，按你的shader名设置）
        if (m_EnvCube) {
            m_EnvCube->BindToUnit(static_cast<unsigned>(TextureSlots::EnvCube));
            shader.SetUniform1i("EnvironmentTex", static_cast<int>(TextureSlots::EnvCube));
        }
    }

private:
    std::shared_ptr<Texture> m_Albedo;
    std::shared_ptr<Texture> m_Normal;
    std::shared_ptr<Texture> m_EnvCube;

    bool  m_UseAlbedo   = false;
    bool  m_UseNormal   = false;
    bool  m_HasTangents = false;  // 新增：是否已为当前网格启用layout(3)切线
    float m_NormalScale = 1.0f;
};