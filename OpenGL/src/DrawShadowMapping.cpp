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

    Engine engine;
    auto scene = std::make_unique<BaseScene>();
    scene->SetEntityInitializer(enityInitializer_func);

    // 控制模式状态
    enum class ControlMode {
        LIGHT_CONTROL,
        CAMERA_CONTROL
    };

    ControlMode currentMode = ControlMode::LIGHT_CONTROL;

    // 相机状态管理
    struct CameraState {
        bool leftMousePressed = false;
        bool cameraInitialized = false;
        double lastUpdateTime = 0.0;
    } cameraState;

    // 设置鼠标按键回调
    glfwSetMouseButtonCallback(window, [](GLFWwindow* window, int button, int action, int mods) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            auto* state = static_cast<CameraState*>(glfwGetWindowUserPointer(window));
            if (action == GLFW_PRESS) {
                state->leftMousePressed = true;
            }
            else if (action == GLFW_RELEASE) {
                state->leftMousePressed = false;
                state->cameraInitialized = false;
            }
        }
        });

    glfwSetWindowUserPointer(window, &cameraState);

    // 🔧 修改鼠标处理器：光源在相机平面移动
    engine.SetMouseHandler([&](double mouseX, double mouseY) {
        auto activeScene = dynamic_cast<BaseScene*>(engine.sceneManager.GetActiveScene());
        if (!activeScene) return;

        // 添加频率限制
        double currentTime = glfwGetTime();
        if (currentTime - cameraState.lastUpdateTime < 0.016) {
            return;
        }
        cameraState.lastUpdateTime = currentTime;

        if (currentMode == ControlMode::CAMERA_CONTROL && cameraState.leftMousePressed) {
            // 相机控制模式
            auto& camera = activeScene->GetCamera();

            if (!cameraState.cameraInitialized) {
                camera.StartMouseLook(mouseX, mouseY);
                cameraState.cameraInitialized = true;
            }
            else {
                camera.UpdateMouseLook(mouseX, mouseY);
            }
        }
        else if (currentMode == ControlMode::LIGHT_CONTROL) {
            // 🔧 光源控制模式：在相机观察平面上移动
            auto& camera = activeScene->GetCamera();

            int windowWidth = engine.GetWindowWidth();
            int windowHeight = engine.GetWindowHeight();

            // 归一化鼠标坐标 [-1, 1]
            float nx = (float)mouseX / (float)windowWidth * 2.0f - 1.0f;
            float ny = 1.0f - (float)mouseY / (float)windowHeight * 2.0f;

            // 🆕 计算相机的局部坐标系
            glm::vec3 forward = glm::normalize(camera.target - camera.position);  // 相机前向量
            glm::vec3 right = glm::normalize(glm::cross(forward, camera.up));     // 相机右向量
            glm::vec3 up = glm::normalize(glm::cross(right, forward));            // 相机上向量

            // 🆕 定义光源移动平面的参数
            float lightPlaneDistance = 5.0f;  // 光源平面距离相机的距离
            float planeWidth = 10.0f;          // 平面宽度范围
            float planeHeight = 8.0f;          // 平面高度范围

            // 🆕 计算光源在相机平面上的位置
            glm::vec3 lightPosition = camera.position                    // 从相机位置开始
                + forward * lightPlaneDistance                           // 向前移动到光源平面
                + right * (nx * planeWidth * 0.5f)                      // 在平面内水平移动
                + up * (ny * planeHeight * 0.5f);                       // 在平面内垂直移动

            // 更新光源实体的位置
            const auto& entities = activeScene->GetAllEntities();
            for (auto& entity : entities) {
                auto lightComp = entity->GetComponent<LightComponent>();
                if (lightComp) {
                    entity->GetTransform()->SetPosition(lightPosition);
                    LOG_DEBUG("Light position updated to ({:.2f}, {:.2f}, {:.2f}) in camera plane",
                        lightPosition.x, lightPosition.y, lightPosition.z);
                    break;
                }
            }
        }
        });

    // 键盘处理：使用L和C键
    engine.SetKeyboardHandler([&](int key, int action) {
        if (action != GLFW_PRESS) return;

        auto activeScene = dynamic_cast<BaseScene*>(engine.sceneManager.GetActiveScene());
        if (!activeScene) return;

        switch (key) {
        case GLFW_KEY_L:
            // L键：光源控制模式
            currentMode = ControlMode::LIGHT_CONTROL;
            activeScene->GetCamera().StopMouseLook();
            cameraState.cameraInitialized = false;
            glfwSetWindowTitle(window, "光源控制模式 | 移动鼠标在相机平面控制光源位置 | 按C切换相机控制");
            LOG_INFO("Switched to LIGHT_CONTROL mode");
            break;

        case GLFW_KEY_C:
            // C键：相机控制模式
            currentMode = ControlMode::CAMERA_CONTROL;
            cameraState.cameraInitialized = false;
            glfwSetWindowTitle(window, "相机控制模式 | 按住左键拖拽改变视角 | 按L切换光源控制");
            LOG_INFO("Switched to CAMERA_CONTROL mode");
            break;

        case GLFW_KEY_R:
            // R键：重置相机（仅在相机模式下有效）
            if (currentMode == ControlMode::CAMERA_CONTROL) {
                activeScene->GetCamera().ResetToOriginal();
                cameraState.cameraInitialized = false;
                glfwSetWindowTitle(window, "相机已重置 | 相机控制模式");
                LOG_INFO("Camera reset to original position");
            }
            break;
        }
        });

    // 设置初始状态
    glfwSetWindowTitle(window, "阴影映射演示 | L键控制光源 | C键控制相机 | 当前：光源控制模式");

    engine.SetScene(std::move(scene));
    engine.Run(window);
}