#include "ShadowComponent.h"
#include "Logger.h"
#include "LightComponent.h"
#include "Entity.h"
#include "TransformComponent.h"
#include "Globals.h"
#include "RenderComponent.h"
#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

// ============================= 构造函数和析构函数 =============================

ShadowComponent::ShadowComponent(ShadowType type)
    : m_shadowType(type) {
    LOG_INFO("ShadowComponent: Creating shadow component with type {}",
        type == ShadowType::SHADOW_MAP ? "SHADOW_MAP" : "NONE");

    if (type == ShadowType::SHADOW_MAP) {
        InitializeShadowMap();
    }
}

ShadowComponent::~ShadowComponent() {
    LOG_INFO("ShadowComponent: Destroying shadow component");

    // 清理 OpenGL 资源
    if (m_shadowFramebuffer != 0) {
        GLCall(glDeleteFramebuffers(1, &m_shadowFramebuffer));
        LOG_DEBUG("ShadowComponent: Deleted framebuffer {}", m_shadowFramebuffer);
    }
    if (m_shadowMapTexture != 0) {
        GLCall(glDeleteTextures(1, &m_shadowMapTexture));
        LOG_DEBUG("ShadowComponent: Deleted shadow map texture {}", m_shadowMapTexture);
    }
}

// ============================= 配置方法 =============================

void ShadowComponent::SetShadowMapSize(int width, int height) {
    m_shadowMapWidth = width;
    m_shadowMapHeight = height;
    
    // 如果已经初始化过，需要重新创建
    if (m_shadowFramebuffer != 0) {
        // 清理旧资源
        glDeleteFramebuffers(1, &m_shadowFramebuffer);
        glDeleteTextures(1, &m_shadowMapTexture);

        // 重新初始化
        InitializeShadowMap();
    }
}

void ShadowComponent::SetShadowMapQuality(Quality quality) {
    switch (quality) {
        case Quality::LOW:
            SetShadowMapSize(512, 512);
            break;
        case Quality::MEDIUM:
            SetShadowMapSize(1024, 1024);
            break;
        case Quality::HIGH:
            SetShadowMapSize(2048, 2048);
            break;
        case Quality::Ultra:
            SetShadowMapSize(4096, 4096);
            break;
    }
}

// ============================= 渲染流程方法 =============================

void ShadowComponent::BeginShadowPass() {
    if (!IsEnabled()) {
        LOG_WARNING("ShadowComponent: Shadow pass called but component is disabled");
        return;
    }

    LOG_DEBUG("ShadowComponent: Beginning shadow pass");

    // Step1:切换到阴影帧缓冲区
    glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFramebuffer);

    // Step2: 设置阴影贴图的视口
    glViewport(0, 0, m_shadowMapWidth, m_shadowMapHeight);

    // Step3: 清空深度缓冲区（准备记录新的深度信息）
    glClear(GL_DEPTH_BUFFER_BIT);

    // Step4: 禁用颜色写入，只写入深度
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    // 斜率偏移，减少阴影痤疮而无需巨大的bias
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.0f, 4.0f);

    LOG_TRACE("ShadowComponent: Shadow framebuffer {} bound, viewport set to {}x{}",
        m_shadowFramebuffer, m_shadowMapWidth, m_shadowMapHeight);
}

void ShadowComponent::EndShadowPass() {
    if (!IsEnabled()) return;

    LOG_DEBUG("ShadowComponent: Ending shadow pass");

    glDisable(GL_POLYGON_OFFSET_FILL);

    // Step1: 恢复颜色写入
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // Step2: 切换回默认帧缓冲区（屏幕）
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Step3: 恢复主视口尺寸
    glViewport(0, 0, g_WindowWidth, g_WindowHeight);

    LOG_TRACE("ShadowComponent: Switched back to default framebuffer, viewport restored to {}x{}",
        g_WindowWidth, g_WindowHeight);
}

// ============================= 生命周期方法 =============================

void ShadowComponent::Update(float deltaTime) {
    if (!IsEnabled()) return;

    // 每帧重新计算光源空间矩阵（因为光源可能在移动）
    CalculateLightSpaceMatrix();
	LOG_INFO("ShadowComponent: Updated light space matrix");
}

void ShadowComponent::RenderShadowCasters(const Renderer& renderer,
    const std::vector<std::unique_ptr<Entity>>& entities) {
    if (!IsEnabled()) return;
    auto shadowShader = GetShadowShader();
    if (!shadowShader) return;

    glm::mat4 lightSpaceMatrix = GetLightSpaceMatrix();

    for (const auto& e : entities) {
        // 跳过光源自己（有 LightComponent 且就是我的 owner）
        if (e.get() == GetOwner() || e->GetComponent<LightComponent>()) continue;

        auto rc = e->GetComponent<RenderComponent>();
        auto tf = e->GetTransform();
        if (!rc || !tf || !rc->GetGeometry()) continue;

        shadowShader->Bind();
        shadowShader->SetUniformMat4fv("lightSpaceMatrix", lightSpaceMatrix);
        shadowShader->SetUniformMat4fv("model", tf->GetMatrix());
        renderer.Draw(*rc->GetGeometry(), *shadowShader);
    }
}

void ShadowComponent::SetShadowQuality(ShadowQuality quality)
{
    switch (quality) {
    case ShadowQuality::HARD_SHADOW:
        m_enablePCF = false;
        m_pcfSamples = 1;
        m_pcfRadius = 1.0f;
        break;
    case ShadowQuality::SOFT_LOW:
        m_enablePCF = true;
        m_pcfSamples = 4;
        m_pcfRadius = 1.0f;
        break;
    case ShadowQuality::SOFT_MEDIUM:
        m_enablePCF = true;
        m_pcfSamples = 9;
        m_pcfRadius = 1.5f;
        break;
    case ShadowQuality::SOFT_HIGH:
        m_enablePCF = true;
        m_pcfSamples = 16;
        m_pcfRadius = 2.0f;
        break;
    case ShadowQuality::SOFT_ULTRA:
        m_enablePCF = true;
        m_pcfSamples = 25;
        m_pcfRadius = 2.5f;
        break;
    }
}

// ============================= 私有辅助方法 =============================

void ShadowComponent::InitializeShadowMap() {
    LOG_INFO("ShadowComponent: Initializing shadow map resources ({}x{})",
        m_shadowMapWidth, m_shadowMapHeight);

    // Step1: 创建帧缓冲区对象
    GLCall(glGenFramebuffers(1, &m_shadowFramebuffer));
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFramebuffer));

    // Step2: 创建深度纹理（用于存储阴影贴图）
    GLCall(glGenTextures(1, &m_shadowMapTexture));
    GLCall(glBindTexture(GL_TEXTURE_2D, m_shadowMapTexture));

    // Step3: 配置深度纹理参数
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        m_shadowMapWidth, m_shadowMapHeight, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, NULL));

    // Step4: 设置纹理过滤参数
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    // Step5: 设置纹理包装模式（防止边界采样问题）
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));

    // Step6: 设置边界颜色为白色（边界外认为没有阴影）
    float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLCall(glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor));

    // Step7: 将深度纹理附加到帧缓冲区的深度附件上
    GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, m_shadowMapTexture, 0));

    // Step8: 禁用颜色缓冲区（只需要深度信息）
    GLCall(glDrawBuffer(GL_NONE));
    GLCall(glReadBuffer(GL_NONE));

    // Step9: 检查帧缓冲区完整性
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOG_ERROR("ShadowComponent: Shadow framebuffer is not complete!");
    }

    // Step10: 切换回默认帧缓冲区
    GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    LOG_INFO("ShadowComponent: Shadow map initialized - FB:{}, Texture:{}",
        m_shadowFramebuffer, m_shadowMapTexture);
}

LightComponent* ShadowComponent::GetAssociatedLight() const
{
    auto owner = GetOwner();
    if (!owner) {
        LOG_ERROR("ShadowComponent: No owner entity found");
        return nullptr;
    }
    // 假设光源组件和阴影组件在同一个实体上
    auto lightComp = owner->GetComponent<LightComponent>();
    if (!lightComp) {
        LOG_ERROR("ShadowComponent: No associated LightComponent found on entity '{}'", owner->GetName());
        return nullptr;
    }
    return lightComp;
}

void ShadowComponent::CalculateLightSpaceMatrix() {
    auto lightComp = GetAssociatedLight();
    auto transform = GetOwner()->GetTransform();

    if (!lightComp || !transform) {
        LOG_ERROR("ShadowComponent: Cannot calculate light space matrix");
        return;
    }

    glm::vec3 lightPos = transform->GetPosition();

    if (lightComp->GetLightType() == LightComponent::LightType::POINT) {
        // 🔧 修复：使用合理的投影参数
        glm::mat4 lightProjection = glm::perspective(
            glm::radians(160.0f),    // 🔧 90度FOV，标准点光源
            1.0f,                   // 正方形纵横比
            1.0f,                   // 🔧 合理的近平面
            20.0f                   // 🔧 合理的远平面
        );

        // 🔧 修复：计算正确的场景中心
        glm::vec3 sceneCenter = glm::vec3(0.0f, -1.25f, -1.0f);  // 基于物体和地面的实际位置
        
        glm::mat4 lightView = glm::lookAt(
            lightPos,                           // 光源位置
            sceneCenter,                        // 场景中心
            glm::vec3(0.0f, 1.0f, 0.0f)        // 上方向
        );

        m_lightSpaceMatrix = lightProjection * lightView;
        
        LOG_INFO("ShadowComponent: Light space matrix - Light:({:.2f},{:.2f},{:.2f}) → Target:({:.2f},{:.2f},{:.2f})",
            lightPos.x, lightPos.y, lightPos.z, sceneCenter.x, sceneCenter.y, sceneCenter.z);
    }
}

std::shared_ptr<Shader> ShadowComponent::GetShadowShader() {
    if (!m_shadowShader) {
        m_shadowShader = CreateShadowShader();
    }
    return m_shadowShader;
}

std::shared_ptr<Shader> ShadowComponent::CreateShadowShader() {
    try {
        auto shader = std::make_shared<Shader>("res/shaders/ShadowShader/ShadowPass.shader");
        LOG_INFO("ShadowComponent: Shadow shader created successfully");
        return shader;
    }
    catch (const std::exception& e) {
        LOG_ERROR("ShadowComponent: Failed to create shadow shader: {}", e.what());
        return nullptr;
    }
}

void ShadowComponent::ApplyToShader(Shader& shader) {

}

