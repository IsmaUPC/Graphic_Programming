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

#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>



u32 Align(u32 value, u32 alignment)
{
    return(value + alignment - 1) & ~(alignment - 1);
}
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
        assert(success);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
        assert(success);
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
        assert(success);
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




void ProcessAssimpMesh(const aiScene* scene, aiMesh* mesh, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
    std::vector<float> vertices;
    std::vector<u32> indices;

    bool hasTexCoords = false;
    bool hasTangentSpace = false;

    // process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);
        vertices.push_back(mesh->mNormals[i].x);
        vertices.push_back(mesh->mNormals[i].y);
        vertices.push_back(mesh->mNormals[i].z);

        if (mesh->mTextureCoords[0]) // does the mesh contain texture coordinates?
        {
            hasTexCoords = true;
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        }

        if (mesh->mTangents != nullptr && mesh->mBitangents)
        {
            hasTangentSpace = true;
            vertices.push_back(mesh->mTangents[i].x);
            vertices.push_back(mesh->mTangents[i].y);
            vertices.push_back(mesh->mTangents[i].z);

            // For some reason ASSIMP gives me the bitangents flipped.
            // Maybe it's my fault, but when I generate my own geometry
            // in other files (see the generation of standard assets)
            // and all the bitangents have the orientation I expect,
            // everything works ok.
            // I think that (even if the documentation says the opposite)
            // it returns a left-handed tangent space matrix.
            // SOLUTION: I invert the components of the bitangent here.
            vertices.push_back(-mesh->mBitangents[i].x);
            vertices.push_back(-mesh->mBitangents[i].y);
            vertices.push_back(-mesh->mBitangents[i].z);
        }
    }

    // process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
        {
            indices.push_back(face.mIndices[j]);
        }
    }

    // store the proper (previously proceessed) material for this mesh
    submeshMaterialIndices.push_back(baseMeshMaterialIndex + mesh->mMaterialIndex);

    // create the vertex format
    VertexBufferLayout vertexBufferLayout = {};
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 0, 3, 0 });
    vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 1, 3, 3 * sizeof(float) });
    vertexBufferLayout.stride = 6 * sizeof(float);
    if (hasTexCoords)
    {
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 2, 2, vertexBufferLayout.stride });
        vertexBufferLayout.stride += 2 * sizeof(float);
    }
    if (hasTangentSpace)
    {
        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 3, 3, vertexBufferLayout.stride });
        vertexBufferLayout.stride += 3 * sizeof(float);

        vertexBufferLayout.attributes.push_back(VertexBufferAttribute{ 4, 3, vertexBufferLayout.stride });
        vertexBufferLayout.stride += 3 * sizeof(float);
    }

    // add the submesh into the mesh
    Submesh submesh = {};
    submesh.vertexBufferLayout = vertexBufferLayout;
    submesh.vertices.swap(vertices);
    submesh.indices.swap(indices);
    myMesh->submeshes.push_back(submesh);
}

void ProcessAssimpMaterial(App* app, aiMaterial* material, Material& myMaterial, String directory)
{
    aiString name;
    aiColor3D diffuseColor;
    aiColor3D emissiveColor;
    aiColor3D specularColor;
    ai_real shininess;
    material->Get(AI_MATKEY_NAME, name);
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseColor);
    material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor);
    material->Get(AI_MATKEY_COLOR_SPECULAR, specularColor);
    material->Get(AI_MATKEY_SHININESS, shininess);

    myMaterial.name = name.C_Str();
    myMaterial.albedo = vec3(diffuseColor.r, diffuseColor.g, diffuseColor.b);
    myMaterial.emissive = vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
    myMaterial.smoothness = shininess / 256.0f;

    aiString aiFilename;
    if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
    {
        material->GetTexture(aiTextureType_DIFFUSE, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.albedoTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_EMISSIVE) > 0)
    {
        material->GetTexture(aiTextureType_EMISSIVE, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.emissiveTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_SPECULAR) > 0)
    {
        material->GetTexture(aiTextureType_SPECULAR, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.specularTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_NORMALS) > 0)
    {
        material->GetTexture(aiTextureType_NORMALS, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.normalsTextureIdx = LoadTexture2D(app, filepath.str);
    }
    if (material->GetTextureCount(aiTextureType_HEIGHT) > 0)
    {
        material->GetTexture(aiTextureType_HEIGHT, 0, &aiFilename);
        String filename = MakeString(aiFilename.C_Str());
        String filepath = MakePath(directory, filename);
        myMaterial.bumpTextureIdx = LoadTexture2D(app, filepath.str);
    }

    //myMaterial.createNormalFromBump();
}

void ProcessAssimpNode(const aiScene* scene, aiNode* node, Mesh* myMesh, u32 baseMeshMaterialIndex, std::vector<u32>& submeshMaterialIndices)
{
    // process all the node's meshes (if any)
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessAssimpMesh(scene, mesh, myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
    }

    // then do the same for each of its children
    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        ProcessAssimpNode(scene, node->mChildren[i], myMesh, baseMeshMaterialIndex, submeshMaterialIndices);
    }
}

u32 LoadModel(App* app, const char* filename)
{
    const aiScene* scene = aiImportFile(filename,
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals |
        aiProcess_CalcTangentSpace |
        aiProcess_JoinIdenticalVertices |
        aiProcess_PreTransformVertices |
        aiProcess_ImproveCacheLocality |
        aiProcess_OptimizeMeshes |
        aiProcess_SortByPType);

    if (!scene)
    {
        ELOG("Error loading mesh %s: %s", filename, aiGetErrorString());
        return UINT32_MAX;
    }

    app->meshes.push_back(Mesh{});
    Mesh& mesh = app->meshes.back();
    u32 meshIdx = (u32)app->meshes.size() - 1u;

    app->models.push_back(Model{});
    Model& model = app->models.back();
    model.meshIdx = meshIdx;
    u32 modelIdx = (u32)app->models.size() - 1u;

    String directory = GetDirectoryPart(MakeString(filename));

    // Create a list of materials
    u32 baseMeshMaterialIndex = (u32)app->materials.size();
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i)
    {
        app->materials.push_back(Material{});
        Material& material = app->materials.back();
        ProcessAssimpMaterial(app, scene->mMaterials[i], material, directory);
    }

    ProcessAssimpNode(scene, scene->mRootNode, &mesh, baseMeshMaterialIndex, model.materialIdx);

    aiReleaseImport(scene);

    u32 vertexBufferSize = 0;
    u32 indexBufferSize = 0;

    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
    {
        vertexBufferSize += mesh.submeshes[i].vertices.size() * sizeof(float);
        indexBufferSize += mesh.submeshes[i].indices.size() * sizeof(u32);
    }

    glGenBuffers(1, &mesh.vertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, NULL, GL_STATIC_DRAW);

    glGenBuffers(1, &mesh.indexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, NULL, GL_STATIC_DRAW);

    u32 indicesOffset = 0;
    u32 verticesOffset = 0;

    for (u32 i = 0; i < mesh.submeshes.size(); ++i)
    {
        const void* verticesData = mesh.submeshes[i].vertices.data();
        const u32   verticesSize = mesh.submeshes[i].vertices.size() * sizeof(float);
        glBufferSubData(GL_ARRAY_BUFFER, verticesOffset, verticesSize, verticesData);
        mesh.submeshes[i].vertexOffset = verticesOffset;
        verticesOffset += verticesSize;

        const void* indicesData = mesh.submeshes[i].indices.data();
        const u32   indicesSize = mesh.submeshes[i].indices.size() * sizeof(u32);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, indicesOffset, indicesSize, indicesData);
        mesh.submeshes[i].indexOffset = indicesOffset;
        indicesOffset += indicesSize;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return modelIdx;
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

    // Àqui pones el nombre del programa(shader)
    //app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    //Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    //app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");


    // Slide 5 {

    ////// Camera

    app->camerai = new Camerai();
    app->camerai->pos = vec3(10, 0, 0);
    app->camerai->target = vec3(0, 0, 0);
    app->camerai->camDir = app->camerai->pos - app->camerai->target;
    app->camerai->rCamVec = glm::normalize(glm::cross(app->camerai->upGlobalVec, app->camerai->camDir));
    app->camerai->upCamVec = glm::cross(app->camerai->camDir, app->camerai->rCamVec);

    ////////////////////// Calcular la projection y la view

    float aspectRatio = (float)app->displaySize.x / (float)app->displaySize.y;
    float znear = 0.1f;
    float zfar = 1000.0f;
    app->worlViewProjectiondMatrix = glm::perspective(glm::radians(60.0f), aspectRatio, znear, zfar);
    app->worldMatrix = glm::lookAt(app->camerai->pos, app->camerai->target, app->camerai->upGlobalVec);

    //////

    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);

    glGenBuffers(1, &app->bufferHandle);
    glBindBuffer(GL_UNIFORM_BUFFER, app->bufferHandle);
    glBufferData(GL_UNIFORM_BUFFER, app->maxUniformBufferSize, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    // } Slide 5 

    GLint attributeCount;
    app->texturedMeshProgramIdx = LoadProgram(app, "albedo_model_shader.glsl", "ALBEDO_MODEL");
    Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    glGetProgramiv(texturedMeshProgram.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    GLchar attributeName[32];
    GLsizei attributeNameLength;
    GLint attributeSize;
    GLenum attributeType;
    u8 attributeLocation;

    for (int i = 0; i < attributeCount; i++)
    {
        glGetActiveAttrib(texturedMeshProgram.handle, i,
            ARRAY_COUNT(attributeName),&attributeNameLength,
            &attributeSize,&attributeType, attributeName);

        attributeLocation = glGetAttribLocation(texturedMeshProgram.handle, attributeName);

        texturedMeshProgram.vertexInputLayout.attributes.push_back(VertexShaderAttribute(attributeLocation,));


    }


    Entity entity1;
    app->model = LoadModel(app, "Patrick/Patrick.obj");
    entity1.modelIndex = app->model;
    entity1.worldMatrix = glm::lookAt(vec3(0), vec3(1,0,0), vec3(0, 1, 0));
    app->entities.push_back(entity1);

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

    //app->mode = Mode_TexturedQuad;
    app->mode = Mode_Mesh;

}

u8 GetComponentCount(GLenum attributeType)
{
    switch (attributeType)
    {
    default:
        break;  

    }
}

void Gui(App* app)
{
    ImGui::Begin("Info");
    ImGui::Text("FPS: %f", 1.0f / app->deltaTime);
    ImGui::End();
    ImGui::Begin("Camera");
    ImGui::Text("X: %f | y: % f | z: %f", app->camerai->pos.x, app->camerai->pos.y, app->camerai->pos.z);
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

        //for (size_t i = 0; i < app->info.glShadingVersion.size(); i++)
        //{
        //	ImGui::Text("%s", app->info.glShadingVersion.c_str());
        //}
        ImGui::EndPopup();
    }
}

void Update(App* app)
{
    // You can handle app->input keyboard/mouse here
    const float cameraSpeed = 0.5f; // adjust accordingly
    if (app->input.keys[Key::K_W] == ButtonState::BUTTON_PRESS) {
        app->camerai->pos += cameraSpeed * app->camerai->camDir;
    }
    if (app->input.keys[Key::K_S] == ButtonState::BUTTON_PRESS) {
        app->camerai->pos -= cameraSpeed * app->camerai->camDir;
    }
    if (app->input.keys[Key::K_A] == ButtonState::BUTTON_PRESS) {
        app->camerai->pos -= glm::normalize(glm::cross(app->camerai->camDir, app->camerai->upCamVec)) * cameraSpeed;
    }
    if (app->input.keys[Key::K_D] == ButtonState::BUTTON_PRESS) {
        app->camerai->pos += glm::normalize(glm::cross(app->camerai->camDir, app->camerai->upCamVec)) * cameraSpeed;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, app->bufferHandle);
    u8* bufferData = (u8*)glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
    u32 bufferHead = 0;

    for (int i = 0; i < app->entities.size(); i++)
    {
        bufferHead = Align(bufferHead, app->uniformBlockAlignment);
        app->entities[i].localParamsOffset = bufferHead;

        memcpy(bufferData + bufferHead, glm::value_ptr(app->worldMatrix), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);
        memcpy(bufferData + bufferHead, glm::value_ptr(app->worlViewProjectiondMatrix), sizeof(glm::mat4));
        bufferHead += sizeof(glm::mat4);

        app->entities[i].localParamsSize = bufferHead - app->entities[i].localParamsOffset;
    }
    glUnmapBuffer(GL_UNIFORM_BUFFER);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

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
            for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
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
            glGetProgramInfoLog(programTexturedGeometry.handle, 1024, NULL, infoLog);
            ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", programTexturedGeometry.programName.c_str(), infoLog);

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
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);

        glBindVertexArray(0);
        glUseProgram(0);
    }
    break;
    case Mode_Mesh:
    {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
        glUseProgram(texturedMeshProgram.handle);

        Model& model = app->models[app->model];
        Mesh& mesh = app->meshes[model.meshIdx];

        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, texturedMeshProgram);
            glBindVertexArray(vao);
            app->blockOffset = 0;
            app->blockSize = sizeof(glm::mat4) * 2;
            glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->bufferHandle, app->blockOffset, app->blockSize);

            u32 submeshMaterialIdx = model.materialIdx[i];
            Material& submeshMaterial = app->materials[submeshMaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);
            glUniform1i(app->texturedMeshProgram_uTexture, 0);

            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
        }
        glUseProgram(0);
    }
    break;

    default:;
    }
}

