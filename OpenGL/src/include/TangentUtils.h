#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

inline void ComputeTangents(
    const std::vector<glm::vec3>& positions,
    const std::vector<glm::vec3>& normals,
    const std::vector<glm::vec2>& uvs,
    const std::vector<unsigned>& indices,
    std::vector<glm::vec3>& outTangents)
{
    outTangents.assign(positions.size(), glm::vec3(0.0f));

    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        unsigned i0 = indices[i], i1 = indices[i+1], i2 = indices[i+2];
        const glm::vec3& p0 = positions[i0];
        const glm::vec3& p1 = positions[i1];
        const glm::vec3& p2 = positions[i2];

        const glm::vec2& uv0 = uvs[i0];
        const glm::vec2& uv1 = uvs[i1];
        const glm::vec2& uv2 = uvs[i2];

        glm::vec3 e1 = p1 - p0;
        glm::vec3 e2 = p2 - p0;
        glm::vec2 d1 = uv1 - uv0;
        glm::vec2 d2 = uv2 - uv0;

        float denom = d1.x * d2.y - d1.y * d2.x;
        if (fabs(denom) < 1e-8f) {
            // UV退化，跳过或用备选切线
            continue;
        }
        float r = 1.0f / denom;
        glm::vec3 T = (e1 * d2.y - e2 * d1.y) * r;

        outTangents[i0] += T;
        outTangents[i1] += T;
        outTangents[i2] += T;
    }

    // 归一化并做一次与法线的Gram-Schmidt正交化
    for (size_t v = 0; v < outTangents.size(); ++v) {
        glm::vec3 N = normals[v];
        glm::vec3 T = outTangents[v];
        if (glm::length2(T) < 1e-12f) {
            // 若没有有效切线，构造一个与法线正交的任意向量
            glm::vec3 a = fabs(N.x) > 0.9f ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            T = glm::normalize(glm::cross(a, N));
        }
        // T' = normalize(T - N * dot(N,T))
        T = glm::normalize(T - N * glm::dot(N, T));
        outTangents[v] = T;
    }
}