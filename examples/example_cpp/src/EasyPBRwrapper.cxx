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

using namespace easy_pbr;
using namespace radu::utils;

// SyntheticGenerator::SyntheticGenerator(const std::string& config_file):
EasyPBRwrapper::EasyPBRwrapper(const std::string& config_file, const std::shared_ptr<Viewer>& view):
    #ifdef WITH_DIR_WATCHER 
        dir_watcher(std::string(PROJECT_SOURCE_DIR)+"/shaders/",5),
    #endif
    m_view(view),
    m_fullscreen_quad(MeshGL::create())
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
    m_blur_shader.compile( std::string(PROJECT_SOURCE_DIR)+"/shaders/render/blur_vert.glsl", std::string(PROJECT_SOURCE_DIR)+"/shaders/render/blur_frag.glsl"  );
    m_toy_shader.compile( std::string(PROJECT_SOURCE_DIR)+"/shaders/render/toy_shader_vert.glsl", std::string(PROJECT_SOURCE_DIR)+"/shaders/render/toy_shader_frag.glsl"  );
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
    // view->add_callback_pre_draw( [this]( Viewer& v ) -> void{ this->pre_draw_animate_mesh(v); }  );
    view->add_callback_pre_draw( [this]( Viewer& v ) -> void{ this->pre_draw_colorize_mesh(v); }  );

    //callbacks that are useful for applying efffects on the whole screen without affecting the gui
    view->add_callback_post_draw( [this]( Viewer& v ) -> void{ this->fullscreen_blur(v); }  );
    // view->add_callback_post_draw( [this]( Viewer& v ) -> void{ this->toy_shader_example(v); }  );

    //post draw functions
    view->add_callback_post_draw( [this]( Viewer& v ) -> void{ this->post_draw(v); }  );
}

void EasyPBRwrapper::create_mesh(){
    // MeshSharedPtr mesh= Mesh::create();
    // mesh->V.resize(4,3);
    // mesh->V.row(0)<< -1,-1,0;
    // mesh->V.row(1)<< 1,-1,0;
    // mesh->V.row(2)<< -1,1,0;
    // mesh->V.row(3)<< 1,1,0;

    // mesh->F.resize(2,3);
    // mesh->F.row(0)<< 0,1,2;
    // mesh->F.row(1)<< 2,1,3;

    // mesh->m_vis.m_show_wireframe=true;
    // mesh->m_vis.m_line_width=5.0;
    // Scene::show(mesh, "mesh");

    //bunny 
    MeshSharedPtr mesh= Mesh::create();
    // mesh->load_from_file("./data/bunny.ply");
    mesh->load_from_file("./data/dragon.obj");
    Scene::show(mesh, "mesh");
}


void EasyPBRwrapper::pre_draw_animate_mesh(Viewer& view){
    //get a handle to the mesh from the scene and move it in the x direction
    MeshSharedPtr mesh= Scene::get_mesh_with_name("mesh");
    mesh->model_matrix_ref().translation().x() = std::sin( view.m_timer->elapsed_s()  )* 0.2; 
    // mesh->model_matrix_ref().translation().y() = std::sin( view.m_timer->elapsed_s() + 3.14  ); 

}

void EasyPBRwrapper::pre_draw_colorize_mesh(Viewer& view){
    //get a handle to the mesh from the scene and modify its color
    MeshSharedPtr mesh= Scene::get_mesh_with_name("mesh");
    mesh->m_vis.m_solid_color.x() = std::fabs(std::sin( view.m_timer->elapsed_s() ));

}

void EasyPBRwrapper::toy_shader_example(Viewer& view){
    //blurs the fullscreen image without affecting the gui


    hotload_shaders();

    TIME_START("toy_shader");

    gl::Texture2D& img = view.rendered_tex_no_gui(/*with_transparency*/false);

    //dont perform depth checking nor write into the depth buffer 
    glDepthMask(false);
    glDisable(GL_DEPTH_TEST);
    
    // Set attributes that the vao will pulll from buffers
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_toy_shader, "position", m_fullscreen_quad->V_buf, 3) );
    GL_C( m_fullscreen_quad->vao.vertex_attribute(m_toy_shader, "uv", m_fullscreen_quad->UV_buf, 2) );
    m_fullscreen_quad->vao.indices(m_fullscreen_quad->F_buf); //Says the indices with we refer to vertices, this gives us the triangles

    //shader setup
    GL_C( m_toy_shader.use() );



    // m_toy_shader.uniform_bool(view."horizontal");
    m_toy_shader.draw_into(img, "color_output"); 
    // draw
    m_fullscreen_quad->vao.bind(); 
    glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);



    TIME_END("toy_shader");

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, view.m_viewport_size.x()/view.m_subsample_factor, view.m_viewport_size.y()/view.m_subsample_factor );

}

void EasyPBRwrapper::fullscreen_blur(Viewer& view){
    //blurs the fullscreen image without affecting the gui


    hotload_shaders();

    TIME_START("blur_img");

    gl::Texture2D& img = view.rendered_tex_no_gui(/*with_transparency*/false);
    int start_mip_map_lvl=0;
    int max_mip_map_lvl=1;
    int bloom_blur_iters=20;

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
            m_blur_shader.bind_texture(view.m_gbuffer.tex_with_name("depth_gtex"), "depth_tex");
            m_blur_shader.uniform_int(mip,"mip_map_lvl");
            m_blur_shader.uniform_bool(true,"horizontal");
            m_blur_shader.draw_into(m_blur_tmp_tex, "blurred_output",mip-start_mip_map_lvl); 
            // draw
            m_fullscreen_quad->vao.bind(); 
            glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);


            //do it in the vertical direction
            m_blur_shader.bind_texture(m_blur_tmp_tex,"img");
            m_blur_shader.bind_texture(view.m_gbuffer.tex_with_name("depth_gtex"), "depth_tex");
            m_blur_shader.uniform_int(mip-start_mip_map_lvl,"mip_map_lvl");
            m_blur_shader.uniform_bool(false,"horizontal");
            m_blur_shader.draw_into(img, "blurred_output", mip); 
            // draw
            m_fullscreen_quad->vao.bind(); 
            glDrawElements(GL_TRIANGLES, m_fullscreen_quad->m_core->F.size(), GL_UNSIGNED_INT, 0);

        }


    }
    
    // view.m_gui->show_gl_texture(m_blur_tmp_tex.tex_id(), "blur_tmp_tex", true);
    // view.m_gui->show_gl_texture(img.tex_id(), "blur1_tex", true);


    TIME_END("blur_img");

    //restore the state
    glDepthMask(true);
    glEnable(GL_DEPTH_TEST);
    glViewport(0.0f , 0.0f, view.m_viewport_size.x()/view.m_subsample_factor, view.m_viewport_size.y()/view.m_subsample_factor );

}

void EasyPBRwrapper::post_draw(Viewer& view){
    //get the final render as a opencv mat
    // cv::Mat mat = view.rendered_tex_no_gui(false).download_to_cv_mat();

    // //the opencv mat can now be written to disk or even rendered in the GUI as a texture
    // cv::flip(mat, mat, 0);
    // cv::cvtColor(mat, mat, cv::COLOR_BGR2RGB);
    // Gui::show(mat, "mat");

    //draw some gui elements 
    ImGuiWindowFlags window_flags = 0;
    ImGui::Begin("WrapperGui", nullptr, window_flags);

    if (ImGui::Button("My new button")){
        VLOG(1) << "Clicked button";
    }

    ImGui::End();

    // m_iter++;
}

