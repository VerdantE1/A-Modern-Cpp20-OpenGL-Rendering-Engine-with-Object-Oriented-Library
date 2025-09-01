#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include <glm/glm.hpp>
#include "Shaper.h"
#include "TangentUtils.h" // 新增

/*
1.Only Support Obj format.
2.Only Suport 3 properties: position, texture coordinate, normal. (Should be current existing in the model)
3.Only Support Triangles.
4.Only Support space separated.
5.No process for concrect texture.
6.Fully Unfold: For simple , we don't use index method to organize the vertex data. We creat 3 vertices for each triangle though it can cause some redundancy.
*/
class ImportedModel :public Shaper
{
private:
	bool m_hasTangents = false; // 新增
	std::vector<unsigned int> m_indexData; // 新增：用于切线计算的索引数据

public:
	int numVertices;
	std::vector<float> vertexData;  // 交错存储: pos(3) + texCoord(2) + normal(3) = 8 floats per vertex

	std::vector<float> positionData;    // 3 floats per vertex (临时存储原始数据)
	std::vector<float> textureCoordData; // 2 floats per vertex (临时存储原始数据)
	std::vector<float> normalData;      // 3 floats per vertex (临时存储原始数据)

	ImportedModel() = delete; // 禁止默认构造函数
	// 新增参数 enableTangents，默认false兼容旧代码
	ImportedModel(const std::string& filePath, bool enableTangents = false);
	void parseOBJ(const std::string& filePath, bool enableTangents = false);

	void Draw(Shader& shader, const Renderer& renderer) override {
		throw std::logic_error("Draw() not implemented for ImportedModel!");
	}

	int getNumVertices() const { return numVertices; }
	bool HasTangents() const { return m_hasTangents; } // 新增：供外部查询
	const std::vector<float>& getVertexData() const { return vertexData; }
	const std::vector<float>& getPositionData() const { return positionData; }
	const std::vector<float>& getTextureCoordData() const { return textureCoordData; }
	const std::vector<float>& getNormalData() const { return normalData; }
};

inline ImportedModel::ImportedModel(const std::string& filePath, bool enableTangents)
	: Shaper(nullptr, 0, nullptr, 0, { (float)3, (float)2, (float)3 }), numVertices(0) // 默认布局(无切线)
{
	parseOBJ(filePath, enableTangents);

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
		ComputeTangents(positions, normals, uvs, m_indexData, tangents);

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
	va.LinkBufferAndLayout(vb, layout);

#ifdef IMPORTED_MODEL_DEBUG
	// 添加调试信息
	std::cout << "ImportedModel loaded:" << std::endl;
	std::cout << "  Vertices: " << numVertices << std::endl;
	std::cout << "  Vertex data size: " << vertexData.size() << std::endl;
	std::cout << "  Expected size: " << numVertices * (m_hasTangents ? 11 : 8) << std::endl;
	std::cout << "  Has tangents: " << (m_hasTangents ? "Yes" : "No") << std::endl;
#endif
}

inline void ImportedModel::parseOBJ(const std::string& filePath, bool enableTangents)
{
	using namespace std;
	std::ifstream ifs(filePath, ios::in);
	if (!ifs.is_open())
	{
		throw std::runtime_error("Failed to open file: " + filePath);
	}
	string line = "";

	// 为切线计算准备索引数据（如果需要）
	if (enableTangents) {
		m_indexData.clear();
	}

	while (getline(ifs, line)) {
		if (line.empty() || line[0] == '#') {
			continue; // Skip empty lines and comments
		}

		istringstream iss(line);
		string prefix;
		iss >> prefix;

		if (prefix == "v")
		{
			float x, y, z;
			iss >> x >> y >> z;
			positionData.push_back(x);
			positionData.push_back(y);
			positionData.push_back(z);

			iss >> std::ws;
			assert(iss.eof()); // 确保行末没有额外数据
		}
		else if (prefix == "vt")
		{
			float u, v;
			iss >> u >> v;
			textureCoordData.push_back(u);
			textureCoordData.push_back(v);

			iss >> std::ws;
			assert(iss.eof()); // 确保行末没有额外数据
		}
		else if (prefix == "vn")
		{
			float nx, ny, nz;
			iss >> nx >> ny >> nz;
			normalData.push_back(nx);
			normalData.push_back(ny);
			normalData.push_back(nz);

			iss >> std::ws;
			assert(iss.eof()); // 确保行末没有额外数据
		}
		else if (prefix == "f")
		{
			int posIndex[3], texIndex[3], normIndex[3];
			char slash; // 用于跳过斜杠
			for (int i = 0; i < 3; ++i) {
				iss >> posIndex[i] >> slash >> texIndex[i] >> slash >> normIndex[i];
				posIndex[i]--; // OBJ索引从1开始，C++从0开始
				texIndex[i]--;
				normIndex[i]--;

				// 如果需要切线，记录索引
				if (enableTangents) {
					m_indexData.push_back(numVertices + i);
				}

				vertexData.push_back(positionData[posIndex[i] * 3]);
				vertexData.push_back(positionData[posIndex[i] * 3 + 1]);
				vertexData.push_back(positionData[posIndex[i] * 3 + 2]);
				vertexData.push_back(textureCoordData[texIndex[i] * 2]);
				vertexData.push_back(textureCoordData[texIndex[i] * 2 + 1]);
				vertexData.push_back(normalData[normIndex[i] * 3]);
				vertexData.push_back(normalData[normIndex[i] * 3 + 1]);
				vertexData.push_back(normalData[normIndex[i] * 3 + 2]);
			}
			numVertices += 3; // 每个面有三个顶点

			iss >> std::ws;
			assert(iss.eof()); // 确保行末没有额外数据
		}
	}
}