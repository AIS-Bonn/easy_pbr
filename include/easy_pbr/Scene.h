#pragma once

#include "easy_pbr/Mesh.h"

#include <vector>
#include <memory>
#include <mutex>

namespace easy_pbr{

class Mesh;

class Scene{

public:
    Scene();
    static void show(const std::shared_ptr<Mesh> mesh, const std::string name); //adds to the scene and overwrites if it has the same name
    static void hide_all();
    static void show_all();
    // static void show(const Mesh& mesh, const std::string name); //convenience function. adds to the scene and overwrites if it has the same name
    static void add_mesh(const std::shared_ptr<Mesh> mesh, const std::string name); //adds to the scene even if it has the same name
    static void clear();
    static int nr_meshes();
    static int nr_vertices();
    static int nr_faces();
    static std::shared_ptr<Mesh> get_mesh_with_name(const std::string name);
    static std::shared_ptr<Mesh> get_mesh_with_idx(const unsigned int idx);
    static int get_idx_for_name(const std::string name);
    static bool does_mesh_with_name_exist(const std::string name);
    static void remove_meshes_starting_with_name(const std::string name_prefix); // check all the meshes and removed the ones that start with a certain name
    static void remove_mesh_with_idx(const unsigned int idx);

    //more high level operations on the meshes in the scene
    static Eigen::Vector3f get_centroid(const bool use_mutex=true); //returns the aproximate center of our scene which consists of all meshes
    static float get_scale(const bool use_mutex=true); //returns how big the scene is as a measure betwen the min and the coefficient of the vertices
    static bool is_empty(const bool use_mutex=true);

    static void set_floor_visible(const bool val);
    static void set_floor_metric(const bool val);
    static void set_automatic_normal_calculation(const bool val);



private:
    static std::vector< std::shared_ptr<Mesh> > m_meshes;
    static std::mutex m_mesh_mutex; // when adding a new mesh to the scene, we need to lock them so it can be thread safe

    static bool m_floor_visible; //storing if the user wants the floor visible or not. We store it here because the user might set it before we even added a floor
    static bool m_floor_metric; // is this is true. the floor will be metric in the sense that each square will have edge being one unit. If this is false, then the floor will be dynamic to the size of the scene
    static bool m_automatic_normal_calculation;

    //params
    static const int m_grid_nr_segments=8;

};

} //namespace easy_pbr
