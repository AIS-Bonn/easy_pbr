#pragma once

#include <memory>
#include<stdarg.h>



//loguru
#define LOGURU_WITH_STREAMS 1
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>


#include "Buf.h"
#include "Texture2D.h"
#include "VertexArrayObject.h"

#include "easy_pbr/Mesh.h"



//in order to dissalow building on the stack and having only ptrs https://stackoverflow.com/a/17135547
class MeshGL;
template <class ...Args>
std::unique_ptr<MeshGL> MeshGLCreate(Args&& ...args);

class Mesh {
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    ~MeshGL()=default;

    void assign_core(std::shared_ptr<Mesh>);
    void create_full_screen_quad();
    void sanity_check() const; //check that all the data inside the mesh is valid, there are enough normals for each face, faces don't idx invalid points etc.


    //GL functions 
    void upload_to_gpu();

    bool m_first_core_assignment;

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

    //we store the textures then as shared ptr so we can have a weak ptr that selects the one we sho
    std::shared_ptr<gl::Texture2D> m_rgb_tex; 
    std::shared_ptr<gl::Texture2D> m_thermal_tex; 
    std::shared_ptr<gl::Texture2D> m_thermal_colored_tex; 
    std::shared_ptr<gl::Texture2D> m_cur_tex_ptr; //points to the texture that is currently being displayed

    std::shared_ptr<Mesh> m_core;
private:
    MeshGL();  // we put the constructor as private so as to dissalow creating Mesh on the stack because we want to only used shared ptr for it
    
    //https://stackoverflow.com/questions/29881107/creating-objects-only-as-shared-pointers-through-a-base-class-create-method
    template<class... Args>
    friend  std::unique_ptr<Mesh> MeshGLCreate(Args&&... args){
        return std::unique_ptr<MeshGL>(new MeshGL(std::forward<Args>(args)...));
    }


};

typedef std::shared_ptr<MeshGL> MeshGLSharedPtr;