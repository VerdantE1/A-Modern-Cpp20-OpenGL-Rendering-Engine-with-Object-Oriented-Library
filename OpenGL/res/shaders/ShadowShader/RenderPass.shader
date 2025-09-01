#shader vertex 
#version 430
layout (location = 0) in vec3 vertPos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 normal;

out vec3 varyingNormal;
out vec3 varyingLightDir;
out vec3 varyingHalfVec;
out vec3 varyingVertPos;
out vec4 shadow_coord; // 阴影坐标/光源坐标

struct PositionalLight
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 position;
};

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform mat4 mv_matrix; 
uniform mat4 proj_matrix;
uniform mat4 norm_matrix;
uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;
uniform mat4 model;    
uniform mat4 lightSpaceMatrix; // 光源空间矩阵（投影矩阵 * 视图矩阵）


void main() {

    vec4 P = mv_matrix * vec4(vertPos, 1.0);
    varyingVertPos = P.xyz;
    varyingNormal = (norm_matrix * vec4(normal, 0.0)).xyz;
    varyingLightDir = light.position - varyingVertPos;

    vec3 viewDir = normalize(-varyingVertPos);
    vec3 lightDir = normalize(varyingLightDir);
    varyingHalfVec = normalize(lightDir + viewDir);

    shadow_coord = lightSpaceMatrix * model * vec4(vertPos, 1.0);

    gl_Position = proj_matrix * P;
}


#shader fragment 
#version 430

// 从顶点着色器接收插值数据
in vec3 varyingNormal;
in vec3 varyingLightDir;
in vec3 varyingHalfVec;
in vec3 varyingVertPos;
in vec4 shadow_coord;

out vec4 fragColor;

struct PositionalLight
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec3 position;
};

struct Material
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

uniform vec4 globalAmbient;
uniform PositionalLight light;
uniform Material material;

uniform sampler2D shadowMap; // 阴影贴图
uniform float shadowBias; // 阴影偏移
uniform bool enablePCF; // 是否启用PCF(true=软阴影,false=硬阴影)
uniform int pcfSamples; // PCF采样数量
uniform float pcfRadius; // PCF采样半径
uniform bool isLightSource;

float HardShadowCalculation(vec4 fragPosLightSpace)
{
	// 执行透视除法, 将齐次坐标转换为标准化设备坐标(NDC)
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

	// 转换到[0,1]范围
	projCoords = projCoords * 0.5 + 0.5; 

	// 边界检查
	if (projCoords.z > 1.0) return 0.0;

	// 获取当前片段的深度值
	float currentDepth = projCoords.z;

	// 从阴影贴图中获取深度值
	float closestDepth = texture(shadowMap, projCoords.xy).r;

	// 检查当前片段是否在阴影中
	float shadow = currentDepth - shadowBias > closestDepth ? 1.0 : 0.0;

	return shadow;
}

float PCFShadowCalculation(vec4 fragPosLightSpace)
{
	// 执行透视除法
	vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;

	// 边界检查
	if (projCoords.z > 1.0) return 0.0;

	float currentDepth = projCoords.z;

	vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
	
	float shadow = 0.0;
	int samples = 0;

	if (pcfSamples <= 4) {
		// 2x2 采样模式 (4个样本)
		for(int x = -1; x <= 0; ++x) {
			for(int y = -1; y <= 0; ++y) {
				vec2 offset = vec2(x, y) * texelSize * pcfRadius;
				float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
				shadow += (currentDepth - shadowBias > pcfDepth) ? 1.0 : 0.0;
				samples++;
			}
		}
	}
	else if (pcfSamples <= 9) {
		// 3x3 采样模式 (9个样本)
		for(int x = -1; x <= 1; ++x) {
			for(int y = -1; y <= 1; ++y) {
				vec2 offset = vec2(x, y) * texelSize * pcfRadius;
				float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
				shadow += (currentDepth - shadowBias > pcfDepth) ? 1.0 : 0.0;
				samples++;
			}
		}
	}
	else if (pcfSamples <= 16) {
		// 4x4 采样模式 (16个样本)
		for(int x = -2; x <= 1; ++x) {
			for(int y = -2; y <= 1; ++y) {
				vec2 offset = vec2(x, y) * texelSize * pcfRadius;
				float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
				shadow += (currentDepth - shadowBias > pcfDepth) ? 1.0 : 0.0;
				samples++;
			}
		}
	}
	else {
		// 5x5 采样模式 (25个样本)
		for(int x = -2; x <= 2; ++x) {
			for(int y = -2; y <= 2; ++y) {
				vec2 offset = vec2(x, y) * texelSize * pcfRadius;
				float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
				shadow += (currentDepth - shadowBias > pcfDepth) ? 1.0 : 0.0;
				samples++;
			}
		}
	}
	
	// 计算平均阴影值 (0.0 = 完全不在阴影中, 1.0 = 完全在阴影中)
	shadow /= float(samples);
	
	return shadow;
}

// 阴影计算函数
float ShadowCalculation(vec4 fragPosLightSpace)
{
	if (enablePCF) {
		return PCFShadowCalculation(fragPosLightSpace);
	} else {
		return HardShadowCalculation(fragPosLightSpace);
	}


}

void main(void)
{
	if (isLightSource) {
        fragColor = material.diffuse;  // 直接使用材质颜色
        return;
    }

    vec3 N = normalize(varyingNormal);
    vec3 L = normalize(varyingLightDir);
    vec3 H = normalize(varyingHalfVec);

	// 计算阴影
	float shadow = ShadowCalculation(shadow_coord);
	float lightIntensity = 1.0 - shadow; // 光照强度：0.0=完全阴影, 1.0=完全光照
	// 环境光
	fragColor = globalAmbient * material.ambient + light.ambient * material.ambient;

	// 漫反射
	fragColor += light.diffuse * material.diffuse * max(dot(N, L), 0.0) * lightIntensity;
	
	// 镜面反射
	fragColor += light.specular * material.specular * 
				pow(max(dot(N, H), 0.0), material.shininess) * lightIntensity;
}