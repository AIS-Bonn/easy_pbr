#include "easy_pbr/SyntheticGenerator.h"

//loguru
// #define LOGURU_IMPLEMENTATION 1
#define LOGURU_NO_DATE_TIME 1
#define LOGURU_NO_UPTIME 1
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

// #include <glad/glad.h>
// #include <GLFW/glfw3.h> //glfw3.h after our OpenGL definitions
 
//My stuff 
#include "UtilsGL.h"
#include "easy_pbr/Viewer.h"
#include "easy_pbr/Recorder.h"
#include "RandGenerator.h"
#include "opencv_utils.h"

//Add this header after we add all opengl stuff because we need the profiler to have glFinished defined
// #define PROFILER_IMPLEMENTATION 1
#define ENABLE_GL_PROFILING 1
#include "Profiler.h" 


//configuru
// #define CONFIGURU_IMPLEMENTATION 1
#define CONFIGURU_WITH_EIGEN 1
#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#include <configuru.hpp>
using namespace configuru;

//only for debugginf why does it segment fault when creating the texture 
#include <GLFW/glfw3.h>

using namespace easy_pbr::utils;

// SyntheticGenerator::SyntheticGenerator(const std::shared_ptr<Viewer>& view):
SyntheticGenerator::SyntheticGenerator(const std::string& config_file):
    #ifdef WITH_DIR_WATCHER 
        dir_watcher(std::string(PROJECT_SOURCE_DIR)+"/shaders/",5)
    #endif
    // m_balloon_outline_tex("balloon_outline_tex")
    // m_recorder(new Recorder()),
    // m_rand_gen(new RandGenerator())
    {
        VLOG(1) << "Creating synthetic generator";
        // init_params(config_file);
        // compile_shaders(); 
        // init_opengl();                     

    VLOG(1) << "getting context";
    GLFWwindow* window = glfwGetCurrentContext();
    VLOG(1) << "got context";
    glfwMakeContextCurrent(window);
    VLOG(1) << "made current";
    gl::Texture2D m_balloon_outline_tex;

}


void SyntheticGenerator::init_params(const std::string config_file){

    //read all the parameters
    Config cfg = configuru::parse_file(std::string(CMAKE_SOURCE_DIR)+"/config/"+config_file, CFG);
    Config synth_config=cfg["synthetic_generator"];

    // m_show_gui = synth_config["show_gui"];

}


void SyntheticGenerator::compile_shaders(){
       
    // m_draw_points_shader.compile( std::string(PROJECT_SOURCE_DIR)+"/shaders/render/points_vert.glsl", std::string(PROJECT_SOURCE_DIR)+"/shaders/render/points_frag.glsl" ) ;
}

void SyntheticGenerator::init_opengl(){
    // // //initialize the g buffer with some textures 
    // GL_C( m_gbuffer.set_size(m_viewport_size.x(), m_viewport_size.y() ) ); //established what will be the size of the textures attached to this framebuffer
    // GL_C( m_gbuffer.add_texture("diffuse_gtex", GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE) ); 
    // GL_C( m_gbuffer.add_texture("normal_gtex", GL_RG16F, GL_RG, GL_HALF_FLOAT) );  //as done by Cry Engine 3 in their presentation "A bit more deferred"  https://www.slideshare.net/guest11b095/a-bit-more-deferred-cry-engine3
    // GL_C( m_gbuffer.add_texture("metalness_and_roughness_gtex", GL_RG8, GL_RG, GL_UNSIGNED_BYTE) ); 
    // GL_C( m_gbuffer.add_depth("depth_gtex") );
    // m_gbuffer.sanity_check();

    // //set all the normal buffer to nearest because we assume that the norm of it values can be used to recover the n.z. However doing a nearest neighbour can change the norm and therefore fuck everything up
    // m_gbuffer.tex_with_name("normal_gtex").set_filter_mode_min_mag(GL_NEAREST);

    // //cubemaps 
    // glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); //to linearly filter across faces of the cube
    // m_environment_cubemap_tex.allocate_tex_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_environment_cubemap_resolution, m_environment_cubemap_resolution);
    // m_irradiance_cubemap_tex.allocate_tex_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_irradiance_cubemap_resolution, m_irradiance_cubemap_resolution);
    // m_prefilter_cubemap_tex.allocate_tex_storage(GL_RGB16F, GL_RGB, GL_HALF_FLOAT, m_prefilter_cubemap_resolution, m_prefilter_cubemap_resolution);
    // m_prefilter_cubemap_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);
    // m_prefilter_cubemap_tex.set_filter_mode_mag(GL_LINEAR);
    // m_prefilter_cubemap_tex.generate_mipmap_full();

}

void SyntheticGenerator::hotload_shaders(){
    #ifdef WITH_DIR_WATCHER

        std::vector<std::string> changed_files=dir_watcher.poll_files();
        if(changed_files.size()>0){
            compile_shaders();
        }

    #endif
}