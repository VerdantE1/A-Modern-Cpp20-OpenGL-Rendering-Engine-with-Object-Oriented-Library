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
#include "TextureComponent.h"

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

    // 创建环境贴图Toruhs
    auto E_envTorus = scene->CreateEntity("envTorus");
    {
		auto shaderForEnv = envMappingShaderPtr ? envMappingShaderPtr : phongShaderPtr; // 如果环境贴图着色器不可用则回退到Phong
        auto TorhusPtr = std::make_shared<Torus>();

        auto renderComp = E_envTorus->AddComponent<RenderComponent>(
            TorhusPtr,
            shaderForEnv
		);

		renderComp->SetNeedsNormalMatrix(true); // 环境贴图需要法线矩阵

        static std::shared_ptr<Texture> s_envCubeMap = nullptr;
        if (!s_envCubeMap) {
            std::vector<std::string> faces = {
                "res/textures/skybox/right.jpg",
                "res/textures/skybox/left.jpg",
                "res/textures/skybox/top.jpg",
                "res/textures/skybox/bottom.jpg",
                "res/textures/skybox/front.jpg",
                "res/textures/skybox/back.jpg"
			};
        }
		// 只加载一次立方体贴图,工厂函数返回unique_ptr
        if(auto cubeUniq = Texture::CreateCubeMapFromSixImages(
            {
                "res/cubemap/Lycksele2/posx.jpg",
                "res/cubemap/Lycksele2/negx.jpg",
                "res/cubemap/Lycksele2/posy.jpg",
                "res/cubemap/Lycksele2/negy.jpg",
                "res/cubemap/Lycksele2/posz.jpg",
                "res/cubemap/Lycksele2/negz.jpg"
            },
            TextureFilterMode::LINEAR,
            TextureFilterMode::LINEAR,
            false
        )) {
            s_envCubeMap = std::move(cubeUniq);
        }
        else {
            LOG_ERROR("Failed to load environment cube map texture for envTorus.");
		}

        // 使用 TextureComponent 管理环境贴图
        auto texComp = E_envTorus->AddComponent<TextureComponent>();
        if (s_envCubeMap) {
            texComp->SetEnvCubeTexture(s_envCubeMap);
        }

        // 让它居中放置，便于对比
        E_envTorus->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::CUSTOM);
        E_envTorus->GetTransform()->SetPosition(3.0f, 3.0f, -5.0f);
        E_envTorus->GetTransform()->SetScale(1.0f, 1.0f, 1.0f);
    }

    auto E_BumpToruhs = scene->CreateEntity("BumpToruhs");
    {
        auto TorhusPtr = std::make_shared<Torus>();
		auto shader = std::make_shared<Shader>("res/shaders/ShadowShader/RenderPass.shader");
        auto renderComp = E_BumpToruhs->AddComponent<RenderComponent>(
            TorhusPtr,
            shader
        );
        E_BumpToruhs->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::GOLD);
        E_BumpToruhs->GetComponent<MaterialComponent>()->SetUseProceduralBump(true);
        E_BumpToruhs->GetTransform()->SetPosition(3.0f, -3.0f, -5.0f);
        E_BumpToruhs->GetTransform()->SetScale(1.0f, 1.0f, 1.0f);
    }

    auto E_MoonSphere = scene->CreateEntity("MoonSphere");
    {
        auto spherePtr = std::make_shared<Sphere>(36, true); // 开启切线
        auto shader = std::make_shared<Shader>("res/shaders/ShadowShader/RenderPass.shader");
        auto renderComp = E_MoonSphere->AddComponent<RenderComponent>(
            spherePtr,
            shader
        );
        renderComp->SetNeedsNormalMatrix(true);

        // 材质（基础高光等，漫反射会被贴图乘上）
        E_MoonSphere->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::CUSTOM);
        auto mat = E_MoonSphere->GetComponent<MaterialComponent>();
        mat->SetAmbient(glm::vec4(0.05f, 0.05f, 0.05f, 1.0f));
        mat->SetDiffuse(glm::vec4(1.0f));   // 作为乘子，保持1
        mat->SetSpecular(glm::vec4(0.1f));  // 月球基本无高光，可很低
        mat->SetShininess(8.0f);

        // 贴图只加载一次并固定到槽位
        static std::shared_ptr<Texture> s_moonAlbedo = nullptr;
        static std::shared_ptr<Texture> s_moonNormal = nullptr;
        if (!s_moonAlbedo) {
            s_moonAlbedo = std::make_shared<Texture>(
                "res/textures/2k_mercury.jpg",
                TextureFilterMode::LINEAR,
                TextureFilterMode::LINEAR_MIPMAP_LINEAR,
                TextureWrapMode::REPEAT,
                TextureWrapMode::REPEAT,
                true,   // mipmap
                true    // flip Y
            );
        }
        if (!s_moonNormal) {
            s_moonNormal = std::make_shared<Texture>(
                "res/textures/normalmap/normal_moon.png",
                TextureFilterMode::LINEAR,
                TextureFilterMode::LINEAR_MIPMAP_LINEAR,
                TextureWrapMode::REPEAT,
                TextureWrapMode::REPEAT,
                true,   // mipmap
                true    // flip Y
            );
        }

        // 使用 TextureComponent 统一应用
        auto texComp = E_MoonSphere->AddComponent<TextureComponent>();
        texComp->SetAlbedoTexture(s_moonAlbedo)
            .SetNormalTexture(s_moonNormal)
            .SetNormalScale(1.0f)
            .EnableAlbedo(true)
            .EnableNormal(true)
            .SetHasTangents(spherePtr->HasTangents()); // 根据几何自动开关

        E_MoonSphere->GetTransform()->SetPosition(-3.0f, -3.0f, -7.0f);
        E_MoonSphere->GetTransform()->SetScale(2.0f, 2.0f, 2.0f);
    }
    // 🔧 简化地面设置
    //auto E_ground = scene->CreateEntity("ground");
    //{
    //    auto renderComp = E_ground->AddComponent<RenderComponent>(
    //        global_cubePtr,
    //        shadowMappingRenderShaderPtr
    //    );
    //    E_ground->AddComponent<MaterialComponent>(MaterialComponent::MaterialType::GRANITE);

    //    // 🔧 修复地面位置 - 更合理的布局
    //    E_ground->GetTransform()->SetPosition(0.0f, -3.5f, -1.0f);   // 地面在物体下方
    //    E_ground->GetTransform()->SetScale(10.0f, 0.1f, 10.0f);      // 适中的地面
    //}

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

        // 重新配置阴影设置
        auto shadowComp = E_light->AddComponent<ShadowComponent>();
        shadowComp->SetShadowMapQuality(ShadowComponent::Quality::HIGH);
        shadowComp->SetBias(0.03f);                                    // 🔧 使用很小的偏移
        shadowComp->SetShadowQuality(ShadowComponent::ShadowQuality::SOFT_ULTRA);
    }
}


void DrawShadowMappingWithECS(GLFWwindow* window) {

    // 在 DrawShadowMappingWithECS 函数开头定义（lambda 外层）
    static float s_normalScale = 1.0f;

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

    // 键盘处理：使用L和C键 + WSAD相机移动
    engine.SetKeyboardHandler([&](int key, int action) {
        auto activeScene = dynamic_cast<BaseScene*>(engine.sceneManager.GetActiveScene());
        if (!activeScene) return;

        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

        if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_L:
                currentMode = ControlMode::LIGHT_CONTROL;
                activeScene->GetCamera().StopMouseLook();
                cameraState.cameraInitialized = false;
                glfwSetWindowTitle(window, "光源控制模式 | 移动鼠标在相机平面控制光源位置 | 按C切换相机控制");
                LOG_INFO("Switched to LIGHT_CONTROL mode");
                return;
            case GLFW_KEY_C:
                currentMode = ControlMode::CAMERA_CONTROL;
                cameraState.cameraInitialized = false;
                glfwSetWindowTitle(window, "相机控制模式 | 按住左键拖拽改变视角 | 按L切换光源控制");
                LOG_INFO("Switched to CAMERA_CONTROL mode");
                return;
            case GLFW_KEY_R:
                if (currentMode == ControlMode::CAMERA_CONTROL) {
                    activeScene->GetCamera().ResetToOriginal();
                    cameraState.cameraInitialized = false;
                    glfwSetWindowTitle(window, "相机已重置 | 相机控制模式");
                    LOG_INFO("Camera reset to original position");
                }
                return;
            default: break;
            }

            // 法线贴图调试热键（仅在按下时触发）
            auto& entities = activeScene->GetAllEntities();
            for (auto& entity : entities) {
                if (entity->GetName() == "MoonSphere") {
                    auto texComp = entity->GetComponent<TextureComponent>();
                    auto renderComp = entity->GetComponent<RenderComponent>();
                    auto shader = renderComp ? renderComp->GetShader() : nullptr;
                    
                    if (texComp && shader) {
                        switch (key) {
                        case GLFW_KEY_F1: // F1: 正常渲染
                            shader->Bind();
                            shader->SetUniform1i("u_DebugMode", 0);
                            LOG_INFO("Debug: Normal rendering mode");
                            break;
                        case GLFW_KEY_F2: // F2: 显示几何法线
                            shader->Bind();
                            shader->SetUniform1i("u_DebugMode", 1);
                            LOG_INFO("Debug: Showing geometric normals");
                            break;
                        case GLFW_KEY_F3: // F3: 显示法线贴图结果
                            shader->Bind();
                            shader->SetUniform1i("u_DebugMode", 2);
                            LOG_INFO("Debug: Showing normal mapped normals");
                            break;
                        case GLFW_KEY_N: // N: 开关法线贴图
                            {
                                static bool enabled = true;
                                enabled = !enabled;
                                texComp->EnableNormal(enabled);
                                LOG_INFO("Normal mapping: {}", enabled ? "ON" : "OFF");
                            }
                            break;
                        case GLFW_KEY_MINUS: // -: 减少法线强度
                            {
                                s_normalScale = std::max(0.0f, s_normalScale - 0.2f);
                                texComp->SetNormalScale(s_normalScale);
                                LOG_INFO("Normal scale: {}", s_normalScale);
                            }
                            break;
                        case GLFW_KEY_EQUAL: // =: 增加法线强度
                            {
                                s_normalScale += 0.2f;
                                texComp->SetNormalScale(s_normalScale);
                                LOG_INFO("Normal scale: {}", s_normalScale);
                            }
                            break;
                        }
                    }
                    break; // 只处理 MoonSphere
                }
            }
        }

        if (currentMode != ControlMode::CAMERA_CONTROL) return;

        // 用引擎每帧dt，避免首次按下产生超大步长
        float dt = Engine::GetInstance() ? Engine::GetInstance()->GetDeltaTime() : 1.0f / 60.0f;

        float baseSpeed = 4.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
            baseSpeed *= 2.0f;
        }
        float move = baseSpeed * dt;

        auto& cam = activeScene->GetCamera();
        glm::vec3 forward = glm::normalize(cam.target - cam.position);
        glm::vec3 right   = glm::normalize(glm::cross(forward, cam.up));

        auto applyDelta = [&](const glm::vec3& d) {
            cam.position += d;
            cam.target   += d;
        };

        switch (key) {
        case GLFW_KEY_W: applyDelta(forward * move); break;
        case GLFW_KEY_S: applyDelta(-forward * move); break;
        case GLFW_KEY_A: applyDelta(-right * move); break;
        case GLFW_KEY_D: applyDelta(right * move); break;
        default: break;
        }
        });

    // 设置初始状态
    // 修改最后的窗口标题：
        glfwSetWindowTitle(window, "阴影映射演示 | L光源/C相机 | F1正常/F2几何法线/F3贴图法线 | N开关法线/-+调强度");

    engine.SetScene(std::move(scene));
    engine.Run(window);
}