#include "example_cpp/EasyPBRwrapper.h"

#include <fstream>
#include <chrono>
#include <thread>

//loguru
#define LOGURU_NO_DATE_TIME 1
#define LOGURU_NO_UPTIME 1
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>

//My stuff 
#include "UtilsGL.h"
#include "easy_pbr/Viewer.h"
#include "easy_pbr/Gui.h"
#include "easy_pbr/MeshGL.h"
#include "easy_pbr/Mesh.h"
#include "easy_pbr/Scene.h"
#include "RandGenerator.h"

//Add this header after we add all opengl stuff because we need the profiler to have glFinished defined
#define ENABLE_GL_PROFILING 1
#include "Profiler.h" 


//configuru
#define CONFIGURU_WITH_EIGEN 1
#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#include <configuru.hpp>
using namespace configuru;

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

// SyntheticGenerator::SyntheticGenerator(const std::string& config_file):
EasyPBRwrapper::EasyPBRwrapper(const std::string& config_file, const std::shared_ptr<Viewer>& view):
    #ifdef WITH_DIR_WATCHER 
        dir_watcher(std::string(PROJECT_SOURCE_DIR)+"/shaders/",5),
    #endif
    m_view(view),
    m_fullscreen_quad(MeshGL::create()),
    m_iter(0)
    {

        init_params(config_file);
        compile_shaders(); 
        init_opengl();                     
        install_callbacks(view);

        //creates a mesh and adds it to the scene
        create_mesh();
}

EasyPBRwrapper::~EasyPBRwrapper(){

}

void EasyPBRwrapper::init_params(const std::string config_file){

    //read all the parameters
    std::string config_file_abs;
    if (fs::path(config_file).is_relative()){
        config_file_abs=(fs::path(PROJECT_SOURCE_DIR) / config_file).string();
    }else{
        config_file_abs=config_file;
    }

    Config cfg = configuru::parse_file(config_file_abs, CFG);
    Config wrapper_config=cfg["wrapper_generator"];

    unsigned int seed=0;
    m_rand_gen=std::make_shared<RandGenerator> (seed);

}


void EasyPBRwrapper::compile_shaders(){
       
    // m_detect_balloon_shader.compile( std::string(PROJECT_SOURCE_DIR)+"/shaders/detect_balloon_vert.glsl", std::string(PROJECT_SOURCE_DIR)+"/shaders/detect_balloon_frag.glsl" ) ;
    // m_detect_copter_shader.compile( std::string(PROJECT_SOURCE_DIR)+"/shaders/detect_copter_vert.glsl", std::string(PROJECT_SOURCE_DIR)+"/shaders/detect_copter_frag.glsl" ) ;
}

void EasyPBRwrapper::init_opengl(){
    //create a fullscreen quad which we will use for rendering full screen images
    m_fullscreen_quad->m_core->create_full_screen_quad();
    m_fullscreen_quad->upload_to_gpu();
}

void EasyPBRwrapper::hotload_shaders(){
    #ifdef WITH_DIR_WATCHER
        std::vector<std::string> changed_files=dir_watcher.poll_files();
        if(changed_files.size()>0){
            compile_shaders();
        }
    #endif
}

void EasyPBRwrapper::install_callbacks(const std::shared_ptr<Viewer>& view){
    //pre draw functions (can install multiple functions and they will be called in order)
    view->add_callback_pre_draw( [this]( Viewer& v ) -> void{ this->pre_draw_animate_mesh(v); }  );
    view->add_callback_pre_draw( [this]( Viewer& v ) -> void{ this->pre_draw_colorize_mesh(v); }  );

    //post draw functions
    view->add_callback_post_draw( [this]( Viewer& v ) -> void{ this->post_draw(v); }  );
}

void EasyPBRwrapper::create_mesh(){
    MeshSharedPtr mesh= Mesh::create();
    mesh->V.resize(4,3);
    mesh->V.row(0)<< -1,-1,0;
    mesh->V.row(1)<< 1,-1,0;
    mesh->V.row(2)<< -1,1,0;
    mesh->V.row(3)<< 1,1,0;

    mesh->F.resize(2,3);
    mesh->F.row(0)<< 0,1,2;
    mesh->F.row(1)<< 2,1,3;

    mesh->m_vis.m_show_wireframe=true;
    mesh->m_vis.m_line_width=5.0;
    Scene::show(mesh, "mesh");
}


void EasyPBRwrapper::pre_draw_animate_mesh(Viewer& view){
    //get a handle to the mesh from the scene and move it in the x direction
    MeshSharedPtr mesh= Scene::get_mesh_with_name("mesh");
    mesh->m_model_matrix.translation().x() = std::sin( m_iter/100.0 ); 

}

void EasyPBRwrapper::pre_draw_colorize_mesh(Viewer& view){
    //get a handle to the mesh from the scene and modify its color
    MeshSharedPtr mesh= Scene::get_mesh_with_name("mesh");
    mesh->m_vis.m_solid_color.x() = std::fabs(std::sin( m_iter/50.0 ));

}

void EasyPBRwrapper::post_draw(Viewer& view){
    //get the final render as a opencv mat
    cv::Mat mat = view.rendered_tex_no_gui().download_to_cv_mat();

    //the opencv mat can now be written to disk or even rendered in the GUI as a texture
    cv::flip(mat, mat, 0);
    cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
    Gui::show(mat, "mat");

    m_iter++;
}

