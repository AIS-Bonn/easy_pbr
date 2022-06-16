#pragma once

#include <memory>
#include<stdarg.h>
#include <stdexcept>


//we add them first because they might contain torch and we need to include torch before we include loguru otherwise loguru doesnt work
#include "easy_gl/Buf.h"
#include "easy_gl/Texture2D.h"
#include "easy_gl/VertexArrayObject.h"

// //loguru
// #define LOGURU_WITH_STREAMS 1
// #define LOGURU_REPLACE_GLOG 1
// #include <loguru.hpp>



// #include "easy_pbr/Mesh.h"

namespace easy_pbr{

//forward declarations
class Mesh;

//in order to dissalow building on the stack and having only ptrs https://stackoverflow.com/a/17135547
class MeshGL;

class MeshGL {
public:
    // EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    //https://stackoverflow.com/questions/29881107/creating-objects-only-as-shared-pointers-through-a-base-class-create-method
    template <class ...Args>
    static std::shared_ptr<MeshGL> create( Args&& ...args ){
        return std::shared_ptr<MeshGL>( new MeshGL(std::forward<Args>(args)...) );
    }
    ~MeshGL()=default;

    void assign_core(std::shared_ptr<Mesh>);
    void create_full_screen_quad();
    void sanity_check() const; //check that all the data inside the mesh is valid, there are enough normals for each face, faces don't idx invalid points etc.


    //GL functions
    void upload_to_gpu();

    bool m_first_core_assignment;
    bool m_sticky; //if it's set to true then this meshgl will not get automtically deleted even if the mesh core is not in the scene. Useful for when external code wants to push data to opengl but doesnt necessarally want to add the meshcore to the scene for visualization

    //GL buffers
    gl::VertexArrayObject vao;
    gl::Buf V_buf;
    gl::Buf F_buf;
    gl::Buf C_buf;
    gl::Buf E_buf;
    gl::Buf D_buf;
    gl::Buf NF_buf;
    gl::Buf NV_buf;
    gl::Buf UV_buf;
    gl::Buf V_tangent_u_buf;
    gl::Buf V_lenght_v_buf;
    gl::Buf L_pred_buf;
    gl::Buf L_gt_buf;
    gl::Buf I_buf;
    //extra stuff. We store them as shared ptr because I don't want to deal with constructors and destructors when I insert into the map
    std::map<std::string, std::shared_ptr<gl::Buf>  > extra_array_buffers;
    std::map<std::string, std::shared_ptr<gl::Buf> > extra_element_array_buffers;
    std::map<std::string, std::shared_ptr<gl::Texture2D> > extra_textures;

    void add_extra_array_buffer(const std::string name){
        // gl::Buf new_buf;
        // new_buf.set_target(GL_ARRAY_BUFFER);
        // extra_array_buffers[name] = new_buf;
        // extra_array_buffers.emplace(name, std::move(new_buf));

        std::shared_ptr<gl::Buf> new_buf = std::make_shared<gl::Buf>();
        new_buf->set_target(GL_ARRAY_BUFFER);
        extra_array_buffers[name] = new_buf;
    }
    bool has_extra_array_buffer(const std::string name){
        if ( extra_array_buffers.find(name) == extra_array_buffers.end() ) {
            return false;
        } else {
            return true;
        }
    }
    gl::Buf& get_extra_array_buffer(const std::string name){
        // CHECK(has_extra_array_buffer(name)) << "The array_buffer you want to acces with name " << name << " does not exist. Please add it with add_extra_array_buffer";
        if (!has_extra_array_buffer(name)){
            throw std::runtime_error( "The array_buffer you want to acces with name " + name + " does not exist. Please add it with add_extra_array_buffer" );
        }
        return *extra_array_buffers.at(name);
    }

    //we store the textures then as shared ptr so we can have a weak ptr that selects the one we sho
    // std::shared_ptr<gl::Texture2D> m_rgb_tex;
    // std::shared_ptr<gl::Texture2D> m_thermal_tex;
    // std::shared_ptr<gl::Texture2D> m_thermal_colored_tex;
    // std::shared_ptr<gl::Texture2D> m_cur_tex_ptr; //points to the texture that is currently being displayed

    //texture used for PBR
    gl::Texture2D m_diffuse_tex;
    gl::Texture2D m_metalness_tex;
    gl::Texture2D m_roughness_tex;
    gl::Texture2D m_normals_tex;
    gl::Texture2D m_matcap_tex;

    std::shared_ptr<Mesh> m_core;
private:
    MeshGL();  // we put the constructor as private so as to dissalow creating Mesh on the stack because we want to only used shared ptr for it

};

typedef std::shared_ptr<MeshGL> MeshGLSharedPtr;


} //namespace easy_pbr
