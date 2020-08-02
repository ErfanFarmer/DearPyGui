#include "Platform/Linux/mvLinuxWindow.h"
#include "mvApp.h"
#include "Core/StandardWindows/mvAppEditor.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>

namespace Marvel {

    mvWindow* mvWindow::CreatemvWindow(unsigned width, unsigned height, bool editor, bool error, bool doc)
    {
        return new mvLinuxWindow(width, height, editor, error, doc);
    }

    static void glfw_error_callback(int error, const char* description)
    {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }

    mvLinuxWindow::mvLinuxWindow(unsigned width, unsigned height, bool editor, bool error, bool doc)
		: mvWindow(width, height, editor, error, doc)
	{

        // Setup window
        glfwSetErrorCallback(glfw_error_callback);
        glfwInit();

        // Create window with graphics context
        // GL 3.0 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        m_window = glfwCreateWindow(1280, 720, "DearPyGui", nullptr, nullptr);

        glfwMakeContextCurrent(m_window);
        glfwSwapInterval(1); // Enable vsync

        bool err = gl3wInit() != 0;

        // Setup Dear ImGui binding
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

        // Setup style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForOpenGL(m_window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

	}

    mvLinuxWindow::~mvLinuxWindow()
	{
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(m_window);
        glfwTerminate();
	}

    void mvLinuxWindow::run()
    {

        setup();
        while (m_running)
        {
            prerender();

            if (m_error)
            {
                mvAppLog::setSize(m_width, m_height);
                mvAppLog::render();
            }

            else if (m_editor)
            {
                m_appEditor->prerender();
                m_appEditor->render(m_editor);
                m_appEditor->postrender();
            }

            else if (m_doc)
            {
                m_documentation->prerender();
                m_documentation->render(m_doc);
                m_documentation->postrender();
            }

            else if (!m_error)
            {
                m_app->prerender();
                m_app->render(m_running);
                m_app->postrender();
            }

            postrender();
        }

    }

    void mvLinuxWindow::prerender()
    {
        m_running = !glfwWindowShouldClose(m_window);

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();


    }

    void mvLinuxWindow::postrender()
    {
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(m_window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(m_clear_color[0], m_clear_color[1], m_clear_color[2], m_clear_color[3]);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }

    void mvLinuxWindow::cleanup()
    {
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(m_window);
        glfwTerminate();
    }
}