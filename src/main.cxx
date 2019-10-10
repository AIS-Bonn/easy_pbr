#include <iostream>

//GL
// #define _GLFW_USE_DWM_SWAP_INTERVAL 1 //to solve this https://github.com/glfw/glfw/issues/177
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//loguru
#define LOGURU_IMPLEMENTATION 1
#define LOGURU_NO_DATE_TIME 1
#define LOGURU_NO_UPTIME 1
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>


// //libigl
// #ifdef WITH_VIEWER
    //ImGui
    #include <imgui.h>
    #include "imgui_impl_glfw.h"

    //My stuff
    #include "easy_pbr/Viewer.h"
    #include "easy_pbr/Gui.h"
// #endif


//configuru
#define CONFIGURU_IMPLEMENTATION 1
#define CONFIGURU_WITH_EIGEN 1
#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#include <configuru.hpp>
using namespace configuru;

#define PROFILER_IMPLEMENTATION 1
#define ENABLE_GL_PROFILING 1
#include "Profiler.h"


// #include "surfel_renderer/Profiler.h"


int main(int argc, char *argv[]) {


    // loguru::init(argc, argv);
    // loguru::g_stderr_verbosity = -1; //By default don't show any logs except warning, error and fatals
    // loguru::g_stderr_verbosity = 3; //Show up until VLOG(3) but not above

    // ros::init(argc, argv, "surfel_renderer");
    // ros::start(); //in order for the node to live otherwise it will die when the last node handle is killed

    // LOG_S(INFO) << "Hello from main!";

    //Objects
    // #ifdef WITH_VIEWER //we intializ the viewer which creates the context, otherwise we make an ofscreen window
        std::shared_ptr<Viewer> view = Viewer::create("empty.cfg"); //need to pass the size so that the viewer also knows the size of the viewport
    // #else
        // init_offscreen_context();
    // #endif
    // std::shared_ptr<Core> core(new Core());
    // #ifdef WITH_VIEWER
        // core->m_view=view;
        // gui->init_fonts(); //needs to be initialized inside the context
    // #endif

    // core->start(); //now that every object is linked correctly we can start


    // //before starting with the main loop we check that none the parameters in the config file was left unchecked
    // ros::NodeHandle private_nh("~");
    // std::string config_file= getParamElseThrow<std::string>(private_nh, "config_file");
    // Config cfg = configuru::parse_file(std::string(CMAKE_SOURCE_DIR)+"/config/"+config_file, CFG);
    // cfg.check_dangling(); // Make sure we haven't forgot reading a key!

    while (true) {
        view->update();
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // LOG_SCOPE(INFO,"main_loop");
        // #ifdef WITH_VIEWER
            // glfwPollEvents();
            // // Start the Dear ImGui frame
            // ImGui_ImplOpenGL3_NewFrame();
            // ImGui_ImplGlfw_NewFrame();
            // ImGui::NewFrame();
            // switch_callbacks(window);  //needs to be done inside the context otherwise it segments faults
            // gui->update();
        // #endif


        // core->update();
        // view->update();


        // #ifdef WITH_VIEWER
            // view->draw();
            // ImGui::Render();
            // ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            // glfwSwapBuffers(window);
        // #endif

    }



    // // Cleanup
    // #ifdef WITH_VIEWER
    //     ImGui_ImplGlfw_Shutdown();
    //     ImGui::DestroyContext();
    // #endif
    // glfwTerminate();

    return 0;
}

// #ifdef WITH_VIEWER
    // void setup_callbacks_viewer(GLFWwindow* window){
    //     // https://stackoverflow.com/a/28660673
    //     auto mouse_pressed_func = [](GLFWwindow* w, int button, int action, int modifier) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_mouse_pressed( w, button, action, modifier );     };
    //     auto mouse_move_func = [](GLFWwindow* w, double x, double y) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_mouse_move( w, x, y );     };
    //     auto mouse_scroll_func = [](GLFWwindow* w, double x, double y) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_mouse_scroll( w, x, y );     }; 
    //     auto key_func = [](GLFWwindow* w, int key, int scancode, int action, int modifier) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_key( w, key, scancode, action, modifier );     }; 
    //     auto char_mods_func  = [](GLFWwindow* w, unsigned int codepoint, int modifier) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_char_mods( w, codepoint, modifier );     }; 
    //     auto resize_func = [](GLFWwindow* w, int width, int height) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_resize( w, width, height );     };   
    //     auto drop_func = [](GLFWwindow* w, int count, const char** paths) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_drop( w, count, paths );     };   
        

    //     glfwSetMouseButtonCallback(window, mouse_pressed_func);
    //     glfwSetCursorPosCallback(window, mouse_move_func);
    //     glfwSetScrollCallback(window,mouse_scroll_func);
    //     glfwSetKeyCallback(window, key_func);
    //     glfwSetCharModsCallback(window,char_mods_func);
    //     glfwSetWindowSizeCallback(window,resize_func);
    //     glfwSetDropCallback(window, drop_func);

    // }
    // void setup_callbacks_imgui(GLFWwindow* window){
    //     glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
    //     glfwSetCursorPosCallback(window, nullptr);
    //     glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
    //     glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
    //     glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    //     glfwSetCharModsCallback(window, nullptr);
    // }


    // void switch_callbacks(GLFWwindow* window) {
    //     // bool hovered_imgui = ImGui::IsMouseHoveringAnyWindow();
    //     bool using_imgui = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    //     // VLOG(1) << "using imgui is " << using_imgui;

    //     // glfwSetMouseButtonCallback(window, nullptr);
    //     // glfwSetCursorPosCallback(window, nullptr);
    //     // glfwSetScrollCallback(window, nullptr);
    //     // glfwSetKeyCallback(window, nullptr);
    //     // glfwSetCharModsCallback(window, nullptr);
    //     // glfwSetWindowSizeCallback(window, nullptr);
    //     // glfwSetCharCallback(window, nullptr);

    //     if (using_imgui) {
    //         setup_callbacks_imgui(window);
    //    } else {
    //         setup_callbacks_viewer(window);   
    //     }
    // }


    // GLFWwindow* init_context(int& window_width, int& window_height){

    //     // Setup window
    //     if (!glfwInit()){
    //         LOG(FATAL) << "GLFW could not initialize";
    //     }

    //     glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    //     glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    //     static GLFWwindow*  window = glfwCreateWindow(window_width, window_height, "Renderer",nullptr,nullptr);
    //     if (!window){
    //         LOG(FATAL) << "GLFW window creation failed";        
    //         glfwTerminate();
    //     }
    //     glfwMakeContextCurrent(window);
    //     // Load OpenGL and its extensions
    //     if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)){
    //         LOG(FATAL) << "GLAD failed to load";        
    //     }
    //     glfwSwapInterval(1); // Enable vsync

    //     glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);

    //     //window will be resized to the screen so we can return the actual widnow values now 
    //     glfwGetWindowSize(window, &window_width, &window_height);

    //     return window;

    // }
// #else
//     void init_offscreen_context(){
//             /* Initialize the library */
//         if (!glfwInit()){
//             LOG(FATAL) << "init_context glfwInit failure";
//         }
//         // glfwWindowHint(GLFW_SAMPLES, 8);
//         glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
//         glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
//         glfwWindowHint(GLFW_VISIBLE, 0);
//         /* Create a ofscreen context and its OpenGL context */
//         GLFWwindow* offscreen_context = glfwCreateWindow(640, 480, "", NULL, NULL);
//         if (!offscreen_context) {
//             glfwTerminate();
//             LOG(FATAL) << "Failed to create the window or the GL context";
//         }
//         /* Make the window's context current */
//         glfwMakeContextCurrent(offscreen_context);
//         if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
//         {
//           LOG(FATAL) << ("Failed to load OpenGL and its extensions");
//         }
//         glfwSwapInterval(1);
//     }

// #endif
