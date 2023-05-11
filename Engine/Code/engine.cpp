//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
	GLchar  infoLogBuffer[1024] = {};
	GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
	GLsizei infoLogSize;
	GLint   success;

	char versionString[] = "#version 430\n";
	char shaderNameDefine[128];
	sprintf(shaderNameDefine, "#define %s\n", shaderName);
	char vertexShaderDefine[] = "#define VERTEX\n";
	char fragmentShaderDefine[] = "#define FRAGMENT\n";

	const GLchar* vertexShaderSource[] = {
		versionString,
		shaderNameDefine,
		vertexShaderDefine,
		programSource.str
	};
	const GLint vertexShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(vertexShaderDefine),
		(GLint)programSource.len
	};
	const GLchar* fragmentShaderSource[] = {
		versionString,
		shaderNameDefine,
		fragmentShaderDefine,
		programSource.str
	};
	const GLint fragmentShaderLengths[] = {
		(GLint)strlen(versionString),
		(GLint)strlen(shaderNameDefine),
		(GLint)strlen(fragmentShaderDefine),
		(GLint)programSource.len
	};

	GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
	glCompileShader(vshader);
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
	glCompileShader(fshader);
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	GLuint programHandle = glCreateProgram();
	glAttachShader(programHandle, vshader);
	glAttachShader(programHandle, fshader);
	glLinkProgram(programHandle);
	glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
		ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
	}

	glUseProgram(0);

	glDetachShader(programHandle, vshader);
	glDetachShader(programHandle, fshader);
	glDeleteShader(vshader);
	glDeleteShader(fshader);

	return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
	String programSource = ReadTextFile(filepath);

	Program program = {};
	program.handle = CreateProgramFromSource(programSource, programName);
	program.filepath = filepath;
	program.programName = programName;
	program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);
	app->programs.push_back(program);

	return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
	Image img = {};
	stbi_set_flip_vertically_on_load(true);
	img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
	if (img.pixels)
	{
		img.stride = img.size.x * img.nchannels;
	}
	else
	{
		ELOG("Could not open file %s", filename);
	}
	return img;
}

void FreeImage(Image image)
{
	stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
	GLenum internalFormat = GL_RGB8;
	GLenum dataFormat = GL_RGB;
	GLenum dataType = GL_UNSIGNED_BYTE;

	switch (image.nchannels)
	{
	case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
	case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
	default: ELOG("LoadTexture2D() - Unsupported number of channels");
	}

	GLuint texHandle;
	glGenTextures(1, &texHandle);
	glBindTexture(GL_TEXTURE_2D, texHandle);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
	for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
		if (app->textures[texIdx].filepath == filepath)
			return texIdx;

	Image image = LoadImage(filepath);

	if (image.pixels)
	{
		Texture tex = {};
		tex.handle = CreateTexture2DFromImage(image);
		tex.filepath = filepath;

		u32 texIdx = app->textures.size();
		app->textures.push_back(tex);

		FreeImage(image);
		return texIdx;
	}
	else
	{
		return UINT32_MAX;
	}
}

void Init(App* app)
{
	// TODO: Initialize your resources here!
	// - vertex buffers
	// - element/index buffers
	// - vaos
	// - programs (and retrieve uniform indices)
	// - textures

	glGenBuffers(1, &app->embeddedVertices);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glGenBuffers(1, &app->embeddedElements);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	glGenVertexArrays(1, &app->vao);
	glBindVertexArray(app->vao);
	glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);

	//los strides saltan de vertice a UV
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
	glEnableVertexAttribArray(0);
	// para saber la longitud del float
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
	glBindVertexArray(0);

	// �qui pones el nombre del programa(shader)
	//app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
	//Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
	//app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");
	
	app->texturedMeshProgramIdx = LoadProgram(app, "albedo_model_shader.glsl", "ALBEDO_MODEL");
	Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
	texturedMeshProgram.vertexInputLayout.attributes.push_back({0,3}); // position
	texturedMeshProgram.vertexInputLayout.attributes.push_back({2,2}); // texCoord
	// TODO: 11/05/23: Slide 8 to PDF 3. Rendering of meshes

	app->info.glVersion = reinterpret_cast<const char*> (glGetString(GL_VERSION));
	app->info.glRender = reinterpret_cast<const char*>  (glGetString(GL_RENDERER));
	app->info.glVendor = reinterpret_cast<const char*>  (glGetString(GL_VENDOR));
	app->info.glShadingVersion = reinterpret_cast<const char*>  (glGetString(GL_SHADING_LANGUAGE_VERSION));

	app->whiteTexIdx = LoadTexture2D(app, "color_white.png");

	GLint numExtensions = 0;
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	for (GLint i = 0; i < numExtensions; i++)
	{
		app->info.glExternsion.push_back(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, GLuint(i))));
	}

	app->mode = Mode_TexturedQuad;


}

void Gui(App* app)
{
	ImGui::Begin("Info");
	ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
	ImGui::End();
	if (app->input.keys[Key::K_SPACE] == ButtonState::BUTTON_PRESS) {
		ImGui::OpenPopup("openGL Info");
	}

	if (ImGui::BeginPopup("openGL Info"))
	{

		ImGui::Text("Version: %s", app->info.glVersion.c_str());
		ImGui::Text("Renderer: %s", app->info.glRender.c_str());
		ImGui::Text("Vendor: %s", app->info.glVendor.c_str());
		ImGui::Text("GLSL Version: %s", app->info.glShadingVersion.c_str());


		ImGui::Separator();

		for (size_t i = 0; i < app->info.glShadingVersion.size(); i++)
		{
			ImGui::Text("%s", app->info.glShadingVersion.c_str());
		}
		ImGui::EndPopup();
	}
}

void Update(App* app)
{
	// You can handle app->input keyboard/mouse here


}

void Render(App* app)
{
	switch (app->mode)
	{
	case Mode_TexturedQuad:
	{
		int success;
		char infoLog[1024];

		// TODO: Draw your textured quad here!
		// - clear the framebuffer
		glClearColor(0.0f, 0.0f, 0.0f, 0.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// - set the viewport
		glViewport(0, 0, app->displaySize.x, app->displaySize.y);

		// - set the blending state	
		// - bind the texture into unit 0
		// - bind the program 
		//   (...and make its texture sample from unit 0)
		// - bind the vao

		Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];

		glGetProgramiv(programTexturedGeometry.handle, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(programTexturedGeometry.handle, 1024,NULL, infoLog);
			ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", programTexturedGeometry.programName, infoLog);

		}

		glUseProgram(programTexturedGeometry.handle);
		glBindVertexArray(app->vao);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glUniform1i(app->programUniformTexture, 0);
		glActiveTexture(GL_TEXTURE0);
		GLuint textureHandle = app->textures[app->diceTexIdx].handle;
		glBindTexture(GL_TEXTURE_2D, textureHandle);


		// - glDrawElements() !!!
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

		glBindVertexArray(0);
		glUseProgram(0);
	}
	break;
	 
	default:;
	}
}

