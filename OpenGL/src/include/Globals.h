#pragma once
#include "Light/Utils.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>



#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "DrawDemo.h"

#include "GlobalLight.h"

#include "Utility.h"
#include "Texture.h"
#include "Renderer.h"

#include "Camera.h"
#include "Entity.h"


#include "Logger.h"


#include "AllShaperTypes.h"
#include "Globals.h"
#include "Transform.h"
#include <memory>

#include "spdlog/spdlog.h"






/******************************************* Shader全局变量  *************************************************/
#define BLINN_PHONG_SHADER "Blinn-Phong.Shader"
#define GOURAUD_SHADER "Gouraud.shader"
#define PHONG_SHADER "Phong.shader"
#define SHADOW_MAPPING_SHADER "Shadow-Mapping.shader"   

#define GOURAUD_SHADER_PATH "res/shaders/Gouraud.shader"
#define BLINN_PHONG_SHADER_PATH "res/shaders/Blinn-Phong.Shader"
#define PHONG_SHADER_PATH "res/shaders/Phong.shader"
#define SHADOW_MAPPING_SHADER_PATH "res/shaders/ShadowShader/ShadowPass.shader"
#define SHADOW_MAPPING_RENDER_SHADER_PATH "res/shaders/ShadowShader/RenderPass.shader"

// 需要OpenGL上下文
inline std::shared_ptr<Shader> gouraudShaderPtr = nullptr;
inline std::shared_ptr<Shader> blinnPhongShaderPtr = nullptr;
inline std::shared_ptr<Shader> phongShaderPtr = nullptr;
inline std::shared_ptr<Shader> shadowMappingShaderPtr = nullptr;
inline std::shared_ptr<Shader> shadowMappingRenderShaderPtr = nullptr;

inline void InitializeGlobalShaders() {
	static bool initialized = false;
	if (initialized) return; // 防止重复初始化

	LOG_INFO("Initializing global shaders...");
	try {
		gouraudShaderPtr = std::make_shared<Shader>(GOURAUD_SHADER_PATH);
		blinnPhongShaderPtr = std::make_shared<Shader>(BLINN_PHONG_SHADER_PATH);
		phongShaderPtr = std::make_shared<Shader>(PHONG_SHADER_PATH);
		//shadowMappingShaderPtr = std::make_shared<Shader>(SHADOW_MAPPING_SHADER_PATH);
		shadowMappingRenderShaderPtr = std::make_shared<Shader>(SHADOW_MAPPING_RENDER_SHADER_PATH);

		initialized = true;
		if (!gouraudShaderPtr)  LOG_ERROR("gouraudShaderPtr is null after initialization!");
		else LOG_LEVEL_INFO(1,"Gouraud shader initialized successfully.");

		if (!blinnPhongShaderPtr) LOG_ERROR("blinnPhongShaderPtr is null after initialization!");
		else LOG_LEVEL_INFO(1,"Blinn-Phong shader initialized successfully.");

		if (!phongShaderPtr) LOG_ERROR("phongShaderPtr is null after initialization!");
		else LOG_LEVEL_INFO(1,"Phong shader initialized successfully.");

		if (!shadowMappingShaderPtr) LOG_ERROR("shadowMappingShaderPtr is null after initialization!");
		else LOG_LEVEL_INFO(1,"Shadow Mapping shader initialized successfully.");

		if (!shadowMappingRenderShaderPtr) LOG_ERROR("shadowMappingRenderShaderPtr is null after initialization!");
		else LOG_LEVEL_INFO(1,"Shadow Mapping Render shader initialized successfully.");

	}
	catch (const std::exception& e) {
		LOG_ERROR("Failed to initialize global shaders: {}", e.what());
		throw;
	}
}
/******************************************* 3D模型全局变量  *************************************************/
inline std::shared_ptr<Sphere> global_spherePtr = nullptr;
inline std::shared_ptr<Torus> global_torusPtr = nullptr;
inline std::shared_ptr<Cube> global_cubePtr = nullptr;	

// ✅ 添加延迟初始化函数
inline void InitializeGlobalObjects() {
	static bool initialized = false;
	if (initialized) return;

	global_spherePtr = std::make_shared<Sphere>();
	global_torusPtr = std::make_shared<Torus>();
	global_cubePtr = std::make_shared<Cube>();

	initialized = true;
}

/******************************************* 天空盒  *********************************************************/

/******************************************* 全局Render变量  *************************************************/
struct RenderConfig {

	bool usePolygonMode = false;
	bool useDepthTest = true;
	bool useCullFace = false;
	bool useBlend = false;


	void ApplyToRenderer(Renderer& renderer) const {
		renderer.SetPolygonMode(usePolygonMode)
			.SetDepthTest(useDepthTest)
			.SetCullFace(useCullFace)
			.SetBlend(useBlend);
	}
	RenderConfig& SetDefault() {
		usePolygonMode = false;
		useDepthTest = true;
		useCullFace = false;
		useBlend = false;
		return *this;
	}

	RenderConfig& SetDebug() {
		usePolygonMode = true;
		useDepthTest = true;
		useCullFace = false;
		useBlend = false;
		return *this;
	}
	
	RenderConfig& SetConfig(bool polygonMode, bool depthTest, bool cullFace, bool blend) {
		usePolygonMode = polygonMode;
		useDepthTest = depthTest;
		useCullFace = cullFace;
		useBlend = blend;
		return *this;
	}

	std::shared_ptr<Renderer> CreateConfiguredRenderer() const {
		std::shared_ptr<Renderer> renderer = std::make_shared<Renderer>();
		ApplyToRenderer(*renderer);
		return renderer;
	}
};


/******************************************* 全局光变量  *************************************************/


/******************************************* 全局窗口变量  *************************************************/
inline int g_WindowWidth = 2560;   
inline int g_WindowHeight = 1660;  


/******************************************* 光照参数相关变量  *************************************************/

// 光照相关的uniform位置
inline GLuint globalAmbLoc = 0;	   
inline GLuint ambLoc = 0;
inline GLuint diffLoc = 0;
inline GLuint specLoc = 0;
inline GLuint posLoc = 0;
inline GLuint mAmbLoc = 0;            
inline GLuint mDiffLoc = 0;          
inline GLuint mSpecLoc = 0;           
inline GLuint mShineLoc = 0;          

// 光源属性
// 位置position（x, y, z）
inline glm::vec3 initialLightLoc(5.0f, 2.0f, 2.0f); // 初始光源位置（模型空间）
inline glm::vec3 currentLightPos;		// 光源位置（模型空间）
inline glm::vec3 lightPosV;			// 变换到视图空间后的光源位置
inline float lightPos[3];				// 传递给GLSL的光源位置数组

// 光照颜色属性
inline float globalAmbient[4] = { 0.7f, 0.7f, 0.7f, 1.0f };
inline float lightAmbient[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
inline float lightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
inline float lightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };	

inline float* matAmb = goldAmbient();   // 物体材质环境光
inline float* matDif = goldDiffuse();   // 物体材质漫反射
inline float* matSpe = goldSpecular();  // 物体材质镜面反射
inline float matShi = goldShininess();    // 物体材质高光系数




// ==================================== 通用配置结构 ====================================
struct WindowConfig {
	static constexpr int default_width = 2560;
	static constexpr int default_height = 1660;

	int _width = default_width;
	int _height = default_height;
	float aspectRatio = static_cast<float>(_width) / static_cast<float>(_height);

	void UpdateFromWindow(GLFWwindow* window) {
		GLCall(glfwGetFramebufferSize(window, &_width, &_height));
		aspectRatio = static_cast<float>(_width) / static_cast<float>(_height);
	}

	void SetSize(int width, int height) {
		_width = width;
		_height = height;
		aspectRatio = static_cast<float>(_width) / static_cast<float>(_height);
	}
};


struct CameraConfig {
	glm::vec3 position = glm::vec3(0, 0, 12);
	glm::vec3 target = glm::vec3(0, 0, 0);
	glm::vec3 up = glm::vec3(0, 1, 0);
	float fov = 70.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;

	Camera CreateCamera(float aspectRatio = (float)2560 / (float)1660) const {
		return Camera(position, target, up, fov, aspectRatio, nearPlane, farPlane);
	}
};





/*
 * Copyright (c) 2025 
 * Email: 2523877046@qq.com
 * Author: Baiqiang Long (Buzzlight)
 * 
 * This file is part of the ReduxGL project.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
