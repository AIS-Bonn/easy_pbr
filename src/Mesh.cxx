#include "easy_pbr/Mesh.h"

//c++
#include <iostream>
#include <algorithm>
#include <unordered_map> //for deduplicating obj vertices

//my stuff
// #include "MiscUtils.h"
#include "easy_pbr/LabelMngr.h"

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

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
#include "tiny_obj_loader.h"

//for reading pcd files
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
// #include <sensor_msgs/PointCloud2.h>

#include "nanoflann.hpp"

#include "RandGenerator.h"
#include "ColorMngr.h"
#include "numerical_utils.h"
#include "eigen_utils.h"
#include "string_utils.h"
using namespace easy_pbr::utils;

//boost
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

Mesh::Mesh():
        id(0),
        m_is_dirty(true),
        m_model_matrix(Eigen::Affine3d::Identity()),
        m_cur_pose(Eigen::Affine3d::Identity()),
        m_width(0),
        m_height(0),
        m_view_direction(-1),
        m_force_vis_update(false),
        m_rand_gen(new RandGenerator())
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
    cloned.m_diffuse_mat.is_dirty=true;
    cloned.m_metalness_mat.is_dirty=true;
    cloned.m_roughness_mat.is_dirty=true;
    cloned.m_normals_mat.is_dirty=true;
    cloned.m_label_mngr=m_label_mngr; //this is just a shallow copy!
    cloned.t=t;
    cloned.id=id;
    cloned.m_height=m_height;
    cloned.m_width=m_width;
    cloned.m_view_direction=m_view_direction;
    cloned.m_min_max_y=m_min_max_y;
    cloned.m_min_max_y_for_plotting=m_min_max_y_for_plotting;

    return cloned;
}


void Mesh::add(const Mesh& new_mesh) {

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
    Eigen::MatrixXd V_tangent_u_new(V_tangent_u.rows() + new_mesh.V_tangent_u.rows(), 4);
    V_tangent_u_new << V_tangent_u, new_mesh.V_tangent_u;
    Eigen::MatrixXd V_lenght_v_new(V_length_v.rows() + new_mesh.V_length_v.rows(), 4);
    V_lenght_v_new << V_length_v, new_mesh.V_length_v;
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
    L_pred=L_pred_new;
    L_gt=L_gt_new;
    I=I_new;


    m_is_dirty=true;
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
    L_pred.resize(0,0);
    L_gt.resize(0,0);
    I.resize(0,0);

    m_min_max_y.setZero();
    m_min_max_y_for_plotting.setZero();

    m_is_dirty=true;
}


void Mesh::clear_C() {
    Eigen::MatrixXd C_empty;
    C = C_empty;
    m_is_dirty=true;

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
}

void Mesh::transform_model_matrix(const Eigen::Affine3d& trans){
    m_model_matrix=trans*m_model_matrix;

    // update all children
    for (size_t i = 0; i < m_child_meshes.size(); i++){
        MeshSharedPtr child=m_child_meshes[i];
        child->transform_model_matrix(trans);
    }
}

void Mesh::translate_model_matrix(const Eigen::Vector3d& translation){
    Eigen::Affine3d tf;
    tf.setIdentity();

    tf.translation()=translation;
    transform_model_matrix(tf);
}

void Mesh::rotate_model_matrix(const Eigen::Vector3d& axis, const float angle_degrees){
    Eigen::Quaterniond q = Eigen::Quaterniond( Eigen::AngleAxis<double>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );

    Eigen::Affine3d tf;
    tf.setIdentity();

    tf.linear()=q.toRotationMatrix();
    transform_model_matrix(tf);
}

void Mesh::rotate_model_matrix_local(const Eigen::Vector3d& axis, const float angle_degrees){
    Eigen::Quaterniond q = Eigen::Quaterniond( Eigen::AngleAxis<double>( angle_degrees * M_PI / 180.0 ,  axis.normalized() ) );

    Eigen::Affine3d rot;
    rot.setIdentity();

    rot.linear()=q.toRotationMatrix();

    Eigen::Affine3d tf=Eigen::Translation3d(m_model_matrix.translation()) * rot *  Eigen::Translation3d(-m_model_matrix.translation());

    transform_model_matrix(tf);
}

void Mesh::rotate_model_matrix_local(const Eigen::Quaterniond& q){
    Eigen::Affine3d rot;
    rot.setIdentity();

    rot.linear()=q.toRotationMatrix();

    Eigen::Affine3d tf=Eigen::Translation3d(m_model_matrix.translation()) * rot *  Eigen::Translation3d(-m_model_matrix.translation());

    transform_model_matrix(tf);
}

void Mesh::apply_model_matrix_to_cpu(){
    transform_vertices_cpu(m_model_matrix);
    m_model_matrix.setIdentity();
}








void Mesh::recalculate_normals(){
    if(!F.size()){
        return; // we have no faces so there will be no normals
    }
    CHECK(V.size()) << named("V is empty");
    igl::per_face_normals(V,F,NF);
    igl::per_vertex_normals(V,F, igl::PerVertexNormalsWeightingType::PER_VERTEX_NORMALS_WEIGHTING_TYPE_ANGLE, NF, NV);
    m_is_dirty=true;

}

void Mesh::load_from_file(const std::string file_path){

    std::string file_path_abs;
    if (fs::path(file_path).is_relative()){
        file_path_abs=(fs::path(PROJECT_SOURCE_DIR) / file_path).string();
    }else{
        file_path_abs=file_path;
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
    }else{
        LOG(WARNING) << "Not a known extension of mesh file: " << file_path_abs;
    }

    //set some sensible things to see 
    if(!F.size()){
        m_vis.m_show_points=true;
        m_vis.m_show_mesh=false;
    }
    if(C.size()){
        m_vis.set_color_pervertcolor();
    }

    //calculate the min and max y which will be useful for coloring
    m_min_max_y(0)=V.col(1).minCoeff();
    m_min_max_y(1)=V.col(1).maxCoeff();
    m_min_max_y_for_plotting=m_min_max_y;

    m_is_dirty=true;

    m_disk_path=file_path_abs;

}

void Mesh::save_to_file(const std::string file_path){

    //in the case of surfels, surfels that don't actually have an extent should be removed from the mesh, so we just set the coresponsing vertex to 0.0.0
    if(V_tangent_u.rows()==V.rows() && V_length_v.rows()==V.rows() && m_vis.m_show_surfels){
        for(int i = 0; i < V_tangent_u.rows(); i++){
            if(V_tangent_u.row(i).isZero() || V_length_v.row(i).isZero() ){
                V.row(i).setZero();
            }
        }
    }
    m_is_dirty=true;

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

}

void Mesh::remove_duplicate_vertices(){

    Eigen::MatrixXd V_new;
    Eigen::MatrixXi indirection; //size of mesh.V, says for each point where they ended up in the V_new array
    Eigen::MatrixXi inverse_indirection; // size of V_new , says for each point where the original one was in mesh.V

    igl::remove_duplicate_vertices(V, 1e-7, V_new, indirection, inverse_indirection);
    
    V=V_new;
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
        std::cout << "is_edge_manifold is " << is_edge_manifold << '\n';
        remove_marked_vertices(is_vertex_non_manifold_original_mesh, false);
    }


    //decimate it
    Eigen::VectorXi I;
    Eigen::VectorXi J;
    igl::qslim(V, F, nr_target_faces, V, F, J,I);

    recalculate_normals(); //we completely changed the V and F so we might as well just recompute NV and NF

    m_is_dirty=true;
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
            std::cout << "non manifold around the 3 faces (v_idx, v_idx, f_idx, e_idx)"  << std::endl;
            std::cout << " edge_1 " << r1[0] << " " << r1[1] << " " << r1[2] << " " << r1[3] << std::endl;
            std::cout << " edge_2 " << r2[0] << " " << r2[1] << " " << r2[2] << " " << r2[3] << std::endl;
            std::cout << " edge_3 " << r3[0] << " " << r3[1] << " " << r3[2] << " " << r3[3] << std::endl;
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

void Mesh::flip_normals(){
    NF=-NF;
    NV=-NV;

    m_is_dirty=true;
}

void Mesh::normalize_size(){
    CHECK(V.size()) << name << "We cannot normalize_size an empty cloud";

    Eigen::VectorXd max= V.colwise().maxCoeff();
    Eigen::VectorXd min= V.colwise().minCoeff();
    double size_diff=(max-min).norm();
    V.array()/=size_diff;
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



void Mesh::create_full_screen_quad(){
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
 

    name="grid_floor";
    m_vis.m_show_lines=true;
    m_vis.m_line_color<<0.6, 0.6, 0.6;
    m_vis.m_show_mesh=false;

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

Eigen::Vector3d Mesh::centroid(){
    Eigen::Vector3d min_point = V.colwise().minCoeff(); 
    Eigen::Vector3d max_point = V.colwise().maxCoeff();
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
    for (size_t i = 0; i < F.rows(); i++) {
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

    for (size_t i = 0; i < V.rows(); i++) {
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
    if(is_empty()){
       return 1.0; 
    }
    // VLOG(1) << "get scale";


    // Eigen::VectorXd min_point; // each row stores the minimum point of the corresponding mesh. 
    // Eigen::VectorXd max_point; // each row stores the minimum point of the corresponding mesh. 
    // min_point.resize(3);
    // max_point.resize(3);
    // min_point.setConstant(std::numeric_limits<float>::max());
    // max_point.setConstant(std::numeric_limits<float>::lowest());
    Eigen::VectorXd min_point = V.colwise().minCoeff();   
    Eigen::VectorXd max_point = V.colwise().maxCoeff();   

    Eigen::VectorXd centroid = (0.5*(min_point + max_point)).eval();

    float scale = (max_point-min_point).array().abs().maxCoeff();

    return scale;

}

void Mesh::color_solid2pervert(){
    C.resize(V.rows(),3);

    for(int i=0; i<V.rows(); i++){
        C.row(i)=m_vis.m_solid_color.cast<double>();
    }

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


void Mesh::set_diffuse_tex(const std::string file_path){
    cv::Mat mat = cv::imread(file_path);
    cv::flip(mat, m_diffuse_mat.mat, 0);
    m_diffuse_mat.is_dirty=true;
}
void Mesh::set_metalness_tex(const std::string file_path){
    cv::Mat mat = cv::imread(file_path);
    cv::flip(mat, m_metalness_mat.mat, 0);
    m_metalness_mat.is_dirty=true;
}
void Mesh::set_roughness_tex(const std::string file_path){
    cv::Mat mat = cv::imread(file_path);
    cv::flip(mat, m_roughness_mat.mat, 0);
    m_roughness_mat.is_dirty=true;
}
void Mesh::set_normals_tex(const std::string file_path){
    cv::Mat mat = cv::imread(file_path);
    cv::flip(mat, m_normals_mat.mat, 0);
    m_normals_mat.is_dirty=true;
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

void Mesh::read_obj(const std::string file_path){

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

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

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
            if (unique_vertices.count(vertex) == 0) {
                unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(unique_vertices[vertex]);

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

}


void Mesh::sanity_check() const{
    // LOG_IF_S(ERROR, F.rows()!=NF.rows()) << name << ": F and NF don't coincide in size, they are " << F.rows() << " and " << NF.rows(); // no need to check for this as I actually don't usually use NF
    LOG_IF_S(ERROR, V.rows()!=NV.rows() && F.size()) << name << ": V and NV don't coincide in size, they are " << V.rows() << " and " << NV.rows();
    LOG_IF_S(ERROR, V.size() && D.size() && V.rows()!=D.rows() ) << name << ": V and D don't coincide " << V.rows() << " and " << D.rows();
    LOG_IF_S(ERROR, V.size() && I.size() && V.rows()!=I.rows() ) << name << ": V and I don't coincide " << V.rows() << " and " << I.rows();
    if (F.size()) LOG_IF_S(ERROR, F.maxCoeff()>V.rows()-1) << name << ": F indexes V at invalid poisitions, max coeff is " << F.maxCoeff() << " and V size is" << V.rows();
    if (E.size()) LOG_IF_S(ERROR, E.maxCoeff()>V.rows()-1) << name << ": E indexes V at invalid poisitions, max coeff is " << E.maxCoeff() << " and V size is" << V.rows() ;
    if (C.size()) LOG_IF_S(ERROR, C.rows()!=V.rows() ) << name << ": We have per vertex color but C and V don't match. C has rows " << C.rows() << " and V size is" << V.rows() ;
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
 
