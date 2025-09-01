#pragma once
#include "Resource.h"
#include <string>
#include <unordered_set>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

typedef int GLint;

// 纹理类型枚举
enum class TextureType {
    TEXTURE_2D,      // 普通2D纹理
    TEXTURE_CUBE     // 立方体贴图
};

// 纹理过滤模式枚举
enum class TextureFilterMode {
    NEAREST,
    LINEAR,
    NEAREST_MIPMAP_NEAREST,
    LINEAR_MIPMAP_NEAREST,
    NEAREST_MIPMAP_LINEAR,
    LINEAR_MIPMAP_LINEAR
};

// 纹理环绕模式枚举
enum class TextureWrapMode {
    REPEAT,
    MIRRORED_REPEAT,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER
};

// 各向异性过滤级别枚举
enum class AnisotropyLevel {
    NONE = 0,
    LOW = 2,
    MEDIUM = 4,
    HIGH = 8,
    VERY_HIGH = 16
};

class Texture : public Resource
{
private:
    std::string m_FilePath;
    unsigned char* m_LocalBuffer;
    int m_Width, m_Height, m_Bpp;
    unsigned int m_AssignedSlot = -1;
    
    // 纹理类型
    TextureType m_TextureType = TextureType::TEXTURE_2D;
    
    // 各向异性过滤等级
    float m_AnisotropyLevel = 1.0f;
    
    // 静态资源管理
    static std::unordered_set<unsigned int> s_AvailableSlots;
    static unsigned int s_MaxSlotUsed;
    static bool s_AnisotropyChecked;
    static bool s_AnisotropySupported;
    static float s_MaxAnisotropy;

    // 辅助函数
    GLint GetGLFilterMode(TextureFilterMode mode) const;
    GLint GetGLWrapMode(TextureWrapMode mode) const;
    static void CheckAnisotropySupport();

public:
    Texture() = default;
    
    // 现有2D纹理构造函数
    Texture(const std::string& filepath, 
            TextureFilterMode magFilter = TextureFilterMode::LINEAR, 
            TextureFilterMode minFilter = TextureFilterMode::LINEAR,
            TextureWrapMode wrapS = TextureWrapMode::CLAMP_TO_EDGE,
            TextureWrapMode wrapT = TextureWrapMode::CLAMP_TO_EDGE,
            bool generateMipmaps = false,
            bool flipVertically = true,
            AnisotropyLevel anisotropy = AnisotropyLevel::NONE);
    
    
    static std::unique_ptr<Texture> CreateCubeMapFromSixImages(const std::vector<std::string>& faces,
            TextureFilterMode magFilter = TextureFilterMode::LINEAR,
            TextureFilterMode minFilter = TextureFilterMode::LINEAR,
            bool generateMipmaps = false);
    
    ~Texture();

    void Bind() const override; 
    void Unbind() const override;

    // 获取纹理类型
    TextureType GetTextureType() const { return m_TextureType; }
    
    // 现有方法保持不变
    void SetBorderColor(const glm::vec4& color);
    void SetAnisotropyLevel(AnisotropyLevel level);
    void SetAnisotropyLevel(float level);
    float GetAnisotropyLevel() const { return m_AnisotropyLevel; }
    
    static bool IsAnisotropySupported() { 
        if (!s_AnisotropyChecked) CheckAnisotropySupport();
        return s_AnisotropySupported; 
    }
    
    static float GetMaxAnisotropy() { 
        if (!s_AnisotropyChecked) CheckAnisotropySupport();
        return s_MaxAnisotropy; 
    }

    // Getter方法
    inline int GetWidth() const { return m_Width; }
    inline int GetHeight() const { return m_Height; }
    inline int GetBpp() const { return m_Bpp; } 
    inline unsigned int GetAssignedSlot() const { return m_AssignedSlot; }
    
    static unsigned int GetNextAvailableSlot();
    static void ReleaseSlot(unsigned int slot);

    // 把此纹理显式绑定到指定槽位（稳定槽位）
    void BindToUnit(unsigned unit);
};



/* Example Usage:

// 默认设置
Texture defaultTexture("res/textures/brick1.jpg");

// 使用各向异性过滤的纹理
Texture anisotropicTexture("res/textures/floor.jpg",
    TextureFilterMode::LINEAR,
    TextureFilterMode::LINEAR_MIPMAP_LINEAR,
    TextureWrapMode::REPEAT,
    TextureWrapMode::REPEAT,
    true,  // 生成mipmap
    true,  // 垂直翻转
    AnisotropyLevel::HIGH);  // 高级别各向异性过滤

// 检查是否支持各向异性过滤
if (Texture::IsAnisotropySupported()) {
    std::cout << "Anisotropic filtering supported! Max level: " 
              << Texture::GetMaxAnisotropy() << "x" << std::endl;
}

*/
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
