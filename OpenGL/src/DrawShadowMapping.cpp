#include "Globals.h"
#include <memory>
#include <utility>

#include "Logger.h"

#include "Entity.h"
#include "Scene.h" 
#include "LightComponent.h"
#include "RenderComponent.h"    
#include "RotationAnimation.h"  
#include "ShadowComponent.h"
#include "Camera.h"             
#include "GlobalLight.h"        
#include "Renderer.h"           
#include "BaseScene.h"
#include "SceneManager.h"
#include "Engine.h"

void enityInitializer_func(Scene* scene) {
    // 创建Torus实体
    auto E_rightToruhs = scene->CreateEntity("rightTorus");
    {
        auto renderComp = E_rightToruhs->AddComponent<RenderComponent>(
            global_torusPtr,
            shadowMappingRenderShaderPtr
        );
        E_rightToruhs->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::GOLD);
        E_rightToruhs->GetTransform()->SetPosition(3.0f, 0.0f, -5.0f);
        E_rightToruhs->GetTransform()->SetScale(1.0f, 1.0f, 1.0f);
    }

    // 创建Sphere实体
    auto E_leftSphere = scene->CreateEntity("leftSphere");
    {
        auto renderComp2 = E_leftSphere->AddComponent<RenderComponent>(
            global_spherePtr,
            shadowMappingRenderShaderPtr
        );
        E_leftSphere->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::PLASTIC_GREEN);
        E_leftSphere->GetTransform()->SetPosition(-3.0f, 0.0f, 0.0f);
        E_leftSphere->GetTransform()->SetScale(1.0f, 1.0f, 1.0f);
    }

    // 🔧 简化地面设置
    auto E_ground = scene->CreateEntity("ground");
    {
        auto renderComp = E_ground->AddComponent<RenderComponent>(
            global_cubePtr,
            shadowMappingRenderShaderPtr
        );
        E_ground->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::GRANITE);

        // 🔧 修复地面位置 - 更合理的布局
        E_ground->GetTransform()->SetPosition(0.0f, -3.5f, -1.0f);   // 地面在物体下方
        E_ground->GetTransform()->SetScale(10.0f, 0.1f, 10.0f);      // 适中的地面
    }

    // 🔧 光源实体
    auto E_light = scene->CreateEntity("light");
    {
        auto lightRenderComp = E_light->AddComponent<RenderComponent>(
            global_spherePtr,
            shadowMappingRenderShaderPtr
        );
        
        auto lightMaterial = E_light->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::CUSTOM);
        lightMaterial->SetAmbient(glm::vec4(1.0f, 1.0f, 0.6f, 1.0f));
        lightMaterial->SetDiffuse(glm::vec4(1.0f, 1.0f, 0.6f, 1.0f));
        lightMaterial->SetSpecular(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));  // 关掉镜面反射
        lightMaterial->SetShininess(1.0f);

        auto lightComp = E_light->AddComponent<LightComponent>();
        lightComp->SetLightType(LightComponent::LightType::POINT);
        lightComp->SetAmbient(glm::vec4(0.1f, 0.1f, 0.1f, 1.0f));      // 🔧 提高环境光
        lightComp->SetDiffuse(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));      // 🔧 标准白光
        lightComp->SetSpecular(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
        lightComp->SetIntensity(1.0f);                                  // 🔧 标准强度

        E_light->GetTransform()->SetPosition({ 2.0f, 3.0f, 1.0f });     // 🔧 调整初始位置
        E_light->GetTransform()->SetScale(0.15f, 0.15f, 0.15f);

        // 🔧 重新配置阴影设置
        auto shadowComp = E_light->AddComponent<ShadowComponent>();
        shadowComp->SetShadowMapQuality(ShadowComponent::Quality::HIGH);
        shadowComp->SetBias(0.03f);                                    // 🔧 使用很小的偏移
        shadowComp->SetShadowQuality(ShadowComponent::ShadowQuality::SOFT_ULTRA);
    }
}




void DrawShadowMappingWithECS(GLFWwindow* window) {

    InitializeGlobalShaders();
    InitializeGlobalObjects(); 

    //创建引擎
    Engine engine;

    //创建并设置场景
    auto scene = std::make_unique<BaseScene>();
    scene->SetEntityInitializer(enityInitializer_func);

    engine.SetMouseHandler([&](double mouseX, double mouseY) {
        auto activeScene = dynamic_cast<BaseScene*>(engine.sceneManager.GetActiveScene());
        if (!activeScene) return;

        // 获取窗口尺寸
        int windowWidth = engine.GetWindowWidth();
        int windowHeight = engine.GetWindowHeight();

        // 将鼠标坐标转换为归一化坐标[-1, 1]
        float nx = (float)mouseX / (float)windowWidth * 2.0f - 1.0f;
        float ny = 1.0f - (float)mouseY / (float)windowHeight * 2.0f;

        // 映射到世界坐标
        glm::vec3 lightPosition;
        lightPosition.x = nx * 8.0f;   // 水平范围 [-8, 8]
        lightPosition.y = ny * 6.0f;   // 垂直范围 [-6, 6]
        lightPosition.z = 2.0f;        // 固定Z位置

        const auto& entities = activeScene->GetAllEntities();
        for (auto& entity : entities) {
            auto lightComp = entity->GetComponent<LightComponent>();
            if (lightComp) {
                entity->GetTransform()->SetPosition(lightPosition);
                LOG_DEBUG("Engine: Light '{}' position updated to ({:.2f}, {:.2f}, {:.2f})",
                    entity->GetName(), lightPosition.x, lightPosition.y, lightPosition.z);
                break; // 只更新第一个光源
            }
        }
    });


    // 设置键盘处理
    engine.SetKeyboardHandler([&](int key, int action) {
        switch (key) {
        case GLFW_KEY_G:
            glfwSetWindowTitle(window, "全部使用 Gouraud 着色（ECS版本）");
            // TODO: 实现切换着色器逻辑
            break;
        case GLFW_KEY_P:
            glfwSetWindowTitle(window, "全部使用 Phong 着色（ECS版本）");
            // TODO: 实现切换着色器逻辑
            break;
        case GLFW_KEY_C:
            glfwSetWindowTitle(window, "对比模式（ECS版本）");
            // TODO: 恢复原始着色器
            break;
        }
        });

	engine.SetScene(std::move(scene));
    engine.Run(window);
}
