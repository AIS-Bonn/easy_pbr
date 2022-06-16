#include "easy_pbr/MeshGL.h"

//c++
#include <iostream>
#include <algorithm>

#include "easy_gl/UtilsGL.h"


//my stuff
#include "easy_pbr/Mesh.h"

namespace easy_pbr{

MeshGL::MeshGL():
    m_first_core_assignment(true),
    m_sticky(false),
    V_buf("V_buf"),
    F_buf("F_buf"),
    C_buf("C_buf"),
    E_buf("E_buf"),
    D_buf("D_buf"),
    NF_buf("NF_buf"),
    NV_buf("NV_buf"),
    UV_buf("UV_buf"),
    V_tangent_u_buf("V_tangent_u_buf"),
    V_lenght_v_buf("V_lenght_v_buf"),
    // m_rgb_tex(new gl::Texture2D("rgb_tex")),
    // m_thermal_tex(new gl::Texture2D("thermal_tex")),
    // m_thermal_colored_tex(new gl::Texture2D("thermal_colored_tex")),
    // m_cur_tex_ptr(m_rgb_tex),
    m_core(new Mesh)
    {

    //Set the parameters for the buffers
    V_buf.set_target(GL_ARRAY_BUFFER);
    F_buf.set_target(GL_ELEMENT_ARRAY_BUFFER);
    C_buf.set_target(GL_ARRAY_BUFFER);
    E_buf.set_target(GL_ELEMENT_ARRAY_BUFFER);
    D_buf.set_target(GL_ARRAY_BUFFER);
    NF_buf.set_target(GL_ARRAY_BUFFER);
    NV_buf.set_target(GL_ARRAY_BUFFER);
    UV_buf.set_target(GL_ARRAY_BUFFER);
    V_tangent_u_buf.set_target(GL_ARRAY_BUFFER);
    V_lenght_v_buf.set_target(GL_ARRAY_BUFFER);
    L_pred_buf.set_target(GL_ARRAY_BUFFER);
    L_gt_buf.set_target(GL_ARRAY_BUFFER);
    I_buf.set_target(GL_ARRAY_BUFFER);


    // V_buf.set_type(GL_FLOAT);
    // F_buf.set_type(GL_UNSIGNED_INT);
    // C_buf.set_type(GL_FLOAT);
    // E_buf.set_type(GL_UNSIGNED_INT);
    // D_buf.set_type(GL_FLOAT);
    // NF_buf.set_type(GL_FLOAT);
    // NV_buf.set_type(GL_FLOAT);
    // UV_buf.set_type(GL_FLOAT);
    // V_tangent_u_buf.set_type(GL_FLOAT);
    // V_lenght_v_buf.set_type(GL_FLOAT);
    // L_pred_buf.set_type(GL_UNSIGNED_INT);
    // L_gt_buf.set_type(GL_UNSIGNED_INT);
    // I_buf.set_type(GL_FLOAT);


    m_diffuse_tex.set_wrap_mode(GL_REPEAT);
    m_metalness_tex.set_wrap_mode(GL_REPEAT);
    m_roughness_tex.set_wrap_mode(GL_REPEAT);
    m_normals_tex.set_wrap_mode(GL_REPEAT);
    m_matcap_tex.set_wrap_mode(GL_REPEAT);

    m_diffuse_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);
    m_metalness_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);
    m_roughness_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);
    m_normals_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);
    m_matcap_tex.set_filter_mode_min(GL_LINEAR_MIPMAP_LINEAR);

}

void MeshGL::assign_core(std::shared_ptr<Mesh> mesh_core){

    // bool visualization_changed= m_core->m_vis!=mesh_core->m_vis;

    if(m_first_core_assignment || mesh_core->m_force_vis_update ){
        //asign the whole core together with all the options like m_show_points and so on
        m_core=mesh_core;
        m_first_core_assignment=false;
    }else{
        //set only the buffers V,F etc and leave the options untouched. We don't overwrite them because we want to modify them through the gui
        VisOptions old_vis=m_core->m_vis;
        m_core=mesh_core;
        m_core->m_vis=old_vis; //restore the visualization options we had in the old core
    }
}

void MeshGL::sanity_check() const{
    m_core->sanity_check();
}


void MeshGL::upload_to_gpu(){


    // Temporary copy of the content of each VBO. need to make it float and row major
    typedef Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXf;
    typedef Eigen::Matrix<unsigned,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXi;
    RowMatrixXf V_f=m_core->V.cast<float>();
    //in order to support alos V vector with only 2 columns, we create a V_f with 3 and the z component will be set to 0. We do this because all the shaders support only positions which are vec3
    // RowMatrixXf V_f;
    // if(m_core->V.cols() ==3){
    //     V_f=m_core->V.cast<float>();
    // }else if( m_core->V.cols() ==2 ){
    //     RowMatrixXf V_f_tmp=m_core->V.cast<float>();
    //     V_f.resize(m_core->V.rows(),3);
    //     V_f.setZero();
    //     V_f.block(0,0, m_core->V.rows(), m_core->V.cols() )=V_f_tmp;
    // }else{
    //     LOG(FATAL) << "Cannot render a mesh with nr of coordinates per point different than 2 or 3. This mesh has V.cols()= " <<m_core->V.cols();
    // }



    RowMatrixXi F_i=m_core->F.cast<unsigned>();
    RowMatrixXf C_f=m_core->C.cast<float>();
    RowMatrixXi E_i=m_core->E.cast<unsigned>();
    RowMatrixXf D_f=m_core->D.cast<float>();
    RowMatrixXf NF_f=m_core->NF.cast<float>();
    RowMatrixXf NV_f=m_core->NV.cast<float>();
    RowMatrixXf UV_f=m_core->UV.cast<float>();
    RowMatrixXf V_tangent_u_f=m_core->V_tangent_u.cast<float>();
    RowMatrixXf V_lenght_v_f=m_core->V_length_v.cast<float>();
    RowMatrixXi L_pred_i=m_core->L_pred.cast<unsigned>();
    RowMatrixXi L_gt_i=m_core->L_gt.cast<unsigned>();
    RowMatrixXf I_f=m_core->I.cast<float>();



    V_buf.upload_data(V_f.size()*sizeof(float), V_f.data(), GL_DYNAMIC_DRAW);
    F_buf.upload_data(F_i.size()*sizeof(unsigned), F_i.data(), GL_DYNAMIC_DRAW);
    C_buf.upload_data(C_f.size()*sizeof(float), C_f.data(), GL_DYNAMIC_DRAW);
    E_buf.upload_data(E_i.size()*sizeof(unsigned), E_i.data(), GL_DYNAMIC_DRAW);
    D_buf.upload_data(D_f.size()*sizeof(float), D_f.data(), GL_DYNAMIC_DRAW);
    NF_buf.upload_data(NF_f.size()*sizeof(float), NF_f.data(), GL_DYNAMIC_DRAW);
    NV_buf.upload_data(NV_f.size()*sizeof(float), NV_f.data(), GL_DYNAMIC_DRAW);
    UV_buf.upload_data(UV_f.size()*sizeof(float), UV_f.data(), GL_DYNAMIC_DRAW);
    V_tangent_u_buf.upload_data(V_tangent_u_f.size()*sizeof(float), V_tangent_u_f.data(), GL_DYNAMIC_DRAW);
    V_lenght_v_buf.upload_data(V_lenght_v_f.size()*sizeof(float), V_lenght_v_f.data(), GL_DYNAMIC_DRAW);
    L_pred_buf.upload_data(L_pred_i.size()*sizeof(unsigned), L_pred_i.data(), GL_DYNAMIC_DRAW);
    L_gt_buf.upload_data(L_gt_i.size()*sizeof(unsigned), L_gt_i.data(), GL_DYNAMIC_DRAW);
    I_buf.upload_data(I_f.size()*sizeof(float), I_f.data(), GL_DYNAMIC_DRAW);

    // if(m_core->m_rgb_tex_cpu.data){
        // GL_C(m_rgb_tex->upload_from_cv_mat(m_core->m_rgb_tex_cpu) );
    // }

    //pbr textures
    if (m_core->m_diffuse_mat.mat.data && m_core->m_diffuse_mat.is_dirty){
        m_core->m_diffuse_mat.is_dirty=false;
        GL_C(m_diffuse_tex.upload_from_cv_mat(m_core->m_diffuse_mat.mat) );
        m_diffuse_tex.generate_mipmap_full();
    }
    if (m_core->m_metalness_mat.mat.data && m_core->m_metalness_mat.is_dirty){
        m_core->m_metalness_mat.is_dirty=false;
        GL_C(m_metalness_tex.upload_from_cv_mat(m_core->m_metalness_mat.mat) );
        m_metalness_tex.generate_mipmap_full();
    }
    if (m_core->m_roughness_mat.mat.data && m_core->m_roughness_mat.is_dirty){
        m_core->m_roughness_mat.is_dirty=false;
        GL_C(m_roughness_tex.upload_from_cv_mat(m_core->m_roughness_mat.mat) );
        m_roughness_tex.generate_mipmap_full();
    }
    if (m_core->m_normals_mat.mat.data && m_core->m_normals_mat.is_dirty){
        m_core->m_normals_mat.is_dirty=false;
        GL_C(m_normals_tex.upload_from_cv_mat(m_core->m_normals_mat.mat) );
        m_normals_tex.generate_mipmap_full();
    }
    if (m_core->m_matcap_mat.mat.data && m_core->m_matcap_mat.is_dirty){
        m_core->m_matcap_mat.is_dirty=false;
        GL_C(m_matcap_tex.upload_from_cv_mat(m_core->m_matcap_mat.mat) );
        m_matcap_tex.generate_mipmap_full();
    }

    m_core->m_is_dirty=false;
}


} //namespace easy_pbr
