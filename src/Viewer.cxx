#include "easy_pbr/Viewer.h"

//opengl stuff
#include <glad/glad.h> // Initialize with gladLoadGL()
// Include glfw3.h after our OpenGL definitions
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <string> //find_last_of
#include <limits> //signaling_nan

//loguru
#define LOGURU_IMPLEMENTATION 1
#define LOGURU_NO_DATE_TIME 1
#define LOGURU_NO_UPTIME 1
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

#include <glad/glad.h>
#include <GLFW/glfw3.h> //glfw3.h after our OpenGL definitions

//My stuff
#include "UtilsGL.h"
#include "easy_pbr/Scene.h"
#include "easy_pbr/Camera.h"
#include "easy_pbr/MeshGL.h"
#include "easy_pbr/Mesh.h"
#include "easy_pbr/Gui.h"
#include "easy_pbr/SpotLight.h"
#include "easy_pbr/Recorder.h"
#include "easy_pbr/LabelMngr.h"
#include "RandGenerator.h"
#include "opencv_utils.h"
#include "string_utils.h"

#ifdef EASYPBR_WITH_DIR_WATCHER
    #include "dir_watcher/dir_watcher.hpp"
#endif

//Add this header after we add all opengl stuff because we need the profiler to have glFinished defined
#define PROFILER_IMPLEMENTATION 1
#define ENABLE_GL_PROFILING 1
#include "Profiler.h"


//configuru
#define CONFIGURU_IMPLEMENTATION 1
#define CONFIGURU_WITH_EIGEN 1
#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#include <configuru.hpp>
using namespace configuru;

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

using namespace radu::utils;

//ros
// #include "easy_pbr/utils/RosTools.h"

namespace easy_pbr{

Viewer::Viewer(const std::string config_file):
    m_default_camera(new Camera), //create first the cameras because init_params depends on it
    m_camera(m_default_camera),
    dummy_init_params_nongl(  init_params_nongl(config_file) ), //we initialize first the params because the init_context depends on it
    dummy_init_context( init_context() ),
    dummy_glad(gladLoadGL() ),
    dummy_init_params_gl(  init_params_gl(config_file) ), //after initializing the context we can initialzie the rest of gl params
    #ifdef EASYPBR_WITH_DIR_WATCHER
        dir_watcher( new  emilib::DelayedDirWatcher( std::string(PROJECT_SOURCE_DIR)+"/shaders/",5)  ),
    #endif
    m_debug(false),
    m_scene(new Scene),
    // m_gui(new Gui(this, m_window )),
    m_recorder(new Recorder( this )),
    m_rand_gen(new RandGenerator()),
    m_timer(new Timer()),
    m_nr_drawn_frames(0),
    m_viewport_size(1920, 1080),
    m_background_color(0.2, 0.2, 0.2),
    // m_background_color(21.0/255.0, 21.0/255.0, 21.0/255.0),
    m_draw_points_shader("draw_points"),
    m_draw_lines_shader("draw_lines"),
    m_draw_mesh_shader("draw_mesh"),
    m_draw_wireframe_shader("draw_wireframe"),
    m_rvec_tex("rvec_tex"),
    m_fullscreen_quad(MeshGL::create()),
    m_ssao_downsample(1),
    m_nr_samples(64),
    m_kernel_radius(-1),
    m_ao_power(9),
    m_sigma_spacial(2.0),
    m_sigma_depth(0.002),
    m_ambient_color( 71.0/255.0, 70.0/255.0, 66.3/255.0  ),
    m_ambient_color_power(0.05),
    m_enable_culling(false),
    m_enable_ssao(true),
    m_enable_bloom(true),
    m_bloom_threshold(0.85),
    m_bloom_start_mip_map_lvl(1),
    m_bloom_max_mip_map_lvl(5),
    m_bloom_blur_iters(3),
    m_lights_follow_camera(false),
    m_environment_cubemap_resolution(512),
    m_irradiance_cubemap_resolution(32),
    m_prefilter_cubemap_resolution(128),
    m_brdf_lut_resolution(512),
    m_environment_map_blur(0),
    m_using_fat_gbuffer(false),
    m_surfel_blend_factor(-10.0),
    m_surfel_blend_scale(0.0),
    m_enable_multichannel_view(false),
    m_multichannel_interline_separation(0.12), // 20% of the screen's width separation between the lines
    m_multichannel_line_width(10),
    m_multichannel_line_angle(31),
    m_multichannel_start_x(1500),
    m_recording_path("./recordings/"),
    m_snapshot_name("img.png"),
    m_record_gui(false),
    m_record_with_transparency(true),
    m_first_draw(true),
    m_swap_buffers(true),
    m_camera_translation_speed_multiplier(1.0)
    {
        #ifdef EASYPBR_WITH_DIR_WATCHER
            // VLOG(1) << "created viewer with dirwatcher";
        #else
            // VLOG(1) << "Created viewer with no dir watcher";
        #endif
        m_timer->start();
        // m_old_time=m_timer->elapsed_ms();
        // m_camera=m_default_camera;
        // init_params(config_file); //tries to get the configurations and if not present it will get them from the default cfg

        compile_shaders();
        init_opengl();
        m_gui=std::make_shared<Gui>(config_file, this, m_window); //needs to be initialized here because here we have done a gladloadgl

}

Viewer::~Viewer(){
    // LOG(WARNING) << "Destroying viewer";
}

bool Viewer::init_params_nongl(const std::string config_file){

    //read all the parameters
    // Config cfg = configuru::parse_file(std::string(CMAKE_SOURCE_DIR)+"/config/"+config_file, CFG);

    std::string config_file_trim=radu::utils::trim_copy(config_file);
    std::string config_file_abs;
    if (fs::path(config_file_trim).is_relative()){
        config_file_abs=(fs::path(PROJECT_SOURCE_DIR) / config_file_trim).string();
    }else{
        config_file_abs=config_file_trim;
    }

    //get all the default configs and all it's sections
    Config default_cfg = configuru::parse_file(std::string(DEFAULT_CONFIG), CFG);
    Config default_vis_cfg=default_cfg["visualization"];
    Config default_cam_cfg=default_cfg["visualization"]["cam"];
    Config default_scene_cfg=default_cfg["visualization"]["scene"];
    Config default_ssao_cfg=default_cfg["visualization"]["ssao"];
    Config default_bloom_cfg=default_cfg["visualization"]["bloom"];
    Config default_edl_cfg=default_cfg["visualization"]["edl"];
    Config default_bg_cfg=default_cfg["visualization"]["background"];
    Config default_ibl_cfg=default_cfg["visualization"]["ibl"];
    Config default_lights_cfg=default_cfg["visualization"]["lights"];

    //get the current config and if the section is not available, fallback to the default one
    Config cfg = configuru::parse_file(config_file_abs, CFG);
    Config vis_cfg=cfg.get_or("visualization", default_cfg);
    Config cam_cfg=vis_cfg.get_or("cam",default_vis_cfg);
    Config scene_cfg=vis_cfg.get_or("scene",default_vis_cfg);
    Config ssao_cfg=vis_cfg.get_or("ssao",default_vis_cfg);
    Config bloom_cfg=vis_cfg.get_or("bloom",default_vis_cfg);
    Config edl_cfg=vis_cfg.get_or("edl",default_vis_cfg);
    Config bg_cfg=vis_cfg.get_or("background",default_vis_cfg);
    Config ibl_cfg=vis_cfg.get_or("ibl",default_vis_cfg);
    Config lights_cfg=vis_cfg.get_or("lights",default_vis_cfg);

    //attempt 2

    // general
    m_show_gui = vis_cfg.get_or("show_gui", default_vis_cfg);
    m_use_offscreen = vis_cfg.get_or("use_offscreen", default_vis_cfg);
    m_subsample_factor = vis_cfg.get_or("subsample_factor", default_vis_cfg);
    m_enable_culling = vis_cfg.get_or("enable_culling", default_vis_cfg);
    m_render_uv_to_gbuffer= vis_cfg.get_or("render_uv_to_gbuffer", default_vis_cfg);
    std::string tonemap_string= (std::string)vis_cfg.get_or("tonemap", default_vis_cfg);
    //go through the tonemapper and see if we find the one
    bool found_tonemapper=false;
    for (size_t n = 0; n < ToneMapType::_size(); n++) {
        if(ToneMapType::_names()[n] == tonemap_string){
            m_tonemap_type= ToneMapType::_values()[n];
            found_tonemapper=true;
        }
    }
    CHECK(found_tonemapper)<< "Tonemapper type not known";


    //cam
    m_camera->m_fov=cam_cfg.get_float_else_default_else_nan("fov", default_cam_cfg)  ;
    m_camera->m_near=cam_cfg.get_float_else_default_else_nan("near",default_cam_cfg);
    m_camera->m_far=cam_cfg.get_float_else_default_else_nan("far",default_cam_cfg);
    m_camera->m_exposure=cam_cfg.get_float_else_default_else_nan("exposure",default_cam_cfg);

    //scene
    bool floor_visible= scene_cfg.get_or("floor_visible", default_scene_cfg);
    bool floor_metric= scene_cfg.get_or("floor_metric", default_scene_cfg);
    bool automatic_normal_calculation= scene_cfg.get_or("automatic_normal_calculation", default_scene_cfg);
    Scene::set_floor_visible(floor_visible);
    Scene::set_floor_metric(floor_metric);
    Scene::set_automatic_normal_calculation(automatic_normal_calculation);



    // //ssao
    m_auto_ssao= ssao_cfg.get_or("auto_settings", default_ssao_cfg);
    m_enable_ssao = ssao_cfg.get_or("enable_ssao", default_ssao_cfg);
    m_ssao_downsample = ssao_cfg.get_or("ao_downsample", default_ssao_cfg);
    m_kernel_radius = ssao_cfg.get_float_else_default_else_nan("kernel_radius", default_ssao_cfg);
    m_nr_samples = ssao_cfg.get_or("nr_samples", default_ssao_cfg);
    m_max_ssao_distance = ssao_cfg.get_or("max_ssao_distance", default_ssao_cfg);
    m_ao_power = ssao_cfg.get_or("ao_power", default_ssao_cfg);
    m_sigma_spacial = ssao_cfg.get_or("ao_blur_sigma_spacial", default_ssao_cfg);
    m_sigma_depth = ssao_cfg.get_or("ao_blur_sigma_depth", default_ssao_cfg);
    m_ssao_estimate_normals_from_depth= ssao_cfg.get_or("ssao_estimate_normals_from_depth", default_ssao_cfg);

    // //bloom
    m_enable_bloom = bloom_cfg.get_or("enable_bloom", default_bloom_cfg);
    m_bloom_threshold = bloom_cfg.get_or("threshold", default_bloom_cfg);
    m_bloom_start_mip_map_lvl = bloom_cfg.get_or("start_mip_map_lvl", default_bloom_cfg);
    m_bloom_max_mip_map_lvl = bloom_cfg.get_or("max_mip_map_lvl", default_bloom_cfg);
    m_bloom_blur_iters = bloom_cfg.get_or("blur_iters", default_bloom_cfg);

    // //edl
    m_auto_edl= edl_cfg.get_or("auto_settings", default_edl_cfg);
    m_enable_edl_lighting= edl_cfg.get_or("enable_edl_lighting", default_edl_cfg);
    m_edl_strength = edl_cfg.get_or("edl_strength", default_edl_cfg);

    // //background
    m_show_background_img = bg_cfg.get_or("show_background_img", default_bg_cfg);
    m_background_img_path = (std::string)bg_cfg.get_or("background_img_path", default_bg_cfg);

    // //ibl
    m_enable_ibl = ibl_cfg.get_or("enable_ibl", default_ibl_cfg);
    m_show_environment_map = ibl_cfg.get_or("show_environment_map", default_ibl_cfg);
    m_show_prefiltered_environment_map = ibl_cfg.get_or("show_prefiltered_environment_map", default_ibl_cfg);
    m_environment_map_blur = ibl_cfg.get_or("environment_map_blur", default_ibl_cfg);
    m_environment_map_path = (fs::path(EASYPBR_DATA_DIR) / (std::string)ibl_cfg.get_or("environment_map_path", default_ibl_cfg) ).string();
    m_environment_cubemap_resolution = ibl_cfg.get_or("environment_cubemap_resolution", default_ibl_cfg);
    m_irradiance_cubemap_resolution = ibl_cfg.get_or("irradiance_cubemap_resolution", default_ibl_cfg);
    m_prefilter_cubemap_resolution = ibl_cfg.get_or("prefilter_cubemap_resolution", default_ibl_cfg);
    m_brdf_lut_resolution = ibl_cfg.get_or("brdf_lut_resolution", default_ibl_cfg);

    // //create the spot lights
    // int nr_spot_lights = lights_cfg.get_or("nr_spot_lights", default_lights_cfg);
    // for(int i=0; i<nr_spot_lights; i++){
    //     Config light_cfg=lights_cfg.get_or("spot_light_"+std::to_string(i), default_lights_cfg);
    //     std::shared_ptr<SpotLight> light=  Generic::SmartPtrBuilder::CreateSharedPtr< SpotLight, Camera >(new SpotLight(light_cfg));
    //     m_spot_lights.push_back(light);
    // }

    return true;

}


bool Viewer::init_params_gl(const std::string config_file){

    //read all the parameters
    // Config cfg = configuru::parse_file(std::string(CMAKE_SOURCE_DIR)+"/config/"+config_file, CFG);

    std::string config_file_trim=radu::utils::trim_copy(config_file);
    std::string config_file_abs;
    if (fs::path(config_file_trim).is_relative()){
        config_file_abs=(fs::path(PROJECT_SOURCE_DIR) / config_file_trim).string();
    }else{
        config_file_abs=config_file_trim;
    }

    //get all the default configs and all it's sections
    Config default_cfg = configuru::parse_file(std::string(DEFAULT_CONFIG), CFG);
    Config default_vis_cfg=default_cfg["visualization"];
    Config default_cam_cfg=default_cfg["visualization"]["cam"];
    Config default_scene_cfg=default_cfg["visualization"]["scene"];
    Config default_ssao_cfg=default_cfg["visualization"]["ssao"];
    Config default_bloom_cfg=default_cfg["visualization"]["bloom"];
    Config default_edl_cfg=default_cfg["visualization"]["edl"];
    Config default_bg_cfg=default_cfg["visualization"]["background"];
    Config default_ibl_cfg=default_cfg["visualization"]["ibl"];
    Config default_lights_cfg=default_cfg["visualization"]["lights"];

    //get the current config and if the section is not available, fallback to the default one
    Config cfg = configuru::parse_file(config_file_abs, CFG);
    Config vis_cfg=cfg.get_or("visualization", default_cfg);
    Config cam_cfg=vis_cfg.get_or("cam",default_vis_cfg);
    Config scene_cfg=vis_cfg.get_or("scene",default_vis_cfg);
    Config ssao_cfg=vis_cfg.get_or("ssao",default_vis_cfg);
    Config bloom_cfg=vis_cfg.get_or("bloom",default_vis_cfg);
    Config edl_cfg=vis_cfg.get_or("edl",default_vis_cfg);
    Config bg_cfg=vis_cfg.get_or("background",default_vis_cfg);
    Config ibl_cfg=vis_cfg.get_or("ibl",default_vis_cfg);
    Config lights_cfg=vis_cfg.get_or("lights",default_vis_cfg);

    //attempt 2

    // // general
    // m_show_gui = vis_cfg.get_or("show_gui", default_vis_cfg);
    // m_use_offscreen = vis_cfg.get_or("use_offscreen", default_vis_cfg);
    // m_subsample_factor = vis_cfg.get_or("subsample_factor", default_vis_cfg);
    // m_enable_culling = vis_cfg.get_or("enable_culling", default_vis_cfg);
    // m_render_uv_to_gbuffer= vis_cfg.get_or("render_uv_to_gbuffer", default_vis_cfg);
    // std::string tonemap_string= (std::string)vis_cfg.get_or("tonemap", default_vis_cfg);
    // //go through the tonemapper and see if we find the one
    // bool found_tonemapper=false;
    // for (size_t n = 0; n < ToneMapType::_size(); n++) {
    //     if(ToneMapType::_names()[n] == tonemap_string){
    //         m_tonemap_type= ToneMapType::_values()[n];
    //         found_tonemapper=true;
    //     }
    // }
    // CHECK(found_tonemapper)<< "Tonemapper type not known";


    // //cam
    // m_camera->m_fov=cam_cfg.get_float_else_default_else_nan("fov", default_cam_cfg)  ;
    // m_camera->m_near=cam_cfg.get_float_else_default_else_nan("near",default_cam_cfg);
    // m_camera->m_far=cam_cfg.get_float_else_default_else_nan("far",default_cam_cfg);
    // m_camera->m_exposure=cam_cfg.get_float_else_default_else_nan("exposure",default_cam_cfg);

    // //scene
    // bool floor_visible= scene_cfg.get_or("floor_visible", default_scene_cfg);
    // bool floor_metric= scene_cfg.get_or("floor_metric", default_scene_cfg);
    // bool automatic_normal_calculation= scene_cfg.get_or("automatic_normal_calculation", default_scene_cfg);
    // Scene::set_floor_visible(floor_visible);
    // Scene::set_floor_metric(floor_metric);
    // Scene::set_automatic_normal_calculation(automatic_normal_calculation);



    // // //ssao
    // m_auto_ssao= ssao_cfg.get_or("auto_settings", default_ssao_cfg);
    // m_enable_ssao = ssao_cfg.get_or("enable_ssao", default_ssao_cfg);
    // m_ssao_downsample = ssao_cfg.get_or("ao_downsample", default_ssao_cfg);
    // m_kernel_radius = ssao_cfg.get_float_else_default_else_nan("kernel_radius", default_ssao_cfg);
    // m_max_ssao_distance = ssao_cfg.get_or("max_ssao_distance", default_ssao_cfg);
    // m_ao_power = ssao_cfg.get_or("ao_power", default_ssao_cfg);
    // m_sigma_spacial = ssao_cfg.get_or("ao_blur_sigma_spacial", default_ssao_cfg);
    // m_sigma_depth = ssao_cfg.get_or("ao_blur_sigma_depth", default_ssao_cfg);
    // m_ssao_estimate_normals_from_depth= ssao_cfg.get_or("ssao_estimate_normals_from_depth", default_ssao_cfg);

    // // //bloom
    // m_enable_bloom = bloom_cfg.get_or("enable_bloom", default_bloom_cfg);
    // m_bloom_threshold = bloom_cfg.get_or("threshold", default_bloom_cfg);
    // m_bloom_start_mip_map_lvl = bloom_cfg.get_or("start_mip_map_lvl", default_bloom_cfg);
    // m_bloom_max_mip_map_lvl = bloom_cfg.get_or("max_mip_map_lvl", default_bloom_cfg);
    // m_bloom_blur_iters = bloom_cfg.get_or("blur_iters", default_bloom_cfg);

    // // //edl
    // m_auto_edl= edl_cfg.get_or("auto_settings", default_edl_cfg);
    // m_enable_edl_lighting= edl_cfg.get_or("enable_edl_lighting", default_edl_cfg);
    // m_edl_strength = edl_cfg.get_or("edl_strength", default_edl_cfg);

    // // //background
    // m_show_background_img = bg_cfg.get_or("show_background_img", default_bg_cfg);
    // m_background_img_path = (std::string)bg_cfg.get_or("background_img_path", default_bg_cfg);

    // // //ibl
    // m_enable_ibl = ibl_cfg.get_or("enable_ibl", default_ibl_cfg);
    // m_show_environment_map = ibl_cfg.get_or("show_environment_map", default_ibl_cfg);
    // m_show_prefiltered_environment_map = ibl_cfg.get_or("show_prefiltered_environment_map", default_ibl_cfg);
    // m_environment_map_blur = ibl_cfg.get_or("environment_map_blur", default_ibl_cfg);
    // m_environment_map_path = (fs::path(EASYPBR_DATA_DIR) / (std::string)ibl_cfg.get_or("environment_map_path", default_ibl_cfg) ).string();
    // m_environment_cubemap_resolution = ibl_cfg.get_or("environment_cubemap_resolution", default_ibl_cfg);
    // m_irradiance_cubemap_resolution = ibl_cfg.get_or("irradiance_cubemap_resolution", default_ibl_cfg);
    // m_prefilter_cubemap_resolution = ibl_cfg.get_or("prefilter_cubemap_resolution", default_ibl_cfg);
    // m_brdf_lut_resolution = ibl_cfg.get_or("brdf_lut_resolution", default_ibl_cfg);

    //create the spot lights
    int nr_spot_lights = lights_cfg.get_or("nr_spot_lights", default_lights_cfg);
    for(int i=0; i<nr_spot_lights; i++){
        Config light_cfg=lights_cfg.get_or("spot_light_"+std::to_string(i), default_lights_cfg);
        std::shared_ptr<SpotLight> light=  Generic::SmartPtrBuilder::CreateSharedPtr< SpotLight, Camera >(new SpotLight(light_cfg));
        m_spot_lights.push_back(light);
    }

    return true;

}

bool Viewer::init_context(){
    // GLFWwindow* window;
    int window_width, window_height;
    window_width=640;
    window_height=480;

     // Setup window
    if (!glfwInit()){
        LOG(FATAL) << "GLFW could not initialize";
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    // VLOG(1) << " m_use_offscreen " <<
    if(m_use_offscreen){
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    }
    m_window = glfwCreateWindow(window_width, window_height, "Renderer",nullptr,nullptr);
    if (!m_window){
        LOG(FATAL) << "GLFW window creation failed. It may be that you are requesting a too high version of opengl that is not supported by your drivers. It may happen especially if you are running mesa drivers instead of nvidia.";
        glfwTerminate();
    }
    glfwMakeContextCurrent(m_window);
    // Load OpenGL and its extensions
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)){
        LOG(FATAL) << "GLAD failed to load";
    }
    // glfwSwapInterval(1); // Enable vsync

    glfwSetInputMode(m_window,GLFW_CURSOR,GLFW_CURSOR_NORMAL);

    //window will be resized to the screen so we can return the actual widnow values now
    glfwGetWindowSize(m_window, &window_width, &window_height);
    m_viewport_size<< window_width, window_height;


    glfwSetWindowUserPointer(m_window, this); // so in the glfw we can acces the viewer https://stackoverflow.com/a/28660673
    setup_callbacks_viewer(m_window);

    return true;
}

 void Viewer::setup_callbacks_viewer(GLFWwindow* window){
        // https://stackoverflow.com/a/28660673
        auto mouse_pressed_func = [](GLFWwindow* w, int button, int action, int modifier) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_mouse_pressed( w, button, action, modifier );     };
        auto mouse_move_func = [](GLFWwindow* w, double x, double y) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_mouse_move( w, x, y );     };
        auto mouse_scroll_func = [](GLFWwindow* w, double x, double y) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_mouse_scroll( w, x, y );     };
        auto key_func = [](GLFWwindow* w, int key, int scancode, int action, int modifier) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_key( w, key, scancode, action, modifier );     };
        auto char_mods_func  = [](GLFWwindow* w, unsigned int codepoint, int modifier) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_char_mods( w, codepoint, modifier );     };
        auto resize_func = [](GLFWwindow* w, int width, int height) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_resize( w, width, height );     };
        auto drop_func = [](GLFWwindow* w, int count, const char** paths) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->glfw_drop( w, count, paths );     };


        glfwSetMouseButtonCallback(window, mouse_pressed_func);
        glfwSetCursorPosCallback(window, mouse_move_func);
        glfwSetScrollCallback(window,mouse_scroll_func);
        glfwSetKeyCallback(window, key_func);
        glfwSetCharModsCallback(window,char_mods_func);
        glfwSetWindowSizeCallback(window,resize_func);
        glfwSetDropCallback(window, drop_func);

}

void Viewer::setup_callbacks_imgui(GLFWwindow* window){
    auto imgui_drop_func = [](GLFWwindow* w, int count, const char** paths) {   static_cast<Viewer*>(glfwGetWindowUserPointer(w))->imgui_drop( w, count, paths );     };

    glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
    glfwSetCursorPosCallback(window, nullptr);
    glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
    glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
    glfwSetCharModsCallback(window, nullptr);
    glfwSetDropCallback(window, imgui_drop_func);
}


void Viewer::switch_callbacks(GLFWwindow* window) {
    // bool hovered_imgui = ImGui::IsMouseHoveringAnyWindow();
    bool using_imgui = ImGui::GetIO().WantCaptureMouse || ImGui::GetIO().WantCaptureKeyboard;
    // VLOG(1) << "using imgui is " << using_imgui;

    // glfwSetMouseButtonCallback(window, nullptr);
    // glfwSetCursorPosCallback(window, nullptr);
    // glfwSetScrollCallback(window, nullptr);
    // glfwSetKeyCallback(window, nullptr);
    // glfwSetCharModsCallback(window, nullptr);
    // // glfwSetWindowSizeCallback(window, nullptr); //this should just be left to the one resize_func in this viewer
    // glfwSetCharCallback(window, nullptr);

    if (using_imgui) {
        setup_callbacks_imgui(window);
    } else {
        setup_callbacks_viewer(window);
    }
}

void Viewer::compile_shaders(){

    m_draw_points_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/points_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/points_frag.glsl" ) ;
    m_draw_lines_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/lines_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/lines_frag.glsl"  );
    m_draw_mesh_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/mesh_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/mesh_frag.glsl"  );
    m_draw_wireframe_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/wireframe_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/wireframe_frag.glsl"  );
    m_draw_surfels_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/render/surfels_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/surfels_frag.glsl" , std::string(EASYPBR_SHADERS_PATH)+"/render/surfels_geom.glsl" );
    m_draw_normals_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/render/normals_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/normals_frag.glsl" , std::string(EASYPBR_SHADERS_PATH)+"/render/normals_geom.glsl" );
    m_compose_final_quad_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/compose_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/compose_frag.glsl"  );
    m_blur_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/blur_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/blur_frag.glsl"  );
    m_apply_postprocess_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/apply_postprocess_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/apply_postprocess_frag.glsl"  );
    m_blend_bg_shader.compile( std::string(EASYPBR_SHADERS_PATH)+"/render/blend_bg_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/render/blend_bg_frag.glsl"  );

    m_ssao_ao_pass_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ssao/ao_pass_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ssao/ao_pass_frag.glsl" );
    // m_ssao_ao_pass_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ssao/ao_pass_pure_depth_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ssao/ao_pass_pure_depth_frag.glsl" );
    // m_depth_linearize_shader.compile(std::string(PROJECT_SOURCE_DIR)+"/shaders/ssao/depth_linearize_compute.glsl");
    m_depth_linearize_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ssao/depth_linearize_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ssao/depth_linearize_frag.glsl");
    m_bilateral_blur_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ssao/bilateral_blur_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ssao/bilateral_blur_frag.glsl");

    m_equirectangular2cubemap_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ibl/equirectangular2cubemap_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ibl/equirectangular2cubemap_frag.glsl");
    m_radiance2irradiance_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ibl/radiance2irradiance_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ibl/radiance2irradiance_frag.glsl");
    m_prefilter_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ibl/prefilter_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ibl/prefilter_frag.glsl");
    m_integrate_brdf_shader.compile(std::string(EASYPBR_SHADERS_PATH)+"/ibl/integrate_brdf_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/ibl/integrate_brdf_frag.glsl");

    //debugging shaders
    m_decode_gbuffer_debugging.compile( std::string(EASYPBR_SHADERS_PATH)+"/debug/decode_gbuffer_vert.glsl", std::string(EASYPBR_SHADERS_PATH)+"/debug/decode_gbuffer_frag.glsl"  );
}

void Viewer::init_opengl(){
    // //initialize the g buffer with some textures
    GL_C( m_gbuffer.set_size(m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor ) ); //established what will be the size of the textures attached to this framebuffer
    GL_C( m_gbuffer.add_texture("diffuse_gtex", GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE) );
    // GL_C( m_gbuffer.add_texture("normal_gtex", GL_RG16F, GL_RG, GL_HALF_FLOAT) );  //as done by Cry Engine 3 in their presentation "A bit more deferred"  https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
    GL_C( m_gbuffer.add_texture("normal_gtex", GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE) );
    GL_C( m_gbuffer.add_texture("metalness_and_roughness_gtex", GL_RG8, GL_RG, GL_UNSIGNED_BYTE) );
    GL_C( m_gbuffer.add_texture("mesh_id_gtex", GL_R8I, GL_RED_INTEGER, GL_INT) );
    GL_C( m_gbuffer.add_depth("depth_gtex") );
    if (m_render_uv_to_gbuffer){
        GL_C( m_gbuffer.add_texture("uv_gtex", GL_RG32F, GL_RG, GL_FLOAT) );
    }
    m_gbuffer.sanity_check();

    //we compose the gbuffer into this fbo, together with a bloom texture for storing the birght areas
    m_composed_fbo.set_size(m_gbuffer.width(), m_gbuffer.height() ); //established what will be the size of the textures attached to this framebuffer
    m_composed_fbo.add_texture("composed_gtex", GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT);
    m_composed_fbo.add_texture("bloom_gtex", GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT);
    m_composed_fbo.sanity_check();

    //initialize the final fbo
    GL_C( m_final_fbo_no_gui.set_size(m_gbuffer.width(), m_gbuffer.height() ) ); //established what will be the size of the textures attached to this framebuffer
    GL_C( m_final_fbo_no_gui.add_texture("color_with_transparency_gtex", GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) );
    GL_C( m_final_fbo_no_gui.add_texture("color_without_transparency_gtex", GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE) );
    GL_C( m_final_fbo_no_gui.add_depth("depth_gtex") ); //we need a depth for this one too because in this buffer we do all the forward rendering of lines and things like that
    m_final_fbo_no_gui.sanity_check();
    //initilize the final_fbo which also has the gui
    GL_C( m_final_fbo_with_gui.set_size(m_viewport_size.x(), m_viewport_size.y() ) ); //established what will be the size of the textures attached to this framebuffer
    GL_C( m_final_fbo_with_gui.add_texture("color_gtex", GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE) );
    m_final_fbo_with_gui.sanity_check();



    //set all the normal buffer to nearest because we assume that the norm of it values can be used to recover the n.z. However doing a nearest neighbour can change the norm and therefore fuck everything up
    m_gbuffer.tex_with_name("normal_gtex").set_filter_mode_min_mag(GL_NEAREST);



    //cubemaps
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); //to linearly filter across faces of the cube
    m_environment_cubemap_tex.allocate_tex_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_environment_cubemap_resolution, m_environment_cubemap_resolution);
    m_irradiance_cubemap_tex.allocate_tex_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_irradiance_cubemap_resolution, m_irradiance_cubemap_resolution);
    m_prefilter_cubemap_tex.allocate_tex_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_prefilter_cubemap_resolution, m_prefilter_cubemap_resolution);
    m_prefilter_cubemap_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);
    m_prefilter_cubemap_tex.set_filter_mode_mag(GL_LINEAR);
    m_prefilter_cubemap_tex.generate_mipmap_full();

    //brdf_lut_tex
    m_brdf_lut_tex.allocate_storage(GL_RG16F, GL_RG, GL_HALF_FLOAT, m_brdf_lut_resolution, m_brdf_lut_resolution);

    m_rvec_tex.set_wrap_mode(GL_REPEAT); //so we repeat the random vectors specified in this 4x4 matrix over the whole image

    // make a 4x4 texture for the random vectors that will be used for rotating the hemisphere
    int rvec_tex_size=4;
    cv::Mat rvec_mat=cv::Mat(rvec_tex_size, rvec_tex_size, CV_32FC4);
    for(int x=0; x<rvec_tex_size; x++){
        for(int y=0; y<rvec_tex_size; y++){
            Eigen::Vector3f rvec;
            rvec.x()=m_rand_gen->rand_float(-1.0f, 1.0f);
            rvec.y()=m_rand_gen->rand_float(-1.0f, 1.0f);
            rvec.z()=0.0;
            // rvec.normalize();
            rvec_mat.at<cv::Vec4f>(y, x)[0]=rvec.x();
            rvec_mat.at<cv::Vec4f>(y, x)[1]=rvec.y();
            rvec_mat.at<cv::Vec4f>(y, x)[2]=0.0; //set to 0 because we rotate along the z axis of the hemisphere
            rvec_mat.at<cv::Vec4f>(y, x)[3]=1.0; //not actually used but we only use textures of channels 1,2 and 4
        }
    }
    GL_C( m_rvec_tex.upload_from_cv_mat(rvec_mat,false) );
    //make some random samples in a hemisphere
    create_random_samples_hemisphere();



    //create a fullscreen quad which we will use for composing the final image after the deffrred render pass
    m_fullscreen_quad->m_core->create_full_screen_quad();
    GL_C( m_fullscreen_quad->upload_to_gpu() );

    //add the background image
    if(m_show_background_img){
        read_background_img(m_background_tex, m_background_img_path);
    }



    //initialize a cubemap
    integrate_brdf(m_brdf_lut_tex); //we leave it outside the if because when we drag some hdr map into the viewer we don't want to integrate the brdf every time
    if(m_enable_ibl){
        load_environment_map(m_environment_map_path);
    }

    //load a dummy uv_checkermap
    cv::Mat mat = cv::imread(std::string(EASYPBR_DATA_DIR)+"/uv_checker.png");
    m_uv_checker_tex.upload_from_cv_mat(mat, false);


}

void Viewer::hotload_shaders(){
    #ifdef EASYPBR_WITH_DIR_WATCHER
        // VLOG(1) << "chekcing hotload";
        std::vector<std::string> changed_files=dir_watcher->poll_files();
        if(changed_files.size()>0){
            compile_shaders();
        }
    #else
        // VLOG(1) << "not using dirwatcher";
    #endif
}

void Viewer::configure_auto_params(){
    Eigen::Vector3f centroid = m_scene->get_centroid();
    float scale = m_scene->get_scale();

    // std::cout << " scene centroid " << centroid << std::endl;
    // std::cout << " scene scale " << scale << std::endl;


    // //CAMERA------------
    // if (!m_camera->m_is_initialized){
    //     if (!m_camera->m_lookat_initialized){
    //         m_camera->set_lookat(centroid);
    //     }
    //     if (!m_camera->m_position_initialized){
    //         m_camera->set_position(centroid+Eigen::Vector3f::UnitZ()*5*scale+Eigen::Vector3f::UnitY()*0.5*scale); //move the eye backwards so that is sees the whole scene
    //     }
    //     if (std::isnan(m_camera->m_fov) ){ //signaling nan indicates we should automatically set the values
    //         m_camera->m_fov=30 ;
    //     }
    //     if (std::isnan(m_camera->m_near) ){ //signaling nan indicates we should automatically set the values
    //         m_camera->m_near=( (centroid-m_camera->position()).norm()*0.01 ) ;
    //     }
    //     if (std::isnan(m_camera->m_far) ){
    //         m_camera->m_far=( (centroid-m_camera->position()).norm()*10 ) ;
    //     }
    //     if (std::isnan(m_camera->m_exposure) ){ //signaling nan indicates we should automatically set the values
    //         m_camera->m_exposure=1.0;
    //      }

    //     m_camera->m_is_initialized=true;
    // }

    //SSAO---------------
    if(m_auto_ssao){
        //enable the ssao only if all the meshes have normals
        m_enable_ssao=false;
        for(int i=0; i<m_scene->nr_meshes(); i++){
            if (m_scene->get_mesh_with_idx(i)->NV.size() ){
                m_enable_ssao=true;
                break;
            }
        }

        //set the settings
        if (std::isnan(m_kernel_radius) ){
            m_kernel_radius=0.05*scale;
        }
    }

    //EDL--------
    if (m_auto_edl ){
        //we enable edl only if all the meshes in the scene don't show any meshes
        m_enable_edl_lighting=true;
        for(int i=0; i<m_scene->nr_meshes(); i++){
            if (m_scene->get_mesh_with_idx(i)->m_vis.m_show_mesh){
                m_enable_edl_lighting=false;
                break;
            }
        }
    }


    //LIGHTS-----------------------
    //key light
    if(m_spot_lights.size()>=1){
        std::shared_ptr<SpotLight> key = m_spot_lights[0];
        Eigen::Vector3f dir_movement;
        dir_movement<<0.5, 0.6, 0.5;
        dir_movement=dir_movement.normalized();
        if (!key->m_lookat_initialized){
            key->set_lookat(centroid);
        }
        if(!key->m_position_initialized){
            key->set_position(centroid+dir_movement*3*scale); //move the light starting from the center in the direction by a certain amout so that in engulfs the whole scene
        }
        key->m_near=( (centroid-key->position()).norm()*0.1 ) ;
        key->m_far=( (centroid-key->position()).norm()*10 ) ;
        key->m_fov=40;
        if (std::isnan(key->m_power) ){
            key->set_power_for_point(centroid, 3); //sets the power so that the lookatpoint, after attenuating, gets a certain intesity
        }
        if (!key->m_color.allFinite()){
            // key->m_color<< 255.0/255.0, 185.0/255.0, 100/255.0;
            key->m_color<< 255.0/255.0, 221.0/255.0, 180/255.0;
        }
    }
    //fill light
    if(m_spot_lights.size()>=2){
        std::shared_ptr<SpotLight> fill = m_spot_lights[1];
        Eigen::Vector3f dir_movement;
        dir_movement<< -0.5, 0.6, 0.5;
        dir_movement=dir_movement.normalized();
        if (!fill->m_lookat_initialized){
            fill->set_lookat(centroid);
        }
        if (!fill->m_position_initialized){
            fill->set_position(centroid+dir_movement*3*scale); //move the light starting from the center in the direction by a certain amout so that in engulfs the whole scene
        }
        fill->m_near=( (centroid-fill->position()).norm()*0.1 ) ;
        fill->m_far=( (centroid-fill->position()).norm()*10 ) ;
        fill->m_fov=40;
        if (std::isnan(fill->m_power) ){
            fill->set_power_for_point(centroid, 0.8); //sets the power so that the lookatpoint, after attenuating, gets a certain intesity
        }
        if (!fill->m_color.allFinite()){
            fill->m_color<< 118.0/255.0, 255.0/255.0, 230/255.0;
        }
    }
    //rim light
    if(m_spot_lights.size()>=3){
        std::shared_ptr<SpotLight> rim = m_spot_lights[2];
        Eigen::Vector3f dir_movement;
        dir_movement<< -0.5, 0.6, -0.5;
        dir_movement=dir_movement.normalized();
        if (!rim->m_lookat_initialized){
            rim->set_lookat(centroid);
        }
        if (!rim->m_position_initialized){
            rim->set_position(centroid+dir_movement*3*scale); //move the light starting from the center in the direction by a certain amout so that in engulfs the whole scene
        }
        rim->m_near=( (centroid-rim->position()).norm()*0.1 ) ;
        rim->m_far=( (centroid-rim->position()).norm()*10 ) ;
        rim->m_fov=40;
        if (std::isnan(rim->m_power) ){
            rim->set_power_for_point(centroid, 3); //sets the power so that the lookatpoint, after attenuating, gets a certain intesity
        }
        if (!rim->m_color.allFinite()){
            // rim->m_color<< 100.0/255.0, 210.0/255.0, 255.0/255.0;
            rim->m_color<< 157.0/255.0, 227.0/255.0, 255.0/255.0;
        }
    }
}

void Viewer::configure_camera(){
    if (m_scene->is_empty()){
        return;
    }

    //CAMERA------------
    if (!m_camera->m_is_initialized){
        Eigen::Vector3f centroid = m_scene->get_centroid();
        float scale = m_scene->get_scale();
        if (!m_camera->m_lookat_initialized){
            m_camera->set_lookat(centroid);
        }
        if (!m_camera->m_position_initialized){
            m_camera->set_position(centroid+Eigen::Vector3f::UnitZ()*5*scale+Eigen::Vector3f::UnitY()*0.5*scale); //move the eye backwards so that is sees the whole scene
        }
        if (std::isnan(m_camera->m_fov) ){ //signaling nan indicates we should automatically set the values
            m_camera->m_fov=30 ;
        }
        if (std::isnan(m_camera->m_near) ){ //signaling nan indicates we should automatically set the values
            m_camera->m_near=( (centroid-m_camera->position()).norm()*0.01 ) ;
        }
        if (std::isnan(m_camera->m_far) ){
            m_camera->m_far=( (centroid-m_camera->position()).norm()*10 ) ;
        }
        if (std::isnan(m_camera->m_exposure) ){ //signaling nan indicates we should automatically set the values
            m_camera->m_exposure=1.0;
         }

        m_camera->m_is_initialized=true;
    }
}

void Viewer::add_callback_pre_draw(const std::function<void(Viewer& viewer)> func){
    m_callbacks_pre_draw.push_back(func);
}
void Viewer::add_callback_post_draw(const std::function<void(Viewer& viewer)> func){
    m_callbacks_post_draw.push_back(func);
}

void Viewer::update(const GLuint fbo_id){
    // //make some fuzzy timer updates like in https://medium.com/@tglaiel/how-to-make-your-game-run-at-60fps-24c61210fe75
    // // if(!m_timer->is_running() && !m_first_draw){ // we do not start the timer on the first draw because the first frame always takes the longest
    // if(!m_timer->is_running()){ // we do not start the timer on the first draw because the first frame always takes the longest
    //     m_timer->start();
    //     m_old_time=m_timer->elapsed_s();
    //     m_accumulator_time=0;
    // }

    // double current_time=m_timer->elapsed_s();
    // double delta_time = current_time-m_old_time;
    // m_old_time = current_time;
    // m_accumulator_time += delta_time;
    // if(m_accumulator_time>8.0/60.0) m_accumulator_time=8.0/60.0; //cam the maximum accumulator time so that it doeesnt become arbitartly large and make the code spend all the time in the internal while loop


    // while(m_accumulator_time > 1.0/32.0){
    //     // update()
    //     // VLOG(1) << m_accumulator_time;
    //     // VLOG(1) << "render";

    //     pre_draw();
    //     draw(fbo_id);

    //     if(m_show_gui){
    //         m_gui->update();
    //     }

    //     post_draw();
    //     switch_callbacks(m_window);


    //     m_accumulator_time -= 1.0/30.0;
    //     if(m_accumulator_time < 0) m_accumulator_time = 0;

    //     // //debug
    //     // if(m_accumulator_time>300){
    //     //     m_accumulator_time=0;
    //     // }
    // }


    // //test the intrincis to proj and back
    // std::cout.precision(11);
    // std::setprecision(20);
    // Eigen::Matrix3f K;
    // K.setIdentity();
    // K(0,0)=2264.805259224865;
    // K(1,1)=2264.9097603949817;
    // K(0,2)= 1069.6593940596147;
    // K(1,2)=  782.3950731603901;
    // int width=4;
    // int height=4;
    // Eigen::Matrix4f P = intrinsics_to_opengl_proj(K, width, height);
    // VLOG(1) <<  std::fixed << "P is \n " << P;
    // Eigen::Matrix3f new_K = opengl_proj_to_intrinsics(P, width, height);
    // VLOG(1) << "new K is " << new_K;


    pre_draw();
    draw(fbo_id);

    if(m_show_gui){
        m_gui->update(); //queues the rendering of the widgets but doesnt yet render them. This happens in the post draw with imgui::render. Therefore post_draw callbacks can still add more widgets
    }

    post_draw();
    switch_callbacks(m_window);

    m_nr_drawn_frames++;
}

void Viewer::pre_draw(){
    glfwPollEvents();
    if(m_show_gui){
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    // VLOG(1) << "camera is " << m_camera;
    // VLOG(1) << "camera pos is " << m_camera->position();

    for(size_t i=0; i<m_callbacks_pre_draw.size(); i++){
        m_callbacks_pre_draw[i](*this);
    }

}

void Viewer::post_draw(){

    //call any callbacks before we finish the imgui frame so the callbacks have a chance to insert some imgui code
    for(size_t i=0; i<m_callbacks_post_draw.size(); i++){
        m_callbacks_post_draw[i](*this);
    }

    //blit into the fbo_with_gui
    glViewport(0.0f , 0.0f, m_viewport_size.x(), m_viewport_size.y() );
    if(m_viewport_size.x()!=m_final_fbo_with_gui.width() || m_viewport_size.y()!=m_final_fbo_with_gui.height()){
        m_final_fbo_with_gui.set_size(m_viewport_size.x(), m_viewport_size.y() );
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_final_fbo_no_gui.tex_with_name("color_without_transparency_gtex").fbo_id() );
    m_final_fbo_with_gui.bind_for_draw();
    glBlitFramebuffer(0, 0, m_final_fbo_no_gui.width(), m_final_fbo_no_gui.height(), 0, 0, m_final_fbo_with_gui.width(),  m_final_fbo_with_gui.height(), GL_COLOR_BUFFER_BIT, GL_LINEAR);

    if(m_show_gui){
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }


    // finally just blit the final fbo to the default framebuffer
    glViewport(0.0f , 0.0f, m_viewport_size.x(), m_viewport_size.y() );
    // m_final_fbo_no_gui.bind_for_read();
    m_final_fbo_with_gui.bind_for_read();
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    // glBlitFramebuffer(0, 0, m_final_fbo_no_gui.width(), m_final_fbo_no_gui.height(), 0, 0, m_viewport_size.x(), m_viewport_size.y(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBlitFramebuffer(0, 0, m_final_fbo_with_gui.width(), m_final_fbo_with_gui.height(), 0, 0, m_viewport_size.x(), m_viewport_size.y(), GL_COLOR_BUFFER_BIT, GL_NEAREST);


    if (m_swap_buffers){
        glfwSwapBuffers(m_window);
    }

    // m_recorder->update();
    if (m_recorder->is_recording()){
        std::string next_img = std::to_string(m_recorder->nr_images_recorded()) +".png";
        // m_recorder->record(next_img, m_gui->m_recording_path);

            if(m_record_gui){
                m_recorder->record(m_final_fbo_with_gui.tex_with_name("color_gtex"), next_img, m_recording_path);
            }else{
                if (m_record_with_transparency){
                    m_recorder->record( m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex") , next_img, m_recording_path);
                }else{
                    m_recorder->record( m_final_fbo_no_gui.tex_with_name("color_without_transparency_gtex") , next_img, m_recording_path);
                }
            }

    }
}


void Viewer::draw(const GLuint fbo_id){

    TIME_SCOPE("draw");
    hotload_shaders();

    //GL PARAMS--------------
    if(m_enable_culling){
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }else{
        glDisable(GL_CULL_FACE);
    }
    glBindFramebuffer(GL_FRAMEBUFFER,fbo_id);
    clear_framebuffers();
    glViewport(0.0f , 0.0f, m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor );
    glEnable(GL_DEPTH_TEST);


    //set the camera to that it sees the whole scene
    if(m_first_draw && !m_scene->is_empty() ){
        m_first_draw=false;
        configure_auto_params(); //automatically sets parameters that were left as "auto" in the config file
    }
    configure_camera(); //we configure the camera (if needed) on every loop. this is because we might change to another camera during rendering and therefore we need to configure new camera even if it's not the first rendering we make



    TIME_START("update_meshes");
    update_meshes_gl();
    TIME_END("update_meshes");


    TIME_START("shadow_pass");
    //check if any of the objects has moved, and if it has we need to clear all shadow maps and redo them. We need to redo them from scratch because an object moving may have revealed what is behind another object
    bool need_shadow_map_update=false;
    for(size_t i=0; i<m_meshes_gl.size(); i++){
        MeshGLSharedPtr mesh=m_meshes_gl[i];
        if (mesh->m_core->m_is_shadowmap_dirty){
            need_shadow_map_update=true;
            break;
        }
    }
    // VLOG(1) << "need_shadow_map_update " <<need_shadow_map_update;
    //loop through all the light and each mesh into their shadow maps as a depth map
    if(!m_enable_edl_lighting && need_shadow_map_update){
        for(size_t l_idx=0; l_idx<m_spot_lights.size(); l_idx++){
            if(m_spot_lights[l_idx]->m_create_shadow){
                m_spot_lights[l_idx]->clear_shadow_map();

                // VLOG(1) << " l_idx " << l_idx;

                //loop through all the meshes
                for(size_t i=0; i<m_meshes_gl.size(); i++){
                    MeshGLSharedPtr mesh=m_meshes_gl[i];
                    // VLOG(1) << " mesh->m_core->name " << mesh->m_core->name;
                    // VLOG(1) << " mesh->m_core->m_vis.m_is_visible " << mesh->m_core->m_vis.m_is_visible;
                    // VLOG(1) << " mesh->m_core->m_vis.m_force_cast_shadow " << mesh->m_core->m_vis.m_force_cast_shadow;
                    if( (mesh->m_core->m_vis.m_is_visible || mesh->m_core->m_vis.m_force_cast_shadow) && !mesh->m_core->is_empty() ){

                        // VLOG(1) << "rendering shadow";

                        if(mesh->m_core->m_vis.m_show_mesh){
                            // VLOG(1) << "rendering mesh to shadowmap-------------------" << mesh->m_core->name;
                            m_spot_lights[l_idx]->render_mesh_to_shadow_map(mesh);
                        }
                        if(mesh->m_core->m_vis.m_show_points){
                            // VLOG(1) << "rendering points to shadowmap-------------------" << mesh->m_core->name;
                            m_spot_lights[l_idx]->render_points_to_shadow_map(mesh);
                        }

                        //if we use a custom shader we try to make an educated guess weather we should render this mesh as a mesh or as point cloud in the shadow map
                        if(mesh->m_core->m_vis.m_use_custom_shader && mesh->m_core->custom_render_func ){
                            if(mesh->m_core->F.size()){
                                m_spot_lights[l_idx]->render_mesh_to_shadow_map(mesh);
                            }else{
                                m_spot_lights[l_idx]->render_points_to_shadow_map(mesh);
                            }
                        }


                    }
                    mesh->m_core->m_is_shadowmap_dirty=false;
                }
            }
        }
    }
    TIME_END("shadow_pass");



    TIME_START("gbuffer");
    //set the gbuffer size in case it changed
    if(m_viewport_size.x()/m_subsample_factor!=m_gbuffer.width() || m_viewport_size.y()/m_subsample_factor!=m_gbuffer.height()){
        m_gbuffer.set_size(m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor);
    }
    m_gbuffer.bind_for_draw();
    m_gbuffer.clear();
    TIME_END("gbuffer");


    TIME_START("geom_pass");
    glViewport(0.0f , 0.0f, m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor ); //set the viewport again because rendering the shadow maps, changed it
    //render every mesh into the gbuffer
    for(size_t i=0; i<m_meshes_gl.size(); i++){
        MeshGLSharedPtr mesh=m_meshes_gl[i];
        if(mesh->m_core->m_vis.m_is_visible && !mesh->m_core->is_empty() ){
            if(mesh->m_core->m_vis.m_show_mesh){
                render_mesh_to_gbuffer(mesh);
            }
            if(mesh->m_core->m_vis.m_show_surfels){
                render_surfels_to_gbuffer(mesh);
            }
            if(mesh->m_core->m_vis.m_show_points){
                render_points_to_gbuffer(mesh);
            }

            //renders to the gbuffer by calling whatever function the user defined. T
            if(mesh->m_core->m_vis.m_use_custom_shader){
                mesh->m_core->custom_render_func( mesh, shared_from_this() );
            }

        }
    }
    TIME_END("geom_pass");

    //ao_pass
    if(m_enable_ssao){
        ssao_pass(m_gbuffer, m_camera);
    }else{
        // m_gbuffer.tex_with_name("position_gtex").generate_mipmap(m_ssao_downsample); //kinda hacky thing to account for possible resizes of the gbuffer and the fact that we might not have mipmaps in it. This solves the black background issue
    }
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_id);







    //compose the final image
    compose_final_image(fbo_id);

    //blur the bloom image if we do have it
    if (m_enable_bloom){
        blur_img(m_composed_fbo.tex_with_name("bloom_gtex"), m_bloom_start_mip_map_lvl, m_bloom_max_mip_map_lvl, m_bloom_blur_iters);
    }

    apply_postprocess(); //read the composed_fbo and writes into m_final_fbo_no_gui




    //attempt 3 at forward rendering
    // TIME_START("blit");
    glViewport(0.0f , 0.0f, m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor );
    //blit the depth from the gbuffer to the final_fbo_no_gui so that we can forward render stuff into it
    m_gbuffer.bind_for_read();
    m_final_fbo_no_gui.bind_for_draw();
    glBlitFramebuffer( 0, 0, m_gbuffer.width(), m_gbuffer.height(), 0, 0, m_final_fbo_no_gui.width(), m_final_fbo_no_gui.height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST );
    // TIME_END("blit");

    //forward render the lines and edges
    TIME_START("forward_render");
    for(size_t i=0; i<m_meshes_gl.size(); i++){
        MeshGLSharedPtr mesh=m_meshes_gl[i];
        if(mesh->m_core->m_vis.m_is_visible){
            if(mesh->m_core->m_vis.m_show_lines){
                render_lines(mesh);
            }
            if(mesh->m_core->m_vis.m_show_wireframe){
                render_wireframe(mesh);
            }
            if(mesh->m_core->m_vis.m_show_normals){
                render_normals(mesh);
            }
        }
    }
    TIME_END("forward_render");

    blend_bg();



    //restore state
    glDisable(GL_CULL_FACE);


}



void Viewer::upload_single_mesh_to_gpu(const std::shared_ptr<Mesh>& mesh_core, const bool is_meshgl_sticky){

    auto possible_mesh_gl= mesh_core->m_mesh_gpu.lock(); //check if we have a mesh gl

    if(mesh_core->m_vis.m_is_visible && (mesh_core->m_is_dirty || mesh_core->is_any_texture_dirty()  || !possible_mesh_gl)) { //the mesh gl needs updating

        // VLOG(1) << "mesh with name " << mesh_core->name << " needs updating is dirty is " << mesh_core->m_is_dirty << "texture dirty is " << mesh_core->is_any_texture_dirty();

        //find the meshgl  with the same name
        bool found=false;
        int idx_found=-1;
        for(size_t gl_idx = 0; gl_idx < m_meshes_gl.size(); gl_idx++){
            if(m_meshes_gl[gl_idx]->m_core->name==mesh_core->name){
                found=true;
                idx_found=gl_idx;
                break;
            }
        }


        if(found){
            // VLOG(1) << "found";
            m_meshes_gl[idx_found]->assign_core(mesh_core);
            mesh_core->assign_mesh_gpu(m_meshes_gl[idx_found]); // cpu data points to the gpu implementation
            m_meshes_gl[idx_found]->upload_to_gpu();
            m_meshes_gl[idx_found]->sanity_check(); //check that we have for sure all the normals for all the vertices and faces and that everything is correct
        }else{
            // VLOG(1) << "not found";
            MeshGLSharedPtr mesh_gpu=MeshGL::create();
            // VLOG(1) << " mesh_gpu has adress " << mesh_gpu;
            mesh_gpu->assign_core(mesh_core); //GPU implementation points to the cpu data
            mesh_core->assign_mesh_gpu(mesh_gpu); // cpu data points to the gpu implementation
            mesh_gpu->upload_to_gpu();
            mesh_gpu->sanity_check(); //check that we have for sure all the normals for all the vertices and faces and that everything is correct
            m_meshes_gl.push_back(mesh_gpu);
        }


        // auto again_mesh_gl= mesh_core->m_mesh_gpu.lock();
        // CHECK(again_mesh_gl) <<"we should have a meshgl here";
        // again_mesh_gl->m_sticky=is_meshgl_sticky;
        // VLOG(1) << "ahain meshgl fbuf has rgarget" << again_mesh_gl->F_buf.target();
        // VLOG(1) << " again_mesh_gl has adress " << again_mesh_gl;



    }

    if (is_meshgl_sticky){
        auto again_mesh_gl= mesh_core->m_mesh_gpu.lock();
        CHECK(again_mesh_gl) <<"we should have a meshgl here";
        again_mesh_gl->m_sticky=is_meshgl_sticky;
    }

}

void Viewer::update_meshes_gl(){



    //Check if we need to upload to gpu
    for(int i=0; i<m_scene->nr_meshes(); i++){
        MeshSharedPtr mesh_core=m_scene->get_mesh_with_idx(i);
        upload_single_mesh_to_gpu(mesh_core, /*ism_meshgl sticky*/ false);
    }


    //check if any of the mesh in the scene got deleted, in which case we should also delete the corresponding mesh_gl
    //need to do it after updating first the meshes_gl with the new meshes in the scene a some of them may have been added newly just now
    //we check if we have any mesh_gl which has no corresponding mesh_core in the scene with the same name
    std::vector< std::shared_ptr<MeshGL> > meshes_gl_filtered;
    for(size_t gl_idx=0; gl_idx<m_meshes_gl.size(); gl_idx++){
        // MeshSharedPtr mesh_core=m_scene->get_mesh_with_idx(i);

        //find the mesh in the scene with the same name
        bool found=false;
        for(int i = 0; i<m_scene->nr_meshes(); i++){
            MeshSharedPtr mesh_core=m_scene->get_mesh_with_idx(i);
            if(m_meshes_gl[gl_idx]->m_core->name==mesh_core->name){
                found=true;
                break;
            }
        }

        //we found it in the scene and in the gpu so we keep it, OR if the meshgl is sticky and we dont care if the mesh core is not in the scene
        if(found || m_meshes_gl[gl_idx]->m_sticky){
            meshes_gl_filtered.push_back(m_meshes_gl[gl_idx]);
        }else{
            //the mesh_gl has no corresponding mesh_core in the scene which means we discard this mesh_gl which will in turn also garbage collect whatever shared ptr if has over the mesh_core
        }
    }
    m_meshes_gl=meshes_gl_filtered;


}

void Viewer::clear_framebuffers(){
    glClearColor(m_background_color[0],
               m_background_color[1],
               m_background_color[2],
               0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}


void Viewer::render_points_to_gbuffer(const MeshGLSharedPtr mesh){

    //sanity checks
    if( (mesh->m_core->m_vis.m_color_type==+MeshColorType::SemanticGT || mesh->m_core->m_vis.m_color_type==+MeshColorType::SemanticPred) && !mesh->m_core->m_label_mngr  ){
        LOG(WARNING) << "We are trying to show the semantic gt but we have no label manager set for this mesh";
    }

    gl::Shader& shader= m_draw_points_shader;

    // Set attributes that the vao will pulll from buffers
    if(mesh->m_core->V.size()){
        mesh->vao.vertex_attribute(shader, "position", mesh->V_buf, 3);
    }
    if(mesh->m_core->NV.size()){
        mesh->vao.vertex_attribute(shader, "normal", mesh->NV_buf, 3);
        shader.uniform_bool(true, "has_normals");
    }else{
        shader.uniform_bool(false, "has_normals");
    }
    if(mesh->m_core->C.size()){
        GL_C(mesh->vao.vertex_attribute(shader, "color_per_vertex", mesh->C_buf, 3) );
    }
    if(mesh->m_core->UV.size()){
        GL_C(mesh->vao.vertex_attribute(shader, "uv", mesh->UV_buf, 2) );
    }
    if(mesh->m_core->I.size()){
        GL_C(mesh->vao.vertex_attribute(shader, "intensity_per_vertex", mesh->I_buf, 1) );
    }
    if(mesh->m_core->L_pred.size()){
        mesh->vao.vertex_attribute(shader, "label_pred_per_vertex", mesh->L_pred_buf, 1);
    }
    if(mesh->m_core->L_gt.size()){
        mesh->vao.vertex_attribute(shader, "label_gt_per_vertex", mesh->L_gt_buf, 1);
    }


    //matrices setuo
    // Eigen::Matrix4f M = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f M=mesh->m_core->model_matrix().cast<float>().matrix();
    Eigen::Matrix4f V = m_camera->view_matrix();
    Eigen::Matrix4f P = m_camera->proj_matrix(m_gbuffer.width(), m_gbuffer.height());
    Eigen::Matrix4f MV = V*M;
    Eigen::Matrix4f MVP = P*V*M;

    //shader setup
    shader.use();
    shader.uniform_4x4(M, "M");
    shader.uniform_4x4(MV, "MV");
    shader.uniform_4x4(MVP, "MVP");
    shader.uniform_bool(m_using_fat_gbuffer , "using_fat_gbuffer");
    shader.uniform_bool(mesh->m_core->m_vis.m_points_as_circle , "points_as_circle");
    shader.uniform_int(mesh->m_core->m_vis.m_color_type._to_integral() , "color_type");
    shader.uniform_v3_float(mesh->m_core->m_vis.m_point_color , "point_color");
    shader.uniform_float(mesh->m_core->m_vis.m_metalness , "metalness");
    shader.uniform_float(mesh->m_core->m_vis.m_roughness , "roughness");
    // shader.uniform_array_v3_float(m_colormngr.viridis_colormap(), "color_scheme_height"); //for height color type
    shader.uniform_array_v3_float(m_colormngr.colormap(mesh->m_core->m_vis.m_color_scheme._to_integral()), "color_scheme_height"); //for height color type
    shader.uniform_float(mesh->m_core->min_y(), "min_y");
    shader.uniform_float(mesh->m_core->max_y(), "max_y");
    if(mesh->m_core->m_label_mngr){
        shader.uniform_array_v3_float(mesh->m_core->m_label_mngr->color_scheme().cast<float>(), "color_scheme"); //for semantic labels
    }

    //pbr textures
    // if(mesh->m_cur_tex_ptr->storage_initialized() ){
    //     shader.bind_texture(*mesh->m_cur_tex_ptr, "tex");
    // }

     //pbr textures
    if(mesh->m_diffuse_tex.storage_initialized() ){  shader.bind_texture(mesh->m_diffuse_tex, "diffuse_tex");   }
    if(mesh->m_metalness_tex.storage_initialized() ){  shader.bind_texture(mesh->m_metalness_tex, "metalness_tex");   }
    if(mesh->m_roughness_tex.storage_initialized() ){  shader.bind_texture(mesh->m_roughness_tex, "roughness_tex");   }
    // if(mesh->m_normals_tex.storage_initialized() ){  shader.bind_texture(mesh->m_normals_tex, "normals_tex");   }
    shader.uniform_bool(mesh->m_diffuse_tex.storage_initialized(), "has_diffuse_tex");
    shader.uniform_bool(mesh->m_metalness_tex.storage_initialized(), "has_metalness_tex");
    shader.uniform_bool(mesh->m_roughness_tex.storage_initialized(), "has_roughness_tex");


    m_gbuffer.bind_for_draw();
    shader.draw_into(m_gbuffer,
                    {
                    std::make_pair("normal_out", "normal_gtex"),
                    std::make_pair("diffuse_out", "diffuse_gtex"),
                    std::make_pair("metalness_and_roughness_out", "metalness_and_roughness_gtex"),
                    }
                    ); //makes the shaders draw into the buffers we defines in the gbuffer

    float point_size=std::max(1.0f, mesh->m_core->m_vis.m_point_size); //need to cap the point size at a minimum of 1 because something like 0.5 will break opengl :(
    glPointSize(point_size);

    // draw
    mesh->vao.bind();
    if(mesh->m_core->m_vis.m_overlay_points){
        glDepthFunc(GL_ALWAYS);
    }
    glDrawArrays(GL_POINTS, 0, mesh->m_core->V.rows());


    GL_C( glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0) );
    glDepthFunc(GL_LESS);

}

void Viewer::render_lines(const MeshGLSharedPtr mesh){

    // glEnable( GL_LINE_SMOOTH ); //draw lines antialiased (destroys performance)

    // Set attributes that the vao will pulll from buffers
    if(mesh->m_core->V.size()){
        mesh->vao.vertex_attribute(m_draw_lines_shader, "position", mesh->V_buf, 3);
    }
    if(mesh->m_core->C.size()){
        mesh->vao.vertex_attribute(m_draw_lines_shader, "color_per_vertex", mesh->C_buf, 3);
    }
    if(mesh->m_core->E.size()){
        mesh->vao.indices(mesh->E_buf); //Says the indices with we refer to vertices, this gives us the triangles
    }

    Eigen::Matrix4f M=mesh->m_core->model_matrix().cast<float>().matrix();
    Eigen::Matrix4f V = m_camera->view_matrix();
    Eigen::Matrix4f P = m_camera->proj_matrix(m_viewport_size);
    Eigen::Matrix4f MVP = P*V*M;


    //shader setup
    // glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    // glEnable( GL_LINE_SMOOTH ); //draw lines antialiased (destroys performance)
    m_draw_lines_shader.use();
    // Eigen::Matrix4f MVP=compute_mvp_matrix(mesh);
    m_draw_lines_shader.uniform_4x4(MVP, "MVP");
    m_draw_lines_shader.uniform_v3_float(mesh->m_core->m_vis.m_line_color, "line_color");
    m_draw_lines_shader.uniform_int(mesh->m_core->m_vis.m_color_type._to_integral() , "color_type");
    glLineWidth( std::max(mesh->m_core->m_vis.m_line_width, 0.0001f) ); //a line width of 0.0 causes it to crash

    m_draw_lines_shader.draw_into(m_final_fbo_no_gui,
                                    {
                                    // std::make_pair("position_out", "position_gtex"),
                                    std::make_pair("out_color", "color_with_transparency_gtex"),
                                    }
                                    ); //makes the shaders draw into the buffers we defines in the gbuffer


    // draw
    mesh->vao.bind();
    if(mesh->m_core->m_vis.m_overlay_lines){
        glDepthFunc(GL_ALWAYS);
    }
    glDrawElements(GL_LINES, mesh->m_core->E.size(), GL_UNSIGNED_INT, 0);

    glLineWidth( 1.0f );
    glDepthFunc(GL_LESS);

}

void Viewer::render_normals(const MeshGLSharedPtr mesh){

    if (mesh->m_core->m_vis.m_normals_scale==-1.0){
        mesh->m_core->m_vis.m_normals_scale=mesh->m_core->get_scale()*0.1;
    }

    gl::Shader& shader = m_draw_normals_shader;

    if(mesh->m_core->V.size()){
        mesh->vao.vertex_attribute(shader, "position", mesh->V_buf, 3);
    }
    if(mesh->m_core->NV.size()){
        mesh->vao.vertex_attribute(shader, "normal", mesh->NV_buf, 3);
    }


    Eigen::Matrix4f M=mesh->m_core->model_matrix().cast<float>().matrix();
    Eigen::Matrix4f V = m_camera->view_matrix();
    Eigen::Matrix4f P = m_camera->proj_matrix(m_viewport_size);
    Eigen::Matrix4f MVP = P*V*M;


    //shader setup
    shader.use();
    // Eigen::Matrix4f MVP=compute_mvp_matrix(mesh);
    shader.uniform_4x4(MVP, "MVP");
    shader.uniform_v3_float(mesh->m_core->m_vis.m_line_color, "line_color");
    shader.uniform_float(mesh->m_core->m_vis.m_normals_scale , "normals_scale");
    glLineWidth( std::max(mesh->m_core->m_vis.m_line_width, 0.0001f) ); //a line width of 0.0 causes it to crash

    shader.draw_into(m_final_fbo_no_gui,
                                    {
                                    // std::make_pair("position_out", "position_gtex"),
                                    std::make_pair("out_color", "color_with_transparency_gtex"),
                                    }
                                    ); //makes the shaders draw into the buffers we defines in the gbuffer


    // draw
    mesh->vao.bind();
    if(mesh->m_core->m_vis.m_overlay_lines){
        glDepthFunc(GL_ALWAYS);
    }
    // glDrawElements(GL_LINES, mesh->m_core->E.size(), GL_UNSIGNED_INT, 0);
    glDrawArrays(GL_POINTS, 0, mesh->m_core->V.rows());

    glLineWidth( 1.0f );
    glDepthFunc(GL_LESS);


}

void Viewer::render_wireframe(const MeshGLSharedPtr mesh){

     // Set attributes that the vao will pulll from buffers
    if(mesh->m_core->V.size()){
        mesh->vao.vertex_attribute(m_draw_wireframe_shader, "position", mesh->V_buf, 3);
    }
    if(mesh->m_core->F.size()){
        mesh->vao.indices(mesh->F_buf); //Says the indices with we refer to vertices, this gives us the triangles
    }

    Eigen::Matrix4f M=mesh->m_core->model_matrix().cast<float>().matrix();
    Eigen::Matrix4f V = m_camera->view_matrix();
    Eigen::Matrix4f P = m_camera->proj_matrix(m_viewport_size);
    Eigen::Matrix4f MVP = P*V*M;


    //shader setup
    m_draw_wireframe_shader.use();
    m_draw_wireframe_shader.uniform_4x4(MVP, "MVP");

    //openglsetup
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glEnable(GL_POLYGON_OFFSET_LINE); //Avoid Z-buffer fighting between filled triangles & wireframe lines
    glPolygonOffset(-2.0, -10.0);
    glLineWidth( mesh->m_core->m_vis.m_line_width );
    // glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    // glEnable( GL_LINE_SMOOTH ); //draw lines antialiased (destroys performance)


    // draw
    mesh->vao.bind();
    glDrawElements(GL_TRIANGLES, mesh->m_core->F.size(), GL_UNSIGNED_INT, 0);


    //revert to previous openglstat
    glDisable(GL_POLYGON_OFFSET_LINE);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    // glDisable( GL_LINE_SMOOTH );
    glLineWidth( 1.0f );

}

void Viewer::render_mesh_to_gbuffer(const MeshGLSharedPtr mesh){

    // //sanity checks
    // if( (mesh->m_core->m_vis.m_color_type==+MeshColorType::SemanticGT || mesh->m_core->m_vis.m_color_type==+MeshColorType::SemanticPred) && !mesh->m_core->m_label_mngr  ){
    //     LOG(WARNING) << "We are trying to show the semantic gt but we have no label manager set for this mesh";
    // }

    // bool enable_solid_color=!mesh->m_core->C.size();

    // Set attributes that the vao will pulll from buffers
    if(mesh->m_core->V.size()){
        mesh->vao.vertex_attribute(m_draw_mesh_shader, "position", mesh->V_buf, 3);
    }
    if(mesh->m_core->NV.size()){
        mesh->vao.vertex_attribute(m_draw_mesh_shader, "normal", mesh->NV_buf, 3);
    }
    if(mesh->m_core->UV.size()){
        GL_C(mesh->vao.vertex_attribute(m_draw_mesh_shader, "uv", mesh->UV_buf, 2) );
    }
    if(mesh->m_core->V_tangent_u.size()){
        GL_C(mesh->vao.vertex_attribute(m_draw_mesh_shader, "tangent", mesh->V_tangent_u_buf, 3) );
    }
    if(mesh->m_core->C.size()){
        GL_C(mesh->vao.vertex_attribute(m_draw_mesh_shader, "color_per_vertex", mesh->C_buf, 3) );
    }
    if(mesh->m_core->I.size()){
        GL_C(mesh->vao.vertex_attribute(m_draw_mesh_shader, "intensity_per_vertex", mesh->I_buf, 1) );
    }
    if(mesh->m_core->L_pred.size()){
        mesh->vao.vertex_attribute(m_draw_mesh_shader, "label_pred_per_vertex", mesh->L_pred_buf, 1);
    }
    if(mesh->m_core->L_gt.size()){
        mesh->vao.vertex_attribute(m_draw_mesh_shader, "label_gt_per_vertex", mesh->L_gt_buf, 1);
    }
    if(mesh->m_core->F.size()){
        mesh->vao.indices(mesh->F_buf); //Says the indices with we refer to vertices, this gives us the triangles
    }

    //matrices setuo
    // Eigen::Matrix4f M = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f M=mesh->m_core->model_matrix().cast<float>().matrix();
    Eigen::Matrix4f V = m_camera->view_matrix();
    Eigen::Matrix4f P = m_camera->proj_matrix(m_gbuffer.width(), m_gbuffer.height());
    Eigen::Matrix4f MV = V*M;
    Eigen::Matrix4f MVP = P*V*M;

    //shader setup
    m_draw_mesh_shader.use();
    m_draw_mesh_shader.uniform_bool(m_render_uv_to_gbuffer, "render_uv_to_gbuffer");
    m_draw_mesh_shader.uniform_4x4(V, "V");
    m_draw_mesh_shader.uniform_4x4(M, "M");
    m_draw_mesh_shader.uniform_4x4(MV, "MV");
    m_draw_mesh_shader.uniform_4x4(MVP, "MVP");
    m_draw_mesh_shader.uniform_bool(m_using_fat_gbuffer , "using_fat_gbuffer");
    m_draw_mesh_shader.uniform_array_v3_float(m_colormngr.viridis_colormap(), "color_scheme_height"); //for height color type
    m_draw_mesh_shader.uniform_float(mesh->m_core->min_y(), "min_y");
    m_draw_mesh_shader.uniform_float(mesh->m_core->max_y(), "max_y");
    m_draw_mesh_shader.uniform_int(mesh->m_core->m_vis.m_color_type._to_integral() , "color_type");
    m_draw_mesh_shader.uniform_v3_float(mesh->m_core->m_vis.m_solid_color , "solid_color");
    m_draw_mesh_shader.uniform_float(mesh->m_core->m_vis.m_metalness , "metalness");
    m_draw_mesh_shader.uniform_float(mesh->m_core->m_vis.m_roughness , "roughness");
    m_draw_mesh_shader.uniform_int(mesh->m_core->id , "mesh_id");
    if(mesh->m_core->m_label_mngr){
        m_draw_mesh_shader.uniform_array_v3_float(mesh->m_core->m_label_mngr->color_scheme().cast<float>(), "color_scheme"); //for semantic labels
    }
    // m_draw_mesh_shader.uniform_bool( enable_solid_color, "enable_solid_color");
    // m_draw_mesh_shader.uniform_v3_float(mesh->m_ambient_color , "ambient_color");
    // m_draw_mesh_shader.uniform_v3_float(mesh->m_core->m_vis.m_specular_color , "specular_color");
    // m_draw_mesh_shader.uniform_float(mesh->m_ambient_color_power , "ambient_color_power");
    // m_draw_mesh_shader.uniform_float(mesh->m_core->m_vis.m_shininess , "shininess");

    //pbr textures
    if(mesh->m_diffuse_tex.storage_initialized() ){  m_draw_mesh_shader.bind_texture(mesh->m_diffuse_tex, "diffuse_tex");   }
    if(mesh->m_metalness_tex.storage_initialized() ){  m_draw_mesh_shader.bind_texture(mesh->m_metalness_tex, "metalness_tex");   }
    if(mesh->m_roughness_tex.storage_initialized() ){  m_draw_mesh_shader.bind_texture(mesh->m_roughness_tex, "roughness_tex");   }
    if(mesh->m_normals_tex.storage_initialized() ){  m_draw_mesh_shader.bind_texture(mesh->m_normals_tex, "normals_tex");   }
    m_draw_mesh_shader.uniform_bool(mesh->m_diffuse_tex.storage_initialized(), "has_diffuse_tex");
    m_draw_mesh_shader.uniform_bool(mesh->m_metalness_tex.storage_initialized(), "has_metalness_tex");
    m_draw_mesh_shader.uniform_bool(mesh->m_roughness_tex.storage_initialized(), "has_roughness_tex");
    m_draw_mesh_shader.uniform_bool(mesh->m_normals_tex.storage_initialized(), "has_normals_tex");

    m_gbuffer.bind_for_draw();
    std::vector<  std::pair<std::string, std::string> > draw_list=
        {
        std::make_pair("normal_out", "normal_gtex"),
        std::make_pair("diffuse_out", "diffuse_gtex"),
        std::make_pair("metalness_and_roughness_out", "metalness_and_roughness_gtex"),
        std::make_pair("mesh_id_out", "mesh_id_gtex"),
    };
    if(m_render_uv_to_gbuffer){
        draw_list.push_back(std::make_pair("uv_out", "uv_gtex"));
    }
    m_draw_mesh_shader.draw_into(m_gbuffer, draw_list ); //makes the shaders draw into the buffers we defines in the gbuffer

    // draw
    mesh->vao.bind();
    glDrawElements(GL_TRIANGLES, mesh->m_core->F.size(), GL_UNSIGNED_INT, 0);


    GL_C( glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0) );

}
void Viewer::render_surfels_to_gbuffer(const MeshGLSharedPtr mesh){

    if (!m_using_fat_gbuffer){
        LOG(WARNING) << "Switching to surfel rendering mode so now the gbuffer will have half floats so that surfel accumulation can happen. This is necessary for surfel showing but will make the rendering a bit slower.";
        m_using_fat_gbuffer=true;
    }

    // if we are rendering surfels, we need to switch the gbuffer to diffuse: GL_RGBA16F and normal normal: GL_RGB16F
    if (m_gbuffer.tex_with_name("diffuse_gtex").internal_format()!=GL_RGBA16F){
        m_gbuffer.tex_with_name("diffuse_gtex").allocate_storage(GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT, m_gbuffer.width(), m_gbuffer.height() );
    }
    if (m_gbuffer.tex_with_name("normal_gtex").internal_format()!=GL_RGB16F){
        m_gbuffer.tex_with_name("normal_gtex").allocate_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_gbuffer.width(), m_gbuffer.height() );
    }
    if (m_gbuffer.tex_with_name("metalness_and_roughness_gtex").internal_format()!=GL_RG16F){
        m_gbuffer.tex_with_name("metalness_and_roughness_gtex").allocate_storage(GL_RG16F, GL_RG, GL_HALF_FLOAT, m_gbuffer.width(), m_gbuffer.height() );
    }





    //we disable surfel rendering for the moment because I changed the gbuffer diffuse texture from Half float to RGBA8 because it's a lot faster. This however means that the color accumulation cannot happen anymore in that render target. Also I didn't modify the surfel shader to output the encoded normals as per the CryEngine3 pipeline. Due to all these reasons I will disable for now the surfel rendering
    // LOG(FATAL) << "Surfel rendering disabled because we disabled the accumulation of color into the render target. this makes the rest of the program way faster. Also we would need to modify the surfel fragment shader to output encoded normals";

    //sanity checks
    CHECK(mesh->m_core->V.rows()==mesh->m_core->V_tangent_u.rows() ) << "Mesh does not have tangent for each vertex. We cannot render surfels without the tangent" << mesh->m_core->V.rows() << " " << mesh->m_core->V_tangent_u.rows();
    CHECK(mesh->m_core->V.rows()==mesh->m_core->V_length_v.rows() ) << "Mesh does not have lenght_u for each vertex. We cannot render surfels without the V_lenght_u" << mesh->m_core->V.rows() << " " << mesh->m_core->V_length_v.rows();
    if( (mesh->m_core->m_vis.m_color_type==+MeshColorType::SemanticGT || mesh->m_core->m_vis.m_color_type==+MeshColorType::SemanticPred) && !mesh->m_core->m_label_mngr  ){
        LOG(WARNING) << "We are trying to show the semantic gt but we have no label manager set for this mesh";
    }

    // bool enable_solid_color=!mesh->m_core->C.size();

     // Set attributes that the vao will pulll from buffers
    if(mesh->m_core->V.size()){
        mesh->vao.vertex_attribute(m_draw_surfels_shader, "position", mesh->V_buf, 3);
    }
    if(mesh->m_core->NV.size()){
        mesh->vao.vertex_attribute(m_draw_surfels_shader, "normal", mesh->NV_buf, 3);
    }
    if(mesh->m_core->V_tangent_u.size()){
        mesh->vao.vertex_attribute(m_draw_surfels_shader, "tangent_u", mesh->V_tangent_u_buf, 3);
    }
    if(mesh->m_core->V_length_v.size()){
        mesh->vao.vertex_attribute(m_draw_surfels_shader, "lenght_v", mesh->V_lenght_v_buf, 1);
    }
    if(mesh->m_core->C.size()){
        mesh->vao.vertex_attribute(m_draw_surfels_shader, "color_per_vertex", mesh->C_buf, 3);
    }
    if(mesh->m_core->L_pred.size()){
        mesh->vao.vertex_attribute(m_draw_surfels_shader, "label_pred_per_vertex", mesh->L_pred_buf, 1);
    }
    if(mesh->m_core->L_gt.size()){
        mesh->vao.vertex_attribute(m_draw_surfels_shader, "label_gt_per_vertex", mesh->L_gt_buf, 1);
    }
    // if(mesh->m_core->UV.size()){
    //     GL_C(mesh->vao.vertex_attribute(m_draw_surfels_shader, "uv", mesh->UV_buf, 2) );
    // }

    //matrices setuo
    Eigen::Matrix4f M=mesh->m_core->model_matrix().cast<float>().matrix();
    Eigen::Matrix4f V = m_camera->view_matrix();
    Eigen::Matrix4f P = m_camera->proj_matrix(m_viewport_size);
    Eigen::Matrix4f MV = V*M;
    Eigen::Matrix4f MVP = P*V*M;

    // if(m_enable_surfel_splatting){
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE,GL_ONE);
    // }
    //params
    // glDisable(GL_DEPTH_TEST);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA_SATURATE,GL_DST_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA,GL_DST_ALPHA);
    // glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    // glBlendEquation(GL_MAX);
    // glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE_MINUS_SRC_ALPHA);
    // glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD); //add the rgb and alpha components
    // glBlendEquationSeparate(GL_FUNC_ADD,GL_FUNC_ADD); //add the rgb and alpha components

    //shader setup
    if(m_gbuffer.width()!= m_viewport_size.x() || m_gbuffer.height()!=m_viewport_size.y() ){
        m_gbuffer.set_size(m_viewport_size.x(), m_viewport_size.y());
    }
    m_draw_surfels_shader.use();
    m_draw_surfels_shader.uniform_4x4(M, "M");
    m_draw_surfels_shader.uniform_4x4(MV, "MV");
    m_draw_surfels_shader.uniform_4x4(MVP, "MVP");
    m_draw_surfels_shader.uniform_bool(m_using_fat_gbuffer , "using_fat_gbuffer");
    m_draw_surfels_shader.uniform_int(mesh->m_core->m_vis.m_color_type._to_integral() , "color_type");
    m_draw_surfels_shader.uniform_v3_float(mesh->m_core->m_vis.m_solid_color , "solid_color");
    m_draw_surfels_shader.uniform_float(mesh->m_core->m_vis.m_metalness , "metalness");
    m_draw_surfels_shader.uniform_float(mesh->m_core->m_vis.m_roughness , "roughness");
    if(mesh->m_core->m_label_mngr){
        m_draw_surfels_shader.uniform_array_v3_float(mesh->m_core->m_label_mngr->color_scheme().cast<float>(), "color_scheme"); //for semantic labels
    }
    // m_draw_surfels_shader.uniform_bool( enable_solid_color , "enable_solid_color");
    // m_draw_mesh_shader.uniform_v3_float(mesh->m_ambient_color , "ambient_color");
    // m_draw_surfels_shader.uniform_v3_float(m_specular_color , "specular_color");
    // m_draw_mesh_shader.uniform_float(mesh->m_ambient_color_power , "ambient_color_power");
    // m_draw_surfels_shader.uniform_float(m_shininess , "shininess");


    // //pbr textures
    // if(mesh->m_diffuse_tex.storage_initialized() ){  m_draw_surfels_shader.bind_texture(mesh->m_diffuse_tex, "diffuse_tex");   }
    // if(mesh->m_metalness_tex.storage_initialized() ){  m_draw_surfels_shader.bind_texture(mesh->m_metalness_tex, "metalness_tex");   }
    // if(mesh->m_roughness_tex.storage_initialized() ){  m_draw_surfels_shader.bind_texture(mesh->m_roughness_tex, "roughness_tex");   }
    // // if(mesh->m_normals_tex.storage_initialized() ){  m_draw_mesh_shader.bind_texture(mesh->m_normals_tex, "normals_tex");   }
    // m_draw_surfels_shader.uniform_bool(mesh->m_diffuse_tex.storage_initialized(), "has_diffuse_tex");
    // m_draw_surfels_shader.uniform_bool(mesh->m_metalness_tex.storage_initialized(), "has_metalness_tex");
    // m_draw_surfels_shader.uniform_bool(mesh->m_roughness_tex.storage_initialized(), "has_roughness_tex");



    //draw only into depth map
    m_draw_surfels_shader.uniform_bool(true , "enable_visibility_test");
    m_draw_surfels_shader.draw_into( m_gbuffer,{} );
    mesh->vao.bind();
    glDrawArrays(GL_POINTS, 0, mesh->m_core->V.rows());



    //now draw into the gbuffer only the ones that pass the visibility test
    glDepthMask(false); //don't write to depth buffer but do perform the checking
    glEnable( GL_POLYGON_OFFSET_FILL );
    glPolygonOffset(m_surfel_blend_factor, m_surfel_blend_factor); //offset the depth in the depth buffer a bit further so we can render surfels that are even a bit overlapping
    m_draw_surfels_shader.uniform_bool(false , "enable_visibility_test");
    m_gbuffer.bind_for_draw();
    m_draw_surfels_shader.draw_into(m_gbuffer,
                                    {
                                    // std::make_pair("position_out", "position_gtex"),
                                    std::make_pair("normal_out", "normal_gtex"),
                                    std::make_pair("diffuse_out", "diffuse_gtex"),
                                    std::make_pair("metalness_and_roughness_out", "metalness_and_roughness_gtex"),
                                    // std::make_pair("specular_out", "specular_gtex"),
                                    // std::make_pair("shininess_out", "shininess_gtex")
                                    }
                                    );
    mesh->vao.bind();
    glDrawArrays(GL_POINTS, 0, mesh->m_core->V.rows());



    GL_C( glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0) );
    glDisable(GL_BLEND);
    glDisable( GL_POLYGON_OFFSET_FILL );
    glDepthMask(true);


}

void Viewer::ssao_pass(gl::GBuffer& gbuffer, std::shared_ptr<Camera> camera){

    TIME_SCOPE("ssao_pass_full");

    //SSAO needs to perform a lot of accesses to the depth map in order to calculate occlusion. Due to cache coherency it is faster to sampler from a downsampled depth map
    //furthermore we only need the linearized depth map. So we first downsample the depthmap, then we linearize it and we run the ao shader and then the bilateral blurring

    //SETUP-------------------------
    //dont perform depth checking nor write into the depth buffer
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);
    //viewport setup. We render into a smaller viewport so tha the ao_tex is a bit smaller
    Eigen::Vector2i new_viewport_size=calculate_mipmap_size(gbuffer.width(), gbuffer.height(), m_ssao_downsample);
    glViewport(0.0f , 0.0f, new_viewport_size.x(), new_viewport_size.y() );
    //deal with the textures
    m_ao_tex.allocate_or_resize(GL_R8, GL_RED, GL_UNSIGNED_BYTE, new_viewport_size.x(), new_viewport_size.y() ); //either fully allocates it or resizes if the size changes
    m_ao_tex.clear();
    gbuffer.tex_with_name("depth_gtex").generate_mipmap(m_ssao_downsample);




    //LINEARIZE-------------------------
    // TIME_START("depth_linearize_pass");
    m_depth_linear_tex.allocate_or_resize( GL_R32F, GL_RED, GL_FLOAT, new_viewport_size.x(), new_viewport_size.y() );
    m_depth_linear_tex.clear();

    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_depth_linearize_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_depth_linearize_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles

    m_depth_linearize_shader.use();
    m_depth_linearize_shader.uniform_int(m_ssao_downsample, "pyr_lvl");
    m_depth_linearize_shader.uniform_float( camera->m_far / (camera->m_far - camera->m_near), "projection_a"); // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    m_depth_linearize_shader.uniform_float( (-camera->m_far * camera->m_near) / (camera->m_far - camera->m_near) , "projection_b");
    m_depth_linearize_shader.bind_texture(gbuffer.tex_with_name("depth_gtex"), "depth_tex");

    m_depth_linearize_shader.draw_into(m_depth_linear_tex, "depth_linear_out");

    // draw
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);
    // TIME_END("depth_linearize_pass");








    //SSAO----------------------------------------
    // TIME_START("ao_pass");
    //matrix setup
    Eigen::Matrix3f V_rot = Eigen::Affine3f(camera->view_matrix()).linear(); //for rotating the normals from the world coords to the cam_coords
    Eigen::Matrix4f P = camera->proj_matrix(gbuffer.width(), gbuffer.height());
    Eigen::Matrix4f P_inv=P.inverse();


    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_ssao_ao_pass_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_ssao_ao_pass_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles


    m_ssao_ao_pass_shader.use();
    GL_C( m_ssao_ao_pass_shader.uniform_4x4(P, "P") );
    m_ssao_ao_pass_shader.uniform_4x4(P_inv, "P_inv");
    m_ssao_ao_pass_shader.uniform_3x3(V_rot, "V_rot");
    m_ssao_ao_pass_shader.uniform_bool(m_using_fat_gbuffer , "using_fat_gbuffer");
    m_ssao_ao_pass_shader.uniform_array_v3_float(m_random_samples,"random_samples");
    m_ssao_ao_pass_shader.uniform_int(m_random_samples.rows(),"nr_samples");
    m_ssao_ao_pass_shader.uniform_float(m_kernel_radius,"kernel_radius");
    m_ssao_ao_pass_shader.uniform_float(m_max_ssao_distance,"max_ssao_distance");
    // m_ssao_ao_pass_shader.uniform_int(m_ssao_downsample, "pyr_lvl"); //no need for pyramid because we only sample from depth_linear_tex which is already downsampled and has no mipmap
    // m_ssao_ao_pass_shader.bind_texture(m_depth_linear_tex,"depth_linear_tex");
    //attempt 2 with depth Not linear because using the linear depth seems to give wrong ssao when camera is near for some reason..
    m_ssao_ao_pass_shader.uniform_float( camera->m_far / (camera->m_far - camera->m_near), "projection_a"); // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    m_ssao_ao_pass_shader.uniform_float( (-camera->m_far * camera->m_near) / (camera->m_far - camera->m_near) , "projection_b");
    m_ssao_ao_pass_shader.uniform_bool(m_ssao_estimate_normals_from_depth, "ssao_estimate_normals_from_depth");
    m_ssao_ao_pass_shader.bind_texture(gbuffer.tex_with_name("depth_gtex"),"depth_tex");
    if( !m_ssao_estimate_normals_from_depth){
        m_ssao_ao_pass_shader.bind_texture(gbuffer.tex_with_name("normal_gtex"),"normal_tex");
    }
    m_ssao_ao_pass_shader.bind_texture(m_rvec_tex,"rvec_tex");

    m_ssao_ao_pass_shader.draw_into(m_ao_tex, "ao_out");

    // // draw
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);
    // TIME_END("ao_pass");

    //restore the state
    GL_C( glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0) );
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, m_viewport_size.x(), m_viewport_size.y() );






    //dont perform depth checking nor write into the depth buffer
    // TIME_START("blur_pass");
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);

    //viewport setup. We render into a smaller viewport so tha the ao_tex is a bit smaller
    new_viewport_size=calculate_mipmap_size(gbuffer.width(), gbuffer.height(), m_ssao_downsample);
    glViewport(0.0f , 0.0f, new_viewport_size.x(), new_viewport_size.y() );


    // m_ao_blurred_tex.allocate_or_resize(GL_R32F, GL_RED, GL_FLOAT, new_viewport_size.x(), new_viewport_size.y() ); //either fully allocates it or resizes if the size changes
    m_ao_blurred_tex.allocate_or_resize( GL_R8, GL_RED, GL_UNSIGNED_BYTE, new_viewport_size.x(), new_viewport_size.y() );
    m_ao_blurred_tex.clear();



    //matrix setup
    Eigen::Vector2f inv_resolution;
    inv_resolution << 1.0/new_viewport_size.x(), 1.0/new_viewport_size.y();


    ///attempt 3 something is wrong with the clearing of the gbuffer
    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_bilateral_blur_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_bilateral_blur_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles


    m_bilateral_blur_shader.use();
    m_bilateral_blur_shader.uniform_v2_float(inv_resolution, "g_InvResolutionDirection" );
    m_bilateral_blur_shader.uniform_int(m_ao_power, "ao_power");
    m_bilateral_blur_shader.uniform_float(m_sigma_spacial, "sigma_spacial");
    m_bilateral_blur_shader.uniform_float(m_sigma_depth, "sigma_depth");
    m_bilateral_blur_shader.bind_texture(m_ao_tex, "texSource");
    // m_bilateral_blur_shader.bind_texture(m_gbuffer.tex_with_name("depth_gtex"),"texLinearDepth");
    m_bilateral_blur_shader.bind_texture(m_depth_linear_tex,"texLinearDepth");

    m_bilateral_blur_shader.draw_into(m_ao_blurred_tex, "out_Color");



    // // draw
    // m_fullscreen_quad->vao.bind();
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);
    // glColorMask(true, true, true, true);
    // TIME_END("blur_pass");

    //restore the state
    GL_C( glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0) );
    // GLenum draw_buffers[1];
    // draw_buffers[0]=GL_BACK;
    // glDrawBuffers(1,draw_buffers);
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor );



}

void Viewer::compose_final_image(const GLuint fbo_id){

    TIME_START("compose");

    //create a final image the same size as the framebuffer
    // m_environment_cubemap_tex.allocate_tex_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_environment_cubemap_resolution, m_environment_cubemap_resolution);
    // m_composed_tex.allocate_or_resize(GL_RGBA16, GL_RGBA, GL_HALF_FLOAT, m_gbuffer.width(), m_gbuffer.height() );
    // m_composed_tex.set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 0.0);

    // m_bloom_tex.allocate_or_resize(GL_RGBA16, GL_RGBA, GL_HALF_FLOAT, m_gbuffer.width(), m_gbuffer.height() );
    // m_bloom_tex.set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 0.0);
    TIME_START("clearing_compose");
    m_composed_fbo.set_size(m_gbuffer.width(), m_gbuffer.height() ); //established what will be the size of the textures attached to this framebuffer
    m_composed_fbo.clear();
    TIME_END("clearing_compose");
    // GL_C( m_composed_fbo.tex_with_name("composed_gtex").set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 1.0) );
    // GL_C( m_composed_fbo.tex_with_name("bloom_gtex").set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 0.0) );
    // GL_C( m_composed_fbo.sanity_check());
    // VLOG(1) << "Trying to clear the bloom_gtex";
    // GL_C( m_composed_fbo.tex_with_name("bloom_gtex").generate_mipmap(m_bloom_mip_map_lvl) );
    // GL_C( m_composed_fbo.tex_with_name("bloom_gtex").clear() );
    // GL_C( m_composed_fbo.tex_with_name("bloom_gtex").set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 0.0) );
    // VLOG(1) <<  m_composed_fbo.tex_with_name("bloom_gtex").mipmap_nr_levels_allocated();
    // VLOG(1) << "finished clearing bloom gtex";


    //matrices setuo
    Eigen::Matrix4f V = m_camera->view_matrix();
    Eigen::Matrix4f P = m_camera->proj_matrix(m_gbuffer.width(), m_gbuffer.height());
    Eigen::Matrix4f P_inv = P.inverse();
    Eigen::Matrix4f V_inv = V.inverse(); //used for projecting the cam coordinates positions (which were hit with MV) stored into the gbuffer back into the world coordinates (so just makes them be affected by M which is the model matrix which just puts things into a common world coordinate)
    Eigen::Matrix3f V_inv_rot=Eigen::Affine3f(m_camera->view_matrix()).inverse().linear(); //used for getting the view rays from the cam coords to the world coords so we can sample the cubemap


    //dont perform depth checking nor write into the depth buffer
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);

     // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_compose_final_quad_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_compose_final_quad_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles


     //shader setup
    GL_C( m_compose_final_quad_shader.use() );
    m_compose_final_quad_shader.bind_texture(m_gbuffer.tex_with_name("normal_gtex"),"normal_tex");
    m_compose_final_quad_shader.bind_texture(m_gbuffer.tex_with_name("diffuse_gtex"),"diffuse_tex");
    m_compose_final_quad_shader.bind_texture(m_gbuffer.tex_with_name("metalness_and_roughness_gtex"),"metalness_and_roughness_tex");
    m_compose_final_quad_shader.bind_texture(m_gbuffer.tex_with_name("depth_gtex"), "depth_tex");
    if (m_show_background_img){
        m_compose_final_quad_shader.bind_texture(m_background_tex, "background_tex");
    }
    //cubemap has to be always bound otherwise the whole program crashes for some reason...
    m_compose_final_quad_shader.bind_texture(m_environment_cubemap_tex, "environment_cubemap_tex");
    m_compose_final_quad_shader.bind_texture(m_irradiance_cubemap_tex, "irradiance_cubemap_tex");
    m_compose_final_quad_shader.bind_texture(m_prefilter_cubemap_tex, "prefilter_cubemap_tex");
    m_compose_final_quad_shader.bind_texture(m_brdf_lut_tex, "brdf_lut_tex");
    if(m_enable_ssao){
        m_compose_final_quad_shader.bind_texture(m_ao_blurred_tex,"ao_tex");
        // m_compose_final_quad_shader.bind_texture(m_ao_tex,"ao_tex");
    }
    m_compose_final_quad_shader.uniform_4x4(P_inv, "P_inv");
    m_compose_final_quad_shader.uniform_4x4(V_inv, "V_inv");
    m_compose_final_quad_shader.uniform_3x3(V_inv_rot, "V_inv_rot");
    m_compose_final_quad_shader.uniform_bool(m_using_fat_gbuffer , "using_fat_gbuffer");
    m_compose_final_quad_shader.uniform_v3_float(m_camera->position(), "eye_pos");
    m_compose_final_quad_shader.uniform_float( m_camera->m_far / (m_camera->m_far - m_camera->m_near), "projection_a"); // according to the formula at the bottom of article https://mynameismjp.wordpress.com/2010/09/05/position-from-depth-3/
    m_compose_final_quad_shader.uniform_float( (-m_camera->m_far * m_camera->m_near) / (m_camera->m_far - m_camera->m_near) , "projection_b");
    m_compose_final_quad_shader.uniform_v3_float(m_ambient_color , "ambient_color");
    m_compose_final_quad_shader.uniform_float(m_ambient_color_power , "ambient_color_power");
    m_compose_final_quad_shader.uniform_bool(m_enable_ssao , "enable_ssao");
    // m_compose_final_quad_shader.uniform_float(m_shading_factor , "shading_factor");
    // m_compose_final_quad_shader.uniform_float(m_light_factor , "light_factor");
    m_compose_final_quad_shader.uniform_v2_float(m_viewport_size , "viewport_size"); //for eye dome lighing
    m_compose_final_quad_shader.uniform_bool(m_enable_edl_lighting , "enable_edl_lighting"); //for edl lighting
    m_compose_final_quad_shader.uniform_float(m_edl_strength , "edl_strength"); //for edl lighting
    m_compose_final_quad_shader.uniform_bool(m_show_background_img , "show_background_img");
    m_compose_final_quad_shader.uniform_bool(m_show_environment_map, "show_environment_map");
    // VLOG(1) << "m_show_prefiltered_environment_map is " << m_show_prefiltered_environment_map;
    m_compose_final_quad_shader.uniform_bool(m_show_prefiltered_environment_map, "show_prefiltered_environment_map");
    m_compose_final_quad_shader.uniform_float(m_environment_map_blur, "environment_map_blur");
    m_compose_final_quad_shader.uniform_int(m_prefilter_cubemap_tex.mipmap_nr_lvls(), "prefilter_nr_mipmaps");
    m_compose_final_quad_shader.uniform_bool(m_enable_ibl, "enable_ibl");
    m_compose_final_quad_shader.uniform_float(m_camera->m_exposure, "exposure");
    m_compose_final_quad_shader.uniform_bool(m_enable_bloom, "enable_bloom");
    m_compose_final_quad_shader.uniform_float(m_bloom_threshold, "bloom_threshold");


    //fill up the vector of spot lights
    m_compose_final_quad_shader.uniform_int(m_spot_lights.size(), "nr_active_spot_lights");
    for(size_t i=0; i<m_spot_lights.size(); i++){

        Eigen::Matrix4f V_light = m_spot_lights[i]->view_matrix();
        Eigen::Vector2f viewport_size_light;
        viewport_size_light<< m_spot_lights[i]->shadow_map_resolution(), m_spot_lights[i]->shadow_map_resolution();
        Eigen::Matrix4f P_light = m_spot_lights[i]->proj_matrix(viewport_size_light);
        Eigen::Matrix4f VP = P_light*V_light; //projects the world coordinates into the light

        std::string uniform_name="spot_lights";
        //position in cam coords
        std::string uniform_pos_name =  uniform_name +"["+std::to_string(i)+"]"+".pos";
        GLint uniform_pos_loc=m_compose_final_quad_shader.get_uniform_location(uniform_pos_name);
        glUniform3fv(uniform_pos_loc, 1, m_spot_lights[i]->position().data());

        //color
        std::string uniform_color_name = uniform_name +"["+std::to_string(i)+"]"+".color";
        GLint uniform_color_loc=m_compose_final_quad_shader.get_uniform_location(uniform_color_name);
        glUniform3fv(uniform_color_loc, 1, m_spot_lights[i]->m_color.data());

        //power
        std::string uniform_power_name =  uniform_name +"["+std::to_string(i)+"]"+".power";
        GLint uniform_power_loc=m_compose_final_quad_shader.get_uniform_location(uniform_power_name);
        glUniform1f(uniform_power_loc, m_spot_lights[i]->m_power);

        //VP matrix that project world coordinates into the light
        std::string uniform_VP_name =  uniform_name +"["+std::to_string(i)+"]"+".VP";
        GLint uniform_VP_loc=m_compose_final_quad_shader.get_uniform_location(uniform_VP_name);
        glUniformMatrix4fv(uniform_VP_loc, 1, GL_FALSE, VP.data());

        //sampler for shadow map
        if (m_spot_lights[i]->has_shadow_map() ){ //have to check because the light might not yet have a shadow map at the start of the app when no mesh is there to be rendered
            std::string sampler_shadow_map_name =  uniform_name +"["+std::to_string(i)+"]"+".shadow_map";
            m_compose_final_quad_shader.bind_texture(m_spot_lights[i]->get_shadow_map_ref(), sampler_shadow_map_name );
        }

        //color
        std::string uniform_create_shadow_name = uniform_name +"["+std::to_string(i)+"]"+".create_shadow";
        GLint uniform_create_shadow_loc=m_compose_final_quad_shader.get_uniform_location(uniform_create_shadow_name);
        //check both if the spotlight creates a shadow AND if the shadow map is actually initialized. It may happen that you have a scene full of surfel meshes which do not project into the light and therefore they will never initialize their shadow maps
        bool creates_shadow=m_spot_lights[i]->m_create_shadow && m_spot_lights[i]->has_shadow_map();
        glUniform1i(uniform_create_shadow_loc, creates_shadow);
    }


    //make the neighbours for edl
    int neighbours_count=8; //same as in https://github.com/potree/potree/blob/65f6eb19ce7a34ce588973c262b2c3558b0f4e60/src/materials/EyeDomeLightingMaterial.js
    Eigen::MatrixXf neighbours;
    neighbours.resize(neighbours_count, 2);
    neighbours.setZero();
    for(int i=0; i<neighbours_count; i++){
        float x = std::cos(2 * i * M_PI / neighbours_count);
        float y = std::sin(2 * i * M_PI / neighbours_count);
        neighbours.row(i) <<x,y;
    }
    // VLOG(1) << "neighbours is " << neighbours;
    m_compose_final_quad_shader.uniform_v2_float(neighbours , "neighbours");

    // m_compose_final_quad_shader.draw_into(m_composed_tex, "out_color");
    m_composed_fbo.bind_for_draw();
    m_compose_final_quad_shader.draw_into(m_composed_fbo,
                                    {
                                    // std::make_pair("position_out", "position_gtex"),
                                    std::make_pair("out_color", "composed_gtex"),
                                    std::make_pair("bloom_color", "bloom_gtex"),
                                    }
                                    );

    // draw
    m_fullscreen_quad->vao.bind();
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);
    TIME_END("compose");

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);


}

void Viewer::blur_img(gl::Texture2D& img, const int start_mip_map_lvl, const int max_mip_map_lvl, const int bloom_blur_iters){

    // TIME_START("blur_img");

    // //first mip map the image so it's faster to blur it when it's smaller
    // GL_C( img.generate_mipmap(mip_map_lvl) );

    // Eigen::Vector2i blurred_img_size;
    // blurred_img_size=calculate_mipmap_size(img.width(), img.height(), mip_map_lvl);
    // // VLOG(1) << "blurred_img_size" << blurred_img_size.transpose();
    // glViewport(0.0f , 0.0f, blurred_img_size.x(), blurred_img_size.y() );

    // m_blur_tmp_tex.allocate_or_resize( img.internal_format(), img.format(), img.type(), blurred_img_size.x(), blurred_img_size.y() );
    // m_blur_tmp_tex.clear();


    // //dont perform depth checking nor write into the depth buffer
    // glDepthMask(false);
    // glDisable(GL_DEPTH_TEST);

    //  // Set attributes that the vao will pulll from buffers
    // GL_C( m_fullscreen_quad->vao.vertex_attribute(m_compose_final_quad_shader, "position", m_fullscreen_quad->V_buf, 3) );
    // GL_C( m_fullscreen_quad->vao.vertex_attribute(m_compose_final_quad_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    // m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles


    // //shader setup
    // GL_C( m_blur_shader.use() );


    // // int iters=2;
    // for (int i = 0; i < m_bloom_blur_iters; i++){

    //     m_blur_shader.bind_texture(img,"img");
    //     m_blur_shader.uniform_int(mip_map_lvl,"mip_map_lvl");
    //     m_blur_shader.uniform_bool(true,"horizontal");
    //     m_blur_shader.draw_into(m_blur_tmp_tex, "blurred_output");
    //     // draw
    //     m_fullscreen_quad->vao.bind();
    //     glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);


    //     //do it in the vertical direction
    //     m_blur_shader.bind_texture(m_blur_tmp_tex,"img");
    //     m_blur_shader.uniform_int(0,"mip_map_lvl");
    //     m_blur_shader.uniform_bool(false,"horizontal");
    //     m_blur_shader.draw_into(m_composed_fbo.tex_with_name("bloom_gtex"), "blurred_output", mip_map_lvl);
    //     // draw
    //     m_fullscreen_quad->vao.bind();
    //     glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);
    // }


    // TIME_END("blur_img");

    // //restore the state
    // glDepthMask(true);
    // glEnable(GL_DEPTH_TEST);
    // glViewport(0.0f , 0.0f, m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor );




    //attempt 2 by creating a mip map of the texture and then blurring a bit each mip map. Inspired by http://kalogirou.net/2006/05/20/how-to-do-good-bloom-for-hdr-rendering/

    TIME_START("blur_img");

    //dont perform depth checking nor write into the depth buffer
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);

    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_blur_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_blur_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles

    //shader setup
    GL_C( m_blur_shader.use() );




    //first mip map the image containing the bright areas

    GL_C( img.generate_mipmap(max_mip_map_lvl) );
    //the blurred tmp only needs to start allocating from start_mip_map_lvl because we dont blur any map that is bigger
    int max_mip_map_lvl_tmp_buffer=max_mip_map_lvl-start_mip_map_lvl;
    Eigen::Vector2i blurred_tmp_start_size=calculate_mipmap_size(img.width(), img.height(), start_mip_map_lvl);
    m_blur_tmp_tex.allocate_or_resize( img.internal_format(), img.format(), img.type(), blurred_tmp_start_size.x(), blurred_tmp_start_size.y() );
    //allocate a mip map
    if(m_blur_tmp_tex.mipmap_nr_levels_allocated()!=max_mip_map_lvl_tmp_buffer){
        m_blur_tmp_tex.generate_mipmap(max_mip_map_lvl_tmp_buffer);
    }
    m_blur_tmp_tex.clear(); //clear also the mip maps

    //for each mip map level of the bright image we blur it a bit
    for (int mip = start_mip_map_lvl; mip < max_mip_map_lvl; mip++){

        for (int i = 0; i < bloom_blur_iters; i++){

            Eigen::Vector2i blurred_img_size;
            blurred_img_size=calculate_mipmap_size(img.width(), img.height(), mip);
            glViewport(0.0f , 0.0f, blurred_img_size.x(), blurred_img_size.y() );


            m_blur_shader.bind_texture(img,"img");
            m_blur_shader.uniform_int(mip,"mip_map_lvl");
            m_blur_shader.uniform_bool(true,"horizontal");
            m_blur_shader.draw_into(m_blur_tmp_tex, "blurred_output",mip-start_mip_map_lvl);
            // draw
            m_fullscreen_quad->vao.bind();
            glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);


            //do it in the vertical direction
            m_blur_shader.bind_texture(m_blur_tmp_tex,"img");
            m_blur_shader.uniform_int(mip-start_mip_map_lvl,"mip_map_lvl");
            m_blur_shader.uniform_bool(false,"horizontal");
            m_blur_shader.draw_into(img, "blurred_output", mip);
            // draw
            m_fullscreen_quad->vao.bind();
            glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);

        }


    }












    // // int iters=2;
    // for (int i = 0; i < m_bloom_blur_iters; i++){

    //     m_blur_shader.bind_texture(img,"img");
    //     m_blur_shader.uniform_int(mip_map_lvl,"mip_map_lvl");
    //     m_blur_shader.uniform_bool(true,"horizontal");
    //     m_blur_shader.draw_into(m_blur_tmp_tex, "blurred_output");
    //     // draw
    //     m_fullscreen_quad->vao.bind();
    //     glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);


    //     //do it in the vertical direction
    //     m_blur_shader.bind_texture(m_blur_tmp_tex,"img");
    //     m_blur_shader.uniform_int(0,"mip_map_lvl");
    //     m_blur_shader.uniform_bool(false,"horizontal");
    //     m_blur_shader.draw_into(m_composed_fbo.tex_with_name("bloom_gtex"), "blurred_output", mip_map_lvl);
    //     // draw
    //     m_fullscreen_quad->vao.bind();
    //     glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);
    // }


    TIME_END("blur_img");

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor );


}

void Viewer::apply_postprocess(){

    TIME_START("apply_postprocess");

    //first mip map the image so it's faster to blur it when it's smaller
    // m_blur_tmp_tex.allocate_or_resize( img.internal_format(), img.format(), img.type(), m_posprocessed_tex.width(), blurred_img_size.y() );
    // m_posprocessed_tex.allocate_or_resize(GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE, m_gbuffer.width(), m_gbuffer.height() );
    // m_posprocessed_tex.set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 0.0);
    if(m_viewport_size.x()/m_subsample_factor!=m_final_fbo_no_gui.width() || m_viewport_size.y()/m_subsample_factor!=m_final_fbo_no_gui.height()){
        m_final_fbo_no_gui.set_size(m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor  );
    }
    m_final_fbo_no_gui.clear_depth();
    // m_final_fbo_no_gui.tex_with_name("color_gtex").set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 0.0);

    Eigen::Vector2f size_final_image;
    size_final_image << m_final_fbo_no_gui.width(), m_final_fbo_no_gui.height();


    //dont perform depth checking nor write into the depth buffer
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_apply_postprocess_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_apply_postprocess_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles


    //shader setup
    GL_C( m_apply_postprocess_shader.use() );



    m_apply_postprocess_shader.bind_texture(m_composed_fbo.tex_with_name("composed_gtex"),"composed_tex");
    m_apply_postprocess_shader.bind_texture(m_composed_fbo.tex_with_name("bloom_gtex"),"bloom_tex");
    m_apply_postprocess_shader.bind_texture(m_gbuffer.tex_with_name("depth_gtex"), "depth_tex");
    m_apply_postprocess_shader.bind_texture(m_gbuffer.tex_with_name("normal_gtex"),"normal_tex");
    m_apply_postprocess_shader.bind_texture(m_gbuffer.tex_with_name("diffuse_gtex"),"diffuse_tex");
    m_apply_postprocess_shader.bind_texture(m_gbuffer.tex_with_name("metalness_and_roughness_gtex"),"metalness_and_roughness_tex");
    if(m_ao_blurred_tex.storage_initialized()){
        m_apply_postprocess_shader.bind_texture(m_ao_blurred_tex,"ao_tex");
    }

    m_apply_postprocess_shader.uniform_bool(m_using_fat_gbuffer , "using_fat_gbuffer");
    m_apply_postprocess_shader.uniform_bool(m_show_background_img , "show_background_img");
    m_apply_postprocess_shader.uniform_bool(m_show_environment_map, "show_environment_map");
    m_apply_postprocess_shader.uniform_bool(m_show_prefiltered_environment_map, "show_prefiltered_environment_map");
    m_apply_postprocess_shader.uniform_bool(m_enable_bloom, "enable_bloom");
    m_apply_postprocess_shader.uniform_int(m_bloom_start_mip_map_lvl,"bloom_start_mip_map_lvl");
    m_apply_postprocess_shader.uniform_int(m_bloom_max_mip_map_lvl,"bloom_max_mip_map_lvl");
    m_apply_postprocess_shader.uniform_float(m_camera->m_exposure, "exposure");
    // m_apply_postprocess_shader.uniform_v3_float(m_background_color, "background_color");
    m_apply_postprocess_shader.uniform_bool(m_enable_multichannel_view, "enable_multichannel_view");
    m_apply_postprocess_shader.uniform_v2_float(size_final_image, "size_final_image");
    m_apply_postprocess_shader.uniform_float(m_multichannel_interline_separation, "multichannel_interline_separation");
    m_apply_postprocess_shader.uniform_float(m_multichannel_line_width, "multichannel_line_width");
    m_apply_postprocess_shader.uniform_float(m_multichannel_line_angle, "multichannel_line_angle");
    m_apply_postprocess_shader.uniform_float(m_multichannel_start_x, "multichannel_start_x");
    m_apply_postprocess_shader.uniform_int(m_tonemap_type._to_integral() , "tonemap_type");
    m_apply_postprocess_shader.draw_into(m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex"), "out_color");
    // draw
    m_fullscreen_quad->vao.bind();
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);



    TIME_END("apply_postprocess");
    //BLEND BACKGROUND -------------------------------------------------------------------------------------------------------------------
    //blend the pure color texture that we just rendered with the bg using the alpha. This is in order to deal with bloom and translucent thing corretly and still have a saved copy of the texture with transparency



    // TIME_START("blend_bg");

    // //  Set attributes that the vao will pulll from buffers
    // GL_C( m_fullscreen_quad->vao.vertex_attribute(m_blend_bg_shader, "position", m_fullscreen_quad->V_buf, 3) );
    // GL_C( m_fullscreen_quad->vao.vertex_attribute(m_blend_bg_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    // m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles

    // // //shader setup
    // GL_C( m_blend_bg_shader.use() );

    // m_blend_bg_shader.bind_texture(m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex"),"color_with_transparency_tex");
    // // m_blend_bg_shader.uniform_bool(m_show_background_img , "show_background_img");
    // // m_blend_bg_shader.uniform_bool(m_show_environment_map, "show_environment_map");
    // // m_blend_bg_shader.uniform_bool(m_show_prefiltered_environment_map, "show_prefiltered_environment_map");
    // m_blend_bg_shader.uniform_v3_float(m_background_color, "background_color");
    // m_blend_bg_shader.draw_into(m_final_fbo_no_gui.tex_with_name("color_without_transparency_gtex"), "out_color");
    // // // draw
    // m_fullscreen_quad->vao.bind();
    // glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);

    // TIME_END("blend_bg");

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    // glDisable(GL_BLEND);

}

void Viewer::blend_bg(){

    // TIME_START("blend_bg");

    if(m_viewport_size.x()/m_subsample_factor!=m_final_fbo_no_gui.width() || m_viewport_size.y()/m_subsample_factor!=m_final_fbo_no_gui.height()){
        m_final_fbo_no_gui.set_size(m_viewport_size.x()/m_subsample_factor, m_viewport_size.y()/m_subsample_factor  );
    }
    // m_final_fbo_no_gui.clear_depth();
    // m_final_fbo_no_gui.tex_with_name("color_gtex").set_val(m_background_color.x(), m_background_color.y(), m_background_color.z(), 0.0);

    Eigen::Vector2f size_final_image;
    size_final_image << m_final_fbo_no_gui.width(), m_final_fbo_no_gui.height();


    //dont perform depth checking nor write into the depth buffer
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);

    //  Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_blend_bg_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_blend_bg_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles

    // //shader setup
    GL_C( m_blend_bg_shader.use() );

    m_blend_bg_shader.bind_texture(m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex"),"color_with_transparency_tex");
    // m_blend_bg_shader.uniform_bool(m_show_background_img , "show_background_img");
    // m_blend_bg_shader.uniform_bool(m_show_environment_map, "show_environment_map");
    // m_blend_bg_shader.uniform_bool(m_show_prefiltered_environment_map, "show_prefiltered_environment_map");
    m_blend_bg_shader.uniform_v3_float(m_background_color, "background_color");
    m_blend_bg_shader.draw_into(m_final_fbo_no_gui.tex_with_name("color_without_transparency_gtex"), "out_color");
    // // draw
    m_fullscreen_quad->vao.bind();
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);

    // TIME_END("blend_bg");

}
// cv::Mat Viewer::download_to_cv_mat(){
//     // glBindFramebuffer(GL_FRAMEBUFFER, 0);
//     // int w=m_viewport_size.x()*m_subsample_factor;
//     // int h=m_viewport_size.y()*m_subsample_factor;
//     // cv::Mat cv_mat(h, w, CV_8UC4);
//     // glReadPixels(0, 0, w, h, GL_BGRA, GL_UNSIGNED_BYTE, cv_mat.data);
//     // cv::Mat cv_mat_flipped;
//     // cv::flip(cv_mat, cv_mat_flipped, 0);
//     // return cv_mat_flipped;
// }

// Eigen::Matrix4f Viewer::compute_mvp_matrix(const MeshGLSharedPtr& mesh){
//     Eigen::Matrix4f M,V,P, MVP;

//     M=mesh->m_core->m_model_matrix.cast<float>().matrix();
//     V=m_camera->view_matrix();
//     P=m_camera->proj_matrix(m_viewport_size);
//     MVP=P*V*M;
//     return MVP;
// }

std::shared_ptr<SpotLight> Viewer::spotlight_with_idx(const size_t idx){
    CHECK(idx<m_spot_lights.size()) << "Indexing the spotlight array out of bounds";

    // for(int i=0; i<m_spot_lights.size(); i++){
    //     VLOG(1) << "light at idx" << i << " has ptr " << m_spot_lights[i];
    // }

    return m_spot_lights[idx];
}

void Viewer::create_random_samples_hemisphere(){
    m_random_samples.resize(m_nr_samples,3);
    for(int i=0; i<m_nr_samples; i++){ // http://john-chapman-graphics.blogspot.com/2013/01/ssao-tutorial.html
        m_random_samples.row(i) << m_rand_gen->rand_float(-1.0, 1.0),
                                   m_rand_gen->rand_float(-1.0, 1.0),
                                   m_rand_gen->rand_float(0.0 ,1.0);
        m_random_samples.row(i).normalize(); //this will make all the samples le on the SURFACE of the hemisphere. We will have to scale the samples so that
    }
    for(int i=0; i<m_nr_samples; i++){
        float scale = float(i) / float(m_nr_samples);
        // scale = lerp(scale*scale, 0.0, 1.0, 0.1, 1.0);
        //try another form of lerp
        scale= 0.1 + scale*scale * (1.0 - 0.1);
        m_random_samples.row(i) *= scale;
    }
}

gl::Texture2D& Viewer::rendered_tex_no_gui(const bool with_transparency){
    if (with_transparency){
        return m_final_fbo_no_gui.tex_with_name("color_with_transparency_gtex");
    }else{
        return m_final_fbo_no_gui.tex_with_name("color_without_transparency_gtex");
    }
}
gl::Texture2D& Viewer::rendered_tex_with_gui(){
    return m_final_fbo_with_gui.tex_with_name("color_gtex");
}
cv::Mat Viewer::rendered_mat_no_gui(const bool with_transparency){
    gl::Texture2D& tex=rendered_tex_no_gui(with_transparency);
    return tex.download_to_cv_mat();
}
cv::Mat Viewer::rendered_mat_with_gui(){
    gl::Texture2D& tex=rendered_tex_with_gui();
    return tex.download_to_cv_mat();
}

cv::Mat Viewer::gbuffer_mat_with_name(const std::string name){
    return m_gbuffer.tex_with_name(name).download_to_cv_mat();
}

void Viewer::load_environment_map(const std::string path){

    //check if the path is relative
    std::string path_trim=radu::utils::trim_copy(path);
    std::string path_abs;
    if (fs::path(path_trim).is_relative()){
        path_abs=(fs::path(PROJECT_SOURCE_DIR) / path_trim).string();
    }else{
        path_abs=path_trim;
    }

    CHECK(fs::is_regular_file(path_abs)) << "Could not open environment map from file " << path_abs;


    m_enable_ibl=true;

    read_background_img(m_background_tex, path_abs);
    //if it's equirectangular we convert it to cubemap because it is faster to sample
    equirectangular2cubemap(m_environment_cubemap_tex, m_background_tex);
    radiance2irradiance(m_irradiance_cubemap_tex, m_environment_cubemap_tex);
    prefilter(m_prefilter_cubemap_tex, m_environment_cubemap_tex);

}

void Viewer::read_background_img(gl::Texture2D& tex, const std::string img_path){
    cv::Mat img=cv::imread(img_path, -1); //the -1 is so that it reads the image as floats because we might read a .hdr image which needs high precision
    CHECK(img.data) << "Could not open background image " << img_path;
    cv::Mat img_flipped;
    cv::flip(img, img_flipped, 0); //flip around the horizontal axis
    tex.upload_from_cv_mat(img_flipped);

}
void Viewer::equirectangular2cubemap(gl::CubeMap& cubemap_tex, const gl::Texture2D& equirectangular_tex){


    Eigen::Vector2f viewport_size;
    viewport_size<< m_environment_cubemap_resolution, m_environment_cubemap_resolution;
    glViewport(0.0f , 0.0f, viewport_size.x(), viewport_size.y() );


    //create cam
    Camera cam;
    cam.m_fov=90;
    cam.m_near=0.01;
    cam.m_far=10.0;
    cam.set_position(Eigen::Vector3f::Zero()); //camera in the middle of the NDC


    //cam matrices.
    // We supply to the shader the coordinates in clip_space. The perspective division by w will leave the coordinates unaffected therefore the NDC is the same
    //we need to revert from clip space back to a world ray so we multiply with P_inv and afterwards with V_inv (but only the rotational part because we don't want to skybox to move when we translate the camera)
    Eigen::Matrix4f P_inv;
    P_inv=cam.proj_matrix(viewport_size).inverse();

    std::vector<Eigen::Vector3f> lookat_vectors(6); //ordering of the faces is from here https://learnopengl.com/Advanced-OpenGL/Cubemaps
    lookat_vectors[0] << 1,0,0; //right
    lookat_vectors[1] << -1,0,0; //left
    lookat_vectors[2] << 0,1,0; //up
    lookat_vectors[3] << 0,-1,0; //down
    lookat_vectors[4] << 0,0,1; //backwards
    lookat_vectors[5] << 0,0,-1; //forward
    std::vector<Eigen::Vector3f> up_vectors(6); //all of the cameras have a up vector towards positive y except the ones that look at the top and bottom faces
    //TODO for some reason the up vectors had to be negated (so the camera is upside down) and only then it works. I have no idea why
    up_vectors[0] << 0,-1,0; //right
    up_vectors[1] << 0,-1,0; //left
    up_vectors[2] << 0,0,1; //up
    up_vectors[3] << 0,0,-1; //down
    up_vectors[4] << 0,-1,0; //backwards
    up_vectors[5] << 0,-1,0; //forward


    //render this cube
    GL_C( glDisable(GL_CULL_FACE) );
    //dont perform depth checking nor write into the depth buffer
    GL_C( glDepthMask(false) );
    GL_C( glDisable(GL_DEPTH_TEST) );

    gl::Shader& shader=m_equirectangular2cubemap_shader;

    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf) ); //Says the indices with we refer to vertices, this gives us the triangles


    // //shader setup
    GL_C( shader.use() );

    for(int i=0; i<6; i++){
        //move the camera to look at the corresponding face of the cube
        cam.set_lookat(lookat_vectors[i]);
        cam.set_up(up_vectors[i]);
        Eigen::Matrix3f V_inv_rot=Eigen::Affine3f(cam.view_matrix()).inverse().linear();


        shader.uniform_3x3(V_inv_rot, "V_inv_rot");
        shader.uniform_4x4(P_inv, "P_inv");
        GL_C( shader.bind_texture(equirectangular_tex,"equirectangular_tex") );
        shader.draw_into(cubemap_tex, "out_color", i);

        // draw
        GL_C( m_fullscreen_quad->vao.bind() );
        GL_C( glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0) );

    }

    //generate mip map so we cna show as background some blurred versions of it
    cubemap_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);
    cubemap_tex.set_filter_mode_mag(GL_LINEAR);
    cubemap_tex.generate_mipmap_full();


    // //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);



}
void Viewer::radiance2irradiance(gl::CubeMap& irradiance_tex, const gl::CubeMap& radiance_tex){

    Eigen::Vector2f viewport_size;
    viewport_size<< m_irradiance_cubemap_resolution, m_irradiance_cubemap_resolution;
    glViewport(0.0f , 0.0f, viewport_size.x(), viewport_size.y() );


    //create cam
    Camera cam;
    cam.m_fov=90;
    cam.m_near=0.01;
    cam.m_far=10.0;
    cam.set_position(Eigen::Vector3f::Zero()); //camera in the middle of the NDC


    //cam matrices.
    // We supply to the shader the coordinates in clip_space. The perspective division by w will leave the coordinates unaffected therefore the NDC is the same
    //we need to revert from clip space back to a world ray so we multiply with P_inv and afterwards with V_inv (but only the rotational part because we don't want to skybox to move when we translate the camera)
    Eigen::Matrix4f P_inv;
    P_inv=cam.proj_matrix(viewport_size).inverse();

    std::vector<Eigen::Vector3f> lookat_vectors(6); //ordering of the faces is from here https://learnopengl.com/Advanced-OpenGL/Cubemaps
    lookat_vectors[0] << 1,0,0; //right
    lookat_vectors[1] << -1,0,0; //left
    lookat_vectors[2] << 0,1,0; //up
    lookat_vectors[3] << 0,-1,0; //down
    lookat_vectors[4] << 0,0,1; //backwards
    lookat_vectors[5] << 0,0,-1; //forward
    std::vector<Eigen::Vector3f> up_vectors(6); //all of the cameras have a up vector towards positive y except the ones that look at the top and bottom faces
    //TODO for some reason the up vectors had to be negated (so the camera is upside down) and only then it works. I have no idea why
    up_vectors[0] << 0,-1,0; //right
    up_vectors[1] << 0,-1,0; //left
    up_vectors[2] << 0,0,1; //up
    up_vectors[3] << 0,0,-1; //down
    up_vectors[4] << 0,-1,0; //backwards
    up_vectors[5] << 0,-1,0; //forward


    //render this cube
    GL_C( glDisable(GL_CULL_FACE) );
    //dont perform depth checking nor write into the depth buffer
    GL_C( glDepthMask(false) );
    GL_C( glDisable(GL_DEPTH_TEST) );

    gl::Shader& shader=m_radiance2irradiance_shader;

    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf) ); //Says the indices with we refer to vertices, this gives us the triangles


    // //shader setup
    GL_C( shader.use() );

    for(int i=0; i<6; i++){
        //move the camera to look at the corresponding face of the cube
        cam.set_lookat(lookat_vectors[i]);
        cam.set_up(up_vectors[i]);
        Eigen::Matrix3f V_inv_rot=Eigen::Affine3f(cam.view_matrix()).inverse().linear();


        shader.uniform_3x3(V_inv_rot, "V_inv_rot");
        shader.uniform_4x4(P_inv, "P_inv");
        GL_C( shader.bind_texture(radiance_tex,"radiance_cubemap_tex") );
        shader.draw_into(irradiance_tex, "out_color", i);

        // draw
        GL_C( m_fullscreen_quad->vao.bind() );
        GL_C( glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0) );

    }

    // //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, m_viewport_size.x(), m_viewport_size.y() );
}
void Viewer::prefilter(gl::CubeMap& prefilter_tex, const gl::CubeMap& radiance_tex){


    //create cam
    Camera cam;
    cam.m_fov=90;
    cam.m_near=0.01;
    cam.m_far=10.0;
    cam.set_position(Eigen::Vector3f::Zero()); //camera in the middle of the NDC


    //cam matrices.
    // We supply to the shader the coordinates in clip_space. The perspective division by w will leave the coordinates unaffected therefore the NDC is the same
    //we need to revert from clip space back to a world ray so we multiply with P_inv and afterwards with V_inv (but only the rotational part because we don't want to skybox to move when we translate the camera)
    Eigen::Matrix4f P_inv;

    std::vector<Eigen::Vector3f> lookat_vectors(6); //ordering of the faces is from here https://learnopengl.com/Advanced-OpenGL/Cubemaps
    lookat_vectors[0] << 1,0,0; //right
    lookat_vectors[1] << -1,0,0; //left
    lookat_vectors[2] << 0,1,0; //up
    lookat_vectors[3] << 0,-1,0; //down
    lookat_vectors[4] << 0,0,1; //backwards
    lookat_vectors[5] << 0,0,-1; //forward
    std::vector<Eigen::Vector3f> up_vectors(6); //all of the cameras have a up vector towards positive y except the ones that look at the top and bottom faces
    //TODO for some reason the up vectors had to be negated (so the camera is upside down) and only then it works. I have no idea why
    up_vectors[0] << 0,-1,0; //right
    up_vectors[1] << 0,-1,0; //left
    up_vectors[2] << 0,0,1; //up
    up_vectors[3] << 0,0,-1; //down
    up_vectors[4] << 0,-1,0; //backwards
    up_vectors[5] << 0,-1,0; //forward


    //render this cube
    GL_C( glDisable(GL_CULL_FACE) );
    //dont perform depth checking nor write into the depth buffer
    GL_C( glDepthMask(false) );
    GL_C( glDisable(GL_DEPTH_TEST) );

    gl::Shader& shader=m_prefilter_shader;

    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf) ); //Says the indices with we refer to vertices, this gives us the triangles


    // //shader setup
    GL_C( shader.use() );
    GL_C( shader.bind_texture(radiance_tex,"radiance_cubemap_tex") );
    shader.uniform_int(m_environment_cubemap_resolution, "radiance_cubemap_resolution");

    int mip_lvls=prefilter_tex.mipmap_nr_lvls();
    for (int mip = 0; mip < mip_lvls; ++mip){
        // reisze viewport according to mip-level size.
        Eigen::Vector2f viewport_size;
        viewport_size<< m_prefilter_cubemap_resolution * std::pow(0.5, mip), m_prefilter_cubemap_resolution * std::pow(0.5, mip);
        glViewport(0.0f , 0.0f, viewport_size.x(), viewport_size.y() );


        for(int i=0; i<6; i++){
            //move the camera to look at the corresponding face of the cube
            cam.set_lookat(lookat_vectors[i]);
            cam.set_up(up_vectors[i]);
            Eigen::Matrix3f V_inv_rot=Eigen::Affine3f(cam.view_matrix()).inverse().linear();
            P_inv=cam.proj_matrix(viewport_size).inverse();

            float roughness = (float)mip / (float)(mip_lvls - 1);
            shader.uniform_float(roughness, "roughness");
            shader.uniform_3x3(V_inv_rot, "V_inv_rot");
            shader.uniform_4x4(P_inv, "P_inv");
            shader.draw_into(prefilter_tex, "out_color", i, mip);

            // draw
            GL_C( m_fullscreen_quad->vao.bind() );
            GL_C( glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0) );

        }
    }

    // //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, m_viewport_size.x(), m_viewport_size.y() );
}
void Viewer::integrate_brdf(gl::Texture2D& brdf_lut_tex){

    TIME_START("compose");

    //dont perform depth checking nor write into the depth buffer
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);

    Eigen::Vector2f viewport_size;
    viewport_size<< m_brdf_lut_resolution, m_brdf_lut_resolution;
    glViewport(0.0f , 0.0f, viewport_size.x(), viewport_size.y() );


    gl::Shader& shader=m_integrate_brdf_shader;

     // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles


     //shader setup
    GL_C( shader.use() );
    shader.draw_into(brdf_lut_tex, "out_color");

    // draw
    m_fullscreen_quad->vao.bind();
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);
    TIME_END("compose");

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, m_viewport_size.x(), m_viewport_size.y() );
}


// void Viewer::print_pointers(){
//     for(int i=0; i<m_spot_lights.size(); i++){
//         VLOG(1) << "light at idx" << i << " has ptr " << m_spot_lights[i].get();
//         m_spot_lights[i]->print_ptr();
//     }
// }

// void Viewer::set_position(const int i, Eigen::Vector3f& pos){
//     // VLOG(1) << "C++ position is " << m_spot_lights[0]->position().transpose();
//     // Eigen::Vector3f pos;
//     // pos << 19.6983, 41.4829, -79.779
//     VLOG(1)<<"C++ is settig position of object " << m_spot_lights[i].get();
//     m_spot_lights[i]->set_position(pos);
// }
// void Viewer::check_position(const int i){
//     VLOG(1) << "C++ object with ptr "  <<m_spot_lights[i]<< "has position " << m_spot_lights[i]->position().transpose();
// }

void Viewer::write_gbuffer_to_folder(){
    //read the normals and dencode them for writing to file
    // gl::Texture2D normals_deencoded_debugging;
    // gl::Texture2D metalness_roughness_triple_channel_debugging; //metalness and roughness are stored as a RG but Opencv Cannot write rg so we make a rgb texture
    // gl::Texture2D depth_float_debugging;  //the depth is type depth component
    // normals_deencoded_debugging.allocate_or_resize(GL_RGB32F, GL_RGB, GL_FLOAT, m_gbuffer.width(), m_gbuffer.height());
    // metalness_roughness_triple_channel_debugging.allocate_or_resize(GL_RGB32F, GL_RGB, GL_FLOAT, m_gbuffer.width(), m_gbuffer.height());
    // depth_float_debugging.allocate_or_resize(GL_R32F, GL_RED, GL_FLOAT, m_gbuffer.width(), m_gbuffer.height());

    gl::GBuffer debug_gbuffer;
    GL_C( debug_gbuffer.set_size(m_gbuffer.width(), m_gbuffer.height() ) ); //established what will be the size of the textures attached to this framebuffer
    GL_C( debug_gbuffer.add_texture("normals_debug_gtex", GL_RGB32F, GL_RGB, GL_FLOAT) );
    GL_C( debug_gbuffer.add_texture("metalness_and_roughness_debug_gtex", GL_RGB32F, GL_RGB, GL_FLOAT) ); //metalness and roughness are stored as a RG but Opencv Cannot write rg so we make a rgb texture
    GL_C( debug_gbuffer.add_texture("depth_debug_gtex", GL_R32F, GL_RED, GL_FLOAT) ); //the depth is stored as depth_component which cannot be easily convertible to opencv so we record it here as R32F
    debug_gbuffer.sanity_check();


    glViewport(0.0f , 0.0f, m_gbuffer.width(), m_gbuffer.height() );
    gl::Shader& shader =m_decode_gbuffer_debugging;

    //dont perform depth checking nor write into the depth buffer
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);

     // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles

    //shader setup
    GL_C( shader.use() );
    shader.bind_texture(m_gbuffer.tex_with_name("normal_gtex"), "normals_encoded_tex");
    shader.bind_texture(m_gbuffer.tex_with_name("metalness_and_roughness_gtex"), "metalness_and_roughness_tex");
    shader.bind_texture(m_gbuffer.tex_with_name("depth_gtex"), "depth_tex");
    debug_gbuffer.bind_for_draw();
    shader.draw_into(debug_gbuffer,
                    {
                    std::make_pair("normal_out", "normals_debug_gtex"),
                    std::make_pair("metalness_and_roughness_out", "metalness_and_roughness_debug_gtex"),
                    std::make_pair("depth_out", "depth_debug_gtex"),
                    }
                    ); //makes the shaders draw into the buffers we defines in the gbuffer
    // draw
    m_fullscreen_quad->vao.bind();
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);




    // glFinish();
    //get all the textures as cv::mats
    cv::Mat mat, normal_mat, diffuse_mat, depth_mat, metalness_and_roughness_mat, ao_blurred_mat;
    //normal
    mat=debug_gbuffer.tex_with_name("normals_debug_gtex").download_to_cv_mat();
    cv::flip(mat, normal_mat, 0);
    cv::cvtColor(normal_mat, normal_mat, cv::COLOR_BGR2RGB);
    normal_mat*=255;
    //diffuse
    mat=m_gbuffer.tex_with_name("diffuse_gtex").download_to_cv_mat();
    cv::flip(mat, diffuse_mat, 0);
    cv::cvtColor(diffuse_mat, diffuse_mat, cv::COLOR_BGR2RGB);
    //depth
    mat=debug_gbuffer.tex_with_name("depth_debug_gtex").download_to_cv_mat();
    cv::flip(mat, depth_mat, 0);
    depth_mat*=255;
    //metalness_and_roughness
    mat=debug_gbuffer.tex_with_name("metalness_and_roughness_debug_gtex").download_to_cv_mat();
    cv::flip(mat, metalness_and_roughness_mat, 0);
    cv::cvtColor(metalness_and_roughness_mat, metalness_and_roughness_mat, cv::COLOR_BGR2RGB);
    metalness_and_roughness_mat*=255;
    //ao
    mat=m_ao_blurred_tex.download_to_cv_mat();
    cv::flip(mat, ao_blurred_mat, 0);

    fs::path path = "./debug";
    fs::create_directories(path);
    VLOG(1) << "Writing debug images in " << path;
    cv::imwrite( (path/"./g_normal.png").string(), normal_mat );
    cv::imwrite( (path/"./g_diffuse.png").string(), diffuse_mat );
    cv::imwrite( (path/"./depth.png").string(), depth_mat );
    cv::imwrite( (path/"./g_metalness_and_roughness.png").string(), metalness_and_roughness_mat );
    cv::imwrite( (path/"./ao_blurred.png").string(), ao_blurred_mat );

}

void Viewer::glfw_mouse_pressed(GLFWwindow* window, int button, int action, int modifier){
    Camera::MouseButton mb;

    if (button == GLFW_MOUSE_BUTTON_1)
        mb = Camera::MouseButton::Left;
    else if (button == GLFW_MOUSE_BUTTON_2)
        mb = Camera::MouseButton::Right;
    else //if (button == GLFW_MOUSE_BUTTON_3)
        mb = Camera::MouseButton::Middle;

    if (action == GLFW_PRESS){
        m_camera->mouse_pressed(mb,modifier);
        if(m_lights_follow_camera && m_camera==m_default_camera){
            for(size_t i=0; i<m_spot_lights.size(); i++){
                m_spot_lights[i]->mouse_pressed(mb,modifier);
            }
        }
    }
    else{
        m_camera->mouse_released(mb,modifier);
        if(m_lights_follow_camera && m_camera==m_default_camera){
            for(size_t i=0; i<m_spot_lights.size(); i++){
                m_spot_lights[i]->mouse_released(mb,modifier);
            }
        }
    }

}
void Viewer::glfw_mouse_move(GLFWwindow* window, double x, double y){
    m_camera->mouse_move(x, y, m_viewport_size, m_camera_translation_speed_multiplier);
    //only move if we are controlling the main camera and only if we rotating
    if(m_lights_follow_camera && m_camera==m_default_camera && m_camera->mouse_mode==Camera::MouseMode::Rotation){
        for(size_t i=0; i<m_spot_lights.size(); i++){
            m_spot_lights[i]->mouse_move(x, y, m_viewport_size, m_camera_translation_speed_multiplier);
        }
    }

}
void Viewer::glfw_mouse_scroll(GLFWwindow* window, double x, double y){
    m_camera->mouse_scroll(x,y);
    //do not use scroll on the lights because they will get closer to the surface and therefore appear way brither than they should be
    // if(m_lights_follow_camera && m_camera==m_default_camera){
    //     for(int i=0; i<m_spot_lights.size(); i++){
    //         m_spot_lights[i]->mouse_scroll(x,y);
    //     }
    // }

}
void Viewer::glfw_key(GLFWwindow* window, int key, int scancode, int action, int modifier){

    if (action == GLFW_PRESS){
        switch(key){
            // case '1':{
            //     VLOG(1) << "pressed 1";
            //     if (auto mesh_gpu =  m_scene->get_mesh_with_name("mesh_test")->m_mesh_gpu.lock()) {
            //             mesh_gpu->m_cur_tex_ptr=mesh_gpu->m_rgb_tex;
            //             m_scene->get_mesh_with_name("mesh_test")->m_vis.m_color_type=MeshColorType::Texture;
            //             // m_light_factor=0.0;
            //     }
            //     break;
            // }
            // case '2':{
            //     VLOG(1) << "pressed 2";
            //     if (auto mesh_gpu =  m_scene->get_mesh_with_name("mesh_test")->m_mesh_gpu.lock()) {
            //             mesh_gpu->m_cur_tex_ptr=mesh_gpu->m_thermal_tex;
            //             m_scene->get_mesh_with_name("mesh_test")->m_vis.m_color_type=MeshColorType::Texture;
            //             // m_light_factor=0.0;
            //     }
            //     break;
            // }
            // case '3':{
            //     VLOG(1) << "pressed 3";
            //     if (auto mesh_gpu =  m_scene->get_mesh_with_name("mesh_test")->m_mesh_gpu.lock()) {
            //             mesh_gpu->m_cur_tex_ptr=mesh_gpu->m_thermal_colored_tex;
            //             m_scene->get_mesh_with_name("mesh_test")->m_vis.m_color_type=MeshColorType::Texture;
            //             // m_light_factor=0.0;
            //     }
            //     break;
            // }
            case GLFW_KEY_H :{
                VLOG(1) << "toggled the main menu. Press h again for toggling";
                m_gui->toggle_main_menu();
                // m_show_gui^=1;
                break;
            }
            case GLFW_KEY_S :{
                VLOG(1) << "Snapshot";
                if(m_record_gui){
                    m_recorder->write_without_buffering(m_final_fbo_with_gui.tex_with_name("color_gtex"), m_snapshot_name, m_recording_path);
                }else{
                    m_recorder->write_without_buffering(m_final_fbo_no_gui.tex_with_name("color_gtex"), m_snapshot_name, m_recording_path);
                }
                break;
            }
            case GLFW_KEY_R :{
                VLOG(1) << "Record";
                 if(m_recorder->is_recording()){
                    VLOG(1) << "Stopping recording";
                    m_recorder->stop_recording();
                }else{
                    VLOG(1) << "Starting recording";
                    m_recorder->start_recording();
                }
                break;
            }

        }

    }else if(action == GLFW_RELEASE){

    }

    //handle ctrl c and ctrl v for camera pose copying and pasting
    if (key == GLFW_KEY_C && modifier==GLFW_MOD_CONTROL && action == GLFW_PRESS){
        VLOG(1) << "Pressed ctrl-c, copying current pose of the camera to clipoard";
        glfwSetClipboardString(window, m_camera->to_string().c_str());
    }
    if (key == GLFW_KEY_V && modifier==GLFW_MOD_CONTROL && action == GLFW_PRESS){
        VLOG(1) << "Pressed ctrl-v, copying current clipboard to camera pose";
        const char* text = glfwGetClipboardString(window);
        if(text!=NULL){
            std::string pose{text};
            m_camera->from_string(pose);
        }

    }


    // __viewer->key_down(key, modifier);
    // __viewer->key_up(key, modifier);

}
void Viewer::glfw_char_mods(GLFWwindow* window, unsigned int codepoint, int modifier){

}
void Viewer::glfw_resize(GLFWwindow* window, int width, int height){
    glfwSetWindowSize(window, width, height);
    // glfwSetWindowAspectRatio(window, width, height);
    int framebuffer_width;
    int framebuffer_height;
    glfwGetFramebufferSize(window, &framebuffer_width, &framebuffer_height);
    m_viewport_size = Eigen::Vector2f(framebuffer_width, framebuffer_height);
    // m_viewport_size = Eigen::Vector2f(width/m_subsample_factor, height/m_subsample_factor);
}

void Viewer::glfw_drop(GLFWwindow* window, int count, const char** paths){
    for(int i=0; i<count; i++){
        VLOG(1) << "loading from path " << paths[i];

        std::string file_ext = std::string(paths[i]).substr(std::string(paths[i]).find_last_of(".") + 1);
        trim(file_ext); //remove whitespaces from beggining and end
        if(file_ext=="hdr" || file_ext=="HDR" || file_ext=="exr" || file_ext=="EXR"){
            //load environment map
            // read_background_img(m_background_tex, paths[i]);
            // equirectangular2cubemap(m_environment_cubemap_tex, m_background_tex); //if it's equirectangular we convert it to cubemap because it is faster to sample
            // radiance2irradiance(m_irradiance_cubemap_tex, m_environment_cubemap_tex);
            // prefilter(m_prefilter_cubemap_tex, m_environment_cubemap_tex);
            load_environment_map(paths[i]);
        }else{
            MeshSharedPtr mesh = Mesh::create();
            bool success=mesh->load_from_file(std::string(paths[i]));
            if (success){
                std::string name= "mesh_" + std::to_string(m_scene->nr_meshes());
                m_scene->add_mesh(mesh,name);
                //select the newest mesh I added
                m_gui->select_mesh_with_idx( m_scene->nr_meshes()-1 );
            }
        }


    }
}

void Viewer::imgui_drop(GLFWwindow* window, int count, const char** paths){
    for(int i=0; i<count; i++){
        // VLOG(1) << "loading from path " << paths[i];

        //check that do we do with this drag and drop

        //check if maybe we are loading a texture
        if (m_scene->nr_meshes()>0){

            MeshSharedPtr mesh=m_scene->get_mesh_with_idx(m_gui->selected_mesh_idx() );
            if (m_gui->m_diffuse_tex_hovered){
                VLOG(1) << "setting diffuse tex from " << paths[i];
                mesh->set_diffuse_tex(paths[i]);
            }
            if (m_gui->m_normals_tex_hovered){
                VLOG(1) << "setting normals tex from " << paths[i];
                mesh->set_normals_tex(paths[i]);
            }
            if (m_gui->m_metalness_tex_hovered){
                VLOG(1) << "setting metalness tex from " << paths[i];
                mesh->set_metalness_tex(paths[i]);
            }
            if (m_gui->m_roughness_tex_hovered){
                VLOG(1) << "setting roughness tex from " << paths[i];
                mesh->set_roughness_tex(paths[i]);
            }
        }


        // std::string file_ext = std::string(paths[i]).substr(std::string(paths[i]).find_last_of(".") + 1);
        // trim(file_ext); //remove whitespaces from beggining and end
        // if(file_ext=="hdr" || file_ext=="HDR" || file_ext=="exr" || file_ext=="EXR"){
        //     //load environment map
        //     // read_background_img(m_background_tex, paths[i]);
        //     // equirectangular2cubemap(m_environment_cubemap_tex, m_background_tex); //if it's equirectangular we convert it to cubemap because it is faster to sample
        //     // radiance2irradiance(m_irradiance_cubemap_tex, m_environment_cubemap_tex);
        //     // prefilter(m_prefilter_cubemap_tex, m_environment_cubemap_tex);
        //     load_environment_map(paths[i]);
        // }else{
        //     MeshSharedPtr mesh = Mesh::create();
        //     mesh->load_from_file(std::string(paths[i]));
        //     std::string name= "mesh_" + std::to_string(m_scene->nr_meshes());
        //     m_scene->add_mesh(mesh,name);
        //     //select the newest mesh I added
        //     m_gui->select_mesh_with_idx( m_scene->nr_meshes()-1 );
        // }


    }
}



// PYBIND11_MODULE(EasyPBR, m) {
//     //hacky rosinit because I cannot call rosinit from python3 because it requires installing python3-catkin-pkg and python3-rospkg which for some reason deinstalls all of melodic
//     // m.def("rosinit", []( std::string name ) {
//     //     std::vector<std::pair<std::string, std::string> > dummy_remappings;
//     //     ros::init(dummy_remappings, name);
//     //  } );

//     pybind11::class_<Viewer> (m, "Viewer")
//     .def(pybind11::init<const std::string>())
//     .def("update", &Viewer::update, pybind11::arg("fbo_id") = 0)
//     .def_readwrite("m_gui", &Viewer::m_gui)
//     .def_readwrite("m_enable_edl_lighting", &Viewer::m_enable_edl_lighting)
//     .def_readwrite("m_enable_ssao", &Viewer::m_enable_ssao)
//     .def_readwrite("m_shading_factor", &Viewer::m_shading_factor)
//     .def_readwrite("m_light_factor", &Viewer::m_light_factor)
//     .def_readwrite("m_camera", &Viewer::m_camera)
//     .def_readwrite("m_recorder", &Viewer::m_recorder)
//     .def_readwrite("m_viewport_size", &Viewer::m_viewport_size)
//     ;

// }


} //namespace easy_pbr
