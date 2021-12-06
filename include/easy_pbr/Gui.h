#pragma once

//c++
#include <iostream>
#include <memory>
#include <unordered_map>
#include <mutex>

// #include <glad/glad.h> // Initialize with gladLoadGL()
// Include glfw3.h after our OpenGL definitions
// #include <GLFW/glfw3.h>

// //imgui
#include <imgui.h> //is needed to be included here because imguizmo needs it
// #include "imgui_impl_glfw.h"
// #include "imgui_ext/IconsFontAwesome.h"
#include "ImGuizmo.h"

//opencv
#include "opencv2/opencv.hpp"

//eigen
#include <Eigen/Core>


//gl
#include "Texture2D.h"


class GLFWwindow;
class ImGuiContext;

namespace easy_pbr{

//forward declarations
class DataLoaderPNG;
class Viewer;
class Mesh;
class MeshGL;
class Camera;


class Gui{
public:
    Gui( const std::string config_file,
        Viewer* view,
        GLFWwindow* window
       );
    void update();
    static void show(const cv::Mat cv_mat, const std::string name);
    static void show_gl_texture(const int tex_id, const std::string window_name, const bool flip=false);
    void select_mesh_with_idx(const int idx); //set the selection fo the meshes to the one with a certain index
    int selected_mesh_idx(); //get the index of the mesh we have selected
    void toggle_main_menu();

    //dragging and dropping textures
    bool m_diffuse_tex_hovered;
    bool m_normals_tex_hovered;
    bool m_metalness_tex_hovered;
    bool m_roughness_tex_hovered;



private:

    Viewer* m_view;
    ImGuiContext* m_imgui_context;

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
    bool m_draw_main_menu;
    ImFont* m_roboto_regular;
    ImFont* m_roboto_bold;

    static std::mutex m_cv_mats_mutex; //adding or registering images for viewing must be thread safe
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

    //debug stuff
    ImVec2 m_curve_points[10];

    // trajectory follower:
    int m_selected_trajectory_idx;
    float m_trajectory_frustum_size = 0.01f;
    std::string m_traj_file_name = "./traj.txt";
    ImGuizmo::MODE m_traj_guizmo_mode = ImGuizmo::LOCAL;
    ImGuizmo::OPERATION m_traj_guizmo_operation=ImGuizmo::ROTATE;

    bool m_traj_should_draw = true;
    bool m_traj_is_playing = false;
    bool m_traj_is_paused = false;
    bool m_traj_preview = false;
    bool m_traj_use_time_not_frames = true;
    int m_traj_fps = 30;
    int m_traj_view_updates = 0;
    std::shared_ptr<Camera> m_preview_camera;

    void init_params(const std::string config_file);
    void init_style();
    void edit_transform(const std::shared_ptr<Mesh>& mesh);
    void edit_trajectory(const std::shared_ptr<Camera> & cam);
    void help_marker(const char* desc);
    void show_images(); // uplaod all cv mats to gl textures and displays them
    void draw_overlays(); //draw all the overlays like the vert ids above each vertex of the meshes that are visible
    void draw_overlay_text(const Eigen::Vector3d pos, const Eigen::Matrix4f model_matrix, const std::string text, const Eigen::Vector3f color); //draw any type of overlaid text to the viewer
    void draw_label_mngr_legend();
    void draw_main_menu();
    void draw_profiler();
    void draw_drag_drop_text();
    void draw_trajectory(const std::string & trajectory_mesh_name, const std::string & frustum_mesh_name);

};

} //namespace easy_pbr
