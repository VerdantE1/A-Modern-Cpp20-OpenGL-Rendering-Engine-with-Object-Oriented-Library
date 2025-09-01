#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Shaper.h"
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <stdexcept>
#include "TangentUtils.h" // 新增

class Sphere : public Shaper
{
private:
    int numSegment;
    int numVertices;
    int numIndices;
    std::vector<float> vertexData;
    std::vector<unsigned int> indexData;
    bool m_hasTangents = false; // 新增

    void generateVertices(std::vector<float>& vertexData, std::vector<unsigned int>& indexDate);

    float toRadians(float angle) const {
        return angle * 3.14159265358979323846f / 180.0f;
    }

public:
    // 新增参数 enableTangents，默认false兼容旧代码
    Sphere(int numSegment = 36, bool enableTangents = false)
        : Shaper(nullptr, 0, nullptr, 0, { (float)3, (float)2, (float)3 }), // 默认布局(无切线)
          numVertices(0), numIndices(0), numSegment(numSegment)
    {
        generateVertices(vertexData, indexData);

        if (enableTangents) {
            // 从交错数据拆出pos/uv/normal
            std::vector<glm::vec3> positions(numVertices);
            std::vector<glm::vec3> normals(numVertices);
            std::vector<glm::vec2> uvs(numVertices);
            for (int v = 0; v < numVertices; ++v) {
                int idx = v * 8;
                positions[v] = glm::vec3(vertexData[idx+0], vertexData[idx+1], vertexData[idx+2]);
                uvs[v]       = glm::vec2(vertexData[idx+3], vertexData[idx+4]);
                normals[v]   = glm::vec3(vertexData[idx+5], vertexData[idx+6], vertexData[idx+7]);
            }
            std::vector<glm::vec3> tangents;
            ComputeTangents(positions, normals, uvs, indexData, tangents);

            // 重新交错为 3+2+3+3 = 11 floats
            std::vector<float> interleaved;
            interleaved.resize(static_cast<size_t>(numVertices) * 11);
            for (int v = 0; v < numVertices; ++v) {
                int o = v * 11;
                int i = v * 8;
                interleaved[o+0]  = vertexData[i+0];
                interleaved[o+1]  = vertexData[i+1];
                interleaved[o+2]  = vertexData[i+2];
                interleaved[o+3]  = vertexData[i+3];
                interleaved[o+4]  = vertexData[i+4];
                interleaved[o+5]  = vertexData[i+5];
                interleaved[o+6]  = vertexData[i+6];
                interleaved[o+7]  = vertexData[i+7];
                interleaved[o+8]  = tangents[v].x;
                interleaved[o+9]  = tangents[v].y;
                interleaved[o+10] = tangents[v].z;
            }
            vertexData.swap(interleaved);

            // 更新布局为包含切线
            layout = VertexBufferLayout({ (float)3, (float)2, (float)3, (float)3 });
            m_hasTangents = true;
        }

        vb = std::move(VertexBuffer(vertexData));
        ib = std::move(IndexBuffer(indexData));
        va.LinkBufferAndLayout(vb, layout);
    }

    int getNumVertices() const { return numVertices; }
    int getNumIndices() const { return numIndices; }
    bool HasTangents() const { return m_hasTangents; } // 新增：供外部查询

    void Draw(Shader& shader, const Renderer& renderer) override {
        throw std::logic_error(" Draw() not implemented for this subclass!");
    }
};

inline void Sphere::generateVertices(std::vector<float>& vertexData, std::vector<unsigned int>& indexDate)
{
    numVertices = (numSegment + 1) * (numSegment + 1);
    numIndices = numSegment * numSegment * 6;

    vertexData.resize(numVertices * 8); // 3+2+3

    for (int i = 0; i <= numSegment; i++) {
        for (int j = 0; j <= numSegment; j++) {
            float y = (float)cos(toRadians(180.0f - i * 180.0f / numSegment));
            float x = -(float)cos(toRadians(j * 360.0f / numSegment)) * (float)abs(cos(asin(y)));
            float z = (float)sin(toRadians(j * 360.0f / numSegment)) * (float)abs(cos(asin(y)));

            int idx = (i * (numSegment + 1) + j) * 8;
            vertexData[idx + 0] = x;
            vertexData[idx + 1] = y;
            vertexData[idx + 2] = z;

            vertexData[idx + 3] = (float)j / numSegment;
            vertexData[idx + 4] = (float)i / numSegment;

            vertexData[idx + 5] = x;
            vertexData[idx + 6] = y;
            vertexData[idx + 7] = z;
        }
    }

    indexDate.resize(numIndices);
    for (int i = 0; i < numSegment; i++) {
        for (int j = 0; j < numSegment; j++) {
            int base = 6 * (i * numSegment + j);
            int row1 = i * (numSegment + 1);
            int row2 = (i + 1) * (numSegment + 1);

            indexDate[base + 0] = row1 + j;
            indexDate[base + 1] = row1 + j + 1;
            indexDate[base + 2] = row2 + j;
            indexDate[base + 3] = row1 + j + 1;
            indexDate[base + 4] = row2 + j + 1;
            indexDate[base + 5] = row2 + j;
        }
    }
}
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
