#define GLFW_INCLUDE_NONE
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <GLFW/glfw3native.h>
#endif
#include <glad/glad.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "Application.h"
#include "NativeFileDialog.h"

#ifdef _WIN32
// Initializes COM for the main thread so Windows native dialogs can be used safely.
class NativeDialogComScope {
public:
	NativeDialogComScope() {
		std::string errorMessage;
		initialized = NativeFileDialog::initializeComForCurrentThread(errorMessage);
	}

	~NativeDialogComScope() {
		if (initialized) {
			NativeFileDialog::shutdownComForCurrentThread();
		}
	}

private:
	bool initialized = false;
};
#endif

// Updates the OpenGL viewport when the window framebuffer changes size.
void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	(void)window;
	glViewport(0, 0, width, height);
}

int main()
{
#ifdef _WIN32
	NativeDialogComScope nativeDialogComScope;
#endif

	// GLFW initialization
	if (!glfwInit())
		return -1;

	// OpenGL version 3.3 Core Profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Create window
	GLFWwindow* window = glfwCreateWindow(1980, 1080, "DDEditor", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	// Load OpenGL function pointers using GLAD
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
	}

	// Setup ImGui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	void* nativeWindowHandle = nullptr;
#ifdef _WIN32
	nativeWindowHandle = glfwGetWin32Window(window);
#endif
	Application application(nativeWindowHandle);

	// Main loop
	while (!application.shouldClose())
	{
		glfwPollEvents();
		if (glfwWindowShouldClose(window)) {
			glfwSetWindowShouldClose(window, GLFW_FALSE);
			application.requestClose();
			if (application.shouldClose()) {
				break;
			}
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		application.updateFrame(static_cast<float>(glfwGetTime()));
		application.runCurrentScreenFrame();

		// Render
		ImGui::Render();
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
	}

	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
