#pragma once

//c++
#include <memory>

// #include "imgui.h"
// #include "imgui_impl_glfw.h"
// #include "imgui_impl_opengl3.h" 
// #include <glad/glad.h> // Initialize with gladLoadGL()
// // Include glfw3.h after our OpenGL definitions
// #include <GLFW/glfw3.h>

#include <Eigen/Geometry>

#include "Shader.h"
#include "GBuffer.h"
#include "CubeMap.h"
#include "ColorMngr.h"

//dir watcher 
// #ifdef EASYPBR_WITH_DIR_WATCHER
// DO NOT USE A IFDEF because other C++ libs may include this Viewer.h without the compile definitions and therefore the Viewer.h that was used to compile easypbr and the one included will be different leading to issues
#include "dir_watcher/dir_watcher.hpp"
// #endif

// pybind
// #include <pybind11/pybind11.h>
// #include <pybind11/stl.h>
// #include <pybind11/eigen.h>
class GLFWwindow;

namespace radu { namespace utils { 
    class RandGenerator; 
    class Timer; 
    }}

namespace easy_pbr{


class Scene;
class MeshGL;
class Camera;
class Gui;
class Recorder;
class SpotLight;

//in order to dissalow building on the stack and having only ptrs https://stackoverflow.com/a/17135547
class Viewer;

class Viewer: public std::enable_shared_from_this<Viewer> {
public:
    // EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    //https://stackoverflow.com/questions/29881107/creating-objects-only-as-shared-pointers-through-a-base-class-create-method
    template <class ...Args>
    static std::shared_ptr<Viewer> create( Args&& ...args ){
        return std::shared_ptr<Viewer>( new Viewer(std::forward<Args>(args)...) );
        // return std::make_shared<Viewer>( std::forward<Args>(args)... );
    }
    // ~Viewer()=default;
    ~Viewer();

    
    bool dummy;  //to initialize the window we provide this dummy variable so we can call initialie context
    bool dummy_glad;
    GLFWwindow* m_window;
    // #ifdef EASYPBR_WITH_DIR_WATCHER
        // emilib::DelayedDirWatcher dir_watcher;
    // #endif
    std::shared_ptr<emilib::DelayedDirWatcher> dir_watcher;
    std::shared_ptr<Scene> m_scene;
    std::shared_ptr<Camera> m_default_camera;
    std::shared_ptr<Camera> m_camera; //just a point to either the default camera or one of the point light so that we render the view from the point of view of the light
    std::shared_ptr<Gui> m_gui;
    std::shared_ptr<Recorder> m_recorder;
    std::shared_ptr<radu::utils::RandGenerator> m_rand_gen;
    std::vector<std::shared_ptr<SpotLight>> m_spot_lights;
    std::vector<std::shared_ptr<Camera>> m_trajectory;


    //params
    Eigen::Vector2f m_viewport_size;
    Eigen::Vector3f m_background_color;

    void init_params(const std::string config_file);
    bool init_context();
    void setup_callbacks_viewer(GLFWwindow* window);
    void setup_callbacks_imgui(GLFWwindow* window);
    void switch_callbacks(GLFWwindow* window);
    void add_callback_pre_draw(const std::function<void(Viewer& viewer)> func);
    void add_callback_post_draw(const std::function<void(Viewer& viewer)> func);
    void update(const GLuint fbo_id=0); //draw into a certain framebuffer, by default its the screen (default framebuffer) 
    void pre_draw();
    void post_draw();
    void draw(const GLuint fbo_id=0); //draw into a certain framebuffer, by default its the screen (default framebuffer)
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
    std::shared_ptr<SpotLight> spotlight_with_idx(const size_t);
    // cv::Mat download_to_cv_mat(); //downloads the last drawn framebuffer into a cv::Mat. It is however sloas it forces a stall of the pipeline. For recording the viewer look into the Recorder class
    void load_environment_map(const std::string path);

    // //for debuggin
    // void print_pointers();
    // void set_position(const int i, Eigen::Vector3f&);
    // void check_position(const int i);
    void write_gbuffer_to_folder();


    //rendering passes 
    void ssao_pass();
    void compose_final_image(const GLuint fbo_id);

    //other
    void create_random_samples_hemisphere();

    //getters 
    gl::Texture2D& rendered_tex_no_gui(const bool with_transparency);
    gl::Texture2D& rendered_tex_with_gui();

    // Callbacks
    void set_callbacks();
    void glfw_mouse_pressed(GLFWwindow* window, int button, int action, int modifier);
    void glfw_mouse_move(GLFWwindow* window, double x, double y);
    void glfw_mouse_scroll(GLFWwindow* window, double x, double y);
    void glfw_key(GLFWwindow* window, int key, int scancode, int action, int modifier);
    void glfw_char_mods(GLFWwindow* w, unsigned int codepoint, int modifier);
    void glfw_resize(GLFWwindow* window, int width, int height);
    void glfw_drop(GLFWwindow* window, int count, const char** paths);
    void imgui_drop(GLFWwindow* window, int count, const char** paths);

    radu::utils::ColorMngr m_colormngr;
    
    //timing for having fuzzy time updates at 30fps https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
    std::shared_ptr<radu::utils::Timer> m_timer;
    double m_old_time;
    double m_accumulator_time;
    unsigned long long m_nr_drawn_frames;

    gl::Shader m_draw_points_shader;
    gl::Shader m_draw_lines_shader;
    gl::Shader m_draw_mesh_shader;
    gl::Shader m_draw_wireframe_shader;
    gl::Shader m_draw_surfels_shader;
    gl::Shader m_compose_final_quad_shader;
    gl::Shader m_ssao_ao_pass_shader;
    gl::Shader m_depth_linearize_shader;
    gl::Shader m_bilateral_blur_shader;
    gl::Shader m_equirectangular2cubemap_shader;
    gl::Shader m_radiance2irradiance_shader;
    gl::Shader m_prefilter_shader;
    gl::Shader m_integrate_brdf_shader;
    gl::Shader m_blur_shader;
    gl::Shader m_apply_postprocess_shader;
    gl::Shader m_decode_gbuffer_debugging;
    gl::Shader m_blend_bg_shader;;

    gl::GBuffer m_gbuffer; //contains all the textures of a normal gbuffer. So normals, diffuse, depth etc.
    gl::GBuffer m_composed_fbo; //contains the composed image between the foreground and background before tonemapping and gamma correction. Contains also the bright spots of the image
    // gl::Texture2D m_composed_tex; //after gbuffer composing the foreground with the background but before tonemapping and gamme correction. Is in half float
    // gl::Texture2D m_bloom_tex; //while composing we also write the colors corresponding to the bright areas. Is in half float
    // gl::Texture2D m_posprocessed_tex; //after adding also any post processing like bloom and tone mapping and gamma correcting. Is in RGBA8
    gl::GBuffer m_final_fbo_no_gui; //after rendering also the lines and edges but before rendering the gui
    gl::GBuffer m_final_fbo_with_gui; //after we also render the gui into it

    gl::Texture2D m_ao_tex;
    gl::Texture2D m_ao_blurred_tex;
    gl::Texture2D m_rvec_tex;
    gl::Texture2D m_depth_linear_tex;
    gl::Texture2D m_blur_tmp_tex; //stores the blurring temporary results
    gl::Texture2D m_background_tex; //in the case we want an image as the background
    gl::CubeMap m_environment_cubemap_tex; //used for image-based ligthing
    gl::CubeMap m_irradiance_cubemap_tex; //averages the radiance around the hermisphere for each direction. Used for diffuse IBL
    gl::CubeMap m_prefilter_cubemap_tex; //stores filtered maps for various roughness. Used for specular IBL
    gl::Texture2D m_brdf_lut_tex;
    gl::Texture2D m_uv_checker_tex;
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
    bool m_enable_culling;
    bool m_auto_ssao;
    bool m_enable_ssao;
    bool m_enable_bloom;
    float m_bloom_threshold;
    int m_bloom_start_mip_map_lvl;
    int m_bloom_max_mip_map_lvl;
    int m_bloom_blur_iters;
    // float m_shading_factor; // dicates how much the lights and ambient occlusion influence the final color. If at zero then we only output the diffuse color
    // float m_light_factor; // dicates how much the lights influence the final color. If at zero then we only output the diffuse color but also multipled by ambient occlusion ter
    bool m_auto_edl;
    bool m_enable_edl_lighting;
    float m_edl_strength;
    bool m_enable_surfel_splatting;
    bool m_show_background_img;
    std::string m_background_img_path;
    bool m_enable_ibl; //we need an environment map for ibl
    bool m_show_environment_map; //we can still use ibl without showing the environment map
    bool m_show_prefiltered_environment_map; //show the prefiltered environment which means we can run down the mip maps and show blurred versions of it
    float m_environment_map_blur;
    std::string m_environment_map_path;
    bool m_lights_follow_camera; //if set to true, the movement and the rotation of the main camera will also influence the lights so that they make the same movements as if they are rigidly anchored to the default_camera
    int m_environment_cubemap_resolution; //environment cubemap have 6 faces each with a resolution of m_environment_cubemap_resolution X m_environment_cubemap_resolution
    int m_irradiance_cubemap_resolution;
    int m_prefilter_cubemap_resolution;
    int m_brdf_lut_resolution;
    float m_surfel_blend_factor;
    float m_surfel_blend_scale;
    //params for multi-channel view
    bool m_enable_multichannel_view;
    float m_multichannel_interline_separation; //separation between the lines of different channels. Is a percentage of the screen's width
    float m_multichannel_line_width; //we also draw a white line in between the channels, the width is indicated by this
    float m_multichannel_line_angle; // angle between the lines separating the channels It starts at 0 which means the lines are vertical and goes to 90 when the lines are horizontal
    float m_multichannel_start_x; //the start of the first line, defalt is 0 which means it start on the left

    std::vector< std::shared_ptr<MeshGL> > m_meshes_gl; //stored the gl meshes which will get updated if the meshes in the scene are dirty


    // Eigen::Matrix4f compute_mvp_matrix(const std::shared_ptr<MeshGL>& mesh);
    bool m_first_draw;

    //recorder stuff 
    std::string m_recording_path;
    std::string m_snapshot_name;
    bool m_record_gui;
    bool m_record_with_transparency;

private:
    Viewer(const std::string config_file=std::string(DEFAULT_CONFIG) ); // we put the constructor as private so as to dissalow creating Viewer on the stack because we want to only used shared ptr for it
    // Eigen::Matrix4f compute_mvp_matrix();


    std::vector< std::function<void(Viewer& viewer)> > m_callbacks_pre_draw; //before any drawing happened
    std::vector< std::function<void(Viewer& viewer)> > m_callbacks_post_draw; //after all the drawing happened including the gui

    // float try_float_else_nan(const configuru::Config& cfg); //tries to parse a float and if it fails, returns signaling nan
    void configure_auto_params();
    void read_background_img(gl::Texture2D& tex, const std::string img_path);
    void equirectangular2cubemap(gl::CubeMap& cubemap_tex, const gl::Texture2D& equirectangular_tex);
    void radiance2irradiance(gl::CubeMap& irradiance_tex, const gl::CubeMap& radiance_tex); //precomputes the irradiance around a hemisphere given the radiance
    void prefilter(gl::CubeMap& prefilter_tex, const gl::CubeMap& radiance_tex); //prefilter the radiance tex for various levels of roughness. Used for specular IBL
    void integrate_brdf(gl::Texture2D& brdf_lut_tex);
    void blur_img(gl::Texture2D& img, const int start_mip_map_lvl, const int max_mip_map_lvl, const int m_bloom_blur_iters);
    void apply_postprocess(); //grabs the composed_tex and the bloom_tex and sums them together, applies tone mapping and gamme correction
    void blend_bg(); //takes the post_processed image and blends a solid background color into it if needed.

    bool m_using_fat_gbuffer; //surfel splatting starts requires to use a gbuffer with half floats, this makes is so that there is no need for encoding an decoding normals, we can just sum them

};

} //namespace easy_pbr
