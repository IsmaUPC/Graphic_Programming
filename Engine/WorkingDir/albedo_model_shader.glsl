///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
#ifdef ALBEDO_MODEL

#if defined(VERTEX) ///////////////////////////////////////////////////

struct Light
{
    unsigned int type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
//layout(location = 3) in vec3 aTangent;
//layout(location = 4) in vec3 aBitangent;

layout(binding = 0, std140) uniform GlobalParams
{
	mat4 uCameraPosition;
	mat4 uLightCount;
	Light uLight[16];
};
layout(binding = 1, std140) uniform LocalParams
{
	mat4 uWorldMatrix;
	mat4 uWorldViewProjectionMatrix;

};

out vec2 vTexCoord;
out vec3 vPosition;
out vec3 vNormal;
out vec3 vViewDir;


void main()
{
	vTexCoord = aTexCoord;
	// We will usually not define the clipping scale manually...
	// it is usually conputed by the pojection matrix, Because
	// we are not clipping scale so that Patrick fits the screen.

	vPosition = vec3( uWorldMatrix * vec4(aPosition, 1.0));
	vNormal =	vec3( uWorldMatrix * vec4(aNormal, 0.0));

	gl_Position = uWorldViewProjectionMatrix * vec4(aPosition, 1.0);

	//float clippingScale = 5.0;
	//gl_Position = vec4(aPosition, clippingScale);

	// Patrick looks away from the camera by default, so i flip it here.
	//gl_Position.z = -gl_Position.z;

}
//TODO: Seguir desde la slide 3 del pdf 3.Rendering of meshes

#elif defined(FRAGMENT) ///////////////////////////////////////////////


in vec2 vTexCoord;
in vec3 vPosition;
in vec3 vNormal;
in vec3 vViewDir;


uniform sampler2D uTexture;

layout(location=0) out vec4 oColor;

void main()
{
	oColor= texture(uTexture,vTexCoord);
	
}

#endif
#endif


// NOTE: You can write several shaders in the same file if you want as
// long as you embrace them within an #ifdef block (as you can see above).
// The third parameter of the LoadProgram function in engine.cpp allows
// chosing the shader you want to load by name.
