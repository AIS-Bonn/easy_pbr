#include "easy_pbr/Scene.h"

//c++

//my stuff
#define LOGURU_REPLACE_GLOG 1
#include <loguru.hpp>


//redeclared things here so we can use them from this file even though they are static
std::vector<MeshSharedPtr>  Scene::m_meshes;
std::mutex Scene::m_mesh_mutex;

Scene::Scene()    
{

}

void Scene::show(const std::shared_ptr<Mesh> mesh, const std::string name){

    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe

    //check if there is already a mesh with the same name 
    bool found=false;
    int idx_found=-1;
    for(size_t i = 0; i < m_meshes.size(); i++){
        if(m_meshes[i]->name==name){
            found=true;
            idx_found=i;
        }
    }

    if(found){
        m_meshes[idx_found]=mesh; //it's a shared ptr so it just gets asigned to this one and the previous one dissapears
        m_meshes[idx_found]->name=name;
        // m_meshes[idx_found]->recalculate_normals();
    }else{
        m_meshes.push_back(mesh);
        m_meshes.back()->name=name;
        m_meshes.back()->recalculate_normals();
    }


    //if that was the first mesh that was added, add also a grid for the ground 
    if(m_meshes.size()==1 && !m_meshes.back()->is_empty()){

        // MeshSharedPtr mesh_grid=create_grid(8, mesh->V.col(1).minCoeff());
        MeshSharedPtr mesh_grid=create_floor(mesh->V.col(1).minCoeff());
        m_meshes.push_back(mesh_grid);

    }
    
}

void Scene::show(const Mesh& mesh, const std::string name){
    show(std::make_shared<Mesh>(mesh), name);
}


void Scene::add_mesh(const std::shared_ptr<Mesh> mesh, const std::string name){

    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe

    m_meshes.push_back(mesh);
    m_meshes.back()->name=name;
    m_meshes.back()->recalculate_normals();

    //if that was the first mesh that was added, add also a grid for the ground 
    if(m_meshes.size()==1 && !m_meshes.back()->is_empty()){

        // MeshSharedPtr mesh_grid=Mesh::create();
        // MeshSharedPtr mesh_grid=create_grid(8, mesh->V.col(1).minCoeff());
        MeshSharedPtr mesh_grid=create_floor(mesh->V.col(1).minCoeff());
        m_meshes.push_back(mesh_grid);
      
    }
    
}

void Scene::clear(){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    m_meshes.clear();
}

int Scene::get_nr_meshes(){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    return m_meshes.size();
}

int Scene::get_total_nr_vertices(){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    int V_nr=0;
    for (size_t i = 0; i < m_meshes.size(); i++) {
        if(m_meshes[i]->m_vis.m_is_visible){
            V_nr+=m_meshes[i]->V.rows();
        }
    }
    return V_nr;
}

int Scene::get_total_nr_faces(){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    int F_nr=0;
    for (size_t i = 0; i < m_meshes.size(); i++) {
        if(m_meshes[i]->m_vis.m_is_visible){
            F_nr+=m_meshes[i]->F.rows();
        }
    }
    return F_nr;
}

std::shared_ptr<Mesh> Scene::get_mesh_with_name(const std::string name){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    for (size_t i = 0; i < m_meshes.size(); i++) {
        if(m_meshes[i]->name==name){
            return m_meshes[i];
        }
    }
    LOG_S(FATAL) << "No mesh with name " << name;
    return m_meshes[0]; //HACK because this line will never occur because the previous line will kill it but we just put it to shut up the compiler warning
}

std::shared_ptr<Mesh> Scene::get_mesh_with_idx(const unsigned int idx){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    if(idx<m_meshes.size()){
        return m_meshes[idx];
    }else{
        LOG_S(FATAL) << "No mesh with idx " << idx;
        return m_meshes[0]; //HACK because this line will never occur because the previous line will kill it but we just put it to shut up the compiler warning
    }
}

int Scene::get_idx_for_name(const std::string name){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    for (size_t i = 0; i < m_meshes.size(); i++) {
        if(m_meshes[i]->name==name){
            return i;
        }
    }
    LOG_S(FATAL) << "No mesh with name " << name;
    return -1; //HACK because this line will never occur because the previous line will kill it but we just put it to shut up the compiler warning
}

bool Scene::does_mesh_with_name_exist(const std::string name){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe
    for (size_t i = 0; i < m_meshes.size(); i++) {
        if(m_meshes[i]->name==name){
            return true;
        }
    }
    return false;
}

void Scene::remove_meshes_starting_with_name(const std::string name_prefix){
    std::lock_guard<std::mutex> lock(m_mesh_mutex);  // so that accesed to the map are thread safe

    std::vector< std::shared_ptr<Mesh> > meshes_filtered;

    for(int i=0; i<m_meshes.size(); i++){
        VLOG(1) << "checking mesh with name " << m_meshes[i]->name;
        if(! m_meshes[i]->name.find(name_prefix, 0) == 0){
            VLOG(1) << "Keeping mesh " << m_meshes[i]->name;
            //mesh does not have the prefix, we keep it
            meshes_filtered.push_back(m_meshes[i]);
        }else{
            VLOG(1) << "Removing mesh " << m_meshes[i]->name;
        }

    }

    m_meshes=meshes_filtered;

}

// void Scene::update(const Mesh& mesh, const std::string name){
//     if(does_mesh_with_name_exist(name)){
//         Mesh& to_be_updated=get_mesh_with_name(name);
//         to_be_updated=mesh;
//         to_be_updated.name=name;
//         to_be_updated.m_is_dirty=true;
//         recalculate_normals
//     }else{
//         add_mesh(mesh, name); //doesn't exist, add it to the scene
//     }
// }


Eigen::Vector3f Scene::get_centroid(){

    //if the scene is empty just return the 0.0.0 
    if(is_empty()){
        return Eigen::Vector3f::Zero();
    }


    Eigen::MatrixXd min_point_per_mesh; // each row stores the minimum point of the corresponding mesh. 
    Eigen::MatrixXd max_point_per_mesh; // each row stores the minimum point of the corresponding mesh. 
    min_point_per_mesh.resize(m_meshes.size(), 3);
    max_point_per_mesh.resize(m_meshes.size(), 3);
    min_point_per_mesh.setZero();
    max_point_per_mesh.setZero();
    for(size_t i=0; i<m_meshes.size(); i++){
        if(m_meshes[i]->is_empty()){ continue; }  
        min_point_per_mesh.row(i) = m_meshes[i]->V.colwise().minCoeff();   
        max_point_per_mesh.row(i) = m_meshes[i]->V.colwise().maxCoeff();   
    }

    //absolute minimum between all meshes
    Eigen::Vector3d min_point = min_point_per_mesh.colwise().minCoeff(); 
    Eigen::Vector3d max_point = max_point_per_mesh.colwise().maxCoeff();
    Eigen::Vector3d centroid = (0.5*(min_point + max_point)).eval();

    return centroid.cast<float>();
}
  
float Scene::get_scale(){

    //if the scene is empty just return the scale 1.0 
    if(is_empty()){
       return 1.0; 
    }


    Eigen::MatrixXd min_point_per_mesh; // each row stores the minimum point of the corresponding mesh. 
    Eigen::MatrixXd max_point_per_mesh; // each row stores the minimum point of the corresponding mesh. 
    min_point_per_mesh.resize(m_meshes.size(), 3);
    max_point_per_mesh.resize(m_meshes.size(), 3);
    min_point_per_mesh.setConstant(std::numeric_limits<float>::max());
    max_point_per_mesh.setConstant(std::numeric_limits<float>::lowest());
    for(size_t i=0; i<m_meshes.size(); i++){
        if(m_meshes[i]->is_empty()){ continue; }  
        if(m_meshes[i]->name=="grid_floor"){
            continue;
        }
        min_point_per_mesh.row(i) = m_meshes[i]->V.colwise().minCoeff();   
        max_point_per_mesh.row(i) = m_meshes[i]->V.colwise().maxCoeff();   
    }

    //absolute minimum between all meshes
    Eigen::Vector3d min_point = min_point_per_mesh.colwise().minCoeff(); 
    Eigen::Vector3d max_point = max_point_per_mesh.colwise().maxCoeff();
    Eigen::Vector3d centroid = (0.5*(min_point + max_point)).eval();

    // VLOG(1) << "min_point " << min_point.transpose();
    // VLOG(1) << "max_point " << max_point.transpose();
    float scale = (max_point-min_point).array().abs().maxCoeff();

    // VLOG(1) << "scale is " << scale;

    return scale;
}

std::shared_ptr<Mesh> Scene::create_grid(const int nr_segments, const float y_pos){

    int nr_segments_even= round(nr_segments / 2) * 2; // so we have an even number of segments on each side and then one running thgou th emiddle
    int half_size=nr_segments_even/2;
    int nr_points_per_side=nr_segments_even+1;// the +1 is because we will have 3 lines if we choose 2 segments, we have to consider the one that runs through the middle of the scene
    MeshSharedPtr mesh_grid=MeshCreate();
    mesh_grid->V.resize( (nr_points_per_side)*(nr_points_per_side), 3 ); 
    int idx=0;
    for(int x=-half_size; x<=half_size; x++){
        for(int y=-half_size; y<=half_size; y++){
            mesh_grid->V.row(idx) << x, 0, y;
            idx++;
        }
    }
    //make edges horizontally
    int nr_edges=half_size*2*nr_points_per_side*2; // we will have half_size*2 for each line, and we have nr_points_per_size lines and since we both hotiz and ertical we multiply by 2
    mesh_grid->E.resize( nr_edges, 2 ); 
    idx=0;
    int idx_prev_point=-1;
    for(int y=-half_size; y<=half_size; y++){
        for(int x=-half_size; x<=half_size; x++){
            int idx_cur_point=(y+half_size)*nr_points_per_side + x + half_size;
            if(idx_prev_point!=-1){
                mesh_grid->E.row(idx) << idx_prev_point, idx_cur_point;
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
                mesh_grid->E.row(idx) << idx_prev_point, idx_cur_point;
                idx++;
            }

            idx_prev_point=idx_cur_point;
        }
        idx_prev_point=-1; //we start a new row so invalidate the previous one so we dont connect with the points in the row above
    } 

    //scale it to be in range [-1, 1]
    mesh_grid->V.array()/=half_size;
    //scale to be in the range of the mesh 
    mesh_grid->V.array()*=get_scale();

    //find the minimal point in y of the mesh so we put the grid there 
    Eigen::Affine3d trans=Eigen::Affine3d::Identity();
    Eigen::Vector3d t;
    t<< 0, y_pos, 0;
    trans.translate(t);
    mesh_grid->apply_transform(trans,true);
 

    mesh_grid->name="grid_floor";
    mesh_grid->m_vis.m_show_lines=true;
    mesh_grid->m_vis.m_line_color<<0.6, 0.6, 0.6;
    return mesh_grid;

}


std::shared_ptr<Mesh> Scene::create_floor(const float y_pos){

    MeshSharedPtr mesh_floor=MeshCreate();

    //make 4 vertices
    mesh_floor->V.resize( 4, 3 ); 
    mesh_floor->V.row(0) << -1, 0, -1;
    mesh_floor->V.row(1) << 1, 0, -1;
    mesh_floor->V.row(2) << 1, 0, 1;
    mesh_floor->V.row(3) << -1, 0, 1;

    //make 2 faces
    mesh_floor->F.resize( 2, 3 ); 
    mesh_floor->F.row(0) << 2, 1, 0;
    mesh_floor->F.row(1) << 3, 2, 0;
    mesh_floor->recalculate_normals();

    //scale to be in the range of the mesh 
    mesh_floor->V.array()*=get_scale();

    //find the minimal point in y of the mesh so we put the grid there 
    Eigen::Affine3d trans=Eigen::Affine3d::Identity();
    Eigen::Vector3d t;
    t<< 0, y_pos, 0;
    trans.translate(t);
    mesh_floor->apply_transform(trans,true);
 

    mesh_floor->name="grid_floor";
    mesh_floor->m_vis.m_show_mesh=true;
    mesh_floor->m_vis.m_line_color<<0.6, 0.6, 0.6;
    return mesh_floor;

}

bool Scene::is_empty(){
    //return true when all of the meshes have no vertices`
    for(size_t i=0; i<m_meshes.size(); i++){
        if(!m_meshes[i]->is_empty()){
            return false;
        }
    }
    return true;
}
