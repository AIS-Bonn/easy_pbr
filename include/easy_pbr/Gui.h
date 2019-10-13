#pragma once

//c++
#include <iostream>
#include <memory>
#include <unordered_map>
#include <mutex>

#include <glad/glad.h> // Initialize with gladLoadGL()
// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

//imgui
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_ext/IconsFontAwesome.h"
#include "ImGuizmo.h"

//opencv
#include "opencv2/opencv.hpp"

//gl
#include "Texture2D.h"

//my sutff 
// #include "surfel_renderer/data_loader/RosBagPlayer.h"




//forward declarations
// class Core;
class DataLoaderPNG;
class Viewer;
class Mesh;
class MeshGL;
class Texturer;
class FireDetector;
class Mesher;
class LatticeCPU_test;
class LatticeGPU_test;
class BallDetector;

//Since the source directory is not known we get it from the cmake variable {CMAKE_SOURCE_DIR} through target_compile_definitions
#ifdef AWESOMEFONT_DIR
    #define AWESOMEFONT_PATH AWESOMEFONT_DIR
#else
    #define AWESOMEFONT_PATH "."
#endif



class Gui{
public:
    Gui( const std::string config_file,
        Viewer* view,
        GLFWwindow* window
       );
    void update();
    // static void show(const cv::Mat& cv_mat, const std::string name);

    //Needs to be inline otherwise it doesn't work since it needs to be executed from a GL context
    inline void init_fonts(){
        // ImGui::GetIO().Fonts->AddFontDefault();
        ImFontConfig config;
        config.MergeMode = true;
        const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        ImGui::GetIO().Fonts->AddFontFromFileTTF(AWESOMEFONT_PATH, 13.0f*m_hidpi_scaling, &config, icon_ranges);
    };
    void init_style();

    //we leave the show function in the header so if we compile without the viewer and don't have the Gui.cxx anymore it's still fine and we don't get undefined function 
    static void show(const cv::Mat& cv_mat, const std::string name){
        
        #ifdef WITH_VIEWER
            if(!cv_mat.data){
                VLOG(3) << "Showing empty image, discaring";
                return;
            }

            std::lock_guard<std::mutex> lock(m_add_cv_mats_mutex);  // so that "show" can be usef from any thread

            m_cv_mats_map[name] = cv_mat.clone(); //TODO we shouldnt clone on top of this one because it might be at the moment used for transfering between cpu and gpu
            m_cv_mats_dirty_map[name]=true;
        #endif
    }

    //register new modules to have them controlable in the gui 
    // void register_module(const std::shared_ptr<RosBagPlayer> player);
    // void register_module(const std::shared_ptr<Texturer> texturer);
    // void register_module(const std::shared_ptr<FireDetector> fire_detector);
    // void register_module(const std::shared_ptr<Mesher> mesher);
    // void register_module(const std::shared_ptr<LatticeCPU_test> latticeCPU_test);
    // void register_module(const std::shared_ptr<LatticeGPU_test> latticeGPU_test);
    // void register_module(const std::shared_ptr<BallDetector> ball_detector);

private:



    // ImGuiIO& io= ImGui::GetIO();
    // std::shared_ptr<Core> m_core;
    Viewer* m_view;
    ImGuiContext* m_imgui_context;

    // //modules that can be registered 
    // std::shared_ptr<RosBagPlayer> m_player;
    // std::shared_ptr<Texturer> m_texturer;
    // std::shared_ptr<FireDetector> m_fire_detector;
    // std::shared_ptr<Mesher> m_mesher;
    // std::shared_ptr<LatticeCPU_test> m_latticeCPU_test;
    // std::shared_ptr<LatticeGPU_test> m_latticeGPU_test;
    // std::shared_ptr<BallDetector> m_ball_detector;

    bool m_show_demo_window;
    bool m_show_profiler_window;
    bool m_show_player_window;


    int m_selected_mesh_idx; //index of the selected mesh from the scene, we will use this index to modify properties of the selected mesh
    int m_selected_spot_light_idx; 
    int m_mesh_tex_idx;
    bool m_show_debug_textures;
    ImGuizmo::OPERATION m_guizmo_operation=ImGuizmo::ROTATE;
    ImGuizmo::MODE m_guizmo_mode = ImGuizmo::LOCAL;
    float m_hidpi_scaling;
    std::string m_camera_pose;

    static std::mutex m_add_cv_mats_mutex; //adding or registering images for viewing must be thread safe
    //for showing images we store a list of cv_mats and then we render them when the times comes to update the gui. We do this in order to register images for showing from any thread even though it has no opengl context 
    static std::unordered_map<std::string, cv::Mat> m_cv_mats_map;
    static std::unordered_map<std::string, bool> m_cv_mats_dirty_map; // when we register one cv mat we set it to dirty so we know we need to upload data on it 
    //for showing images we store a list of their opengl textures implemented as a map between their name and the gltexture
    std::unordered_map<std::string, gl::Texture2D> m_textures_map;


    //IO stuff
    std::string m_write_mesh_file_path;

    //meshops sutff
    float m_subsample_factor;
    int m_decimate_nr_target_faces;

    ImVec2 foo[10];

    void init_params(const std::string config_file);
    void show_gl_texture(const int tex_id, const std::string window_name, const bool flip=false);
    void edit_transform(const std::shared_ptr<Mesh>& mesh);
    float pixel_ratio();
    float hidpi_scaling();
    void show_images(); // uplaod all cv mats to gl textures and displays them 
    void draw_overlays(); //draw all the overlays like the vert ids above each vertex of the meshes that are visible
    void draw_overlay_text(const Eigen::Vector3d pos, const Eigen::Matrix4f model_matrix, const std::string text, const Eigen::Vector3f color); //draw any type of overlaid text to the viewer


};
