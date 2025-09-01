#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include "Shaper.h"
#include <cmath> 
#include <vector> 
#include <glm/glm.hpp> 
#include <stdexcept>
#include "TangentUtils.h" // 新增

class Torus : public Shaper
{
private:
	int majorSegments;  // 主环方向的分段数
	int minorSegments;  // 小环方向的分段数
	float majorRadius;  // 主半径（环的中心到管子中心的距离）
	float minorRadius;  // 小半径（管子的半径）
	int numVertices;
	int numIndices;
	std::vector<float> vertexData;
	std::vector<unsigned int> indexData;
	bool m_hasTangents = false; // 新增

	void generateVertices(std::vector<float>& vertexData, std::vector<unsigned int>& indexData);

	float toRadians(float angle) const {
		return angle * 3.14159265358979323846f / 180.0f; // π = 3.14159265358979323846
	}

public:
	// 新增参数 enableTangents，默认false兼容旧代码
	// majorRadius: 主半径, minorRadius: 小半径, majorSegments: 主环分段, minorSegments: 小环分段
	Torus(float majorRadius = 2.0f, float minorRadius = 0.5f, int majorSegments = 36, int minorSegments = 18, bool enableTangents = false)
		: Shaper(nullptr, 0, nullptr, 0, { (float)3, (float)2, (float)3 }), // 默认布局(无切线)
		majorRadius(majorRadius), minorRadius(minorRadius),
		majorSegments(majorSegments), minorSegments(minorSegments),
		numVertices(0), numIndices(0)
	{
		generateVertices(vertexData, indexData);

		if (enableTangents) {
			// 从交错数据拆出pos/uv/normal
			std::vector<glm::vec3> positions(numVertices);
			std::vector<glm::vec3> normals(numVertices);
			std::vector<glm::vec2> uvs(numVertices);
			for (int v = 0; v < numVertices; ++v) {
				int idx = v * 8;
				positions[v] = glm::vec3(vertexData[idx + 0], vertexData[idx + 1], vertexData[idx + 2]);
				uvs[v] = glm::vec2(vertexData[idx + 3], vertexData[idx + 4]);
				normals[v] = glm::vec3(vertexData[idx + 5], vertexData[idx + 6], vertexData[idx + 7]);
			}
			std::vector<glm::vec3> tangents;
			ComputeTangents(positions, normals, uvs, indexData, tangents);

			// 重新交错为 3+2+3+3 = 11 floats
			std::vector<float> interleaved;
			interleaved.resize(static_cast<size_t>(numVertices) * 11);
			for (int v = 0; v < numVertices; ++v) {
				int o = v * 11;
				int i = v * 8;
				interleaved[o + 0] = vertexData[i + 0];
				interleaved[o + 1] = vertexData[i + 1];
				interleaved[o + 2] = vertexData[i + 2];
				interleaved[o + 3] = vertexData[i + 3];
				interleaved[o + 4] = vertexData[i + 4];
				interleaved[o + 5] = vertexData[i + 5];
				interleaved[o + 6] = vertexData[i + 6];
				interleaved[o + 7] = vertexData[i + 7];
				interleaved[o + 8] = tangents[v].x;
				interleaved[o + 9] = tangents[v].y;
				interleaved[o + 10] = tangents[v].z;
			}
			vertexData.swap(interleaved);

			// 更新布局为包含切线
			layout = VertexBufferLayout({ (float)3, (float)2, (float)3, (float)3 });
			m_hasTangents = true;
		}

		vb = std::move(VertexBuffer(vertexData));
		ib = std::move(IndexBuffer(indexData));
		// layout已经初始化过
		va.LinkBufferAndLayout(vb, layout);
	}

	int getNumVertices() const { return numVertices; }
	int getNumIndices() const { return numIndices; }
	float getMajorRadius() const { return majorRadius; }
	float getMinorRadius() const { return minorRadius; }
	bool HasTangents() const { return m_hasTangents; } // 新增：供外部查询

	void Draw(Shader& shader, const Renderer& renderer) override {
		throw std::logic_error("Draw() not implemented for this subclass!");
	}
};

inline void Torus::generateVertices(std::vector<float>& vertexData, std::vector<unsigned int>& indexData)
{
	numVertices = (majorSegments + 1) * (minorSegments + 1);
	numIndices = majorSegments * minorSegments * 6;  // 每个面两个三角形，每个三角形三个顶点

	vertexData.resize(numVertices * 8); // 3(位置) + 2(纹理) + 3(法向量) = 8

	// 生成顶点数据
	for (int i = 0; i <= majorSegments; i++) {
		for (int j = 0; j <= minorSegments; j++) {
			// 计算角度
			float u = (float)i / majorSegments * 360.0f;  // 主环角度 (0-360度)
			float v = (float)j / minorSegments * 360.0f;  // 小环角度 (0-360度)

			float uRad = toRadians(u);
			float vRad = toRadians(v);

			// 计算顶点位置
			float x = (majorRadius + minorRadius * cos(vRad)) * cos(uRad);
			float y = minorRadius * sin(vRad);
			float z = (majorRadius + minorRadius * cos(vRad)) * sin(uRad);

			// 计算法向量
			float nx = cos(vRad) * cos(uRad);
			float ny = sin(vRad);
			float nz = cos(vRad) * sin(uRad);

			// 计算数组索引
			int idx = (i * (minorSegments + 1) + j) * 8;

			// 存储位置坐标
			vertexData[idx + 0] = x;
			vertexData[idx + 1] = y;
			vertexData[idx + 2] = z;

			// 存储纹理坐标
			vertexData[idx + 3] = (float)i / majorSegments;  // u坐标
			vertexData[idx + 4] = (float)j / minorSegments;  // v坐标

			// 存储法向量
			vertexData[idx + 5] = nx;
			vertexData[idx + 6] = ny;
			vertexData[idx + 7] = nz;
		}
	}

	// 生成索引数据
	indexData.resize(numIndices);

	for (int i = 0; i < majorSegments; i++) {
		for (int j = 0; j < minorSegments; j++) {
			// 计算四个顶点的索引
			int current = i * (minorSegments + 1) + j;
			int next = current + minorSegments + 1;

			int base = 6 * (i * minorSegments + j);

			// 第一个三角形
			indexData[base + 0] = current;
			indexData[base + 1] = next;
			indexData[base + 2] = current + 1;

			// 第二个三角形
			indexData[base + 3] = current + 1;
			indexData[base + 4] = next;
			indexData[base + 5] = next + 1;
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