//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include <glad/glad.h>

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

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

struct Program
{
	GLuint             handle;
	std::string        filepath;
	std::string        programName;
	u64                lastWriteTimestamp; // What is this for?
	//VertexShaderAttribute  vertexInputLayout; // Old
	VertexShaderLayout  vertexInputLayout;
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
};

struct VertexShaderLayout
{
	std::vector<VertexShaderAttribute> attributes;
};

struct Vao
{
	GLuint handle;
	GLuint programHandle;
};


const u16 indices[] = {
   0,1,2,
   0,2,3

};
struct App
{
	// Loop
	f32  deltaTime;
	bool isRunning;

	// Input
	Input input;

	// Graphics
	char gpuName[64];
	char openGlVersion[64];

	ivec2 displaySize;

	std::vector<Texture>  textures;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Model> models;
	std::vector<Program>  programs;



	// program indices
	u32 texturedGeometryProgramIdx;
	u32 texturedMeshProgramIdx;

	// texture indices
	u32 diceTexIdx;
	u32 whiteTexIdx;
	u32 blackTexIdx;
	u32 normalTexIdx;
	u32 magentaTexIdx;

	// Mode
	Mode mode;

	// Embedded geometry (in-editor simple meshes such as
	// a screen filling quad, a cube, a sphere...)
	GLuint embeddedVertices;
	GLuint embeddedElements;

	// Location of the texture uniform in the textured quad shader
	GLuint programUniformTexture;

	GLuint texturedMeshProgram_uTexture;

	OpenFLInfo info;

	// VAO object to link our screen filling quad with our textured quad shader
	GLuint vao;
};

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
	Submesh& submesh = mesh.submeshes[submeshIndex];

	// Try finding a vao for this submesh/program
	for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
		if (submesh.vaos[i].programHandle == program.handle)
			return submesh.vaos[i].handle;

	GLuint vaoHandle = 0;

	// Create a new vao for this submesh/program
	{
		glGenVertexArrays(1, &vaoHandle);
		glBindVertexArray(vaoHandle);

		glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

		// We have to link all vertex inputs attributes to attributes in the vertex buffer
		for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
		{
			bool attributeWasLinked = false;
			for (u32 j = 8; j < submesh.vertexBufferLayout.attributes.size(); ++j)
			{
				if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
				{
					const u32 index = submesh.vertexBufferLayout.attributes[j].location;
					const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentsCount;
					const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset; // attribute offset + vertex offset
					const u32 stride = submesh.vertexBufferLayout.stride;
					glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
					glEnableVertexAttribArray(index);

					attributeWasLinked = true;
					break;
				}
			}
			assert(attributeWasLinked); // The submesh should provide an attribute for each vertex inputs
		}

		glBindVertexArray(0);
	}


	// Store it in the list of vaos for this submesh
	Vao vao = { vaoHandle, program.handle };
	submesh.vaos.push_back(vao);

	return vaoHandle;
}

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

