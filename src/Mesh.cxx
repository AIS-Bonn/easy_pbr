#include "easy_pbr/Mesh.h"

//c++
#include <iostream>
#include <algorithm>
#include <unordered_map> //for deduplicating obj vertices

//my stuff
// #include "MiscUtils.h"
#include "easy_pbr/LabelMngr.h"
#include "easy_pbr/Scene.h"

//libigl
#include "igl/per_face_normals.h"
#include "igl/per_vertex_normals.h"
#include "igl/readOFF.h"
#include "igl/readSTL.h"
// #include "igl/readOBJ.h" //DO NOT USE! The reader is kinda poop and in some formats of obj it just doesnt read the faces
// #include "igl/readPLY.h" // DO NOT USE! At the moment libigl readPLY has a memory leak https://github.com/libigl/libigl/issues/919
#include "tinyply.h"
#include "igl/writePLY.h"
#include "igl/writeOBJ.h"
#include <igl/qslim.h>
#include <igl/remove_duplicates.h>
#include <igl/facet_components.h>
#include <igl/vertex_triangle_adjacency.h>
#include <igl/remove_duplicate_vertices.h>
#include <igl/connect_boundary_to_infinity.h>
#include <igl/upsample.h>
#include <igl/loop.h>
#include <igl/point_mesh_squared_distance.h>
#ifdef EASYPBR_WITH_EMBREE
    #include <igl/embree/EmbreeIntersector.h>
    #include <igl/embree/ambient_occlusion.h>
#endif

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

//for reading pcd files
#ifdef EASYPBR_WITH_PCL
    #include <pcl/io/pcd_io.h>
    #include <pcl/point_types.h>
    #include <pcl/features/normal_3d.h>
    #include <pcl/kdtree/kdtree_flann.h>
#endif

#include "nanoflann.hpp"

#include "RandGenerator.h"
#include "ColorMngr.h"
#include "numerical_utils.h"
#include "eigen_utils.h"
#include "string_utils.h"
#include "opencv_utils.h"
using namespace radu::utils;

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

//loguru
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>


namespace easy_pbr{

Mesh::Mesh():
        id(0),
        m_is_dirty(true),
        m_is_shadowmap_dirty(true),
        m_model_matrix(Eigen::Affine3d::Identity()),
        m_cur_pose(Eigen::Affine3d::Identity()),
        m_width(0),
        m_height(0),
        m_view_direction(-1),
        m_force_vis_update(false),
        m_rand_gen(new RandGenerator()),
        m_colors_are_precomputed_ao(false),
        m_is_preallocated(false),
        V_blob(V)
    {
    clear();

}

Mesh::Mesh(const std::string file_path):
     Mesh() // chain the default constructor
{
    load_from_file(file_path);

}

Mesh Mesh::clone(){
    Mesh cloned;

    cloned.m_is_dirty=true;
    cloned.m_is_shadowmap_dirty=true;
    cloned.m_vis=m_vis;
    cloned.m_force_vis_update=m_force_vis_update;
    cloned.m_model_matrix=m_model_matrix;
    cloned.m_cur_pose=m_cur_pose;
    cloned.V=V;
    cloned.F=F;
    cloned.C=C;
    cloned.E=E;
    cloned.D=D;
    cloned.NF=NF;
    cloned.NV=NV;
    cloned.UV=UV;
    cloned.V_tangent_u=V_tangent_u;
    cloned.V_length_v=V_length_v;
    cloned.S_pred=S_pred;
    cloned.L_pred=L_pred;
    cloned.L_gt=L_gt;
    cloned.I=I;
    cloned.m_seg_label_pred=m_seg_label_pred;
    cloned.m_seg_label_gt=m_seg_label_gt;
    // cloned.m_rgb_tex_cpu=m_rgb_tex_cpu.clone();
    cloned.m_diffuse_mat.mat=m_diffuse_mat.mat.clone();
    cloned.m_metalness_mat.mat=m_metalness_mat.mat.clone();
    cloned.m_roughness_mat.mat=m_roughness_mat.mat.clone();
    cloned.m_normals_mat.mat=m_normals_mat.mat.clone();
    if(!m_diffuse_mat.mat.empty()) cloned.m_diffuse_mat.is_dirty=true;
    if(!m_metalness_mat.mat.empty()) cloned.m_metalness_mat.is_dirty=true;
    if(!m_roughness_mat.mat.empty()) cloned.m_roughness_mat.is_dirty=true;
    if(!m_normals_mat.mat.empty()) cloned.m_normals_mat.is_dirty=true;
    cloned.m_label_mngr=m_label_mngr; //this is just a shallow copy!
    cloned.t=t;
    cloned.id=id;
    cloned.m_height=m_height;
    cloned.m_width=m_width;
    cloned.m_view_direction=m_view_direction;
    cloned.m_min_max_y=m_min_max_y;
    cloned.m_min_max_y_for_plotting=m_min_max_y_for_plotting;

    cloned.name=name+"_clone";
    cloned.m_disk_path=m_disk_path;

    return cloned;
}


void Mesh::add(Mesh& new_mesh) {

    new_mesh.apply_model_matrix_to_cpu(true);

    if(V_blob.is_preallocated() ){ //if the mesh is preallocated, then we try to just copy to the matrices in the first empty space that has enough contiguous memory
        // V.block<p,q>(i,j)
        // radu::utils::eigen_copy_in_preallocated(V, new_mesh.V);

        //V
        // Eigen::MatrixXd V_new(V.rows() + new_mesh.V.rows(), 3);
        // V_new << V, new_mesh.V;
        V_blob.copy_in_first_empty_block(new_mesh.V);


        Eigen::MatrixXi F_new(F.rows() + new_mesh.F.rows(), 3);
        F_new << F, (new_mesh.F.array() + V.rows());
        Eigen::MatrixXd C_new(C.rows() + new_mesh.C.rows(), 3);
        C_new << C, new_mesh.C;
        Eigen::MatrixXi E_new(E.rows() + new_mesh.E.rows(), 2);
        E_new << E, (new_mesh.E.array() + V.rows());
        Eigen::MatrixXd D_new(D.rows() + new_mesh.D.rows(), 1);
        D_new << D, new_mesh.D;
        Eigen::MatrixXd NF_new(NF.rows() + new_mesh.NF.rows(), 3);
        NF_new << NF, new_mesh.NF;
        Eigen::MatrixXd NV_new(NV.rows() + new_mesh.NV.rows(), 3);
        NV_new << NV, new_mesh.NV;
        Eigen::MatrixXd UV_new(UV.rows() + new_mesh.UV.rows(), 2);
        UV_new << UV, new_mesh.UV;
        Eigen::MatrixXd V_tangent_u_new(V_tangent_u.rows() + new_mesh.V_tangent_u.rows(), 4);
        V_tangent_u_new << V_tangent_u, new_mesh.V_tangent_u;
        Eigen::MatrixXd V_lenght_v_new(V_length_v.rows() + new_mesh.V_length_v.rows(), 4);
        V_lenght_v_new << V_length_v, new_mesh.V_length_v;
        Eigen::MatrixXd S_pred_new(S_pred.rows() + new_mesh.S_pred.rows(), std::max<int>(S_pred.rows(),new_mesh.S_pred.rows()));
        S_pred_new << S_pred, new_mesh.S_pred;
        Eigen::MatrixXi L_pred_new(L_pred.rows() + new_mesh.L_pred.rows(), 1);
        L_pred_new << L_pred, new_mesh.L_pred;
        Eigen::MatrixXi L_gt_new(L_gt.rows() + new_mesh.L_gt.rows(), 1);
        L_gt_new << L_gt, new_mesh.L_gt;
        Eigen::MatrixXd I_new(I.rows() + new_mesh.I.rows(), 1);
        I_new << I, new_mesh.I;


        // V = V_new;
        F = F_new;
        C = C_new;
        E = E_new;
        D = D_new;
        NF=NF_new;
        NV=NV_new;
        UV=UV_new;
        V_tangent_u=V_tangent_u_new;
        V_length_v=V_lenght_v_new;
        S_pred=S_pred_new;
        L_pred=L_pred_new;
        L_gt=L_gt_new;
        I=I_new;


        // m_is_dirty=true;



    }else{

        Eigen::MatrixXd V_new(V.rows() + new_mesh.V.rows(), 3);
        V_new << V, new_mesh.V;
        Eigen::MatrixXi F_new(F.rows() + new_mesh.F.rows(), 3);
        F_new << F, (new_mesh.F.array() + V.rows());
        Eigen::MatrixXd C_new(C.rows() + new_mesh.C.rows(), 3);
        C_new << C, new_mesh.C;
        Eigen::MatrixXi E_new(E.rows() + new_mesh.E.rows(), 2);
        E_new << E, (new_mesh.E.array() + V.rows());
        Eigen::MatrixXd D_new(D.rows() + new_mesh.D.rows(), 1);
        D_new << D, new_mesh.D;
        Eigen::MatrixXd NF_new(NF.rows() + new_mesh.NF.rows(), 3);
        NF_new << NF, new_mesh.NF;
        Eigen::MatrixXd NV_new(NV.rows() + new_mesh.NV.rows(), 3);
        NV_new << NV, new_mesh.NV;
        Eigen::MatrixXd UV_new(UV.rows() + new_mesh.UV.rows(), 2);
        UV_new << UV, new_mesh.UV;
        Eigen::MatrixXd V_tangent_u_new(V_tangent_u.rows() + new_mesh.V_tangent_u.rows(), 3);
        V_tangent_u_new << V_tangent_u, new_mesh.V_tangent_u;
        Eigen::MatrixXd V_lenght_v_new(V_length_v.rows() + new_mesh.V_length_v.rows(), 1);
        V_lenght_v_new << V_length_v, new_mesh.V_length_v;
        Eigen::MatrixXd S_pred_new(S_pred.rows() + new_mesh.S_pred.rows(), std::max<int>(S_pred.rows(),new_mesh.S_pred.rows()));
        S_pred_new << S_pred, new_mesh.S_pred;
        Eigen::MatrixXi L_pred_new(L_pred.rows() + new_mesh.L_pred.rows(), 1);
        L_pred_new << L_pred, new_mesh.L_pred;
        Eigen::MatrixXi L_gt_new(L_gt.rows() + new_mesh.L_gt.rows(), 1);
        L_gt_new << L_gt, new_mesh.L_gt;
        Eigen::MatrixXd I_new(I.rows() + new_mesh.I.rows(), 1);
        I_new << I, new_mesh.I;


        V = V_new;
        F = F_new;
        C = C_new;
        E = E_new;
        D = D_new;
        NF=NF_new;
        NV=NV_new;
        UV=UV_new;
        V_tangent_u=V_tangent_u_new;
        V_length_v=V_lenght_v_new;
        S_pred=S_pred_new;
        L_pred=L_pred_new;
        L_gt=L_gt_new;
        I=I_new;


        m_is_dirty=true;
    }

    m_is_shadowmap_dirty=true;
}

void Mesh::add(const std::vector<std::shared_ptr<Mesh>>&  meshes){

    //get the meshes to cpu
    for(size_t i=0; i<meshes.size(); i++){
        meshes[i]->apply_model_matrix_to_cpu(true);
    }

    //get a cumulative nr of vertices that we would get after adding every mesh and the total number of the other elements
    // std::vector<int> cumulative_nr_verts;
    // cumulative_nr_verts.push_back(0);
    int nr_V_acumulated=V.rows();
    int nr_F_acumulated=F.rows();
    int nr_C_acumulated=C.rows();
    int nr_E_acumulated=E.rows();
    int nr_D_acumulated=D.rows();
    int nr_NF_acumulated=NF.rows();
    int nr_NV_acumulated=NV.rows();
    int nr_UV_acumulated=UV.rows();
    int nr_V_tangent_u_acumulated=V_tangent_u.rows();
    int nr_V_length_v_acumulated=V_length_v.rows();
    int nr_L_pred_acumulated=L_pred.rows();
    int nr_L_gt_acumulated=L_gt.rows();
    int nr_I_acumulated=I.rows();
    for(size_t i=0; i<meshes.size(); i++){
        nr_V_acumulated+=meshes[i]->V.rows();
        // cumulative_nr_verts.push_back(nr_V_acumulated);
        nr_F_acumulated+=meshes[i]->F.rows();
        nr_C_acumulated+=meshes[i]->C.rows();
        nr_E_acumulated+=meshes[i]->E.rows();
        nr_D_acumulated+=meshes[i]->D.rows();
        nr_NF_acumulated+=meshes[i]->NF.rows();
        nr_NV_acumulated+=meshes[i]->NV.rows();
        nr_UV_acumulated+=meshes[i]->UV.rows();
        nr_V_tangent_u_acumulated+=meshes[i]->V_tangent_u.rows();
        nr_V_length_v_acumulated+=meshes[i]->V_length_v.rows();
        nr_L_pred_acumulated+=meshes[i]->L_pred.rows();
        nr_L_gt_acumulated+=meshes[i]->L_gt.rows();
        nr_I_acumulated+=meshes[i]->I.rows();
    }

    //make the new ones
    Eigen::MatrixXd V_new(nr_V_acumulated, 3);
    Eigen::MatrixXi F_new(nr_F_acumulated, 3);
    Eigen::MatrixXd C_new(nr_C_acumulated, 3);
    Eigen::MatrixXi E_new(nr_E_acumulated, 2);
    Eigen::MatrixXd D_new(nr_D_acumulated, 1);
    Eigen::MatrixXd NF_new(nr_NF_acumulated, 3);
    Eigen::MatrixXd NV_new(nr_NV_acumulated, 3);
    Eigen::MatrixXd UV_new(nr_UV_acumulated, 2);
    Eigen::MatrixXd V_tangent_u_new(nr_V_tangent_u_acumulated, 3);
    Eigen::MatrixXd V_lenght_v_new(nr_V_length_v_acumulated, 1);
    Eigen::MatrixXi L_pred_new(nr_L_pred_acumulated, 1);
    Eigen::MatrixXi L_gt_new(nr_L_gt_acumulated, 1);
    Eigen::MatrixXd I_new(nr_I_acumulated, 1);




    int nr_F_added=0;
    int nr_E_added=0;
    int nr_V_added=0;
    int nr_C_added=0;
    int nr_D_added=0;
    int nr_NV_added=0;
    int nr_UV_added=0;
    int nr_V_tangent_u_added=0;
    int nr_V_length_v_added=0;
    int nr_L_pred_added=0;
    int nr_L_gt_added=0;
    int nr_I_added=0;


    //put the current matrices into the new ones
    //F
    int nr_additional_F= F.rows();
    F_new.block(0,0, nr_additional_F,3) = F.array()+nr_V_added;
    nr_F_added+=nr_additional_F;
    //E
    int nr_additional_E= E.rows();
    E_new.block(0,0, nr_additional_E,2) = E.array()+nr_V_added;
    nr_E_added+=nr_additional_E;
    //NF
    // TODO
    //V
    int nr_additional_V= V.rows();
    V_new.block(0,0,nr_additional_V,3) = V;
    nr_V_added+=nr_additional_V;
    //C
    int nr_additional_C= C.rows();
    C_new.block(0,0, nr_additional_C,3) = C;
    nr_C_added+=nr_additional_C;
    //D
    int nr_additional_D= D.rows();
    D_new.block(0,0, nr_additional_D,1) = D;
    nr_D_added+=nr_additional_D;
    //NV
    int nr_additional_NV= NV.rows();
    NV_new.block(0,0, nr_additional_NV,3) = NV;
    nr_NV_added+=nr_additional_NV;
    //UV
    int nr_additional_UV= UV.rows();
    UV_new.block(0,0, nr_additional_UV,2) = UV;
    nr_UV_added+=nr_additional_UV;
    //V_tangent_u
    int nr_additional_V_tangent_u= V_tangent_u.rows();
    V_tangent_u_new.block(0,0, nr_additional_V_tangent_u,3) = V_tangent_u;
    nr_V_tangent_u_added+=nr_additional_V_tangent_u;
    //V_lenght_v_
    int nr_additional_V_lenght_v= V_length_v.rows();
    V_lenght_v_new.block(0,0, nr_additional_V_lenght_v,1) = V_length_v;
    nr_V_length_v_added+=nr_additional_V_lenght_v;
    //L_pred
    int nr_additional_L_pred= L_pred.rows();
    L_pred_new.block(0,0, nr_additional_L_pred,1) = L_pred;
    nr_L_pred_added+=nr_additional_L_pred;
    //L_gt
    int nr_additional_L_gt= L_gt.rows();
    L_gt_new.block(0,0, nr_additional_L_gt,1) = L_gt;
    nr_L_gt_added+=nr_additional_L_gt;
    //I
    int nr_additional_I= I.rows();
    I_new.block(0,0, nr_additional_I,1) = I;
    nr_I_added+=nr_additional_I;





    //write into the new matrices
    for(size_t i=0; i<meshes.size(); i++){

        //F
        int nr_additional_F= meshes[i]->F.rows();
        int F_block_row= nr_F_added;
        // F_new.block<nr_additional_F,3>(F_block_row,0) = meshes[i]->F.array()+nr_V_added;
        F_new.block(F_block_row,0, nr_additional_F,3) = meshes[i]->F.array()+nr_V_added;
        nr_F_added+=nr_additional_F;

        //E
        int nr_additional_E= meshes[i]->E.rows();
        int E_block_row= nr_E_added;
        E_new.block(E_block_row,0, nr_additional_E,2) = meshes[i]->E.array()+nr_V_added;
        nr_E_added+=nr_additional_E;

        //NF
        // TODO


        //all the per-vertex things
        //V
        int nr_additional_V= meshes[i]->V.rows();
        int V_block_row= nr_V_added;
        V_new.block(V_block_row,0,nr_additional_V,3) = meshes[i]->V;
        nr_V_added+=nr_additional_V;
        //C
        int nr_additional_C= meshes[i]->C.rows();
        int C_block_row= nr_C_added;
        C_new.block(C_block_row,0, nr_additional_C,3) = meshes[i]->C;
        nr_C_added+=nr_additional_C;
        //D
        int nr_additional_D= meshes[i]->D.rows();
        int D_block_row= nr_D_added;
        D_new.block(D_block_row,0, nr_additional_D,1) = meshes[i]->D;
        nr_D_added+=nr_additional_D;
        //NV
        int nr_additional_NV= meshes[i]->NV.rows();
        int NV_block_row= nr_NV_added;
        NV_new.block(NV_block_row,0, nr_additional_NV,3) = meshes[i]->NV;
        nr_NV_added+=nr_additional_NV;
        //UV
        int nr_additional_UV= meshes[i]->UV.rows();
        int UV_block_row= nr_UV_added;
        UV_new.block(UV_block_row,0, nr_additional_UV,2) = meshes[i]->UV;
        nr_UV_added+=nr_additional_UV;
        //V_tangent_u
        int nr_additional_V_tangent_u= meshes[i]->V_tangent_u.rows();
        int V_tangent_u_block_row= nr_V_tangent_u_added;
        V_tangent_u_new.block(V_tangent_u_block_row,0, nr_additional_V_tangent_u,3) = meshes[i]->V_tangent_u;
        nr_V_tangent_u_added+=nr_additional_V_tangent_u;
        //V_lenght_v_
        int nr_additional_V_lenght_v= meshes[i]->V_length_v.rows();
        int V_lenght_v_block_row= nr_V_length_v_added;
        V_lenght_v_new.block(V_lenght_v_block_row,0, nr_additional_V_lenght_v,1) = meshes[i]->V_length_v;
        nr_V_length_v_added+=nr_additional_V_lenght_v;
        //L_pred
        int nr_additional_L_pred= meshes[i]->L_pred.rows();
        int L_pred_block_row= nr_L_pred_added;
        L_pred_new.block(L_pred_block_row,0, nr_additional_L_pred,1) = meshes[i]->L_pred;
        nr_L_pred_added+=nr_additional_L_pred;
        //L_gt
        int nr_additional_L_gt= meshes[i]->L_gt.rows();
        int L_gt_block_row= nr_L_gt_added;
        L_gt_new.block(L_gt_block_row,0, nr_additional_L_gt,1) = meshes[i]->L_gt;
        nr_L_gt_added+=nr_additional_L_gt;
        //I
        int nr_additional_I= meshes[i]->I.rows();
        int I_block_row= nr_I_added;
        I_new.block(I_block_row,0, nr_additional_I,1) = meshes[i]->I;
        nr_I_added+=nr_additional_I;




    }

    V = V_new;
    F = F_new;
    C = C_new;
    E = E_new;
    D = D_new;
    NF=NF_new;
    NV=NV_new;
    UV=UV_new;
    V_tangent_u=V_tangent_u_new;
    V_length_v=V_lenght_v_new;
    L_pred=L_pred_new;
    L_gt=L_gt_new;
    I=I_new;


    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}

// void MeshCore::assign(const MeshCore& new_core){

//     // static_cast<MeshCore&>(*this) = new_core;

//     m_model_matrix=new_core.m_model_matrix;

//     m_is_dirty = new_core.m_is_dirty;
//     m_is_visible = new_core.m_is_visible;
//     m_show_points = new_core.m_show_points;
//     m_show_lines = new_core.m_show_lines;
//     m_show_mesh = new_core.m_show_mesh;
//     m_show_wireframe = new_core.m_show_wireframe;
//     m_show_surfels = new_core.m_show_surfels;

//     m_point_size = new_core.m_point_size;
//     m_line_width =new_core.m_line_width;
//     m_color_type = new_core.m_color_type;
//     m_point_color = new_core.m_point_color;
//     m_line_color = new_core.m_line_color;
//     m_solid_color = new_core.m_solid_color;
//     m_specular_color = new_core.m_specular_color;
//     m_shininess = new_core.m_shininess;

//     V = new_core.V;
//     F = new_core.F;
//     C = new_core.C;
//     E = new_core.E;
//     D = new_core.D;
//     NF=new_core.NF;
//     NV=new_core.NV;
//     UV=new_core.UV;
//     V_tangent_u=new_core.V_tangent_u;
//     V_length_v=new_core.V_length_v;

//     t = new_core.t;
//     m_height = new_core.m_height;
//     m_width = new_core.m_width;
//     name = new_core.name;

//     m_is_dirty=true;
// }

void Mesh::clear() {
    V.resize(0,0);
    F.resize(0,0);
    C.resize(0,0);
    E.resize(0,0);
    D.resize(0,0);
    NF.resize(0,0);
    NV.resize(0,0);
    UV.resize(0,0);
    V_tangent_u.resize(0,0);
    V_length_v.resize(0,0);
    S_pred.resize(0,0);
    L_pred.resize(0,0);
    L_gt.resize(0,0);
    I.resize(0,0);

    m_min_max_y.setZero();
    m_min_max_y_for_plotting.setZero();

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;
}


void Mesh::clear_C() {
    Eigen::MatrixXd C_empty;
    C = C_empty;
    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}

void Mesh::set_all_matrices_to_zero(){
    V.setZero();
    F.setZero();
    C.setZero();
    E.setZero();
    D.setZero();
    NF.setZero();
    NV.setZero();
    UV.setZero();
    V_tangent_u.setZero();
    V_length_v.setZero();
    S_pred.setZero();
    L_pred.setZero();
    L_gt.setZero();
    I.setZero();
}

void Mesh::preallocate_V(size_t max_nr_verts){
    // V.resize(max_nr_verts,3);
    // V.setZero();
    // m_is_preallocated=true;
    V_blob.preallocate(max_nr_verts,3);
}



bool Mesh::is_empty() const {

    if (V.size() == 0) {
        return true;
    } else {
        return false;
    }


}

void Mesh::assign_mesh_gpu(std::shared_ptr<MeshGL> mesh_gpu){
    m_mesh_gpu=mesh_gpu;
}

void Mesh::transform_vertices_cpu(const Eigen::Affine3d& trans, const bool transform_points_at_zero){

    m_cur_pose=trans*m_cur_pose;

    for (int i = 0; i < V.rows(); i++) {
        if(!V.row(i).isZero() || transform_points_at_zero){
            V.row(i)=trans.linear()*V.row(i).transpose() + trans.translation();
        }
    }
    if (NF.size())  NF.transpose() = (trans.linear() * NF.transpose());
    if (NV.size())  NV.transpose() = (trans.linear() * NV.transpose());

    //we have to also rotat the tangent vector
    if (V_tangent_u.size())  V_tangent_u.transpose() = (trans.linear() * V_tangent_u.transpose());

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;
}




Eigen::Affine3d Mesh::cur_pose(){
    return m_cur_pose;
}
Eigen::Affine3d& Mesh::cur_pose_ref(){
    m_is_shadowmap_dirty=true; //we get a reference to this model matrix and therefore it can be modified from outside. So just to be sure we just set the shadowmap to be dirty
    return m_cur_pose;
}
void Mesh::set_cur_pose(const Eigen::Affine3d& new_cur_pose){
    m_cur_pose=new_cur_pose;
    m_is_shadowmap_dirty=true;
}
Eigen::Affine3d Mesh::model_matrix(){
    return m_model_matrix;
}
Eigen::Affine3d& Mesh::model_matrix_ref(){
    m_is_shadowmap_dirty=true; //we get a reference to this model matrix and therefore it can be modified from outside. So just to be sure we just set the shadowmap to be dirty
    return m_model_matrix;
}
void Mesh::set_model_matrix(const Eigen::Affine3d& new_model_matrix){


    //get first the delta between the current model matrix and the new one
    // the delta will be the matrix that multiplied with the current model matrix will yield the new one  new_model_matrix=delta*m_model_matrix
    // Eigen::Affine3d delta=new_model_matrix*m_model_matrix.inverse();
    // //check
    // VLOG(1) << "new model matrix is " << new_model_matrix.matrix();
    // Eigen::Affine3d calculated_new= delta * m_model_matrix;
    // VLOG(1) << "calculated_new is " << calculated_new.matrix();
    // VLOG(1) << "delta is " << delta.matrix();

    // transform_model_matrix(delta);

    m_model_matrix=new_model_matrix;
    m_is_shadowmap_dirty=true;
}

void Mesh::set_model_matrix_from_string(const std::string& pose_string){
    std::vector<std::string> tokens;
    tokens=radu::utils::split(pose_string, " ");
    CHECK(tokens.size()==7) << "Expected to be able to split the pose_string into 7. But got only " << tokens.size();

    Eigen::Affine3d model_matrix=tf_vecstring2matrix<double>(tokens);

    set_model_matrix(model_matrix);
}


void Mesh::transform_model_matrix(const Eigen::Affine3d& trans){
    m_model_matrix=trans*m_model_matrix;

    // update all children
    for (size_t i = 0; i < m_child_meshes.size(); i++){
        MeshSharedPtr child=m_child_meshes[i];
        child->transform_model_matrix(trans);
    }

    m_is_shadowmap_dirty=true;
}

void Mesh::translate_model_matrix(const Eigen::Vector3d& translation){
    Eigen::Affine3d tf;
    tf.setIdentity();

    tf.translation()=translation;
    transform_model_matrix(tf);

    m_is_shadowmap_dirty=true;
}

void Mesh::rotate_model_matrix(const Eigen::Vector3d& axis, const float angle_degrees){
    Eigen::Quaterniond q = Eigen::Quaterniond( Eigen::AngleAxis<double>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );

    Eigen::Affine3d tf;
    tf.setIdentity();

    tf.linear()=q.toRotationMatrix();
    transform_model_matrix(tf);

    m_is_shadowmap_dirty=true;
}

void Mesh::rotate_model_matrix_local(const Eigen::Vector3d& axis, const float angle_degrees){
    Eigen::Quaterniond q = Eigen::Quaterniond( Eigen::AngleAxis<double>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );

    Eigen::Affine3d rot;
    rot.setIdentity();

    rot.linear()=q.toRotationMatrix();

    Eigen::Affine3d tf=Eigen::Translation3d(m_model_matrix.translation()) * rot *  Eigen::Translation3d(-m_model_matrix.translation());

    transform_model_matrix(tf);

    m_is_shadowmap_dirty=true;
}

void Mesh::rotate_model_matrix_local(const Eigen::Quaterniond& q){
    Eigen::Affine3d rot;
    rot.setIdentity();

    rot.linear()=q.toRotationMatrix();

    Eigen::Affine3d tf=Eigen::Translation3d(m_model_matrix.translation()) * rot *  Eigen::Translation3d(-m_model_matrix.translation());

    transform_model_matrix(tf);

    m_is_shadowmap_dirty=true;
}

void Mesh::apply_model_matrix_to_cpu(const bool transform_points_at_zero){
    transform_vertices_cpu(m_model_matrix, transform_points_at_zero);
    m_model_matrix.setIdentity();

    m_is_shadowmap_dirty=true;
}

void Mesh::scale_mesh(const float scale){
    V=V*scale;
}

// void Mesh::set_model_matrix(const Eigen::VectorXd& xyz_q){
//     m_model_matrix.translation().x() = xyz_q[0];
//     m_model_matrix.translation().y() = xyz_q[1];
//     m_model_matrix.translation().z() = xyz_q[2];
//     Eigen::Quaterniond q;
//     q.x()=xyz_q[3];
//     q.y()=xyz_q[4];
//     q.z()=xyz_q[5];
//     q.w()=xyz_q[6];
//     m_model_matrix.linear() = q.toRotationMatrix();
// }

// Eigen::VectorXd Mesh::model_matrix_as_xyz_and_quaternion(){

//     Eigen::VectorXd out(7);

//     out[0] = m_model_matrix.translation().x() ;
//     out[1] = m_model_matrix.translation().y() ;
//     out[2] = m_model_matrix.translation().z() ;

//     Eigen::Quaterniond q ( m_model_matrix.linear() );
//     out[3]=q.x();
//     out[4]=q.y();
//     out[5]=q.z();
//     out[6]=q.w();

//     return out;
// }

// Eigen::VectorXd Mesh::model_matrix_as_xyz_and_rpy(){

//     Eigen::VectorXd out(6);

//     out[0] = m_model_matrix.translation().x() ;
//     out[1] = m_model_matrix.translation().y() ;
//     out[2] = m_model_matrix.translation().z() ;

//     Eigen::Quaterniond q ( m_model_matrix.linear() );
//     // out[3]=q.x();
//     // out[4]=q.y();
//     // out[5]=q.z();
//     // out[6]=q.w();

//     auto euler = q.toRotationMatrix().eulerAngles(0, 1, 2);

//     out[3]=euler[0];
//     out[4]=euler[1];
//     out[5]=euler[2];

//     // //http://ros-developer.com/2017/11/18/finding-roll-pitch-yaw-3x3-rotation-matrix-eigen/
//     // Eigen::Matrix3d rotationMatrix = q.toRotationMatrix();
//     // float roll = M_PI/atan2( rotationMatrix(2,1),rotationMatrix(2,2) );
//     // float pitch = M_PI/atan2( -rotationMatrix(2,0), std::pow( rotationMatrix(2,1)*rotationMatrix(2,1) +rotationMatrix(2,2)*rotationMatrix(2,2) ,0.5  )  );
//     // float yaw = M_PI/atan2( rotationMatrix(1,0),rotationMatrix(0,0) );
//     // out[3]=M_PI/roll;
//     // out[4]=M_PI/pitch;
//     // out[5]=M_PI/yaw;

//     // tf2::Quaternion q( -0.019, 0.037, 0.127, 0.991);
//     // tf2::Matrix3x3 m(q);
//     // double r, p, y;
//     // m.getRPY(r, p, y, 2);


//     return out;
// }

// void Mesh::premultiply_model_matrix(const Eigen::VectorXd& xyz_q){

//     Eigen::Affine3d matrix;
//     matrix.translation().x() = xyz_q[0];
//     matrix.translation().y() = xyz_q[1];
//     matrix.translation().z() = xyz_q[2];
//     Eigen::Quaterniond q;
//     q.x()=xyz_q[3];
//     q.y()=xyz_q[4];
//     q.z()=xyz_q[5];
//     q.w()=xyz_q[6];
//     matrix.linear() = q.toRotationMatrix();

//     m_model_matrix=matrix*m_model_matrix;
// }

// void Mesh::postmultiply_model_matrix(const Eigen::VectorXd& xyz_q){

//     Eigen::Affine3d matrix;
//     matrix.translation().x() = xyz_q[0];
//     matrix.translation().y() = xyz_q[1];
//     matrix.translation().z() = xyz_q[2];
//     Eigen::Quaterniond q;
//     q.x()=xyz_q[3];
//     q.y()=xyz_q[4];
//     q.z()=xyz_q[5];
//     q.w()=xyz_q[6];
//     matrix.linear() = q.toRotationMatrix();

//     m_model_matrix=m_model_matrix*matrix;
// }



void Mesh::align_to_cloud(const std::shared_ptr<Mesh>& cloud_target){
    // Eigen::Affine3d T;
    // T.setIdentity();
    // VLOG(1) << "T initial is " << T.matrix();
    // this->apply_model_matrix_to_cpu(true);
    // VLOG(1) << "Alignign a cloud of " << this->V.rows() << " to a cloud of  " << cloud_target->V.rows();


    // //if the current mesh doesn't have faces, we compute some dummy faces
    // Eigen::MatrixXi F_this;
    // if(!this->F.size()){
    //     F_this.resize(V.rows().3);
    //     for(int i=0; i<this->V.rows(); i++){
    //         F_this.row(i) << i,i,i;
    //     }
    // }else{
    //     F_this=this->F;
    // }


    // Eigen::MatrixXd R;
    // Eigen::MatrixXd t;

    // iterative_closest_point(this-V, F_this, mesh_target->V, mesh_target->F, this->V.rows(), 10, R,t);

    // //move
    // this->V = (this->V * R).rowwise() + t.transpose();


    // // igl::procrustes(this->V, cloud_target->V, true,false,T);
    // // this->transform_model_matrix(T.inverse());
    // // this->V = (this->V * T.linear()).rowwise() + T.translation().transpose();
    // this->m_is_dirty=true;
    // VLOG(1) << "T is " << T.matrix();






    //attempt 2
    // igl::AABB<Eigen::Matrix<double, -1, -1, 0, -1, -1>,3> Ytree;
    // Ytree.init(mesh_target->V, mesh_target->F);
    Eigen::MatrixXd R;
    Eigen::VectorXd t;
    R.setIdentity(3,3);
    t.resize(3);
    t.setZero();
    Eigen::VectorXd max;
    Eigen::VectorXd min;
    int num_samples=std::min(10000,(int)this->V.rows());
    int max_iters=1;
    
    for(int iter = 0;iter<max_iters;iter++){

        // max= this->V.colwise().maxCoeff();
        // min= this->V.colwise().minCoeff();
        // VLOG(1) << "this->V size is " << this->V.rows() << " " << this->V.cols();
        // VLOG(1) << "this->V min is " << min.transpose() << " max " << max.transpose();


        //sample some point from this cloud
        Eigen::MatrixXd X;
        X.resize(num_samples,3);
        // X.setZero();
        // VLOG(1) << "R is size " << R.rows() << " "<< R.cols();
        // VLOG(1) << "t is size " << t.rows() << " "<< t.cols();
        // Eigen::MatrixXd VXRT = (R*this->V).rowwise()+t.transpose();
        Eigen::MatrixXd VXRT;
        VXRT.resize(this->V.rows(),3);
        for (int i = 0; i < V.rows(); i++) {
            VXRT.row(i)=R*V.row(i).transpose() + t.transpose();
        }
        // VLOG(1) << "VXRT is size " << VXRT.rows() << " " << VXRT.cols();
        // max= VXRT.colwise().maxCoeff();
        // min= VXRT.colwise().minCoeff();
        // VLOG(1) << "VXRT min is " << min.transpose() << " max " << max.transpose();
        for(int i=0; i<num_samples; i++){
            X.row(i) = VXRT.row(m_rand_gen->rand_int(0,VXRT.rows()-1));
        }
        // max= X.colwise().maxCoeff();
        // min= X.colwise().minCoeff();
        // VLOG(1) << "X min is " << min.transpose() << " max " << max.transpose();
        // VLOG(1) << "computed X " << X.rows();


        //get the closest point from this V to cloud target
        //since the cloud target might not have faces, and even if it has we don't care, we create degenerate faces for each point
        Eigen::MatrixXi F_target;
        F_target.resize(cloud_target->V.rows(), 3);
        for(int i=0; i<cloud_target->V.rows(); i++){
            F_target.row(i) << i,i,i;
        }
        // VLOG(1) << "made degenerate F_target";
        //get distances
        Eigen::VectorXd sqrD;
        Eigen::VectorXi I;
        Eigen::MatrixXd closest_points_target;
        igl::point_mesh_squared_distance(X, cloud_target->V, F_target, sqrD, I, closest_points_target);
        // VLOG(1) << "sqrd min max is " << sqrD.minCoeff() << " " << sqrD.maxCoeff();
        // for(int i = 0;i<10;i++){
            // VLOG(1) << "point X at " << i << " is " << X.row(i) << " with closest " << closest_points_target.row(i);
        // }


        //if the squared distance is higher than 10% of the max, we just set the closest point P to be just the point X
        // float thresh=sqrD.maxCoeff()*0.1;
        // for(int i=0; i<num_samples; i++){
        //     if(sqrD[i]>thresh){ //distance is too high
        //         closest_points_target.row(i) = X.row(i);
        //     }
        // }

        //compute best transform between X and P
        // http://nghiaho.com/?page_id=671
        // https://github.com/zjudmd1015/icp/blob/master/icp.cpp
        //Get the centroid of A(this) and B(target)
        Eigen::Vector3d centroid_A;
        Eigen::Vector3d centroid_B;
        centroid_A.setZero();
        centroid_B.setZero();
        int nr_points=X.rows();
        for(int i=0; i<nr_points; i++){
            centroid_A += X.row(i).transpose();
            centroid_B += closest_points_target.row(i).transpose();
        }
        centroid_A /= nr_points;
        centroid_B /= nr_points;
        //Center A and B and compute H(covariance matrix)
        Eigen::MatrixXd X_centered=X.rowwise()-centroid_A.transpose();
        Eigen::MatrixXd target_centered=closest_points_target.rowwise()-centroid_B.transpose();
        Eigen::MatrixXd H = X_centered.transpose()*target_centered;
        // VLOG(1) << "computed H of size " << H.rows() << " " << H.cols();
        // VLOG(1) << "computed H of min max " << H.minCoeff() << " " << H.maxCoeff();

        //compute rotation
        Eigen::MatrixXd U;
        Eigen::VectorXd S;
        Eigen::MatrixXd V;
        Eigen::MatrixXd Vt;
        Eigen::Matrix3d R_increment;
        Eigen::Vector3d t_increment;
        Eigen::JacobiSVD<Eigen::MatrixXd> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
        U = svd.matrixU();
        S = svd.singularValues();
        V = svd.matrixV();
        Vt = V.transpose();
        R_increment = Vt.transpose()*U.transpose();
        if (R_increment.determinant() < 0 ){
            // VLOG(1) << "determinnant is negative";
            Vt.block<1,3>(2,0) *= -1;
            R_increment = Vt.transpose()*U.transpose();
        }

        //compute translation
        t_increment = centroid_B - R_increment*centroid_A;


        //add it to our global R and t
        R = (R_increment*R).eval();
        t = (R_increment*t + t_increment).eval().transpose().eval();
        // VLOG(1) << "computed R_increment " << R_increment; 
        // VLOG(1) << "computed t_increment " << t_increment; 


    }


    //apply 
    Eigen::Affine3d T;
    T.setIdentity();
    T.linear()=R;
    T.translation()=t;
    // VLOG(1) << "T is " << T.matrix();
    this->transform_model_matrix(T);
    // this->V=(R*this->V).rowwise() + t.transpose();
    this->apply_model_matrix_to_cpu(true);



    this->m_is_dirty=true;
}
    






void Mesh::recalculate_normals(){
    if(!F.size()){
        return; // we have no faces so there will be no normals
    }
    CHECK(V.size()) << named("V is empty");
    igl::per_face_normals(V,F,NF);
    igl::per_vertex_normals(V,F, igl::PerVertexNormalsWeightingType::PER_VERTEX_NORMALS_WEIGHTING_TYPE_ANGLE, NF, NV);
    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}

bool Mesh::load_from_file(const std::string file_path){

    std::string filepath_trim= radu::utils::trim_copy(file_path);
    std::string file_path_abs;
    if (fs::path( filepath_trim ).is_relative()){
        file_path_abs=(fs::path(PROJECT_SOURCE_DIR) / filepath_trim).string();
    }else{
        file_path_abs=filepath_trim;
    }

    std::string file_ext = file_path_abs.substr(file_path_abs.find_last_of(".") + 1);
    trim(file_ext); //remove whitespaces from beggining and end
    if (file_ext == "off" || file_ext == "OFF") {
        igl::readOFF(file_path_abs, V, F);
    } else if (file_ext == "ply" || file_ext == "PLY") {
        read_ply(file_path_abs);
    } else if (file_ext == "obj" || file_ext == "OBJ") {
        read_obj(file_path_abs);
    } else if (file_ext == "stl" || file_ext == "STL") {
        igl::readSTL(file_path_abs, V, F, NV);
    }else if (file_ext == "pcd") {
        #ifdef EASYPBR_WITH_PCL
            //read the cloud as general binary blob and then parse it to a certain type of point cloud http://pointclouds.org/documentation/tutorials/reading_pcd.php
            pcl::PCLPointCloud2 cloud_blob;
            Eigen::Vector4f origin = Eigen::Vector4f::Zero();
            Eigen::Quaternionf orientation = Eigen::Quaternionf::Identity();
            pcl::PCDReader p;
            int pcd_version;
            p.read (file_path_abs, cloud_blob, origin, orientation, pcd_version);
            Eigen::Affine3f cloud_pose = Eigen::Affine3f::Identity();
            cloud_pose.linear() = orientation.toRotationMatrix();
            cloud_pose.translation() = origin.head<3>();
            //pcl::io::loadPCDFile (file_path_abs, cloud_blob);

            // VLOG(1) << " read pcl cloud with header: " << cloud_blob;

            bool has_rgb=false;
            bool has_intensity=false;
            for(size_t i=0; i<cloud_blob.fields.size(); i++){
                if(cloud_blob.fields[i].name=="rgb"){
                    has_rgb=true;
                }
                if(cloud_blob.fields[i].name=="intensity"){
                    has_intensity=true;
                }
            }

            //depending on the fields, read as xyz, as xyzrgb or as xyzi, xyzrgbi
            if(!has_rgb && !has_intensity){
                pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);
                pcl::fromPCLPointCloud2 (cloud_blob, *cloud); //* convert from pcl/PCLPointCloud2 to pcl::PointCloud<T>
                V.resize(cloud->points.size(), 3);
                for (size_t i = 0; i < cloud->points.size (); ++i){
                    //V.row(i) << cloud->points[i].x, cloud->points[i].y, cloud->points[i].z;
                    V.row(i) << (cloud_pose * cloud->points[i].getVector3fMap()).transpose().cast<double>();
                }

            }else if (has_rgb && !has_intensity){
                pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZRGB>);
                pcl::fromPCLPointCloud2 (cloud_blob, *cloud); //* convert from pcl/PCLPointCloud2 to pcl::PointCloud<T>
                V.resize(cloud->points.size(), 3);
                C.resize(cloud->points.size(), 3);
                for (size_t i = 0; i < cloud->points.size (); ++i){
                    //V.row(i) << cloud->points[i].x, cloud->points[i].y, cloud->points[i].z;
                    V.row(i) << (cloud_pose * cloud->points[i].getVector3fMap()).transpose().cast<double>();
                    C.row(i) << (float)cloud->points[i].r/255.0 , (float)cloud->points[i].g/255.0, (float)cloud->points[i].b/255.0;
                }

            }else if (has_rgb && has_intensity){
                LOG(FATAL) << "We do not support at the moment point cloud with both rgb and intensity. I would need to add a new point cloud type PointXYZRGBI for that.";

            }else if (!has_rgb && has_intensity){
                pcl::PointCloud<pcl::PointXYZI>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZI>);
                pcl::fromPCLPointCloud2 (cloud_blob, *cloud); //* convert from pcl/PCLPointCloud2 to pcl::PointCloud<T>
                V.resize(cloud->points.size(), 3);
                I.resize(cloud->points.size(), 1);
                for (size_t i = 0; i < cloud->points.size (); ++i){
                    //V.row(i) << cloud->points[i].x, cloud->points[i].y, cloud->points[i].z;
                    V.row(i) << (cloud_pose * cloud->points[i].getVector3fMap()).transpose().cast<double>();
                    I.row(i) << cloud->points[i].intensity;
                }
            }
            LOG(INFO) << "CloudPose=[" << cloud_pose.matrix()<<"]";

            //set the width and height from the pcd file
            m_width=cloud_blob.width;
            m_height=cloud_blob.height;
        #else
            LOG(FATAL) << "Not compiled with PCL so we cannot read pcd filed";
        #endif

    }else{
        LOG(WARNING) << "Not a known extension of mesh file: " << file_path_abs;
        return false;
    }

    //set some sensible things to see
    if(!F.size()){
        m_vis.m_show_points=true;
        m_vis.m_show_mesh=false;
    }
    if(C.size()){
        m_vis.set_color_pervertcolor();
    }

    //https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    //if we have texture coordinates and normal vectors, we will be able to load a normal map from file and therefore we will need the TBN matrix.
    //we precompute here the tangent vector and leave the bitangent in the vertex shader
    recalculate_normals();
    if(NV.size()&&UV.size()){
        compute_tangents(); //
    }

    recalculate_min_max_height();
    

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

    m_disk_path=file_path_abs;

    return true;

}

void Mesh::save_to_file(const std::string file_path){

    this->apply_model_matrix_to_cpu(true);

    //in the case of surfels, surfels that don't actually have an extent should be removed from the mesh, so we just set the coresponsing vertex to 0.0.0
    if(V_tangent_u.rows()==V.rows() && V_length_v.rows()==V.rows() && m_vis.m_show_surfels){
        for(int i = 0; i < V_tangent_u.rows(); i++){
            if(V_tangent_u.row(i).isZero() || V_length_v.row(i).isZero() ){
                V.row(i).setZero();
            }
        }
    }
    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

    remove_vertices_at_zero();


    std::string file_ext = file_path.substr(file_path.find_last_of(".") + 1);
    trim(file_ext); //remove whitespaces from beggining and end
    if(file_ext=="ply" || file_ext=="PLY"){
        // if(NV.size()){
        //     if(UV.size()){
        //         igl::writePLY(file_path, V, F, NV, UV);
        //     }else{
        //         Eigen::MatrixXd UV_empty;
        //         igl::writePLY(file_path, V, F, NV, UV_empty);
        //     }
        // }else{
        //     igl::writePLY(file_path, V, F);
        // }
        write_ply(file_path);
    }else if(file_ext == "obj" || file_ext == "OBJ"){
        igl::writeOBJ(file_path, V, F);
    }else{
        LOG(WARNING) << "Not known extension " << file_ext;
    }

}

void Mesh::add_child(std::shared_ptr<Mesh>& mesh){
    //check that the mesh we want to add has a name assigned
    if(mesh->name.empty()){
        LOG(FATAL) << "Trying to add mesh with no name. Assign a name to it first by adding it to the Scene with Scene.add()";
    }

    //check that we don't have more children with the same name
    for (size_t i = 0; i < m_child_meshes.size(); i++){
        if (m_child_meshes[i]->name==mesh->name){
            LOG(FATAL) << "Found a mesh with the same name " << mesh->name;
        }
    }


    m_child_meshes.push_back(mesh);
    // VLOG(1) << "children nr is now " <<m_child_meshes.size();

}

//removed the vertices that are marked and also reindexes the faces and indices to point to valid vertices
void Mesh::remove_marked_vertices(const std::vector<bool>& mask, const bool keep){

    std::vector<int> V_indir; //points from the original positions of V to where they ended up in the new V matrix
    V=filter_return_indirection(V_indir, V, mask, keep, /*do_checks*/ false);
    C=filter(C, mask, keep, /*do_checks*/ false);
    D=filter(D, mask, keep, /*do_checks*/ false);
    NV=filter(NV, mask, keep, /*do_checks*/ false);
    UV=filter(UV, mask, keep, /*do_checks*/ false);
    V_tangent_u=filter(V_tangent_u, mask, keep, /*do_checks*/ false);
    V_length_v=filter(V_length_v, mask, keep, /*do_checks*/ false);
    S_pred=filter(S_pred, mask, keep, /*do_checks*/ false);
    L_pred=filter(L_pred, mask, keep, /*do_checks*/ false);
    L_gt=filter(L_gt, mask, keep, /*do_checks*/ false);
    I=filter(I, mask, keep, /*do_checks*/ false);

    //deal with faces
    std::vector<bool> F_kept; //says for each face if it has been kept after their indices have been reordered to point to the new V matrix
    F=filter_apply_indirection_return_mask(F_kept, V_indir, F);
    NF=filter(NF, F_kept, true, /*do_checks*/ false); //removes the NF that are no longer corresponding to a face

    //deal with edges
    std::vector<bool> E_kept; //says for each edge if it has been kept after their indices have been reordered to point to the new V matrix
    E=filter_apply_indirection_return_mask(E_kept, V_indir, E);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}

void Mesh::set_marked_vertices_to_zero(const std::vector<bool>& mask, const bool keep){


    std::vector<int> V_indir; //points from the original positions of V to where they ended up in the new V matrix
    V=filter_set_zero_return_indirection(V_indir, V, mask, keep, /*do_checks*/ false);
    C=filter_set_zero(C, mask, keep, /*do_checks*/ false);
    D=filter_set_zero(D, mask, keep, /*do_checks*/ false);
    NV=filter_set_zero(NV, mask, keep, /*do_checks*/ false);
    UV=filter_set_zero(UV, mask, keep, /*do_checks*/ false);
    V_tangent_u=filter_set_zero(V_tangent_u, mask, keep, /*do_checks*/ false);
    V_length_v=filter_set_zero(V_length_v, mask, keep, /*do_checks*/ false);
    S_pred=filter_set_zero(S_pred, mask, keep, /*do_checks*/ false);
    L_pred=filter_set_zero(L_pred, mask, keep, /*do_checks*/ false);
    L_gt=filter_set_zero(L_gt, mask, keep, /*do_checks*/ false);
    I=filter_set_zero(I, mask, keep, /*do_checks*/ false);

    //deal with faces
    std::vector<bool> F_kept; //says for each face if it has been kept after their indices have been reordered to point to the new V matrix
    F=filter_apply_indirection_return_mask(F_kept, V_indir, F);
    NF=filter(NF, F_kept, true, /*do_checks*/ false); //removes the NF that are no longer corresponding to a face

    //deal with edges
    std::vector<bool> E_kept; //says for each edge if it has been kept after their indices have been reordered to point to the new V matrix
    E=filter_apply_indirection_return_mask(E_kept, V_indir, E);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;


}

void Mesh::remove_vertices_at_zero(){
    std::vector<bool> is_vertex_zero(V.rows(), false);
    for(int i = 0; i < V.rows(); i++){
        if(V.row(i).isZero()){
            is_vertex_zero[i]=true;
        }
    }

    remove_marked_vertices(is_vertex_zero, false);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}

void Mesh::remove_unreferenced_verts(){
    //remove unreferenced vertices to get rid also of the ones at 0,0,0
    std::vector<bool> is_vertex_referenced(V.rows(),false);
    for (int i = 0; i < F.rows(); ++i) {
        for (int j = 0; j < F.cols(); ++j) {
            int idx=F(i,j);
            is_vertex_referenced[idx]=true;
        }
    }

    remove_marked_vertices(is_vertex_referenced, true);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}

Eigen::VectorXi Mesh::remove_duplicate_vertices(){

    Eigen::MatrixXd V_new;
    Eigen::VectorXi indirection; //size of V_new , says for each point where the original one was in mesh.V
    Eigen::VectorXi inverse_indirection; //size of mesh.V, says for each point where they ended up in the V_new array

    igl::remove_duplicate_vertices(V, 1e-7, V_new, indirection, inverse_indirection);

    std::vector<int> inverse_indirection_vec= eigen2vec(inverse_indirection);

    // VLOG(1) << "V is " << V.rows();
    // VLOG(1) << "indirection has rows " << indirection.rows();
    // VLOG(1) << "inverse_indirection has rows " << inverse_indirection.rows();

    F=filter_apply_indirection(inverse_indirection_vec, F);
    E=filter_apply_indirection(inverse_indirection_vec, E);

    //now that we merge the vertices we need to also somehow merge the vertex atributes, so merge the colors for examaple. UV cannot be merged since you would need to choose one or the other uv  coordinate, if you choose the wrong one you end up stretching the face all over the uv map
    //We not efectivelly splat and acummulate the colors from the original mesh into the new mesh. So if V0 and v1 got merged together their colors would average on the new mesh
    std::vector<int> nr_times_merged(V_new.rows(), 0); //stores for each new vertex, how many vertices it merged into itself
    //get the nr of times merged
    for (int i=0; i<V.rows(); i++){
        int idx_new_mesh=inverse_indirection(i); //index saying for this old vertex, where it got merged into in the new mesh
        nr_times_merged[idx_new_mesh]++;
    }
    //calcualte the average color, Intensity and the other per-vertex things for the merged vertices
    //COLOR
    if (C.size()){
        Eigen::MatrixXd C_new;
        C_new.resize(V_new.rows(), 3);
        C_new.setZero();
        for (int i=0; i<V.rows(); i++){
            int idx_new_mesh=inverse_indirection(i); //index saying for this old vertex, where it got merged into in the new mesh
            Eigen::Vector3d original_color=C.row(i);
            C_new.row(idx_new_mesh)+=original_color;
        }
        //renormalize the colors
        for (int i=0; i<V_new.rows(); i++){
            C_new.row(i).array()/=nr_times_merged[i];
        }
        C=C_new;
    }

    V=V_new;

    m_is_dirty=true;

    return inverse_indirection;
}

void Mesh::undo_remove_duplicate_vertices(const std::shared_ptr<Mesh>& original_mesh, const Eigen::VectorXi& inverse_indirection ){

    CHECK(inverse_indirection.size()==original_mesh->V.rows()) << "The inverse_indirection has to have the same size as the original mesh vertices. Indirection is " << inverse_indirection.size() << " original mesh V is " << original_mesh->V.rows();

    //check the position that the vertices now have in the merged mesh and copy them
    Eigen::MatrixXd V_undone=original_mesh->V;
    for (int i=0; i<V_undone.rows(); i++){
        int idx_merged_mesh=inverse_indirection(i);
        Eigen::Vector3d merged_point=V.row(idx_merged_mesh);
        V_undone.row(i)=merged_point;
    }
    V=V_undone;


    //original indices
    if(original_mesh->F.size()) F=original_mesh->F;
    if(original_mesh->E.size()) E=original_mesh->E;
    //copy rest of atributes that got merged in the removing of duplicates
    if(original_mesh->C.size()) C=original_mesh->C;
    m_is_dirty=true;

}

//instead of removing the duplicate verts, we sometimes just want them set to zero so they don't interfere with the organized datastrucutre of a velodyne cloud
void Mesh::set_duplicate_verts_to_zero(){
    LOG(FATAL) << "Function is not completed, at the moment it doesnt do anything";

    Eigen::MatrixXd V_new;
    Eigen::MatrixXi indirection; //size of mesh.V, says for each point where they ended up in the V_new array
    Eigen::MatrixXi inverse_indirection; // size of V_new , says for each point where the original one was in mesh.V

    igl::remove_duplicate_vertices(V, 1e-7, V_new, indirection, inverse_indirection);

    VLOG(1) << "size of indirection is" <<indirection.rows();
    VLOG(1) << "size of inverse_indirection is" <<inverse_indirection.rows();


}

//to go from worldROS to worldGL we rotate90 degrees
void Mesh::rotate_90_x_axis(){
    Eigen::Affine3d tf_worldGL_worldROS;
    tf_worldGL_worldROS.setIdentity();
    Eigen::Matrix3d worldGL_worldROS_rot;
    worldGL_worldROS_rot = Eigen::AngleAxisd(-0.5*M_PI, Eigen::Vector3d::UnitX());
    tf_worldGL_worldROS.matrix().block<3,3>(0,0)=worldGL_worldROS_rot;
    transform_vertices_cpu(tf_worldGL_worldROS);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;
}

//this maps a mesh from world_GL to world ros, so it multiplies with tf_worldROS_worldGL
void Mesh::worldGL2worldROS(){
    Eigen::Affine3d tf_worldGL_worldROS;
    tf_worldGL_worldROS.setIdentity();
    Eigen::Matrix3d worldGL_worldROS_rot;
    worldGL_worldROS_rot = Eigen::AngleAxisd(-0.5*M_PI, Eigen::Vector3d::UnitX());
    tf_worldGL_worldROS.matrix().block<3,3>(0,0)=worldGL_worldROS_rot;
    Eigen::Affine3d tf_worldROS_worldGL=tf_worldGL_worldROS.inverse();
    transform_vertices_cpu(tf_worldROS_worldGL);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;
}

//this maps a mesh from world_ROS to world GL, so it multiplies with tf_worldGL_worldROS
void Mesh::worldROS2worldGL(){
    Eigen::Affine3d tf_worldGL_worldROS;
    tf_worldGL_worldROS.setIdentity();
    Eigen::Matrix3d worldGL_worldROS_rot;
    worldGL_worldROS_rot = Eigen::AngleAxisd(-0.5*M_PI, Eigen::Vector3d::UnitX());
    tf_worldGL_worldROS.matrix().block<3,3>(0,0)=worldGL_worldROS_rot;
    transform_vertices_cpu(tf_worldGL_worldROS);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;
}

// void Mesh::rotate_x_axis(const float degrees ){
//     Eigen::Affine3d tf;
//     tf.setIdentity();
//     Eigen::Matrix3d tf_rot;
//     float rads=degrees2radians(degrees);
//     tf_rot = Eigen::AngleAxisd(rads, Eigen::Vector3d::UnitX());
//     tf.matrix().block<3,3>(0,0)=tf_rot;
//     transform_vertices_cpu(tf);

//     m_is_dirty=true;
// }

// void Mesh::rotate_y_axis(const float degrees ){
//     Eigen::Affine3d tf;
//     tf.setIdentity();
//     Eigen::Matrix3d tf_rot;
//     float rads=degrees2radians(degrees);
//     tf_rot = Eigen::AngleAxisd(rads, Eigen::Vector3d::UnitY());
//     tf.matrix().block<3,3>(0,0)=tf_rot;
//     transform_vertices_cpu(tf);

//     m_is_dirty=true;
// }


//subsamples the point cloud a certain nr of times by randomly dropping points. If percentage_removal is 1 then we remove all the points, if it's 0 then we keep all points
void Mesh::random_subsample(const float percentage_removal){

    float prob_of_death=percentage_removal;
    int vertices_marked_for_removal=0;
    std::vector<bool> is_vertex_to_be_removed(V.rows(), false);
    for(int i = 0; i < V.rows(); i++){
        float random= m_rand_gen->rand_float(0.0, 1.0);
        if(random<prob_of_death){
            is_vertex_to_be_removed[i]=true;
            vertices_marked_for_removal++;
        }
    }
    VLOG(1) << "Vertices marked for removal " << vertices_marked_for_removal;


    remove_marked_vertices(is_vertex_to_be_removed, false);

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}


void Mesh::decimate(const int nr_target_faces){

    // delete degenerate faces
    std::vector<bool> is_face_degenerate(F.rows(),false);
    double eps=1e-5;
    for (int i = 0; i < F.rows(); i++) {
        double dif_0= (V.row(F(i,0)) - V.row(F(i,1))).norm();
        double dif_1= (V.row(F(i,1)) - V.row(F(i,2))).norm();
        double dif_2= (V.row(F(i,2)) - V.row(F(i,0))).norm();
        if( dif_0 < eps || dif_1 < eps || dif_2 < eps ){
            is_face_degenerate[i]=true;
        }
    }
    F=filter(F, is_face_degenerate, false);
    NF=filter(NF, is_face_degenerate, false);

    //remove the unreferenced vertices
    remove_unreferenced_verts();

    //remove faces that are non manifold (need to iterate because it is not guaranteed that the resulting mesh is manifold)
    bool is_edge_manifold=false;
    while(!is_edge_manifold){
        std::vector<bool> is_face_non_manifold;
        std::vector<bool> is_vertex_non_manifold;
        Mesh mesh_connected_to_infinity;
        igl::connect_boundary_to_infinity(V, F, mesh_connected_to_infinity.V, mesh_connected_to_infinity.F);
        is_edge_manifold=compute_non_manifold_edges(is_face_non_manifold, is_vertex_non_manifold,  mesh_connected_to_infinity.F);
        std::vector<bool> is_vertex_non_manifold_original_mesh(V.rows());
        for (int i = 0; i < V.rows(); i++) {
            is_vertex_non_manifold_original_mesh[i]=is_vertex_non_manifold[i];
        }
        VLOG(3) << "is_edge_manifold is " << is_edge_manifold;
        remove_marked_vertices(is_vertex_non_manifold_original_mesh, false);
    }


    //decimate it
    Eigen::VectorXi I;
    Eigen::VectorXi J;
    igl::qslim(V, F, nr_target_faces, V, F, J,I);

    recalculate_normals(); //we completely changed the V and F so we might as well just recompute NV and NF

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;
}


bool Mesh::compute_non_manifold_edges(std::vector<bool>& is_face_non_manifold, std::vector<bool>& is_vertex_non_manifold,  const Eigen::MatrixXi& F_in){
    // List of edges (i,j,f,c) where edge i<j is associated with corner i of face
    // f

    is_face_non_manifold.resize(F_in.rows(),false);
    int nr_vertices=F_in.maxCoeff()+1;
    is_vertex_non_manifold.resize(nr_vertices,false);
    std::vector<std::vector<int> > TTT;
    for(int f=0;f<F_in.rows();++f)
        for (int i=0;i<3;++i)
        {
            // v1 v2 f ei
            int v1 = F_in(f,i);
            int v2 = F_in(f,(i+1)%3);
            if (v1 > v2) std::swap(v1,v2);
            std::vector<int> r(4);
            r[0] = v1; r[1] = v2;
            r[2] = f;  r[3] = i;
            TTT.push_back(r);
        }
    // Sort lexicographically
    std::sort(TTT.begin(),TTT.end());

    bool is_edge_manifold=true;
    for(int i=2;i<(int)TTT.size();++i)
    {
        // Check any edges occur 3 times
        std::vector<int>& r1 = TTT[i-2];
        std::vector<int>& r2 = TTT[i-1];
        std::vector<int>& r3 = TTT[i];
        if ( (r1[0] == r2[0] && r2[0] == r3[0])
             &&
             (r1[1] == r2[1] && r2[1] == r3[1]) )
        {
            VLOG(3) << "non manifold around the 3 faces (v_idx, v_idx, f_idx, e_idx)";;
            VLOG(3) << " edge_1 " << r1[0] << " " << r1[1] << " " << r1[2] << " " << r1[3];
            VLOG(3) << " edge_2 " << r2[0] << " " << r2[1] << " " << r2[2] << " " << r2[3];
            VLOG(3) << " edge_3 " << r3[0] << " " << r3[1] << " " << r3[2] << " " << r3[3];
            is_face_non_manifold[r1[2]]=true;
            is_face_non_manifold[r2[2]]=true;
            is_face_non_manifold[r3[2]]=true;

            is_vertex_non_manifold[r1[0]]=true;
            is_vertex_non_manifold[r1[1]]=true;
            is_vertex_non_manifold[r2[0]]=true;
            is_vertex_non_manifold[r2[1]]=true;
            is_vertex_non_manifold[r3[0]]=true;
            is_vertex_non_manifold[r3[1]]=true;


            is_edge_manifold=false;
        }
    }
    return is_edge_manifold;
}


void Mesh::upsample(const int nr_of_subdivisions, const bool smooth){
    Eigen::MatrixXd new_V;
    Eigen::MatrixXi new_F;

    if(smooth){
        igl::loop(V,F, new_V, new_F, nr_of_subdivisions);
    }else{
        igl::upsample(V,F, new_V, new_F, nr_of_subdivisions);
    }

    V=new_V;
    F=new_F;

    recalculate_normals();

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;

}

void Mesh::flip_winding(){
    Eigen::MatrixXi new_F=F;

    for(int f=0;f<F.rows();++f){
        new_F(f,0)=F(f,0);
        new_F(f,1)=F(f,2);
        new_F(f,2)=F(f,1);
    }

    F=new_F;
}

void Mesh::flip_normals(){
    NF=-NF;
    NV=-NV;
    flip_winding();


    m_is_dirty=true;
    m_is_shadowmap_dirty=true;
}

void Mesh::normalize_size(){
    CHECK(V.size()) << name << "We cannot normalize_size an empty cloud";

    Eigen::VectorXd max= V.colwise().maxCoeff();
    Eigen::VectorXd min= V.colwise().minCoeff();
    // double size_diff=(max-min).norm();
    float scale= get_scale();
    V.array()/=scale;
}

void Mesh::normalize_position(){
    CHECK(V.size()) << name << "We cannot normalize_position an empty cloud";

    Eigen::Vector3d max= V.colwise().maxCoeff();
    Eigen::Vector3d min= V.colwise().minCoeff();
    Eigen::Vector3d mid=(max+min)/2.0;
    Eigen::Affine3d tf;
    tf.setIdentity();
    tf.translation()=-mid;

    transform_vertices_cpu(tf);
}

void Mesh::recalculate_min_max_height(){
    CHECK( !is_empty() ) << "Mesh is empty";
    //calculate the min and max y which will be useful for coloring
    m_min_max_y(0)=V.col(1).minCoeff();
    m_min_max_y(1)=V.col(1).maxCoeff();
    m_min_max_y_for_plotting=m_min_max_y;
}


// void Mesh::move_in_x(const float amount){
//     Eigen::Affine3d tf;
//     tf.setIdentity();
//     tf.translation().x()=amount;
//     transform_vertices_cpu(tf);
// }
// void Mesh::move_in_y(const float amount){
//     Eigen::Affine3d tf;
//     tf.setIdentity();
//     tf.translation().y()=amount;
//     transform_vertices_cpu(tf);
// }
// void Mesh::move_in_z(const float amount){
//     Eigen::Affine3d tf;
//     tf.setIdentity();
//     tf.translation().z()=amount;
//     transform_vertices_cpu(tf);
// }
void Mesh::random_translation(const float translation_strength){
    Eigen::Affine3d tf;
    tf.setIdentity();
    tf.translation().x()=m_rand_gen->rand_float(-1.0, 1.0)*translation_strength;
    tf.translation().y()=m_rand_gen->rand_float(-1.0, 1.0)*translation_strength;
    tf.translation().z()=m_rand_gen->rand_float(-1.0, 1.0)*translation_strength;
    transform_vertices_cpu(tf);
}
void Mesh::random_rotation(const float rotation_strength){
    Eigen::Quaterniond q;
    q=Eigen::Quaterniond::UnitRandom();

    Eigen::AngleAxisd angle_axis(q);
    //the vector around which we rotate will stay the same but we scale the angle by rotation_strength
    angle_axis.angle() = m_rand_gen->rand_float(0, 2*M_PI) * degrees2radians(rotation_strength);

    Eigen::Matrix3d R(angle_axis);
    Eigen::Affine3d tf;
    tf.setIdentity();
    tf.linear()=R;

    transform_vertices_cpu(tf);

}
void Mesh::random_stretch(const float stretch_strength){
    float stretch_factor_x=1.0 + m_rand_gen->rand_float(-stretch_strength, stretch_strength);
    float stretch_factor_y=1.0 + m_rand_gen->rand_float(-stretch_strength, stretch_strength);
    float stretch_factor_z=1.0 + m_rand_gen->rand_float(-stretch_strength, stretch_strength);
    V.col(0)*=stretch_factor_x;
    V.col(1)*=stretch_factor_y;
    V.col(2)*=stretch_factor_z;
}
void Mesh::random_noise(const float noise_stddev){
    Eigen::MatrixXd noise=V;
    for(int i=0; i<noise.rows(); i++){
        for(int j=0; j<noise.cols(); j++){
            noise(i,j)=m_rand_gen->rand_normal_float(0.0, noise_stddev);
        }
    }
    V+=noise;
}



void Mesh::create_quad(){
    V.resize(4,3);
    V.row(0) << -1.0, -1.0, 0.0;
    V.row(1) << 1.0, -1.0, 0.0;
    V.row(2) << 1.0, 1.0, 0.0;
    V.row(3) << -1.0, 1.0, 0.0;

    UV.resize(4,2);
    UV.row(0) << 0.0, 0.0;
    UV.row(1) << 1.0, 0.0;
    UV.row(2) << 1.0, 1.0;
    UV.row(3) << 0.0, 1.0;

    F.resize(2,3);
    F.row(0) << 0,1,2;
    F.row(1) << 0,2,3;


}

void Mesh::create_box_ndc(){
    //makes a 1x1x1 vox in NDC. which has Z going into the screen
    V.resize(8,3);
    //behind face (which has negative Z as the camera is now looking in the positive Z direction and this will be the face that is behind the camera)
    V.row(0) << -1.0, -1.0, -1.0; //bottom-left
    V.row(1) << 1.0, -1.0, -1.0; //bottom-right
    V.row(2) << 1.0, 1.0, -1.0; //top-right
    V.row(3) << -1.0, 1.0, -1.0; //top-left
    //front face
    V.row(4) << -1.0, -1.0, 1.0; //bottom-left
    V.row(5) << 1.0, -1.0, 1.0; //bottom-right
    V.row(6) << 1.0, 1.0, 1.0; //top-right
    V.row(7) << -1.0, 1.0, 1.0; //top-left

    //faces (2 triangles per faces, with 6 faces which makes 12 triangles)
    F.resize(12,3);
    F.setZero();
    //behind //we put them clockwise because we look at them from the outside, all of the other faces are counterclockwise because we see them from inside the cube
    F.row(0) << 2,1,0;
    F.row(1) << 2,0,3;
    //front
    // 7,6
    // 4,5
    F.row(2) << 4,5,6;
    F.row(3) << 7,4,6;
    //bottom
    // 45
    // 01
    F.row(4) << 5,0,1;
    F.row(5) << 4,0,5;
    //top
    // 32
    // 76
    F.row(6) << 7,6,3;
    F.row(7) << 3,6,2;
    //left
        // 7
        // 4
    // 3
    // 0
    F.row(8) << 3,0,4;
    F.row(9) << 3,4,7;
    // 6
    // 5
        // 2
        // 1
    F.row(10) << 6,5,1;
    F.row(11) << 6,1,2;
}

void Mesh::create_box(const float w, const float h, const float l){
    V.resize(8,3);
    //behind face (which has negative Z as the camera is now looking in the positive Z direction and this will be the face that is behind the camera)
    V.row(0) << -1.0, -1.0, -1.0; //bottom-left
    V.row(1) << 1.0, -1.0, -1.0; //bottom-right
    V.row(2) << 1.0, 1.0, -1.0; //top-right
    V.row(3) << -1.0, 1.0, -1.0; //top-left
    //front face
    V.row(4) << -1.0, -1.0, 1.0; //bottom-left
    V.row(5) << 1.0, -1.0, 1.0; //bottom-right
    V.row(6) << 1.0, 1.0, 1.0; //top-right
    V.row(7) << -1.0, 1.0, 1.0; //top-left

    //faces (2 triangles per faces, with 6 faces which makes 12 triangles)
    F.resize(12,3);
    F.setZero();
    //behind //we put them clockwise because we look at them from the outside, all of the other faces are counterclockwise because we see them from inside the cube
    F.row(0) << 2,1,0;
    F.row(1) << 2,0,3;
    //front
    // 7,6
    // 4,5
    F.row(2) << 4,5,6;
    F.row(3) << 7,4,6;
    //bottom
    // 45
    // 01
    F.row(4) << 5,0,1;
    F.row(5) << 4,0,5;
    //top
    // 32
    // 76
    F.row(6) << 7,6,3;
    F.row(7) << 3,6,2;
    //left
        // 7
        // 4
    // 3
    // 0
    F.row(8) << 3,0,4;
    F.row(9) << 3,4,7;
    // 6
    // 5
        // 2
        // 1
    F.row(10) << 6,5,1;
    F.row(11) << 6,1,2;




    // //make also some edges that define just the cube edges
    // E.resize(4*3,2);
    // E.setZero();
    // //behind face
    // E.row(0)<<0,1;
    // E.row(1)<<1,2;
    // E.row(2)<<2,3;
    // E.row(3)<<3,0;
    // //front face
    // E.row(4)<<4,5;
    // E.row(5)<<5,6;
    // E.row(6)<<6,7;
    // E.row(7)<<7,4;
    // //in between
    // E.row(8)<<0,4;
    // E.row(9)<<1,5;
    // E.row(10)<<2,6;
    // E.row(11)<<3,7;



    //since we want each vertex at the cornet to have multiple normals, we just duplicate them
    std::vector<Eigen::VectorXd> points;
    std::vector<Eigen::VectorXi> faces;
    std::vector<Eigen::VectorXi> edges;
    for(int i=0; i<F.rows(); i++){
        Eigen::VectorXd p1=V.row( F(i,0) );
        Eigen::VectorXd p2=V.row( F(i,2) );
        Eigen::VectorXd p3=V.row( F(i,1) );
        points.push_back(p1);
        points.push_back(p2);
        points.push_back(p3);

        Eigen::VectorXi face;
        face.resize(3);
        face << points.size()-1, points.size()-2, points.size()-3; //corresponds to p3,p2 and p1
        faces.push_back(face);

        //we make edges between the straing lines, which are the points that have a distance of 2 between them, the two points on the triangle that would connect as a diagonal would have a higher distance so they will not create a line
        float dist_1=(p1-p2).norm();
        float dist_2=(p2-p3).norm();
        float dist_3=(p3-p1).norm();
        Eigen::VectorXi edge;
        edge.resize(2);
        if( std::abs(dist_1-2)<0.001 ){
            //coonect p1 and p2
            edge<< points.size()-3,  points.size()-2;
            edges.push_back(edge);
        }
        if( std::abs(dist_2-2)<0.001 ){
            //coonect p2 and p3
            edge<< points.size()-1,  points.size()-2;
            edges.push_back(edge);
        }
        if( std::abs(dist_3-2)<0.001 ){
            //coonect p3 and p1
            edge<< points.size()-1,  points.size()-3;
            edges.push_back(edge);
        }

    }

    V=radu::utils::vec2eigen(points);
    F=radu::utils::vec2eigen(faces);
    E=radu::utils::vec2eigen(edges);



    //the box now has a size of 2 in every direction but we and to have it size of w,l,h
    V.col(0) = V.col(0)*0.5*w;
    V.col(1) = V.col(1)*0.5*h;
    V.col(2) = V.col(2)*0.5*l;


    recalculate_normals();


}

void Mesh::create_grid(const int nr_segments, const float y_pos, const float scale){

    int nr_segments_even= round(nr_segments / 2) * 2; // so we have an even number of segments on each side and then one running thgou th emiddle
    int half_size=nr_segments_even/2;
    int nr_points_per_side=nr_segments_even+1;// the +1 is because we will have 3 lines if we choose 2 segments, we have to consider the one that runs through the middle of the scene
    V.resize( (nr_points_per_side)*(nr_points_per_side), 3 );
    int idx=0;
    for(int x=-half_size; x<=half_size; x++){
        for(int y=-half_size; y<=half_size; y++){
            V.row(idx) << x, 0, y;
            idx++;
        }
    }
    //make edges horizontally
    int nr_edges=half_size*2*nr_points_per_side*2; // we will have half_size*2 for each line, and we have nr_points_per_size lines and since we both hotiz and ertical we multiply by 2
    E.resize( nr_edges, 2 );
    idx=0;
    int idx_prev_point=-1;
    for(int y=-half_size; y<=half_size; y++){
        for(int x=-half_size; x<=half_size; x++){
            int idx_cur_point=(y+half_size)*nr_points_per_side + x + half_size;
            if(idx_prev_point!=-1){
                E.row(idx) << idx_prev_point, idx_cur_point;
                idx++;
            }
            idx_prev_point=idx_cur_point;
        }
        idx_prev_point=-1; //we start a new row so invalidate the previous one so we dont connect with the points in the row above
    }

    //maked edges vertically
    idx_prev_point=-1;
    for(int x=-half_size; x<=half_size; x++){
        for(int y=-half_size; y<=half_size; y++){
            int idx_cur_point=(y+half_size)*nr_points_per_side + x + half_size;
            // std::cout << "idx cur points is " << idx_cur_point << '\n';
            if(idx_prev_point!=-1){
                E.row(idx) << idx_prev_point, idx_cur_point;
                idx++;
            }

            idx_prev_point=idx_cur_point;
        }
        idx_prev_point=-1; //we start a new row so invalidate the previous one so we dont connect with the points in the row above
    }


    //make some faces in case the grid is used for something else except as a floor for the scene
    std::vector<Eigen::VectorXi> faces_vec;
    for(int x=0; x<nr_segments_even; x++){
        for(int y=0; y<nr_segments_even; y++){

            int idx_v = y + x*(nr_segments_even+1);

            Eigen::VectorXi f1(3);
            Eigen::VectorXi f2(3);
            f1 << idx_v, idx_v+1, idx_v+nr_segments_even+1;
            f2 << idx_v+nr_segments_even+1, idx_v+1, idx_v+nr_segments_even+2;

            // VLOG(1) << "f1 is " << f1.transpose();
            // VLOG(1) << "f2 is " << f2.transpose();

            faces_vec.push_back(f1);
            faces_vec.push_back(f2);
        }
    }
    F=vec2eigen(faces_vec);


    //scale it to be in range [-1, 1]
    V.array()/=half_size;
    //scale to be in the range of the mesh
    V.array()*=scale;

    //find the minimal point in y of the mesh so we put the grid there
    Eigen::Affine3d trans=Eigen::Affine3d::Identity();
    Eigen::Vector3d t;
    t<< 0, y_pos, 0;
    trans.translate(t);
    transform_vertices_cpu(trans,true);

    recalculate_normals(); //just so we have them in case the grid is used for something else except as a floor for the scene

    name="grid_floor";
    m_vis.m_show_lines=true;
    m_vis.m_line_color<<0.6, 0.6, 0.6;
    m_vis.m_show_mesh=false;

    // VLOG(1) << "get scale " << get_scale();

}

void Mesh::create_floor(const float y_pos, const float scale){

    //make 4 vertices
    V.resize( 4, 3 );
    V.row(0) << -1, 0, -1;
    V.row(1) << 1, 0, -1;
    V.row(2) << 1, 0, 1;
    V.row(3) << -1, 0, 1;

    //make 2 faces
    F.resize( 2, 3 );
    F.row(0) << 2, 1, 0;
    F.row(1) << 3, 2, 0;
    recalculate_normals();

    //scale to be in the range of the mesh
    V.array()*=2*get_scale();

    //find the minimal point in y of the mesh so we put the grid there
    Eigen::Affine3d trans=Eigen::Affine3d::Identity();
    Eigen::Vector3d t;
    t<< 0, y_pos, 0;
    trans.translate(t);
    transform_vertices_cpu(trans,true);


    name="grid_floor";
    m_vis.m_show_mesh=true;
    m_vis.m_solid_color<<47.0/255, 47.0/255, 47.0/255;
    m_vis.m_roughness=1.0;

}

void Mesh::create_sphere(const Eigen::Vector3d& center, const double radius){
    load_from_file( std::string(EASYPBR_DATA_DIR)+"/sphere.obj" );
    // V/= 0.0751893; //normalize by the radius of this sphere that we loaded so not we have a radius of 1.0
    for (int i = 0; i < V.rows(); i++) {
        // V.row(i) =V.row(i)*radius+center;
        V.row(i) = Eigen::Vector3d(V.row(i))*radius+center;
    }
}

void Mesh::create_cylinder(const Eigen::Vector3d& main_axis, const double height, const double radius, const bool origin_at_bottom, const bool with_cap){
    if (with_cap){
        load_from_file( std::string(EASYPBR_DATA_DIR)+"/cylinder_14.obj" );
    }else{
        load_from_file( std::string(EASYPBR_DATA_DIR)+"/cylinder_14_nocap.obj" );
        remove_unreferenced_verts(); //the no cap one was done by removing the faces of the caps in blender but that leaves the vertices so we need to remove them
    }

    //make it a height of 1
    V.col(1) = V.col(1)*0.5;


    //if the origin is at the bottom, then we move the primtive
    if(origin_at_bottom){
        for (int i = 0; i < V.rows(); i++) {
            Eigen::Vector3d point = V.row(i);
            V.row(i) << point.x(), point.y()+0.5, point.z();
        }
    }


    //change the height and radius
    for (int i = 0; i < V.rows(); i++) {
        Eigen::Vector3d point = V.row(i);
        V.row(i) << point.x()*radius, point.y()*height, point.z()*radius;
    }



    //rotate towards the main axis. The cylinder stads with the main axis being the y axis so it's upwright
    Eigen::Vector3d canonical_direction=  Eigen::Vector3d::UnitY();
    Eigen::Vector3d new_direction=main_axis.normalized();
    if (  (new_direction-canonical_direction).norm()>1e-7  ){ //if the vectors are the same, then there is no need to rotate, also the cross product would fail

        Eigen::Vector3d axis= (new_direction.cross(canonical_direction)).normalized();
        double angle= - std::acos( new_direction.dot(canonical_direction)  );

        //get axis angle
        Eigen::AngleAxisd angle_axis_eigen;
        angle_axis_eigen.axis()=axis;
        angle_axis_eigen.angle()=angle;

        //get rotation
        Eigen::Matrix3d R=angle_axis_eigen.toRotationMatrix();

        //rotate the mesh
        Eigen::Affine3d tf;
        tf.setIdentity();
        tf.linear()=R;
        transform_vertices_cpu(tf,true);

    }
}

void Mesh::create_line_strip_from_points(const std::vector<Eigen::Vector3d>& points_vec){
    CHECK(points_vec.size()>=2 ) << "We need at least 2 points to create line_strip";

    V=vec2eigen(points_vec);
    E.resize(points_vec.size()-1, 2);
    for(size_t i=0; i<points_vec.size()-1; i++){
        E.row(i) << i, i+1; 
    }

    m_vis.m_show_lines=true;
    m_vis.m_show_mesh=false;

}




void Mesh::color_from_label_indices(Eigen::MatrixXi label_indices){
    CHECK(m_label_mngr) << "Mesh does not have a label_mngr so we cannot assign colors to it through the labels_indices";
    CHECK(label_indices.rows()==V.rows()) << "label indices does not have the same size as V. label indices has rows " << label_indices.rows() << " V has rows " << V.rows();

    C.resize(V.rows(),3);
    C.setZero();

    for(int i=0; i<V.rows(); i++){
        int label=label_indices(i,0);
        Eigen::Vector3d color=m_label_mngr->color_for_label(label);
        C(i,0)=color(0);
        C(i,1)=color(1);
        C(i,2)=color(2);
    }
}

void Mesh::color_from_mat(const cv::Mat& mat){
    CHECK(UV.size()) << "In order to use color_from_mat you need UVs";
    CHECK(mat.data) << "Mat is empty";

    VLOG(1) << "mat in is " << radu::utils::type2string(mat.type());
    int channels=mat.channels();

    cv::Mat mat_float; //just convert it to float so that we don't need to deal with different types
    if (mat.depth()==CV_32F){
        VLOG(1) << "Mat is already in float";
        if (channels==1) mat.convertTo(mat_float, CV_32FC1);
        if (channels==2) mat.convertTo(mat_float, CV_32FC2);
        if (channels==3) mat.convertTo(mat_float, CV_32FC3);
        if (channels==4) mat.convertTo(mat_float, CV_32FC4);
    }else{
        VLOG(1) << "conveting mat to float";
        if (channels==1) mat.convertTo(mat_float, CV_32FC1, 1.0/255.0);
        if (channels==2) mat.convertTo(mat_float, CV_32FC2, 1.0/255.0);
        if (channels==3) mat.convertTo(mat_float, CV_32FC3, 1.0/255.0);
        if (channels==4) mat.convertTo(mat_float, CV_32FC4, 1.0/255.0);
    }

    VLOG(1) << "mat_float in is " << radu::utils::type2string(mat_float.type());

    int rows=mat_float.rows;
    int cols=mat_float.cols;
    VLOG(1) << "ne channels is " << channels;

    C.resize(V.rows(),3);
    C.setZero();

    for(int i=0; i<V.rows(); i++){
        float u=UV(i,0);
        float v=UV(i,1);
        //get the uv from [0,1] to range [0,img size]
        u=u*cols;
        v=v*rows;

        int x=(int)u;
        int y=rows-(int)v; //flip up and down

        //if it's out of bounds by one pixel, try to bring it back within the bounds of the image
        if (x<0) x+=1;
        if (y<0) y+=1;
        if (x>=cols) x-=1;
        if (y>=rows) y-=1;


        if(x>=0 && x<cols && y>=0 && y<rows ){
            float r,g,b;
            r=0;
            g=0;
            b=0;
            if (channels==1){
                r=mat_float.at<float>(y,x);
                g=r;
                b=r;
            }else if (channels==2){
                r=mat_float.at<cv::Vec2f>(y,x)[0];
                g=mat_float.at<cv::Vec2f>(y,x)[1];
                b=r;
            }else if (channels==3){
                r=mat_float.at<cv::Vec3f>(y,x)[0];
                g=mat_float.at<cv::Vec3f>(y,x)[1];
                b=mat_float.at<cv::Vec3f>(y,x)[2];
            }else if (channels==4){
                r=mat_float.at<cv::Vec4f>(y,x)[0];
                g=mat_float.at<cv::Vec4f>(y,x)[1];
                b=mat_float.at<cv::Vec4f>(y,x)[2];
            }
            // float r=mat_float.at<cv::Vec3f>(y,x)[0];
            // float g=mat_float.at<cv::Vec3f>(y,x)[1];
            // float b=mat_float.at<cv::Vec3f>(y,x)[2];
            // VLOG(1) << "Pixel at " << u << " " << v << " is " << r << " " << g << " " << b;
            C(i,0)=r;
            C(i,1)=g;
            C(i,2)=b;
        }else{
            // VLOG(1) << "out of bounds at" << u << " " << v << " xy is " << x << " " << y;
        }

    }


}

#ifdef EASYPBR_WITH_EMBREE
    void Mesh::compute_embree_ao(const int nr_samples){
        this->apply_model_matrix_to_cpu(true); // We need this because we may have modified the model matrix which modifiest eh normals and they need to be consistent with the rest of the scene

        Eigen::VectorXd ao;
        igl::embree::EmbreeIntersector ei;
        // ei.init(V.cast<float>(),F.cast<int>());

        //init with the whole scene
        // MeshSharedPtr mesh_merged= Mesh::create();
        // for(int i=0; i<Scene::nr_meshes(); i++){
        //     MeshSharedPtr mesh=Scene::get_mesh_with_idx(i);
        //     if(mesh->name!="grid_floor"){
        //         mesh->transform_vertices_cpu(mesh->model_matrix());
        //         mesh->set_model_matrix( Eigen::Affine3d::Identity() );
        //         mesh_merged->add(*mesh);
        //     }
        // }
        // ei.init(mesh_merged->V.cast<float>(),mesh_merged->F.cast<int>());

        ei.init(V.cast<float>(),F.cast<int>());

        Eigen::MatrixXd N_vertices;
        igl::per_vertex_normals(V, F, N_vertices);
        igl::embree::ambient_occlusion(ei, V, N_vertices, nr_samples, ao);
        ao=1.0-ao.array(); //a0 is 1.0 in occluded places and 0.0 in non ocluded so we flip it to have 0.0 (dark) in occluded
        C = ao.replicate(1,3);

        m_colors_are_precomputed_ao=true;
        m_is_dirty=true;

    }
#endif

Eigen::Vector3d Mesh::centroid(){
    Eigen::Vector3d min_point = model_matrix()*Eigen::Vector3d(V.colwise().minCoeff());
    Eigen::Vector3d max_point = model_matrix()*Eigen::Vector3d(V.colwise().maxCoeff());
    Eigen::Vector3d centroid = (0.5*(min_point + max_point)).eval();

    return centroid;

}
Eigen::VectorXi Mesh::fix_oversplit_due_to_blender_uv(){

    //Blender exports a ply in which every triangles has independant vertices, we need to merge them but we cannot merge if vertices have a split in UV space
    //merge if they are both spacially close and close in uv space
    //build a 5D vertices which contain xyz,uv and let igl::remove_duplicates do the job
    Eigen::MatrixXd V_UV(V.rows(),5);


    V_UV.block(0,0,V.rows(),3)=V;
    V_UV.block(0,3,V.rows(),2)=UV;

    Eigen::MatrixXd V_UV_merged;
    Eigen::MatrixXi F_merged;
    Eigen::VectorXi I; //size of V_original and it maps to where each vertex ended up in the merged vertices
    igl::remove_duplicates(V_UV, F ,V_UV_merged,F_merged,I,1e-14);

    V=V_UV_merged.block(0,0,V_UV_merged.rows(),3);
    UV=V_UV_merged.block(0,3,V_UV_merged.rows(),2);
    F=F_merged;

    return I;

}

void Mesh::color_connected_components(){
    // Mesh merged=*this;
    // Eigen::VectorXi I; //size of V_original and it maps to where each vertex ended up in the merged vertices
    // I=merged.fix_oversplit_due_to_blender_uv();

    Eigen::VectorXi components;
    igl::facet_components(F,components);
    int nr_components=components.maxCoeff()+1;
    std::cout << "nr of components: " << components.maxCoeff() << '\n';

    Eigen::VectorXd colors_per_components;
    colors_per_components.resize(nr_components,3);
    for (int i = 0; i < nr_components; i++) {
        Eigen::Vector3d color=random_color(m_rand_gen);
        colors_per_components(i,0)=color(0);
        colors_per_components(i,1)=color(1);
        colors_per_components(i,2)=color(2);
    }

    //assign the colors to the this mesh
    Eigen::MatrixXd C_per_f;
    C_per_f.resize(F.rows(),3);
    C_per_f.setZero();

    //need the adyancency becase the components are deifned per face
    std::vector<std::vector<int>> v2f;
    std::vector<std::vector<int>> v2fi;
    igl::vertex_triangle_adjacency(V.rows(), F, v2f, v2fi);

    for (int i = 0; i < F.rows(); i++) {
        int idx_v=F(i,0); //grab the first one, it doesn't really matter
        for (size_t f = 0; f < v2f[idx_v].size(); f++) {
            int idx_F=v2f[idx_v][f];
            int component_idx=components(idx_F);
            C_per_f.row(i)=colors_per_components.row(component_idx);
        }

    }

    //assign the colors per vertex because for some reason assigning them for each face doesn't make them show...
    C.resize(V.rows(),3);
    for (int i = 0; i < F.rows(); i++) {
        for (size_t v = 0; v < 3; v++) {
            int idx_v=F(i,v);
            C.row(idx_v)=C_per_f.row(i);
        }
    }

    // std::cout << "C is " << C << '\n';

}


void Mesh::remove_small_uv_charts(){

    Eigen::VectorXi components;
    igl::facet_components(F,components);
    int nr_components=components.maxCoeff()+1;
    std::cout << "nr of components: " << components.maxCoeff() << '\n';

    //nr of faces per components
    Eigen::VectorXi tris_per_component(nr_components);
    std::vector< std::vector<int>> tris_idxs_per_comp(nr_components); //first idx is the component, second idx are the indexes of the faces
    tris_per_component.setZero();
    for (int i = 0; i < F.rows(); i++) {
        int idx_component=components(i);
        tris_per_component(idx_component)++;
        tris_idxs_per_comp[idx_component].push_back(i);
    }

    //the small components set their corresponding faces to 0
    size_t min_nr_trigs=50;
    for (int i = 0; i < nr_components; i++) {
        if(tris_idxs_per_comp[i].size()<min_nr_trigs){
            //all the faces from this component set their respective uv coords and V coords to 0 so the animation doesn't show for them
            for (size_t f = 0; f < tris_idxs_per_comp[i].size(); f++) {
                int idx_f=tris_idxs_per_comp[i][f];
                for (size_t v = 0; v < 3; v++) {
                    int idx_v=F(idx_f,v);
                    V.row(idx_v).setZero();
                    UV.row(idx_v).setZero();
                }
            }
        }
    }

}

void Mesh::apply_D(){

    Eigen::Vector3d origin=m_cur_pose.translation();

    for (int i = 0; i < V.rows(); ++i) {
        if (!V.row(i).isZero()){
            //get a direction going from the origin to the vertex
            Eigen::Vector3d dir_normalized = (Eigen::Vector3d(V.row(i)) - origin).normalized();
            //move along that direction with spedsize (D(i)) and then add the origin to get the position back in world coordinatees
            V.row(i) = dir_normalized*D(i) + origin;
        }

    }

}

//TODO do the to image not with the coordinates of the points in the point cloud but rather the angle that the point have with respect to the axis
void Mesh::to_image() {

    to_image(m_cur_pose);

}

void Mesh::to_image(Eigen::Affine3d tf_world_vel) {

    CHECK(m_view_direction!=-1) << "View direction has to be set. It indicates where the gap in the point cloud is so that we can unwrap it from there. Make sure the data_loader has do_random_gap_removal to true";

    //make a frame in which it is easier to make a unwrap to image. This new frame will be called algorithm frame
    Eigen::Affine3d tf_alg_vel;
    tf_alg_vel.setIdentity();
    Eigen::Matrix3d alg_vel_rot;
    alg_vel_rot = Eigen::AngleAxisd(-0.5*M_PI+m_view_direction, Eigen::Vector3d::UnitY())  //this rotation is done second and rotates around the Y axis of alg frame
      * Eigen::AngleAxisd(0.5*M_PI, Eigen::Vector3d::UnitX());   //this rotation is done first. Performed on the X axis of alg frame (after this the y is pointing towards camera, x is right and z is down)
    tf_alg_vel.matrix().block<3,3>(0,0)=alg_vel_rot;




    //TODO can maybe be done more efficiently without the change of frame
    Eigen::MatrixXd V_alg_frame(V.rows(),3);
    V_alg_frame.setZero();
    // Eigen::Affine3d tf_alg_currframe=tf_currframe_alg.inverse(); //this now goes from the current frame to the algorithm frame
    Eigen::Affine3d tf_alg_world = tf_alg_vel * tf_world_vel.inverse(); //now goes from world to the velodyne frame and then from the velodyne to the algorithm frame
    for (int i = 0; i < V.rows(); i++) {
        if(!V.row(i).isZero()){
            V_alg_frame.row(i)=tf_alg_world.linear()*V.row(i).transpose() + tf_alg_world.translation();  //mapping from the current frame to the algorithm one
        }
    }

    for (int i = 0; i < V.rows(); ++i) {
        if (!V_alg_frame.row(i).isZero()) {
            double r, theta, phi;
            r = V_alg_frame.row(i).norm();
            phi = std::atan2(V_alg_frame(i,0), -V_alg_frame(i,2));
            if (phi < 0.0) {     //atan goes from -pi to pi, it's easier to think of it going from 0 to 2pi
                phi += 2 * M_PI;
            }
            theta = (std::asin(V_alg_frame(i, 1) / r));
//            V.row(i) << phi, theta, r;
            V.row(i) << phi, theta, 0.0; // change back to r and not 0.0
            D(i)=r;  //store it back in D so that we can access it to_mesh (some points may not have D in the case when we fuse another mesh in it)

        }
    }

}

void Mesh::to_mesh() {

    to_mesh(m_cur_pose);

}

void Mesh::to_mesh(Eigen::Affine3d tf_world_vel) {

    Eigen::Affine3d tf_alg_vel;
    tf_alg_vel.setIdentity();
    Eigen::Matrix3d alg_vel_rot;
    alg_vel_rot = Eigen::AngleAxisd(-0.5*M_PI+m_view_direction, Eigen::Vector3d::UnitY())  //this rotation is done second and rotates around the Y axis of alg frame
      * Eigen::AngleAxisd(0.5*M_PI, Eigen::Vector3d::UnitX());   //this rotation is done first. Performed on the X axis of alg frame (after this the y is pointing towards camera, x is right and z is down)
    tf_alg_vel.matrix().block<3,3>(0,0)=alg_vel_rot;
    Eigen::Affine3d tf_vel_alg= tf_alg_vel.inverse();

    //the previous one works for normal spherical coordinates but to take into account the fact that the points overlap at the gap we need to use this one
    for (int i = 0; i < V.rows(); ++i) {
        if(!V.row(i).isZero()){
            double r=D(i);
            double theta=V(i,1);
            double phi=V(i,0) - M_PI;   //since in to_image we added the 2m_pi here we subtract pi to put it back in the same place

            V.row(i)(0) = - r* std::cos(theta)*std::sin(phi);  //take cos(theta) instead of sin(theta) because it's the angle from the x axis not the y one
            V.row(i)(1) = r* std::sin(theta);
            V.row(i)(2) = r* std::cos(theta)*std::cos(phi);


            //TODO this should not be needed. We need to trace why exactly are there nans in the D vector when it gets returned from igl::triangulate
            if (!V.row(i).allFinite()){
                V.row(i) << 0.0, 0.0, 0.0;
                D(i) = 0;
            }

        }

    }

    // apply_transform(tf_vel_alg);
    // apply_transform(tf_world_vel);

    Eigen::Affine3d tf_world_alg=tf_world_vel * tf_vel_alg;

    for (int i = 0; i < V.rows(); i++) {
        if(!V.row(i).isZero()){
            V.row(i)=tf_world_alg.linear()*V.row(i).transpose() + tf_world_alg.translation();  //mapping from the current frame to the algorithm one
        }
    }

}


void Mesh::to_3D(){
  Eigen::MatrixXd V_new(V.rows(),3);
  V_new.setZero();
  V_new.leftCols(2)=V;
  V=V_new;
}

void Mesh::to_2D(){
    Eigen::MatrixXd V_new(V.rows(),2);
    V_new=V.leftCols(2);
    V=V_new;
}

void Mesh::restrict_around_azimuthal_angle(const float angle, const float range){

    CHECK(angle<=2*M_PI && angle>=0) << "Azimuthal angle should be in the range [0,2pi]. However it is " << angle;
    CHECK(range<=2*M_PI && angle>=0) << "Azimuthal range should be in the range [0,2pi]. However it is " << range;

    //make a frame in which it is easier to make a unwrap to image. This new frame will be called algorithm frame
    Eigen::Affine3d tf_alg_vel;
    tf_alg_vel.setIdentity();
    Eigen::Matrix3d alg_vel_rot;
    alg_vel_rot = Eigen::AngleAxisd(-0.5*M_PI, Eigen::Vector3d::UnitY())  //this rotation is done second and rotates around the Y axis of alg frame
      * Eigen::AngleAxisd(0.5*M_PI, Eigen::Vector3d::UnitX());   //this rotation is done first. Performed on the X axis of alg frame (after this the y is pointing towards camera, x is right and z is down)
    tf_alg_vel.matrix().block<3,3>(0,0)=alg_vel_rot;




    //TODO can maybe be done more efficiently without the change of frame
    Eigen::MatrixXd V_alg_frame(V.rows(),3);
    V_alg_frame.setZero();
    // Eigen::Affine3d tf_alg_currframe=tf_currframe_alg.inverse(); //this now goes from the current frame to the algorithm frame
    Eigen::Affine3d tf_alg_world = tf_alg_vel * m_cur_pose.inverse(); //now goes from world to the velodyne frame and then from the velodyne to the algorithm frame
    for (int i = 0; i < V.rows(); i++) {
        if(!V.row(i).isZero()){
            V_alg_frame.row(i)=tf_alg_world.linear()*V.row(i).transpose() + tf_alg_world.translation();  //mapping from the current frame to the algorithm one
        }
    }

    for (int i = 0; i < V.rows(); ++i) {
        if (!V_alg_frame.row(i).isZero()) {
            double phi;
            phi = std::atan2(V_alg_frame(i,0), -V_alg_frame(i,2));
            if (phi < 0.0) {     //atan goes from -pi to pi, it's easier to think of it going from 0 to 2pi
                phi += 2 * M_PI;
            }

            //set to zero all the points that are outside the range
            if(phi < angle - range || phi> angle+range){
                V.row(i).setZero();
            }
            //deal with angle ranges that are lower than 0 and actually should wrap around to 2pi
            if(angle-range<0.0){
                float wrap=fabs(angle-range);
                if(phi > 2*M_PI-wrap){
                    V.row(i).setZero();
                }
            }
            //deal with angle ranges that are higher than 2pi and actually should wrap around to 0
            if(angle+range>2*M_PI){
                float wrap=2*M_PI-(angle+range);
                if(phi < wrap){
                    V.row(i).setZero();
                }
            }


        }
    }

}


void Mesh::compute_tangents(const float tangent_length){
    //Fills up the V_tangent_u and the V_lenght_v given a mesh with normals. The tangent and bitangent will be normalized to 1 unless a r
    CHECK(NV.size()) << "We expect a mesh with normals but thise one has none. Please use recalculate_normals() first";
    CHECK(NV.rows()==V.rows()) << "We expect that evey point has a normal vector however we have V.rows " << V.rows() << " and NV.rows " << NV.rows();


    V_tangent_u.resize(V.rows(),3);
    Eigen::MatrixXd  V_bitangent;
    V_bitangent.resize(V.rows(),3);
    V_length_v.resize(V.rows(),1);
    V_tangent_u.setZero();
    V_length_v.setOnes();
    V_bitangent.setZero();

    Eigen::VectorXi degree_vertices;
    degree_vertices.resize(V.rows());
    degree_vertices.setZero();

    //if we have UV per vertex then we can calculate a tangent that is aligned with the U direction
    //code from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-13-normal-mapping/
    //more explanation in https://learnopengl.com/Advanced-Lighting/Normal-Mapping
    if(UV.size() && F.size()){
        //compute the tangent for each triangle and then average for each vertex
        for(int f=0; f<F.rows(); f++){
            //increase the degree for the vertices we touch
            degree_vertices(F(f,0))++;
            degree_vertices(F(f,1))++;
            degree_vertices(F(f,2))++;

            Eigen::Vector3d v0 = V.row(F(f,0));
            Eigen::Vector3d v1 = V.row(F(f,1));
            Eigen::Vector3d v2 = V.row(F(f,2));

            Eigen::Vector2d uv0 = UV.row(F(f,0));
            Eigen::Vector2d uv1 = UV.row(F(f,1));
            Eigen::Vector2d uv2 = UV.row(F(f,2));

             // Edges of the triangle : position delta
            Eigen::Vector3d deltaPos1 = v1-v0;
            Eigen::Vector3d deltaPos2 = v2-v0;

            // UV delta
            Eigen::Vector2d deltaUV1 = uv1-uv0;
            Eigen::Vector2d deltaUV2 = uv2-uv0;

            float r = 1.0f / (deltaUV1.x() * deltaUV2.y() - deltaUV1.y() * deltaUV2.x() );
            Eigen::Vector3d tangent = (deltaPos1 * deltaUV2.y()   - deltaPos2 * deltaUV1.y() )*r;
            Eigen::Vector3d  bitangent = (deltaPos2 * deltaUV1.x()   - deltaPos1 * deltaUV2.x() )*r;

            V_tangent_u.row(F(f,0)) += tangent;
            V_tangent_u.row(F(f,1)) += tangent;
            V_tangent_u.row(F(f,2)) += tangent;
            V_bitangent.row(F(f,0)) += bitangent;
            V_bitangent.row(F(f,1)) += bitangent;
            V_bitangent.row(F(f,2)) += bitangent;
        }

        //average the tangent and bitangent per vector and then normalize then
        // V_tangent_u = V_tangent_u.array()/ degree_vertices.array();
        // V_bitangent = V_bitangent.array()/ degree_vertices.array();
        for(int i=0; i<V.rows(); i++){
            V_tangent_u.row(i) = V_tangent_u.row(i)/degree_vertices(i);
            V_bitangent.row(i) = V_bitangent.row(i)/degree_vertices(i);
        }
        V_tangent_u.rowwise().normalize();
        V_bitangent.rowwise().normalize();


        //compute the normal as the cross between the tangent and bitangnet
        for(int i=0; i<V.rows(); i++){
            Eigen::Vector3d T = V_tangent_u.row(i);
            Eigen::Vector3d B = V_bitangent.row(i);
            NV.row(i) = T.cross( B );
        }

    }else{
        //we don't have uv so we get a random tangent vector
        //in order to get the tangent and bitanget from a series of normal vectors, we do a cross product between a template vector and the normal, this will get us the tangent. Afterward, another cross product between normalized tangent and normals gives us the bitangent.


        Eigen::Matrix3d basis;
        basis.setIdentity();
        int cur_template_idx=0;

        for (int i = 0; i < NV.rows(); i++){
            Eigen::Vector3d n = NV.row(i);

            //if the normal and the vector we are going to do the cross product with are almost the same then we won't get a tangent
            float diff=0.0;
            float eps=0.001;
            Eigen::Vector3d vec;
            do {
                vec = basis.col(cur_template_idx);
                diff = (n-vec).norm();
                if(diff<eps){
                    cur_template_idx +=1;
                    cur_template_idx=cur_template_idx%3;
                }
            } while (diff<eps);

            //cross product to get the tangent

            Eigen::Vector3d tangent = n.cross(vec).normalized();
            V_tangent_u.row(i) = tangent*tangent_length;
            V_length_v(i,0) = tangent_length;
            V_bitangent.row(i)=  n.cross(tangent).normalized();
        }



    }

    V_bitangent_v=V_bitangent;

    // auto ret=std::make_tuple (V_tangent_u, V_bitangent);

    // return ret;

}

void Mesh::as_uv_mesh_paralel_to_axis(const int axis, const float size_modifier){

    CHECK(V.rows()==UV.rows()) << "Showing the UVs require to have the same number of uvs as vertices. V.rows is " << V.rows() << " UV.rows is " << UV.rows();

    Eigen::MatrixXd new_V=V;
    new_V.setZero();
    if(axis==0){ //we put it aligned to the X axis, so it will span in the  X-Z plane (the floor)
        new_V.col(0)=UV.col(0);
        new_V.col(2)=UV.col(1);
    }else if(axis==1){ //spain along the far wall, so put in the X-Y plane
        new_V.col(0)=UV.col(0);
        new_V.col(1)=UV.col(1);
    }else if(axis==2){ //span along Y-Z
        new_V.col(1)=UV.col(0);
        new_V.col(2)=UV.col(1);
    }

    // make the uv mesh as big as the mesh
    float span_x=V.col(0).maxCoeff()-V.col(0).minCoeff();
    float span_y=V.col(1).maxCoeff()-V.col(1).minCoeff();
    float span_z=V.col(2).maxCoeff()-V.col(2).minCoeff();
    float max_mesh_size=std::max(span_x,std::max(span_y,span_z));
    new_V.array()*=max_mesh_size*size_modifier;

    V=new_V;

    m_is_dirty=true;
    m_is_shadowmap_dirty=true;


}


Mesh Mesh::interpolate(const Mesh& target_mesh, const float factor){
    if(V.rows()!=target_mesh.V.rows()){
        LOG(FATAL) << "Cannot interpolate between meshes with different nr of vertices. This has: " << V.rows() << " target_mesh has: " << target_mesh.V.rows();
    }

    Mesh new_mesh;
    new_mesh.add(*this);

    for (int i = 0; i < V.rows(); i++) {
        new_mesh.V.row(i)=V.row(i).array()*(1-factor) + target_mesh.V.row(i).array()*factor;
    }

    return new_mesh;

}
float Mesh::get_scale(){
    //if the mesh is empty just return the scale 1.0
    if(is_empty() || V.rows()==1){ //If we have only one vertex than the scale should just be 1
       return 1.0;
    }
    // VLOG(1) << "get scale";


    // Eigen::VectorXd min_point; // each row stores the minimum point of the corresponding mesh.
    // Eigen::VectorXd max_point; // each row stores the minimum point of the corresponding mesh.
    // min_point.resize(3);
    // max_point.resize(3);
    // min_point.setConstant(std::numeric_limits<float>::max());
    // max_point.setConstant(std::numeric_limits<float>::lowest());
    Eigen::VectorXd min_point = model_matrix()*Eigen::Vector3d(V.colwise().minCoeff());
    Eigen::VectorXd max_point = model_matrix()*Eigen::Vector3d(V.colwise().maxCoeff());

    Eigen::VectorXd centroid = (0.5*(min_point + max_point)).eval();

    float scale = (max_point-min_point).array().abs().maxCoeff();

    //if the difference between min and max is too tiny, just put the scale at 1
    if( (min_point-max_point).norm()<1e-50 ){
        scale=1.0;
    }

    //sanity check
    if(std::isnan(scale)){
        LOG(ERROR) << "V is " << V;
        LOG(ERROR) << "The scale of mesh " << name << " is nan ";
        LOG(FATAL) << "min point is " << min_point << " max point is " << max_point;
    }

    return scale;

}

void Mesh::color_solid2pervert(){
    C.resize(V.rows(),3);

    for(int i=0; i<V.rows(); i++){
        C.row(i)=m_vis.m_solid_color.cast<double>();
    }

}

void Mesh::estimate_normals_from_neighbourhood(const float radius){
    #ifdef EASYPBR_WITH_PCL
        CHECK(V.size()) << "We have no vertices";

        //https://pointclouds.org/documentation/tutorials/normal_estimation.html
        pcl::PointCloud<pcl::PointXYZ>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZ>);

    //   ... read, pass in or create a point cloud ...
        for(int i=0; i<V.rows(); i++){
            pcl::PointXYZ p;
            p.x=V(i,0);
            p.y=V(i,1);
            p.z=V(i,2);
            cloud->points.push_back(p);
        }

        // Create the normal estimation class, and pass the input dataset to it
        pcl::NormalEstimation<pcl::PointXYZ, pcl::Normal> ne;
        ne.setInputCloud (cloud);

        // Create an empty kdtree representation, and pass it to the normal estimation object.
        // Its content will be filled inside the object, based on the given input dataset (as no other search surface is given).
        pcl::search::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::search::KdTree<pcl::PointXYZ> ());
        // pcl::KdTree<pcl::PointXYZ>::Ptr tree (new pcl::KdTree<pcl::PointXYZ> ());
        ne.setSearchMethod (tree);

        // Output datasets
        pcl::PointCloud<pcl::Normal>::Ptr cloud_normals (new pcl::PointCloud<pcl::Normal>);

        // Use all neighbors in a sphere of radius 3cm
        ne.setRadiusSearch (radius);

        // Compute the features
        ne.compute (*cloud_normals);

        NV.resize(cloud_normals->points.size(),3);
        for(size_t i=0; i<cloud_normals->points.size(); i++){
            pcl::Normal p=cloud_normals->points[i];
            NV(i,0)=p.normal_x;
            NV(i,1)=p.normal_y;
            NV(i,2)=p.normal_z;
            // p.x=V(i,0);
            // p.y=V(i,1);
            // p.z=V(i,2);
            // cloud.push_back(p);
        }


    #else 
        LOG(WARNING) << "estimate_normals_from_neighbourhood not available because easy_pbr was not compiled with PCL";
    #endif
}

float Mesh::min_y(){
    return m_min_max_y_for_plotting(0);
}
float Mesh::max_y(){
    return m_min_max_y_for_plotting(1);
}


// std::vector<std::pair<long int,double> > Mesh::radius_search(const Eigen::Vector3d& query_point, const double radius){
int Mesh::radius_search(const Eigen::Vector3d& query_point, const double radius){
    //follows https://github.com/jlblancoc/nanoflann/blob/master/examples/matrix_example.cpp
    // typedef nanoflann::KDTreeEigenMatrixAdaptor<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> my_kd_tree_t;


    // my_kd_tree_t mat_index(3, std::cref(V), 10 /* max leaf */);
    // mat_index.index->buildIndex();

    // std::vector<std::pair<long int,double> >   ret_matches;
    // nanoflann::SearchParams params;
    // //params.sorted = false;

    // std::vector<double> query_pt(3);

    // const size_t nr_matches = mat_index.index->radiusSearch(query_point.data(), radius, ret_matches, params);

    // VLOG(1)<< "Found nr of matches: " << nr_matches;





    //brute force
    int nr_matches_brute=0;
    for(int i=0; i<V.rows(); i++){
        Eigen::Vector3d p = Eigen::Vector3d(V.row(i));
        double dist = (p-query_point).norm();
        if(dist<radius){
            nr_matches_brute++;
        }
    }
    // VLOG(1)<< "Found nr of matches brute: " << nr_matches_brute;




    return nr_matches_brute;

    // return ret_matches;
}

//similar to https://pytorch3d.readthedocs.io/en/latest/modules/ops.html
//based on https://github.com/jlblancoc/nanoflann/blob/master/examples/pointcloud_kdd_radius.cpp
//based also on https://github.com/jlblancoc/nanoflann/blob/master/examples/matrix_example.cpp
void Mesh::ball_query(const Eigen::MatrixXd& query_points, const Eigen::MatrixXd& target_points, const float search_radius){

    LOG(FATAL) << "not finished";




    typedef nanoflann::KDTreeEigenMatrixAdaptor<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>>
      my_kd_tree_t;
    my_kd_tree_t mat_index(3, std::cref(target_points), 10 /* max leaf */);
    mat_index.index->buildIndex();


    const size_t num_results = 3;
    //initialize a big matrix containing for each query point, the indices of the points in the ball
    Eigen::MatrixXi full_indices;
    //initialize a big matrix containing for each query point, the distance towards the points in the ball
    Eigen::MatrixXd full_dists;


    // do a knn search
    for (int i=0; i<query_points.rows(); i++){
        //initialize the returned values from the knn
        std::vector<size_t> ret_indexes(num_results);
        std::vector<double> out_dists_sqr(num_results);
        nanoflann::KNNResultSet<double> resultSet(num_results);
        resultSet.init(&ret_indexes[0], &out_dists_sqr[0]);


        std::vector<double> query_pt(3);
        query_pt[0]=query_points(i,0);
        query_pt[1]=query_points(i,1);
        query_pt[2]=query_points(i,2);
        mat_index.index->findNeighbors(resultSet, &query_pt[0], nanoflann::SearchParams(10));
    }


}

Eigen::MatrixXd Mesh::compute_distance_to_mesh(const MeshSharedPtr& target_mesh){
    //we use the point_mesh_squared_distance.h
    //it computes the distances to a mesh, so if the target mesh doesnt have F then we make F for each vertex, a degenerate triangle with [0,0,0]
    Eigen::MatrixXi F=target_mesh->F;
    if (!F.size()){
        F.resize(target_mesh->V.rows(),3);
        for(int i=0; i<target_mesh->V.rows(); i++){
            F.row(i) << i,i,i;
        }
    }

    //compute the distance
    Eigen::MatrixXi indices_closest;
    Eigen::MatrixXd closest_points;
    igl::point_mesh_squared_distance(
        V, target_mesh->V, F,
        D, indices_closest, closest_points
    );

    return D;
}

void Mesh::set_diffuse_tex(const std::string file_path, const int subsample, const bool read_alpha){
    cv::Mat mat;
    if (read_alpha){
        mat= cv::imread(file_path, cv::IMREAD_UNCHANGED);
    }else{
        mat= cv::imread(file_path);
    }
    set_diffuse_tex(mat, subsample);
}
void Mesh::set_metalness_tex(const std::string file_path, const int subsample, const bool read_alpha){
    cv::Mat mat;
    if (read_alpha){
        mat= cv::imread(file_path, cv::IMREAD_UNCHANGED);
    }else{
        mat= cv::imread(file_path);
    }
    set_metalness_tex(mat, subsample);
}
void Mesh::set_roughness_tex(const std::string file_path, const int subsample, const bool read_alpha){
    cv::Mat mat;
    if (read_alpha){
        mat= cv::imread(file_path, cv::IMREAD_UNCHANGED);
    }else{
        mat= cv::imread(file_path);
    }
    set_roughness_tex(mat, subsample);
}
void Mesh::set_smoothness_tex(const std::string file_path, const int subsample, const bool read_alpha){
    cv::Mat mat;
    if (read_alpha){
        mat= cv::imread(file_path, cv::IMREAD_UNCHANGED);
    }else{
        mat= cv::imread(file_path);
    }
    cv::Mat gloss = cv::imread(file_path);
    set_smoothness_tex(gloss, subsample);
}
void Mesh::set_normals_tex(const std::string file_path, const int subsample, const bool read_alpha){
    cv::Mat mat;
    if (read_alpha){
        mat= cv::imread(file_path, cv::IMREAD_UNCHANGED);
    }else{
        mat= cv::imread(file_path);
    }
    set_normals_tex(mat, subsample);
}
void Mesh::set_matcap_tex(const std::string file_path, const int subsample, const bool read_alpha){
    cv::Mat mat;
    if (read_alpha){
        mat= cv::imread(file_path, cv::IMREAD_UNCHANGED);
    }else{
        mat= cv::imread(file_path);
    }
    set_matcap_tex(mat, subsample);
}
//using a mat directly
void Mesh::set_diffuse_tex(const cv::Mat& mat, const int subsample){
    CHECK(mat.data) << "Diffuse mat is empty";
    CHECK(subsample>=1) << "Expected the subsample to be 1 or above";
    cv::Mat mat_internal=mat; //it's just a shallow copy, so a pointer assignment. This is in order to make the mat_internal point towards the original input mat or a resized version of it if necessary
    //resize if necessary
    if(subsample>1){
        cv::Mat resized;
        cv::resize(mat_internal, resized, cv::Size(), 1.0/subsample, 1.0/subsample, cv::INTER_AREA );
        mat_internal=resized;
    }
    cv::flip(mat_internal, m_diffuse_mat.mat, 0); //opencv mat has origin of the texture on the upper left but opengl expect it to be on the lower left so we flip the texture. https://gamedev.stackexchange.com/questions/26175/how-do-i-load-a-texture-in-opengl-where-the-origin-of-the-texture0-0-isnt-in
    m_diffuse_mat.is_dirty=true;
    m_vis.set_color_texture(); //if we have diffuse we might as well just switch to actually display it
    m_vis.m_show_mesh=true; //if we have diffuse we might as well just switch to actually display it
}
void Mesh::set_metalness_tex(const cv::Mat& mat, const int subsample){
    CHECK(mat.data) << "Metalness mat is empty";
    CHECK(subsample>=1) << "Expected the subsample to be 1 or above";
    cv::Mat mat_internal=mat; //it's just a shallow copy, so a pointer assignment. This is in order to make the mat_internal point towards the original input mat or a resized version of it if necessary
    //resize if necessary
    if(subsample>1){
        cv::Mat resized;
        cv::resize(mat_internal, resized, cv::Size(), 1.0/subsample, 1.0/subsample, cv::INTER_AREA );
        mat_internal=resized;
    }
    cv::flip(mat_internal, m_metalness_mat.mat, 0);
    m_metalness_mat.is_dirty=true;
}
void Mesh::set_roughness_tex(const cv::Mat& mat, const int subsample){
    CHECK(mat.data) << "Roughness mat is empty";
    CHECK(subsample>=1) << "Expected the subsample to be 1 or above";
    cv::Mat mat_internal=mat; //it's just a shallow copy, so a pointer assignment. This is in order to make the mat_internal point towards the original input mat or a resized version of it if necessary
    //resize if necessary
    if(subsample>1){
        cv::Mat resized;
        cv::resize(mat_internal, resized, cv::Size(), 1.0/subsample, 1.0/subsample, cv::INTER_AREA );
        mat_internal=resized;
    }
    cv::flip(mat_internal, m_roughness_mat.mat, 0);
    m_roughness_mat.is_dirty=true;
}
void Mesh::set_smoothness_tex(const cv::Mat& mat, const int subsample){
    CHECK(mat.data) << "Gloss mat is empty";
    CHECK(subsample>=1) << "Expected the subsample to be 1 or above";
    cv::Mat mat_internal=mat; //it's just a shallow copy, so a pointer assignment. This is in order to make the mat_internal point towards the original input mat or a resized version of it if necessary
    //resize if necessary
    if(subsample>1){
        cv::Mat resized;
        cv::resize(mat_internal, resized, cv::Size(), 1.0/subsample, 1.0/subsample, cv::INTER_AREA );
        mat_internal=resized;
    }
    cv::Mat rough;
    cv::subtract(cv::Scalar::all(255),mat_internal,rough);
    cv::flip(rough, m_roughness_mat.mat, 0);
    m_roughness_mat.is_dirty=true;
}
void Mesh::set_normals_tex(const cv::Mat& mat, const int subsample){
    CHECK(mat.data) << "Normals mat is empty";
    CHECK(subsample>=1) << "Expected the subsample to be 1 or above";
    cv::Mat mat_internal=mat; //it's just a shallow copy, so a pointer assignment. This is in order to make the mat_internal point towards the original input mat or a resized version of it if necessary
    //resize if necessary
    if(subsample>1){
        cv::Mat resized;
        cv::resize(mat_internal, resized, cv::Size(), 1.0/subsample, 1.0/subsample, cv::INTER_AREA );
        mat_internal=resized;
    }
    cv::flip(mat_internal, m_normals_mat.mat, 0);
    m_normals_mat.is_dirty=true;
}
void Mesh::set_matcap_tex(const cv::Mat& mat, const int subsample){
    CHECK(mat.data) << "Diffuse mat is empty";
    CHECK(subsample>=1) << "Expected the subsample to be 1 or above";
    cv::Mat mat_internal=mat; //it's just a shallow copy, so a pointer assignment. This is in order to make the mat_internal point towards the original input mat or a resized version of it if necessary
    //resize if necessary
    if(subsample>1){
        cv::Mat resized;
        cv::resize(mat_internal, resized, cv::Size(), 1.0/subsample, 1.0/subsample, cv::INTER_AREA );
        mat_internal=resized;
    }
    cv::flip(mat_internal, m_matcap_mat.mat, 0); //opencv mat has origin of the texture on the upper left but opengl expect it to be on the lower left so we flip the texture. https://gamedev.stackexchange.com/questions/26175/how-do-i-load-a-texture-in-opengl-where-the-origin-of-the-texture0-0-isnt-in
    m_matcap_mat.is_dirty=true;
    m_vis.set_color_matcap(); //if we have diffuse we might as well just switch to actually display it
    m_vis.m_show_mesh=true; //if we have diffuse we might as well just switch to actually display it
}
bool Mesh::is_any_texture_dirty(){
    return  m_diffuse_mat.is_dirty || m_normals_mat.is_dirty || m_metalness_mat.is_dirty || m_roughness_mat.is_dirty || m_matcap_mat.is_dirty;
}





//we use this read_ply instead of the one from Libigl because it has a ton of memory leaks and undefined behaviours https://github.com/libigl/libigl/issues/919
// void MeshCore::read_ply(const std::string file_path, std::initializer_list<  std::pair<std::reference_wrapper<Eigen::MatrixXd>, std::vector<std::string>    > > matrix2properties_list  ){
// void MeshCore::read_ply(const std::string file_path, std::initializer_list<  std::pair<double, std::vector<std::string>    > > matrix2properties_list  ){

//     for(auto matrix2properties : matrix2properties_list){
//         // Eigen::MatrixXd& mat = matrix2properties.first;
//         // std::vector<std::string> elems=matrix2elems.second;
//         // std::initializer_list<std::string> properites =matrix2properties.second;
//         // mat.resize(5,3);
//         // mat.setZero();
//     }
// }
// void MeshCore::read_ply(const std::string file_path, std::initializer_list<std::pair<std::reference_wrapper<Eigen::MatrixXd>, std::initializer_list<std::string> >> matrix2properties_list){
void Mesh::read_ply(const std::string file_path){


    //open file
    std::ifstream ss(file_path, std::ios::binary);
    CHECK(ss.is_open()) << "Failed to open " << file_path;
    tinyply::PlyFile file;
    file.parse_header(ss);


    // Tinyply treats parsed data as untyped byte buffers. See below for examples.
    std::shared_ptr<tinyply::PlyData> vertices, normals, texcoords, color,  faces;

    // The header information can be used to programmatically extract properties on elements
    // known to exist in the header prior to reading the data. For brevity of this sample, properties
    // like vertex position are hard-coded:
    try { vertices = file.request_properties_from_element("vertex", { "x", "y", "z" }, 3); }
    catch (const std::exception & e) { LOG(FATAL) <<  e.what();  }

    bool has_vertex_normals=true;
    try { normals = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }, 3); }
    catch(const std::exception & e)  { has_vertex_normals=false; }

    bool has_texcoords=true;
    try { texcoords = file.request_properties_from_element("vertex", { "u", "v" }, 2); }
    catch(const std::exception & e)  { has_texcoords=false; }
    //if we don't find uv coordinates, maybe they are named s and t
    if (!has_texcoords){
        has_texcoords=true;
        try { texcoords = file.request_properties_from_element("vertex", { "s", "t" }, 2); }
        catch(const std::exception & e)  { has_texcoords=false; }
    }

    bool has_color=true;
    try { color = file.request_properties_from_element("vertex", { "red", "green", "blue" }, 3); }
    catch (const std::exception & e) { has_color=false; }

    // Providing a list size hint (the last argument) is a 2x performance improvement. If you have
    // arbitrary ply files, it is best to leave this 0.
    bool has_faces=true;
    try { faces = file.request_properties_from_element("face", { "vertex_indices" }, 3); }
    catch (const std::exception & e) { has_faces=false; }


    file.read(ss);

    // std::cout << "Reading took " << read_timer.get() / 1000.f << " seconds." << std::endl;
    // if (vertices) std::cout << "\tRead " << vertices->count << " total vertices "<< std::endl;
    // if (normals) std::cout << "\tRead " << normals->count << " total vertex normals " << std::endl;
    // if (texcoords) std::cout << "\tRead " << texcoords->count << " total vertex texcoords " << std::endl;
    // if (faces) std::cout << "\tRead " << faces->count << " total faces (triangles) " << std::endl;

    // // type casting to your own native types - Option A
    // {
    //     const size_t numVerticesBytes = vertices->buffer.size_bytes();
    //     std::vector<float3> verts(vertices->count);
    //     std::memcpy(verts.data(), vertices->buffer.get(), numVerticesBytes);
    // }

    // type casting to your own native types - Option B
    typedef Eigen::Matrix<unsigned char,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXuc;
    typedef Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXf;
    typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXd;
    typedef Eigen::Matrix<unsigned,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXi;

    //parse data
    //vertices
    if (vertices->t == tinyply::Type::FLOAT32) {
        Eigen::Map<RowMatrixXf> mf( (float*)vertices->buffer.get(), vertices->count, 3);
        V=mf.cast<double>();
    }else if(vertices->t == tinyply::Type::FLOAT64){
        Eigen::Map<RowMatrixXd> mf( (double*)vertices->buffer.get(), vertices->count, 3);
        V=mf.cast<double>();
    }else{ LOG(FATAL) <<" vertex parsing other than float and double not implemented yet"; }
    //normals
    if (has_vertex_normals) {
        if (normals->t == tinyply::Type::FLOAT32) {
            Eigen::Map<RowMatrixXf> mf( (float*)normals->buffer.get(), normals->count, 3);
            NV=mf.cast<double>();
        }else if(normals->t == tinyply::Type::FLOAT64){
            Eigen::Map<RowMatrixXd> mf( (double*)normals->buffer.get(), vertices->count, 3);
            NV=mf.cast<double>();
        }else{ LOG(FATAL) <<"normals parsing other than float not implemented yet"; }
    }
    // texcoords
    if (has_texcoords){
        if (texcoords->t == tinyply::Type::FLOAT32) {
            Eigen::Map<RowMatrixXf> mf( (float*)texcoords->buffer.get(), texcoords->count, 2);
            UV=mf.cast<double>();
        }else{ LOG(FATAL) <<"texcoords parsing other than float not implemented yet"; }
    }
    //color
    if (has_color){
        if (color->t == tinyply::Type::UINT8) {
            Eigen::Map<RowMatrixXuc> mf( (unsigned char*)color->buffer.get(), color->count, 3);
            C=mf.cast<double>();
            C=C.array()/255.0;
            // C=C.array();
        }else if (color->t == tinyply::Type::FLOAT32) {
            Eigen::Map<RowMatrixXf> mf( (float*)color->buffer.get(), color->count, 3);
            C=mf.cast<double>();
        }else{ LOG(FATAL) <<"color parsing other than unsigned char and float not implemented yet"; }
    }
    //faces
    if (has_faces){
        if (faces->t == tinyply::Type::INT32) {
            Eigen::Map<RowMatrixXi> mf( (unsigned int*)faces->buffer.get(), faces->count, 3);
            F=mf.cast<int>();
        }else if (faces->t == tinyply::Type::UINT32) {
            Eigen::Map<RowMatrixXi> mf( (unsigned int*)faces->buffer.get(), faces->count, 3);
            F=mf.cast<int>();
        }else{ LOG(FATAL) <<"We assume that the faces are integers or unsigned integers but for some reason they are not"; }
    }

    //set some sensible visualization values
    if (!has_faces){
        m_vis.m_show_mesh=false;
    }
    if(has_color){
        m_vis.set_color_pervertcolor();
    }






    //doubles
    // if (vertices->t == tinyply::Type::FLOAT64) {
    //     Eigen::Map<RowMatrixXd> mf( (double*)vertices->buffer.get(), vertices->count, 3);
    //     V=mf.cast<double>();
    // }



}

void Mesh::write_ply(const std::string file_path){

    std::filebuf fb_binary;
	fb_binary.open(file_path, std::ios::out | std::ios::binary);
    std::ostream outstream_binary(&fb_binary);
    CHECK(!outstream_binary.fail()) << "Failed to open for writing " << file_path;


    tinyply::PlyFile ply_file;

    //get all the matrices to row major float
    typedef Eigen::Matrix<float,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXf;
    typedef Eigen::Matrix<unsigned,Eigen::Dynamic,Eigen::Dynamic,Eigen::RowMajor> RowMatrixXi;
    RowMatrixXf V_f=V.cast<float>();
    RowMatrixXi F_i=F.cast<unsigned>();
    RowMatrixXf C_f=C.cast<float>();
    RowMatrixXf NV_f=NV.cast<float>();
    RowMatrixXf UV_f=UV.cast<float>();

    //vertices
    ply_file.add_properties_to_element("vertex", { "x", "y", "z" },  tinyply::Type::FLOAT32, V_f.rows(), reinterpret_cast<uint8_t*>(V_f.data()), tinyply::Type::INVALID, 0);

    //normals
    if(NV.size()){
        ply_file.add_properties_to_element("vertex", { "nx", "ny", "nz" }, tinyply::Type::FLOAT32, NV_f.rows(), reinterpret_cast<uint8_t*>(NV_f.data()), tinyply::Type::INVALID, 0);
    }

    //texcoords
    if(UV.size()){
        ply_file.add_properties_to_element("vertex", { "u", "v" }, tinyply::Type::FLOAT32, UV_f.rows(), reinterpret_cast<uint8_t*>(UV_f.data()), tinyply::Type::INVALID, 0);
    }

    //color
    if(C.size()){
        ply_file.add_properties_to_element("vertex", { "red", "green", "blue" }, tinyply::Type::FLOAT32, C_f.rows(), reinterpret_cast<uint8_t*>(C_f.data()), tinyply::Type::INVALID, 0);
    }

    //faces
    if(F.size()){
        CHECK(F.cols()==3) << "We only support writing of triangles right now";
        ply_file.add_properties_to_element("face", { "vertex_indices" }, tinyply::Type::UINT32, F_i.rows(), reinterpret_cast<uint8_t*>(F_i.data()), tinyply::Type::UINT8, 3);
    }


    ply_file.get_comments().push_back("generated by tinyply 2.2");

	// Write a binary file
    ply_file.write(outstream_binary, true);
}

void Mesh::read_obj(const std::string file_path,  bool load_vti, bool load_vni){

    struct Vertex{
        Eigen::Vector3d pos=Eigen::Vector3d::Zero();
        Eigen::Vector3d normal=Eigen::Vector3d::Zero();
        Eigen::Vector2d tex_coord=Eigen::Vector2d::Zero();
        Eigen::Vector3d color=Eigen::Vector3d::Zero();
        bool operator==(const Vertex& other) const {
            return pos == other.pos && normal==other.normal && tex_coord == other.tex_coord && color == other.color ;
        }
    };
    //has function for vertex
    struct HashVertex{
        std::size_t operator()(Vertex const& vertex) const noexcept{
            return ((MatrixHash<Eigen::Vector3d>{}(vertex.pos) ^
                   (MatrixHash<Eigen::Vector3d>{}(vertex.normal) << 1)) >> 1) ^
                   (MatrixHash<Eigen::Vector2d>{}(vertex.tex_coord) << 1);
        }
    };



    //attempt 2
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    VLOG(1) << "read obj with path " << file_path;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, file_path.c_str());

    LOG_IF(WARNING, shapes.size()!=1) << "Warning when loading obj with path " << file_path << " " << warn;
    LOG_IF(FATAL, !err.empty()) << "Failed to load obj with path " << file_path << " " << err;
    LOG_IF(FATAL, !ret) << "Failed to load obj with path: " << file_path;

    //check if it has normals, tex coords or color
    bool has_normals, has_tex_coords, has_colors;
    has_normals=attrib.normals.size()!=0;
    has_tex_coords=attrib.texcoords.size()!=0;
    // has_colors=attrib.colors.size()!=0;
    has_colors=false; //For some reason tinyobj always reads a color of 1,1,1 even though there is no coler in the obj file..
    // VLOG(1) << "atrib colors is " << attrib.colors.size();



    std::vector<Vertex> vertices;
    std::unordered_map<Vertex, uint32_t, HashVertex> unique_vertices = {};
    std::vector<int> indices;
    std::vector<int> vti;
    std::vector<int> vni;

    // if (!check_uniqueness){ //if we don't check for uniqunes we push the vertices in whichever order they appear in the obj and not in the order to which faces reference them
        // vertices.resize( attrib.vertices.size()/3 );
    // }

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            // VLOG(1) << "index has texoocrds index " << index.texcoord_index;

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if(has_normals){
            vertex.normal = {
                attrib.normals[3 * index.normal_index + 0],
                attrib.normals[3 * index.normal_index + 1],
                attrib.normals[3 * index.normal_index + 2]
            };
            }

            if (has_tex_coords){
                vertex.tex_coord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }

            // //there is no index for the color so we use the vertex_index
            // if (has_colors){
            //     vertex.color = {
            //         attrib.colors[3 * index.vertex_index + 0],
            //         attrib.colors[3 * index.vertex_index + 1],
            //         attrib.colors[3 * index.vertex_index + 2]
            //     };
            // }

            //push the vertex only if it's the first one we references
            // if (check_uniqueness){
                if (unique_vertices.count(vertex) == 0) {
                    unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }
                indices.push_back(unique_vertices[vertex]);
            // }else{
                // vertices[index.vertex_index]=vertex;
                // indices.push_back(index.vertex_index);
            // }

            if(load_vti){
                // vti.push_back(index.texcoord_index);
                vti.push_back(indices[indices.size()-1] );
            }
            if(load_vni){
                // vni.push_back(index.normal_index);
                // vni.push_back(index.normal_index);
                vni.push_back(indices[indices.size()-1] );
            }

            // vertices.push_back(vertex);
            // indices.push_back(indices.size());
        }
    }

    //push the vertices into  V, NV and so on
    V.resize(vertices.size(),3);
    if (has_normals) { NV.resize(vertices.size(),3); };
    if (has_tex_coords) { UV.resize(vertices.size(),2); };
    if (has_colors) { C.resize(vertices.size(),3); };

    for(size_t i=0; i<vertices.size(); i++){
        V.row(i) = vertices[i].pos;
        if (has_normals) { NV.row(i) = vertices[i].normal; }
        if (has_tex_coords) { UV.row(i) = vertices[i].tex_coord; }
        if (has_colors) { C.row(i) = vertices[i].color; }
    }

    F.resize(indices.size()/3, 3);
    for(size_t i=0; i<indices.size()/3; i++){
        F.row(i) << indices[3*i+0], indices[3*i+1], indices[3*i+2];
    }

    //vti and vni
    if (load_vti){
        VTI.resize(vti.size()/3, 3);
        for(size_t i=0; i<vti.size()/3; i++){
            VTI.row(i) << vti[3*i+0], vti[3*i+1], vti[3*i+2];
        }
    }
    if (load_vni){
        VNI.resize(vni.size()/3, 3);
        for(size_t i=0; i<vni.size()/3; i++){
            VNI.row(i) << vni[3*i+0], vni[3*i+1], vni[3*i+2];
        }
    }


}


void Mesh::sanity_check() const{
    // LOG_IF_S(ERROR, F.rows()!=NF.rows()) << name << ": F and NF don't coincide in size, they are " << F.rows() << " and " << NF.rows(); // no need to check for this as I actually don't usually use NF
    if (NV.size()) LOG_IF_S(ERROR, V.rows()!=NV.rows() && F.size()) << name << ": V and NV don't coincide in size, they are " << V.rows() << " and " << NV.rows();
    LOG_IF_S(ERROR, V.size() && D.size() && V.rows()!=D.rows() ) << name << ": V and D don't coincide " << V.rows() << " and " << D.rows();
    LOG_IF_S(ERROR, V.size() && I.size() && V.rows()!=I.rows() ) << name << ": V and I don't coincide " << V.rows() << " and " << I.rows();
    LOG_IF_S(ERROR, V.size() && C.size() && V.rows()!=C.rows() ) << name << ": V and C don't coincide " << V.rows() << " and " << C.rows();
    LOG_IF_S(ERROR, V.size() && UV.size() && V.rows()!=UV.rows() ) << name << ": V and UV don't coincide " << V.rows() << " and " << UV.rows();
    if (F.size()) LOG_IF_S(ERROR, F.maxCoeff()>V.rows()-1) << name << ": F indexes V at invalid poisitions, max coeff is " << F.maxCoeff() << " and V size is" << V.rows();
    if (E.size()) LOG_IF_S(ERROR, E.maxCoeff()>V.rows()-1) << name << ": E indexes V at invalid poisitions, max coeff is " << E.maxCoeff() << " and V size is" << V.rows() ;
    if (C.size()) LOG_IF_S(ERROR, C.rows()!=V.rows() ) << name << ": We have per vertex color but C and V don't match. C has rows " << C.rows() << " and V size is" << V.rows() ;
    LOG_IF_S(ERROR, S_pred.size() && S_pred.rows()!=V.rows() ) << name << ": S_pred and V don't coincide in size, they are " << S_pred.rows() << " and " << V.rows();
    LOG_IF_S(ERROR, L_pred.size() && L_pred.rows()!=V.rows() ) << name << ": L_pred and V don't coincide in size, they are " << L_pred.rows() << " and " << V.rows();
    LOG_IF_S(ERROR, L_gt.size() && L_gt.rows()!=V.rows() ) << name << ": L_gt and V don't coincide in size, they are " << L_gt.rows() << " and " << V.rows();
}

std::ostream& operator<<(std::ostream& os, const Mesh& m)
{
    os << "\n";

    m.name==""?  os << "\t name is empty \n"    :   os << "\t name: " << m.name << "\n";
    os << "\t m_width: " << m.m_width << "\n";
    os << "\t m_height: " << m.m_height << "\n";
    os << "\t t: " << m.t << "\n";

    m.V.size()?  os << "\t V has size: " << m.V.rows() << " x " << m.V.cols() << "\n"   :   os << "\t V is empty \n";
    m.F.size()?  os << "\t F has size: " << m.F.rows() << " x " << m.F.cols() << "\n"   :   os << "\t F is empty \n";
    m.C.size()?  os << "\t C has size: " << m.C.rows() << " x " << m.C.cols() << "\n"   :   os << "\t C is empty \n";
    m.E.size()?  os << "\t E has size: " << m.E.rows() << " x " << m.E.cols() << "\n"   :   os << "\t E is empty \n";
    m.V_tangent_u.size()?  os << "\t V_tangent_u has size: " << m.V_tangent_u.rows() << " x " << m.V_tangent_u.cols() << "\n"   :   os << "\t V_tangent_u is empty \n";
    m.V_length_v.size()?  os << "\t V_lenght_v has size: " << m.V_length_v.rows() << " x " << m.V_length_v.cols() << "\n"   :   os << "\t V_lenght_v is empty \n";
    m.D.size()?  os << "\t D has size: " << m.D.rows() << " x " << m.D.cols() << "\n"   :   os << "\t D is empty \n";
    m.L_pred.size()?  os << "\t L_pred has size: " << m.L_pred.rows() << " x " << m.L_pred.cols() << "\n"   :   os << "\t L_pred is empty \n";
    m.L_gt.size()?  os << "\t L_gt has size: " << m.L_gt.rows() << " x " << m.L_gt.cols() << "\n"   :   os << "\t L_gt is empty \n";
    m.I.size()?  os << "\t I has size: " << m.I.rows() << " x " << m.I.cols() << "\n"   :   os << "\t I is empty \n";
    m.NF.size()?  os << "\t NF has size: " << m.NF.rows() << " x " << m.NF.cols() << "\n"   :   os << "\t NF is empty \n";
    m.NV.size()?  os << "\t NV has size: " << m.NV.rows() << " x " << m.NV.cols() << "\n"   :   os << "\t NV is empty \n";


    m.F.size()?  os << "\t F min max coeffs: " << m.F.minCoeff() << " " << m.F.maxCoeff() << "\n"   :   os << "\t F is empty \n";
    m.E.size()?  os << "\t E min max coeffs: " << m.E.minCoeff() << " " << m.E.maxCoeff() << "\n"   :   os << "\t E is empty \n";



    return os;
}


} //namespace easy_pbr
