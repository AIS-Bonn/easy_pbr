#pragma once

//c++
#include <memory>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h" 
#include <glad/glad.h> // Initialize with gladLoadGL()
// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>

#include <Eigen/Core>

#include "Shader.h"
#include "GBuffer.h"
#include "CubeMap.h"
#include "ColorMngr.h"
// #include "surfel_renderer/Camera.h"

//dir watcher
#include "dir_watcher/dir_watcher.hpp"

// pybind
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/eigen.h>


class Scene;
class MeshGL;
class Camera;
class Gui;
class Recorder;
class RandGenerator;
class SpotLight;

class Viewer {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    Viewer(const std::string config_file);
    //https://stackoverflow.com/questions/29881107/creating-objects-only-as-shared-pointers-through-a-base-class-create-method
    template<class... Args>
    static typename std::shared_ptr<Viewer> create(Args&&... args){
        return std::make_shared<Viewer>(args...);
    }

    
    bool dummy;  //to initialize the window we provide this dummy variable so we can call initialie context
    bool dummy_glad;
    GLFWwindow* m_window;
    #ifdef WITH_DIR_WATCHER
        emilib::DelayedDirWatcher dir_watcher;
    #endif
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<Camera> m_default_camera;
    std::shared_ptr<Camera> m_camera; //just a point to either the default camera or one of the point light so that we render the view from the point of view of the light
    std::shared_ptr<Gui> m_gui;
    std::shared_ptr<Recorder> m_recorder;
    std::shared_ptr<RandGenerator> m_rand_gen;
    std::vector<std::shared_ptr<SpotLight>> m_spot_lights;



    //params
    Eigen::Vector2f m_viewport_size;
    Eigen::Vector3f m_background_color;

    void init_params(const std::string config_file);
    bool init_context();
    void setup_callbacks_viewer(GLFWwindow* window);
    void setup_callbacks_imgui(GLFWwindow* window);
    void switch_callbacks(GLFWwindow* window);
    void update(const GLuint fbo_id=0); //draw into a certain framebuffer, by default its the screen (default framebuffer) 
    void pre_draw();
    void post_draw();
    void draw(const GLuint fbo_id=0); //draw into a certain framebuffer, by default its the screen (default framebuffer)
    // void clear_framebuffers(const Eigen::Vector4f background_color);
    void clear_framebuffers();
    void compile_shaders();
    void hotload_shaders();
    void init_opengl();
    void update_meshes_gl();
    void render_points(const std::shared_ptr<MeshGL> mesh);
    void render_points_to_gbuffer(const std::shared_ptr<MeshGL> mesh);
    void render_lines(const std::shared_ptr<MeshGL> mesh);
    void render_wireframe(const std::shared_ptr<MeshGL> mesh);
    void render_mesh_to_gbuffer(const std::shared_ptr<MeshGL> mesh);
    void render_surfels_to_gbuffer(const std::shared_ptr<MeshGL> mesh);


    //rendering passes 
    void ssao_pass();
    void compose_final_image(const GLuint fbo_id);

    //other
    void create_random_samples_hemisphere();

    // Callbacks
    void set_callbacks();
    void glfw_mouse_pressed(GLFWwindow* window, int button, int action, int modifier);
    void glfw_mouse_move(GLFWwindow* window, double x, double y);
    void glfw_mouse_scroll(GLFWwindow* window, double x, double y);
    void glfw_key(GLFWwindow* window, int key, int scancode, int action, int modifier);
    void glfw_char_mods(GLFWwindow* w, unsigned int codepoint, int modifier);
    void glfw_resize(GLFWwindow* window, int width, int height);
    void glfw_drop(GLFWwindow* window, int count, const char** paths);

    ColorMngr m_colormngr;

    gl::Shader m_draw_points_shader;
    gl::Shader m_draw_points_gbuffer_shader; //draw the points into the gbuffer instead of using forward rendering
    gl::Shader m_draw_lines_shader;
    gl::Shader m_draw_mesh_shader;
    gl::Shader m_draw_wireframe_shader;
    gl::Shader m_draw_surfels_shader;
    gl::Shader m_compose_final_quad_shader;
    // gl::Shader m_ssao_geom_pass_shader;
    gl::Shader m_ssao_ao_pass_shader;
    gl::Shader m_depth_linearize_shader;
    gl::Shader m_bilateral_blur_shader;

    gl::GBuffer m_gbuffer;

    gl::Texture2D m_ao_tex;
    gl::Texture2D m_ao_blurred_tex;
    gl::Texture2D m_rvec_tex;
    gl::Texture2D m_depth_linear_tex;
    gl::Texture2D m_background_tex; //in the case we want an image as the background
    gl::CubeMap m_environment_cubemap_tex; //used for image-based ligthing
    Eigen::MatrixXf m_random_samples;
    std::shared_ptr<MeshGL> m_fullscreen_quad; //we store it here because we precompute it and then we use for composing the final image after the deffered geom pass

    //params
    bool m_show_gui;
    float m_subsample_factor; // subsample factor for the whole viewer so that when it's fullscreen it's not using the full resolution of the screen
    int m_ssao_downsample;
    int m_nr_samples;
    float m_kernel_radius;
    int m_ao_power;
    float m_sigma_spacial;
    float m_sigma_depth;    
    Eigen::Vector3f m_ambient_color;   
    float m_ambient_color_power;
    Eigen::Vector3f m_specular_color;
    float m_shininess;
    bool m_enable_culling;
    bool m_enable_ssao;
    float m_shading_factor; // dicates how much the lights and ambient occlusion influence the final color. If at zero then we only output the diffuse color
    float m_light_factor; // dicates how much the lights influence the final color. If at zero then we only output the diffuse color but also multipled by ambient occlusion ter
    float m_surfel_blend_dist; // how much distant to the surfels need to be so that they pass the depth test and they get blended together 
    float m_surfel_blend_dist2; // how much distant to the surfels need to be so that they pass the depth test and they get blended together 
    bool m_enable_edl_lighting;
    float m_edl_strength;
    bool m_enable_surfel_splatting;
    bool m_use_background_img;
    std::string m_background_img_path;
    bool m_use_environment_map;
    std::string m_environment_map_path;

    std::vector< std::shared_ptr<MeshGL> > m_meshes_gl; //stored the gl meshes which will get updated if the meshes in the scene are dirty


    Eigen::Matrix4f compute_mvp_matrix(const std::shared_ptr<MeshGL>& mesh);

private:
    // Eigen::Matrix4f compute_mvp_matrix();

    bool m_first_draw;

    void read_background_img(gl::Texture2D& tex, const std::string img_path);
    void equirectangular2cubemap(gl::CubeMap& cubemap_tex, const gl::Texture2D& equirectangular_tex);



};
