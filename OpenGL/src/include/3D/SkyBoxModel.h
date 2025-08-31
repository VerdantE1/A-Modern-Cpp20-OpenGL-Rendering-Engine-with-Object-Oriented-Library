#pragma once
#include "Shaper.h"
#include "Renderer.h"

class SkyBoxModel : public Shaper {
private:
    // 🔧 正确的天空盒顶点数据：位置(3) + 纹理坐标(2) 交错存储
    inline static constexpr float vertices[] = {
        // 每个顶点：x,y,z,u,v
        // 前面
        -1.0f, -1.0f,  1.0f,   0.50f, 0.33f,  // 左下
         1.0f, -1.0f,  1.0f,   0.25f, 0.33f,  // 右下
         1.0f,  1.0f,  1.0f,   0.25f, 0.66f,  // 右上
        -1.0f,  1.0f,  1.0f,   0.50f, 0.66f,  // 左上

        // 后面
        -1.0f, -1.0f, -1.0f,   1.00f, 0.33f,  
         1.0f, -1.0f, -1.0f,   0.75f, 0.33f,  
         1.0f,  1.0f, -1.0f,   0.75f, 0.66f,  
        -1.0f,  1.0f, -1.0f,   1.00f, 0.66f,  

        // 右面
         1.0f, -1.0f, -1.0f,   0.75f, 0.33f,
         1.0f, -1.0f,  1.0f,   0.50f, 0.33f,
         1.0f,  1.0f,  1.0f,   0.50f, 0.66f,
         1.0f,  1.0f, -1.0f,   0.75f, 0.66f,

        // 左面  
        -1.0f, -1.0f,  1.0f,   0.25f, 0.33f,
        -1.0f, -1.0f, -1.0f,   0.00f, 0.33f,
        -1.0f,  1.0f, -1.0f,   0.00f, 0.66f,
        -1.0f,  1.0f,  1.0f,   0.25f, 0.66f,

        // 上面
        -1.0f,  1.0f,  1.0f,   0.25f, 1.00f,
         1.0f,  1.0f,  1.0f,   0.50f, 1.00f,
         1.0f,  1.0f, -1.0f,   0.50f, 0.66f,
        -1.0f,  1.0f, -1.0f,   0.25f, 0.66f,

        // 下面
        -1.0f, -1.0f, -1.0f,   0.25f, 0.33f,
         1.0f, -1.0f, -1.0f,   0.50f, 0.33f,
         1.0f, -1.0f,  1.0f,   0.50f, 0.00f,
        -1.0f, -1.0f,  1.0f,   0.25f, 0.00f,
    };

    // 索引数据
    inline static constexpr unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,       // 前面
        4, 5, 6, 6, 7, 4,       // 后面
        8, 9, 10, 10, 11, 8,    // 右面
        12, 13, 14, 14, 15, 12, // 左面
        16, 17, 18, 18, 19, 16, // 上面
        20, 21, 22, 22, 23, 20  // 下面
    };

public:
    // 🔧 正确的构造函数：调用Shaper基类
    SkyBoxModel() : Shaper(
        vertices,                                                    // 顶点数据
        sizeof(vertices),                                           // 顶点数据字节数
        indices,                                                    // 索引数据
        sizeof(indices) / sizeof(unsigned int),                    // 索引数量
        std::vector<VertexBufferLayout::SupportedTypes>{3.0f, 2.0f} // 位置3 + 纹理2
    ) {}

    // 🆕 天空盒特殊渲染
    void Draw(Shader& shader, const Renderer& renderer) override {
        // 设置天空盒渲染状态
        glDepthMask(GL_FALSE);      // 禁用深度写入
        glDepthFunc(GL_LEQUAL);     // 设置深度函数
        
        // 🔧 使用基类和Renderer的标准流程
        renderer.Draw(*this, shader);
        
        // 恢复渲染状态
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);       
    }
};