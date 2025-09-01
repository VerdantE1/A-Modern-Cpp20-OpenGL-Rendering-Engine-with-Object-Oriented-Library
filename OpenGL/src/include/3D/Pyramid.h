#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Shaper.h"
#include "Renderer.h"
#include <vector>
#include <glm/glm.hpp>
#include "TangentUtils.h" // 新增

class Pyramid : public Shaper
{
private:
    bool m_hasTangents = false; // 新增
    std::vector<float> m_vertexData; // 新增：动态存储顶点数据
    std::vector<unsigned int> m_indexData; // 新增：动态存储索引数据

    inline static constexpr float vertices[] = {
        // 位置坐标              // 纹理坐标    // 法线坐标

        // 底面 (Y = -1, 法线: 0, -1, 0) - 4个顶点
        -1.0f, -1.0f,  1.0f,    0.0f, 0.0f,   0.0f, -1.0f, 0.0f, // 前左
         1.0f, -1.0f,  1.0f,    1.0f, 0.0f,   0.0f, -1.0f, 0.0f, // 前右
         1.0f, -1.0f, -1.0f,    1.0f, 1.0f,   0.0f, -1.0f, 0.0f, // 后右
        -1.0f, -1.0f, -1.0f,    0.0f, 1.0f,   0.0f, -1.0f, 0.0f, // 后左

        // 前面三角形 (法线计算: 向外法线约为 0, 0.447, 0.894)
         0.0f,  1.0f,  0.0f,    0.5f, 0.0f,   0.0f, 0.447f, 0.894f, // 顶点
        -1.0f, -1.0f,  1.0f,    0.0f, 1.0f,   0.0f, 0.447f, 0.894f, // 左下
         1.0f, -1.0f,  1.0f,    1.0f, 1.0f,   0.0f, 0.447f, 0.894f, // 右下

         // 右面三角形 (法线: 0.894, 0.447, 0)
          0.0f,  1.0f,  0.0f,    0.5f, 0.0f,   0.894f, 0.447f, 0.0f, // 顶点  
          1.0f, -1.0f,  1.0f,    0.0f, 1.0f,   0.894f, 0.447f, 0.0f, // 前下
          1.0f, -1.0f, -1.0f,    1.0f, 1.0f,   0.894f, 0.447f, 0.0f, // 后下

          // 后面三角形 (法线: 0, 0.447, -0.894)
           0.0f,  1.0f,  0.0f,    0.5f, 0.0f,   0.0f, 0.447f, -0.894f, // 顶点
           1.0f, -1.0f, -1.0f,    0.0f, 1.0f,   0.0f, 0.447f, -0.894f, // 右下
          -1.0f, -1.0f, -1.0f,    1.0f, 1.0f,   0.0f, 0.447f, -0.894f, // 左下

          // 左面三角形 (法线: -0.894, 0.447, 0)
           0.0f,  1.0f,  0.0f,    0.5f, 0.0f,   -0.894f, 0.447f, 0.0f, // 顶点
          -1.0f, -1.0f, -1.0f,    0.0f, 1.0f,   -0.894f, 0.447f, 0.0f, // 后下
          -1.0f, -1.0f,  1.0f,    1.0f, 1.0f,   -0.894f, 0.447f, 0.0f, // 前下
    };

    inline static constexpr unsigned int indices[] = {
        // 底面 (两个三角形)
        0, 1, 2,    // 底面三角形1
        2, 3, 0,    // 底面三角形2

        // 侧面 (4个三角形)
        4, 5, 6,    // 前面
        7, 8, 9,    // 右面
        10, 11, 12, // 后面
        13, 14, 15  // 左面
    };

public:
    // 新增参数 enableTangents，默认false兼容旧代码
    Pyramid(bool enableTangents = false) :
        Shaper(nullptr, 0, nullptr, 0, { (float)3, (float)2, (float)3 }) // 默认布局(无切线)
    {
        // 复制静态数据到动态向量
        m_vertexData.assign(vertices, vertices + sizeof(vertices) / sizeof(float));
        m_indexData.assign(indices, indices + sizeof(indices) / sizeof(unsigned int));

        if (enableTangents) {
            int numVerts = 16; // 金字塔有16个顶点
            // 从交错数据拆出pos/uv/normal
            std::vector<glm::vec3> positions(numVerts);
            std::vector<glm::vec3> normals(numVerts);
            std::vector<glm::vec2> uvs(numVerts);

            for (int v = 0; v < numVerts; ++v) {
                int idx = v * 8;
                positions[v] = glm::vec3(m_vertexData[idx + 0], m_vertexData[idx + 1], m_vertexData[idx + 2]);
                uvs[v] = glm::vec2(m_vertexData[idx + 3], m_vertexData[idx + 4]);
                normals[v] = glm::vec3(m_vertexData[idx + 5], m_vertexData[idx + 6], m_vertexData[idx + 7]);
            }

            std::vector<glm::vec3> tangents;
            ComputeTangents(positions, normals, uvs, m_indexData, tangents);

            // 重新交错为 3+2+3+3 = 11 floats
            std::vector<float> interleaved;
            interleaved.resize(static_cast<size_t>(numVerts) * 11);
            for (int v = 0; v < numVerts; ++v) {
                int o = v * 11;
                int i = v * 8;
                interleaved[o + 0] = m_vertexData[i + 0];
                interleaved[o + 1] = m_vertexData[i + 1];
                interleaved[o + 2] = m_vertexData[i + 2];
                interleaved[o + 3] = m_vertexData[i + 3];
                interleaved[o + 4] = m_vertexData[i + 4];
                interleaved[o + 5] = m_vertexData[i + 5];
                interleaved[o + 6] = m_vertexData[i + 6];
                interleaved[o + 7] = m_vertexData[i + 7];
                interleaved[o + 8] = tangents[v].x;
                interleaved[o + 9] = tangents[v].y;
                interleaved[o + 10] = tangents[v].z;
            }
            m_vertexData.swap(interleaved);

            // 更新布局为包含切线
            layout = VertexBufferLayout({ (float)3, (float)2, (float)3, (float)3 });
            m_hasTangents = true;
        }

        vb = std::move(VertexBuffer(m_vertexData));
        ib = std::move(IndexBuffer(m_indexData));
        va.LinkBufferAndLayout(vb, layout);
    }

    bool HasTangents() const { return m_hasTangents; } // 新增：供外部查询

    void Draw(Shader& shader, const Renderer& renderer) override {
        renderer.Draw(va, ib, shader);
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