#pragma once

//c++
#include <iostream>
#include <memory>
#include <set>

// #include <glad/glad.h> // Initialize with gladLoadGL()
// Include glfw3.h after our OpenGL definitions
// #include <GLFW/glfw3.h>

//opencv
// #include "opencv2/opencv.hpp"


//gl
#include "easy_gl/Texture2D.h"
#include "easy_gl/Shader.h"





namespace radu { namespace utils {
    class RandGenerator;
    }}
namespace emilib{
    class DelayedDirWatcher;
}



namespace easy_pbr{

//forward declarations
class Viewer;
class MeshGL;
class Mesh;

//in order to dissalow building on the stack and having only ptrs https://stackoverflow.com/a/17135547
class EasyPBRwrapper;

class EasyPBRwrapper: public std::enable_shared_from_this<EasyPBRwrapper>
{
public:
    //https://stackoverflow.com/questions/29881107/creating-objects-only-as-shared-pointers-through-a-base-class-create-method
    template <class ...Args>
    static std::shared_ptr<EasyPBRwrapper> create( Args&& ...args ){
        return std::shared_ptr<EasyPBRwrapper>( new EasyPBRwrapper(std::forward<Args>(args)...) );
    }
    ~EasyPBRwrapper();



private:
    EasyPBRwrapper(const std::string& config_file, const std::shared_ptr<Viewer>& view); // we put the constructor as private so as to dissalow creating the object on the stack because we want to only used shared ptr for it
    // SyntheticGenerator(const std::string& config_file);

    // #ifdef WITH_DIR_WATCHER
        // emilib::DelayedDirWatcher dir_watcher;
    // #endif
    std::shared_ptr<emilib::DelayedDirWatcher> dir_watcher;

    std::shared_ptr<Viewer> m_view;
    std::shared_ptr<radu::utils::RandGenerator> m_rand_gen;

    gl::Shader m_blur_shader;
    gl::Shader m_toy_shader;
    gl::Texture2D m_blur_tmp_tex;

    std::shared_ptr<MeshGL> m_fullscreen_quad; //we store it here because we precompute it and then we use for composing the final image after the deffered geom pass




    //params



    void init_params(const std::string config_file);
    void compile_shaders();
    void init_opengl();
    void hotload_shaders();
    void install_callbacks(const std::shared_ptr<Viewer>& view); //installs some callbacks that will be called by the viewer after it finishes an update

    void create_mesh();

    //pre draw callbacks
    void pre_draw_animate_mesh(Viewer& view);
    void pre_draw_colorize_mesh(Viewer& view);

    //fullscreen effects
    void fullscreen_blur(Viewer& view);
    void toy_shader_example(Viewer& view);

    //post draw callbacks
    void post_draw(Viewer& view);

};

}//namespace easy_pbr
