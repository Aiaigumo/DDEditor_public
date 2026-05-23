#include "Renderer.h"
#include <GLFW/glfw3.h>
#include <gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

void Renderer::changeColor(const GLint& _locColor, Color c) {
	switch (c) {
	case Color::RED:
		glUniform4f(_locColor, 1.0f, 0.1f, 0.1f, 0.8f);
		break;
	case Color::RED_LN:
		glUniform4f(_locColor, 1.0f, 0.1f, 0.1f, 0.3f);
		break;
	case Color::BLUE:
		glUniform4f(_locColor, 0.0f, 0.7f, 1.0f, 0.8f);
		break;
	case Color::BLUE_LN:
		glUniform4f(_locColor, 0.0f, 0.7f, 1.0f, 0.3f);
		break;
	case Color::WHITE:
		glUniform4f(_locColor, 1.0f, 1.0f, 1.0f, 0.8f);
		break;
	case Color::WHITE_PL:
		glUniform4f(_locColor, 1.0f, 1.0f, 1.0f, 0.3f);
		break;
	case Color::CYAN:
		glUniform4f(_locColor, 0.1f, 1.0f, 1.0f, 0.8f);
		break;
	case Color::YELLOW:
		glUniform4f(_locColor, 1.0f, 1.0f, 0.1f, 0.8f);
		break;
	case Color::YELLOW_SL:
		glUniform4f(_locColor, 1.0f, 1.0f, 0.1f, 0.5f);
		break;
	case Color::PURPLE_OBS:
		glUniform4f(_locColor, 1.0f, 0.1f, 1.0f, 0.3f);
		break;
	case Color::LIGHTGREEN:
		glUniform4f(_locColor, 0.1f, 1.0f, 0.1f, 0.8f);
		break;
	case Color::ORANGE:
		glUniform4f(_locColor, 1.0f, 0.6f, 0.0f, 0.8f);
		break;
	default:
		glUniform4f(_locColor, 1.0f, 0.1f, 0.1f, 1.0f);
	}
	return;
}

Renderer::Renderer(const char* vertexPath, const char* fragmentPath, bool _isPerspective) {
	initialize(vertexPath, fragmentPath);
	isPerspective = _isPerspective;
	initVertexData();
}

void Renderer::initialize(const char* vertexPath, const char* fragmentPath) {
	//disable V-Sync
	glfwSwapInterval(0);

	glEnable(GL_BLEND);									// Enable blending for transparency
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	//load shaders
	loadShaders(vertexPath, fragmentPath);
	//uniform locations
	locModel = glGetUniformLocation(shader, "uModel");
	locView = glGetUniformLocation(shader, "uView");
	locProjection = glGetUniformLocation(shader, "uProjection");
	locColor = glGetUniformLocation(shader, "uColor");

	//initialize vertex data
	initVertexData();
}

void Renderer::loadShaders(const char* vertexPath, const char* fragmentPath) {
	//read file
	std::string vertexCode;
	std::string fragmentCode;

	std::ifstream vShaderFile(vertexPath);
	std::ifstream fShaderFile(fragmentPath);

	std::stringstream vStream, fStream;
	vStream << vShaderFile.rdbuf();
	fStream << fShaderFile.rdbuf();

	vertexCode = vStream.str();
	fragmentCode = fStream.str();

	const char* vSrc = vertexCode.c_str();
	const char* fSrc = fragmentCode.c_str();

	//compile vertex shader
	GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vShader, 1, &vSrc, NULL);
	glCompileShader(vShader);

	GLint success;
	glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(vShader, 512, NULL, log);
		std::cout << "Vertex Shader Error:\n" << log << std::endl;
	}

	//compile fragment shader
	GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fShader, 1, &fSrc, NULL);
	glCompileShader(fShader);

	glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
	if (!success) {
		char log[512];
		glGetShaderInfoLog(fShader, 512, NULL, log);
		std::cout << "Fragment Shader Error:\n" << log << std::endl;
	}

	//create program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vShader);
	glAttachShader(shaderProgram, fShader);
	glLinkProgram(shaderProgram);


	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) {
		char log[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, log);
		std::cout << "Shader Program Linking Error:\n" << log << std::endl;
	}

	glDeleteShader(vShader);
	glDeleteShader(fShader);

	shader = shaderProgram;
}

void Renderer::setVertexAttribPointers()
{
	//aPos (location=0)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(
		0, 3, GL_FLOAT, GL_FALSE,
		3 * sizeof(float), (void*)0
	);
}

void Renderer::initVertexData() {
	//generate VAO, VBO, EBO for basic shapes (sphere, line, cube)

	glGenVertexArrays(1, &sphereVAO);
	glGenVertexArrays(1, &lineVAO);
	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &sphereVBO);
	glGenBuffers(1, &lineVBO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &sphereEBO);
	glGenBuffers(1, &cubeEBO);

	// sphere data
	std::vector<float> sphereVertices;
	std::vector<unsigned int> sphereIndices;

	// Generate sphere vertices
	for (int y = 0; y <= Y_SEGMENTS; ++y) {
		for (int x = 0; x <= X_SEGMENTS; ++x) {
			float xSeg = (float)x / X_SEGMENTS;
			float ySeg = (float)y / Y_SEGMENTS;

			float xPos = cos(xSeg * 2.0f * PI) * sin(ySeg * PI);
			float yPos = cos(ySeg * PI);
			float zPos = sin(xSeg * 2.0f * PI) * sin(ySeg * PI);

			sphereVertices.insert(sphereVertices.end(), { xPos, yPos, zPos });


		}
	}

	// Generate sphere indices
	for (int y = 0; y < Y_SEGMENTS; ++y) {
		for (int x = 0; x < X_SEGMENTS; ++x) {
			unsigned int i0 = y * (X_SEGMENTS + 1) + x;
			unsigned int i1 = i0 + X_SEGMENTS + 1;

			sphereIndices.insert(sphereIndices.end(), {
				i0, i1, i0 + 1,
				i0 + 1, i1, i1 + 1
				});
		}
	}
	sphereIndicesNum = static_cast<int>(sphereIndices.size());

	float cubeVertices[] = {
	-0.5f,-0.5f,-0.5f, // 0
	 0.5f,-0.5f,-0.5f, // 1
	 0.5f, 0.5f,-0.5f, // 2
	-0.5f, 0.5f,-0.5f, // 3
	-0.5f,-0.5f, 0.5f, // 4
	 0.5f,-0.5f, 0.5f, // 5
	 0.5f, 0.5f, 0.5f, // 6
	-0.5f, 0.5f, 0.5f  // 7
	};

	cubeIndicesNum = 36;
	unsigned int cubeIndices[] = {
	4,5,6, 6,7,4, // front
	0,3,2, 2,1,0, // back
	0,4,7, 7,3,0, // left
	1,2,6, 6,5,1, // right
	3,7,6, 6,2,3, // top
	0,1,5, 5,4,0  // bottom
	};

	// position(vec3)
	float lineVertices[] = {
		//  x,    y,   z, 
		 0.0f, 0.0f, 0.0f,
		 0.0f, 1.0f, 0.0f,
	};

	//shpere VBO/EBO setup
	glBindVertexArray(sphereVAO);
	glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
	glBufferData(GL_ARRAY_BUFFER, sphereVertices.size() * sizeof(float), sphereVertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sphereIndices.size() * sizeof(unsigned int), sphereIndices.data(), GL_STATIC_DRAW);
	setVertexAttribPointers();

	//cube VBO/EBO setup
	glBindVertexArray(cubeVAO);
	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
	setVertexAttribPointers();

	//line VBO setup
	glBindVertexArray(lineVAO);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
	setVertexAttribPointers();

	glBindVertexArray(0);
}

void Renderer::initFBO(EditorContext& es)
{
	int width, height;
	if (isPerspective)
	{
		width = es.psFBOWidth;
		height = es.psFBOHeight;
	}
	else
	{
		width = es.orFBOWidth;
		height = es.orFBOHeight;
	}

	//delete previous FBO, texture, RBO if exist
	if (fbo != 0) glDeleteFramebuffers(1, &fbo);
	if (fboTexture != 0) glDeleteTextures(1, &fboTexture);
	if (rbo != 0) glDeleteRenderbuffers(1, &rbo);

	// create FBO
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// create texture to attach to FBO
	glGenTextures(1, &fboTexture);
	glBindTexture(GL_TEXTURE_2D, fboTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
		0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0);

	// create RBO for depth and stencil attachment
	glGenRenderbuffers(1, &rbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cerr << "FBO Error\n";

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Renderer::renderFrame(const EditorContext& ec) {
	int width, height;
	glm::mat4 ViewMatrix;
	glm::mat4 ProjectionMatrix;
	float drawMaxDistance;
	if (isPerspective)
	{
		width = ec.psFBOWidth;
		height = ec.psFBOHeight;
		ViewMatrix = ec.viewPerspective;
		ProjectionMatrix = ec.projectionPerspective;
		drawMaxDistance = ec.perspectiveDrawMaxDistance;
	}
	else
	{
		width = ec.orFBOWidth;
		height = ec.orFBOHeight;
		ViewMatrix = ec.viewOrtho;
		ProjectionMatrix = ec.projectionOrtho;
		drawMaxDistance = ec.orthoDrawMaxDistance;
	}

//redering fbo
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);

	// background clear
	glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shader);

	// Model matrix
	glm::mat4 model = glm::mat4(1.0f);
	glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
	// View matrix
	glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(ViewMatrix));
	// Projection matrix
	glUniformMatrix4fv(locProjection, 1, GL_FALSE, glm::value_ptr(ProjectionMatrix));

	/* draw object*/
	// draw 11 lanes (from x=0 - 10 by 1)
	glBindVertexArray(lineVAO);		//choice what object to draw (choice VAO which is linked VBO(which has vertex info of object))
	changeColor(locColor, Color::WHITE_PL); // white
	for (int i = 0; i < 11; ++i)
	{
		//make thicker line width(left center right) 
		switch (i) {
		case 0:
		case 10:
			glLineWidth(5.0f);
			break;
		case 5:
			glLineWidth(2.0f);
			break;
		default:
			glLineWidth(1.0f);
			break;
		}

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(static_cast<float>(i), 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.0f, 1.0f, drawMaxDistance));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_LINES, 0, 2);

	}

	//draw timing lines
	glLineWidth(1.0f);
	for (float z : ec.timingLinesOnSubbeat) {

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, z));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f, 10.0f, 1.0f));
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_LINES, 0, 2);
	}

	glLineWidth(3.0f);
	for (float z : ec.timingLinesOnBeat) {

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, z));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f, 10.0f, 1.0f));
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_LINES, 0, 2);
	}

	//draw hitline frame
	if (isPerspective) {
		float drawZ = ec.editingZ;
		glLineWidth(1.0f);
		for (int i = 0; i < 11; i++) {
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(static_cast<float>(i), 0.0f, drawZ));
			model = glm::scale(model, glm::vec3(1.0f, 10.0f, 1.0f));
			glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_LINES, 0, 2);
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(0.0f, static_cast<float>(i), drawZ));
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(1.0f, 10.0f, 1.0f));
			glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
			glDrawArrays(GL_LINES, 0, 2);
		}
	}

	//draw hitline
	glLineWidth(5.0f);
	changeColor(locColor, Color::CYAN);
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, 0.0f, -ec.hitLineDistance));
	model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, glm::vec3(1.0f, 10.0f, 1.0f));
	glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
	glDrawArrays(GL_LINES, 0, 2);

	//draw timing start lines
	changeColor(locColor, Color::ORANGE);
	for (float z : ec.timingStartLines) {

		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, z));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(1.0f, 10.0f, 1.0f));
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_LINES, 0, 2);
	}

	//draw notes
	/* draw of sphere */
	glBindVertexArray(sphereVAO);
	changeColor(locColor, Color::RED);	// color set RED
	for (const glm::vec3& n : ec.lhNotes) {	// drawing left hand notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.handnotesScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, sphereIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::BLUE);	// color set BLUE
	for (const glm::vec3& n : ec.rhNotes) {	// drawing right hand notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.handnotesScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, sphereIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::YELLOW);	// color set YELLOW
	for (const glm::vec3& n : ec.yhNotes) {	// drawing yellow notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.handnotesScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, sphereIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::YELLOW_SL);	// color set YELLOW (selected)
	for (const glm::vec3& n : ec.slhNotes) {	// drawing yellow notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.handnotesScale + ec.selectedAdditionalScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, sphereIndicesNum, GL_UNSIGNED_INT, 0);
	}

	/* draw of cube */
	glBindVertexArray(cubeVAO);
	changeColor(locColor, Color::RED); // color set RED
	for (const glm::vec3& n : ec.lfNotes) {	// drawing left foot notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.footnotesScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::RED_LN); // color set transparent RED for long notes line
	for (size_t i = 0; i + 1 < ec.llfLines.size(); i++) {	//drawing long left foot notes (only lines)
		const glm::vec3& nextNodePos = ec.llfLines.at(i + 1);
		if (glm::isnan(nextNodePos.x)) {		//check dummy (parition between groups)
			i++;
			continue;
		}
		const glm::vec3& nodePos = ec.llfLines.at(i);


		// calc model matrix
		glm::vec3 linePos = (nodePos + nextNodePos) * 0.5f;
		glm::vec3 lineScale = glm::vec3(ec.footnotesScale.x, ec.footnotesScale.y * 0.5f, glm::distance(nodePos, nextNodePos));
		glm::vec3 vecLine = glm::normalize(nextNodePos - nodePos);		// normalized vector (node -> next node)
		float rad = acos(glm::clamp(glm::dot(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)), -1.0f, 1.0f));	 // calc rotation abs angle from dot
		if (glm::cross(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)).y > 0.0f) rad = -rad;		// calc sign(+/-) from cross
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(linePos));
		model = glm::rotate(model, rad, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, lineScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);

	}

	changeColor(locColor, Color::BLUE); // color set BLUE
	for (const glm::vec3& n : ec.rfNotes) {	// drawing right foot notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.footnotesScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::BLUE_LN); // color set transparent BLUE for long notes line
	for (size_t i = 0; i + 1 < ec.lrfLines.size(); i++) {	//drawing long right foot notes (only lines)
		const glm::vec3& nextNodePos = ec.lrfLines.at(i + 1);
		if (glm::isnan(nextNodePos.x)) {		//check dummy (parition between groups)
			i++;
			continue;
		}
		const glm::vec3& nodePos = ec.lrfLines.at(i);


		// calc model matrix
		glm::vec3 linePos = (nodePos + nextNodePos) * 0.5f;
		glm::vec3 lineScale = glm::vec3(ec.footnotesScale.x, ec.footnotesScale.y * 0.5f, glm::distance(nodePos, nextNodePos));
		glm::vec3 vecLine = glm::normalize(nextNodePos - nodePos);		// normalized vector (node -> next node)
		float rad = acos(glm::clamp(glm::dot(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)), -1.0f, 1.0f));	 // calc rotation abs angle from dot
		if (glm::cross(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)).y > 0.0f) rad = -rad;		// calc sign(+/-) from cross
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(linePos));
		model = glm::rotate(model, rad, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, lineScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::LIGHTGREEN); // color set Green for long notes line
	for (size_t i = 0; i + 1 < ec.elfLines.size(); i++) {	//drawing editing long foot notes (only lines)
		const glm::vec3& nextNodePos = ec.elfLines.at(i + 1);
		if (glm::isnan(nextNodePos.x)) {		//check dummy (parition between groups)
			i++;
			continue;
		}
		const glm::vec3& nodePos = ec.elfLines.at(i);

		// calc model matrix
		glm::vec3 linePos = (nodePos + nextNodePos) * 0.5f;
		glm::vec3 lineScale = glm::vec3(ec.footnotesScale.x, ec.footnotesScale.y * 0.5f, glm::distance(nodePos, nextNodePos));
		lineScale += ec.editingLongNotesLineAdditionalScale;
		glm::vec3 vecLine = glm::normalize(nextNodePos - nodePos);		// normalized vector (node -> next node)
		float rad = acos(glm::clamp(glm::dot(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)), -1.0f, 1.0f));	 // calc rotation abs angle from dot
		if (glm::cross(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)).y > 0.0f) rad = -rad;		// calc sign(+/-) from cross
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(linePos));
		model = glm::rotate(model, rad, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, lineScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::WHITE); // color set WHITE for stream line
	for (size_t i = 0; i + 1 < ec.streamLines.size(); i++) {
		const glm::vec3& nextNodePos = ec.streamLines.at(i + 1);
		if (glm::isnan(nextNodePos.x)) {
			i++;
			continue;
		}
		const glm::vec3& nodePos = ec.streamLines.at(i);

		glm::vec3 linePos = (nodePos + nextNodePos) * 0.5f;
		glm::vec3 lineScale = glm::vec3(ec.streamLineScale.x, ec.streamLineScale.y, glm::distance(nodePos, nextNodePos));
		glm::vec3 vecLine = glm::normalize(nextNodePos - nodePos);
		float rad = acos(glm::clamp(glm::dot(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)), -1.0f, 1.0f));
		glm::vec3 rotVec = -glm::cross(vecLine, glm::vec3(0.0f, 0.0f, -1.0f));
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(linePos));
		model = glm::rotate(model, rad, rotVec);
		model = glm::scale(model, lineScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::LIGHTGREEN); // color set Green for editing stream line
	for (size_t i = 0; i + 1 < ec.editingStreamLines.size(); i++) {
		const glm::vec3& nextNodePos = ec.editingStreamLines.at(i + 1);
		if (glm::isnan(nextNodePos.x)) {
			i++;
			continue;
		}
		const glm::vec3& nodePos = ec.editingStreamLines.at(i);

		glm::vec3 linePos = (nodePos + nextNodePos) * 0.5f;
		glm::vec3 lineScale = glm::vec3(ec.streamLineScale.x, ec.streamLineScale.y, glm::distance(nodePos, nextNodePos));
		glm::vec3 vecLine = glm::normalize(nextNodePos - nodePos);
		float rad = acos(glm::clamp(glm::dot(vecLine, glm::vec3(0.0f, 0.0f, -1.0f)), -1.0f, 1.0f));
		glm::vec3 rotVec = -glm::cross(vecLine, glm::vec3(0.0f, 0.0f, -1.0f));
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(linePos));
		model = glm::rotate(model, rad, rotVec);
		model = glm::scale(model, lineScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));

		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::WHITE); // color set WHITE for editing Groupnotes nodes
	for (const glm::vec3& n : ec.enodes) {	// drawing editing Groupnotes nodes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::rotate(model, PI / 4, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, PI / 4, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, ec.nodeScale * 2.0f);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::YELLOW); // color set YELLOW
	for (const glm::vec3& n : ec.yfNotes) {	// drawing yellow foot notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.footnotesScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::PURPLE_OBS); // color set PURPLE for obs
	for (const glm::vec3& n : ec.bars) {	// drawing bar
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.barScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}
	for (const glm::vec4& n : ec.obstacles) {	// drawing obstacles
		float length = n.w - n.z;
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(n.x, n.y + ec.obsScale.y / 2, n.z + length / 2));
		model = glm::scale(model, glm::vec3(ec.obsScale, length));
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::WHITE);	//color set white
	for (const glm::vec3& n : ec.traps) {	// drawing trap
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.trapScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	for (const glm::vec3& n : ec.nodes) {	// drawing nodes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::rotate(model, PI / 4, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, PI / 4, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, ec.nodeScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	changeColor(locColor, Color::YELLOW_SL);	// color set YELLOW (selected)
	for (const glm::vec3& n : ec.slfNotes) {	// drawing left foot notes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.footnotesScale + ec.selectedAdditionalScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}
	for (const glm::vec3& n : ec.slbars) {	// drawing bar
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.barScale + ec.selectedAdditionalScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}
	for (const glm::vec4& n : ec.slobstacles) {	// drawing obstacles
		float length = n.w - n.z;
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(n.x, n.y + ec.obsScale.y / 2, n.z + length / 2));
		model = glm::scale(model, glm::vec3(ec.obsScale, length) + ec.selectedAdditionalScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}
	for (const glm::vec3& n : ec.sltraps) {	// drawing trap
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::scale(model, ec.trapScale + ec.selectedAdditionalScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	for (const glm::vec3& n : ec.slnodes) {	// drawing nodes
		model = glm::mat4(1.0f);
		model = glm::translate(model, n);
		model = glm::rotate(model, PI / 4, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, PI / 4, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, ec.nodeScale + ec.selectedAdditionalScale);
		glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
		glDrawElements(GL_TRIANGLES, cubeIndicesNum, GL_UNSIGNED_INT, 0);
	}

	/*
	//draw label(like lane number)
	if (drawLaneNum2d) {
		glm::mat4 matPV = ec.projectionOrtho * ec.viewOrtho;
		ImVec2 screenPos;

		screenPos = worldToScreen(glm::vec3(0.0f, 0.0f, -ec.hitLineDistance), matPV, ec.contentOrigin2d, ec.tdFBOWidth, ec.tdFBOHeight);
		if (!(screenPos.x < -1 || screenPos.y < -1 || screenPos.x > 1 || screenPos.y > 1)) {
			ImDrawList* dl = ImGui::GetForegroundDrawList();
			{
				char buf[32];
				std::snprintf(buf, sizeof(buf), "%d", currentBeat);
				dl->AddText(screenPos, IM_COL32(255, 255, 0, 255), buf);
			}
		}
	}

	//draw axis
			if (drawAxis) {
				glBindVertexArray(lineVAO);
				float axisSize = 0.12f;
				glLineWidth(1.0f);

				glm::vec3 axisPos = ndcToWorld(
					glm::vec4(0.0f, 0.0f, 0.9f, 1.0f),
					glm::inverse(editorContext.projectionPerspective * editorContext.viewPerspective)
				);

				//X axis (red)
				glUniform4f(locColor, 1.0f, 0.0f, 0.0f, 1.0f);
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(axisPos));
				model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
				model = glm::scale(model, glm::vec3(1.0f, axisSize, 1.0f));
				glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
				glDrawArrays(GL_LINES, 0, 2);
				//Y axis (green)
				glUniform4f(locColor, 0.0f, 1.0f, 0.0f, 1.0f);
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(axisPos));
				model = glm::scale(model, glm::vec3(1.0f, axisSize, 1.0f));
				glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
				glDrawArrays(GL_LINES, 0, 2);
				//Z axis (blue)
				glUniform4f(locColor, 0.0f, 0.0f, 1.0f, 1.0f);
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(axisPos));
				model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::scale(model, glm::vec3(1.0f, axisSize, 1.0f));
				glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
				glDrawArrays(GL_LINES, 0, 2);
			}
	*/


	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return fboTexture;
}
