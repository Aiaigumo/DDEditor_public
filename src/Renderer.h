#pragma once
#include <glad/glad.h>
#include "EditorContext.h"

enum class Color {
	RED,
	RED_LN,		//for longnote
	BLUE,
	BLUE_LN,	//for longnote
	WHITE,
	WHITE_PL,	//for perspective view line
	CYAN,
	YELLOW,
	YELLOW_SL,	//for selected note highlight
	PURPLE_OBS,
	LIGHTGREEN,
	ORANGE
};

// Owns OpenGL resources and renders the editor scene into a framebuffer.
class Renderer {
public:
	Renderer(const char* vertexPath, const char* fragmentPath, bool _isPerspective);
	~Renderer() = default;
	void initialize(const char* vertexPath, const char* fragmentPath);
	void initFBO(EditorContext& es);
	GLuint renderFrame(const EditorContext& es);

private:
	void loadShaders(const char* vertexPath, const char* fragmentPath);
	void setVertexAttribPointers();
	void initVertexData();
	void changeColor(const GLint& locColor, Color c);

	bool isPerspective;

	GLuint shader;

	//uniform locations
	GLuint locModel;
	GLuint locView;
	GLuint locProjection;
	GLuint locColor;

	// framebuffer object and its texture and renderbuffer object
	GLuint fbo;
	GLuint fboTexture;
	GLuint rbo;
	
	// Matrices
	glm::mat4 modelMatrix;
	glm::mat4 viewMatrix;
	glm::mat4 projectionMatrix;

	//basic shapes VAO,VBO,EBO
	GLuint sphereVAO, lineVAO, cubeVAO, sphereVBO, lineVBO, cubeVBO, sphereEBO, cubeEBO;
	int sphereIndicesNum;
	int cubeIndicesNum;

	//sphere data generation properties
	static const inline int X_SEGMENTS = 32;
	static const inline int Y_SEGMENTS = 16;
	static const inline float PI = 3.14159265359f;

};
