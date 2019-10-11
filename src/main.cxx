//My stuff
#include "easy_pbr/Viewer.h"

#define ENABLE_GL_PROFILING 1
#include "Profiler.h"


int main(int argc, char *argv[]) {

    std::shared_ptr<Viewer> view = Viewer::create("empty.cfg"); //need to pass the size so that the viewer also knows the size of the viewport

    while (true) {
        view->update();
    }

    // Cleanup
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    return 0;
}
