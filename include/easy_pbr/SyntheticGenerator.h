#pragma once

//c++
#include <iostream>
#include <memory>

// #include <glad/glad.h> // Initialize with gladLoadGL()
// Include glfw3.h after our OpenGL definitions
// #include <GLFW/glfw3.h>

//opencv
// #include "opencv2/opencv.hpp"

//dir watcher
#include "dir_watcher/dir_watcher.hpp"

//gl
#include "Texture2D.h"
#include "Shader.h"


//forward declarations
class Viewer;
class Recorder;
class RandGenerator;

//in order to dissalow building on the stack and having only ptrs https://stackoverflow.com/a/17135547
class SyntheticGenerator;

class SyntheticGenerator: public std::enable_shared_from_this<SyntheticGenerator>
{
public:
    //https://stackoverflow.com/questions/29881107/creating-objects-only-as-shared-pointers-through-a-base-class-create-method
    template <class ...Args>
    static std::shared_ptr<SyntheticGenerator> create( Args&& ...args ){
        return std::shared_ptr<SyntheticGenerator>( new SyntheticGenerator(std::forward<Args>(args)...) );
        // return std::make_shared<SyntheticGenerator>( std::forward<Args>(args)... );
    }

    bool dummy_glad;
    


private:
    // SyntheticGenerator(const std::shared_ptr<Viewer>& view); // we put the constructor as private so as to dissalow creating the object on the stack because we want to only used shared ptr for it
    SyntheticGenerator(const std::string& config_file);

    std::shared_ptr<Viewer> m_view;
    std::shared_ptr<Recorder> m_recorder;
    std::shared_ptr<RandGenerator> m_rand_gen;

    // gl::Texture2D m_balloon_outline_tex;
    // gl::Shader m_balloon_outline_shader;

    #ifdef WITH_DIR_WATCHER
        emilib::DelayedDirWatcher dir_watcher;
    #endif
    

    //params


    void init_params(const std::string config_file);
    void compile_shaders();
    void init_opengl();
    void hotload_shaders();
    void install_callbacks(); //installs some callbacks that will be called by the viewer after it finishes an update
    bool func(); // this will get from the viewer the last framebuffer in which it drew. It will run full screen quad shader that will sample from from the framebuffer and detect when it goes from depth 1 to depth 0 and will write a 1 into our texture right there. 

};
