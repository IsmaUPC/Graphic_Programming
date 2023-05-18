//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once


#define BINDING(b) b

#include "platform.h"
#include <glad/glad.h>


#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;
typedef glm::mat4 mat4;



class Camerai
{
public:
	vec3 pos ;
	vec3 upGlobalVec;
	vec3 rCamVec;
	vec3 upCamVec;
	vec3 camDir;
	
	const float cameraSpeed = 0.05f;

	Camerai() 
	{
		pos = { 0,0,0 };
		upGlobalVec = { 0,1,0 };
		rCamVec = { 1,0,0 };
		camDir = { 0,0,-1 };
	}

};

struct Entity
{
	glm::mat4 worldMatrix;
	u32 modelIndex;
	u32 localParamsOffset;
	u32 localParamsSize;
};

struct Image
{
	void* pixels;
	ivec2 size;
	i32   nchannels;
	i32   stride;
};

struct Texture
{
	GLuint      handle;
	std::string filepath;
};

enum Mode
{
	Mode_TexturedQuad,
	Mode_Mesh,
	Mode_Count
};


struct OpenFLInfo {

	std::string glVersion;
	std::string glRender;
	std::string glVendor;
	std::string glShadingVersion;

	std::vector <std::string> glExternsion;
};

struct VertexV3V2 {
	glm::vec3 pos;
	glm::vec2 uv;
};

const VertexV3V2 vertices[] = {
	{glm::vec3(-0.5,-0.5,0.0),glm::vec2(0.0,0.0)},
	{glm::vec3(0.5,-0.5,0.0),glm::vec2(1.0,0.0)},
	{glm::vec3(0.5,0.5,0.0),glm::vec2(1.0,1.0)},
	{glm::vec3(-0.5,0.5,0.0),glm::vec2(0.0,1.0)},

};


struct VertexBufferAttribute
{
	u8 location;
	u8 componentsCount;
	u8 offset;
};

struct VertexBufferLayout
{
	std::vector<VertexBufferAttribute> attributes;
	u8 stride;
};

struct VertexShaderAttribute
{
	u8 location;
	u8 componentCount;
	VertexShaderAttribute(u8 _location, u8 _componentCount) {
		location = _location;
		componentCount = _componentCount;
	};
};

struct VertexShaderLayout
{
	std::vector<VertexShaderAttribute> attributes;
};
struct Program
{
	GLuint             handle;
	std::string        filepath;
	std::string        programName;
	u64                lastWriteTimestamp; // What is this for?
	//VertexShaderAttribute  vertexInputLayout; // Old
	VertexShaderLayout  vertexInputLayout;
};

struct Vao
{
	GLuint handle;
	GLuint programHandle;
};

struct Model {
	u32 meshIdx;
	std::vector<u32> materialIdx;
};
struct Submesh
{
	VertexBufferLayout vertexBufferLayout;
	std::vector<float> vertices;
	std::vector<u32> indices;
	u32 vertexOffset;
	u32 indexOffset;

	std::vector<Vao> vaos;
};

struct Mesh
{
	std::vector<Submesh> submeshes;
	GLuint vertexBufferHandle;
	GLuint indexBufferHandle;
};

struct Material
{
	std::string name;
	vec3 albedo;
	vec3 emissive;
	f32 smoothness;
	u32 albedoTextureIdx;
	u32 emissiveTextureIdx;
	u32 specularTextureIdx;
	u32 normalsTextureIdx;
	u32 bumpTextureIdx;
};

struct Light
{
	int type;
	vec3 color;
	vec3 position;
	float range;

};

const u16 indices[] = {
   0,1,2,
   0,2,3

};


u32 Align(u32 value, u32 alignment);

u8 GetComponentCount(GLenum attributeType);

struct App
{

	mat4 worlViewProjectiondMatrix;
	mat4 worldMatrix;
	GLuint bufferHandle;
	Camerai* camerai;

	// Loop
	f32  deltaTime;
	bool isRunning;

	// Input
	Input input;
	bool firstMouse = true;
	float lastX;
	float lastY;

	float yaw= -90.0f;
	float pitch ;


	// Graphics
	char gpuName[64];
	char openGlVersion[64];

	ivec2 displaySize;

	std::vector<Texture>  textures;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Model> models;
	std::vector<Program>  programs;

	std::vector<Entity> entities;

	// program indices
	u32 texturedGeometryProgramIdx;
	u32 texturedMeshProgramIdx;
	u32 model;

	// texture indices
	u32 diceTexIdx;
	u32 whiteTexIdx;
	u32 blackTexIdx;
	u32 normalTexIdx;
	u32 magentaTexIdx;

	u32 blockOffset;
	u32 blockSize;

	// Mode
	Mode mode;

	// Embedded geometry (in-editor simple meshes such as
	// a screen filling quad, a cube, a sphere...)
	GLuint embeddedVertices;
	GLuint embeddedElements;

	// Location of the texture uniform in the textured quad shader
	GLuint programUniformTexture;
	GLuint texturedMeshProgram_uTexture;


	GLint maxUniformBufferSize;
	GLint uniformBlockAlignment;

	OpenFLInfo info;

	// VAO object to link our screen filling quad with our textured quad shader
	GLuint vao;
};


void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

