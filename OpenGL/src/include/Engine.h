#pragma once
#include <memory>
#include <functional>
#include "Globals.h"
#include "Scene.h" 
#include "SceneManager.h"

class Engine {
public:
    using KeyboardHandler = std::function<void(int key, int action)>;
    using MouseHandler = std::function<void(double x, double y)>;  // 🆕 鼠标处理器

    WindowConfig windowConfig;
    SceneManager sceneManager;

    // 添加窗口状态管理
    int GetWindowWidth() const { return m_windowWidth; }
    int GetWindowHeight() const { return m_windowHeight; }

    static Engine* GetInstance() { return s_instance; }

    void Run(GLFWwindow* window) {

        s_instance = this;
        InitializeWindowState(window);

        auto scene = sceneManager.GetActiveScene();
        if (!scene)
        {
            LOG_ERROR("No active scene set in SceneManager. Exiting Run loop.");
            return;
        }

        //主循环
        while (!glfwWindowShouldClose(window)) {
            LOG_SUCCESS("New frame start.");

            LOG_LEVEL_INFO(1, "Engine: Processing UpdateTime and Input ");
            UpdateTime();
            UpdateWindowState(window);
            HandleInput(window);
            HandleMouse(window);  // 🆕 处理鼠标输入
            LOG_INFO("\tEngine: Compelete UpdateTime and Input. DeltaTime = {}", deltaTime);

            //通过SceneManager渲染当前场景
            LOG_LEVEL_INFO(1, "Engine: Updating Data in CPU");
            sceneManager.Update(deltaTime);
            LOG_LEVEL_INFO(1, "Engine: Finished Data in CPU");

            LOG_LEVEL_INFO(1, "Engine: Rendering Scene in GPU");
            sceneManager.Render();
            LOG_LEVEL_INFO(1, "Engine: Finished Rendering Scene in GPU");

            glfwSwapBuffers(window);
            glfwPollEvents();

            LOG_SUCCESS("Frame end.");
        }
        // 清理场景
        sceneManager.Cleanup();
        s_instance = nullptr; 
    }
    
    void SetKeyboardHandler(KeyboardHandler handler) { globalInputHandler = handler; }
    void SetMouseHandler(MouseHandler handler) { m_mouseHandler = handler; }  // 🆕 设置鼠标处理器
    void SetScene(std::unique_ptr<Scene> scene) { sceneManager.SetActiveScene(std::move(scene)); }

protected:
    KeyboardHandler globalInputHandler = nullptr;
    MouseHandler m_mouseHandler = nullptr;  // 🆕 鼠标处理器

    float lastTime = 0.0f;
    float deltaTime = 0.0f;
    float currentTime = 0.0f;

    // 🆕 鼠标状态
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_firstMouse = true;

    void UpdateTime() {
        currentTime = static_cast<float>(glfwGetTime());
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;
    }

    // 🆕 鼠标处理方法
    void HandleMouse(GLFWwindow* window) {
        if (!m_mouseHandler) return;

        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        // 初始化时记录第一次鼠标位置
        if (m_firstMouse) {
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
            m_firstMouse = false;
        }

        // 只有鼠标位置发生变化时才调用处理器
        if (mouseX != m_lastMouseX || mouseY != m_lastMouseY) {
            m_mouseHandler(mouseX, mouseY);
            m_lastMouseX = mouseX;
            m_lastMouseY = mouseY;
        }
    }

    void HandleInput(GLFWwindow* window)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }
        if (!globalInputHandler) return;
        
        // 简单的键盘处理
        static double lastKeyTime = 0.0;
        if (currentTime - lastKeyTime > 0.3) { // 防抖
            if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_G, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            else if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_P, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            else if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_C, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            // 🆕 添加L和R键处理
            else if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_L, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_R, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            // 数字键处理
            else if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_1, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_2, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_3, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_4, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
            else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
                globalInputHandler(GLFW_KEY_5, GLFW_PRESS);
                lastKeyTime = currentTime;
            }
        }
    }
private:
    int m_windowWidth = 0;
    int m_windowHeight = 0;
    static inline Engine* s_instance = nullptr;

    void InitializeWindowState(GLFWwindow* window) {
        LOG_INFO("Engine: Initializing window state...");
        UpdateWindowState(window);
        LOG_INFO("\tWindow state initialized: {}x{}", m_windowWidth, m_windowHeight);
    }
    
    void UpdateWindowState(GLFWwindow* window) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // 只在尺寸变化时更新
        if (width != m_windowWidth || height != m_windowHeight) {
            m_windowWidth = width;
            m_windowHeight = height;

            // 同步更新全局变量（兼容老代码）
            g_WindowWidth = width;
            g_WindowHeight = height;

            LOG_DEBUG("Engine: Window size updated to {}x{}", width, height);
        }
    }
};




