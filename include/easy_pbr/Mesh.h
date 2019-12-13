#pragma once

#include <memory>
#include<stdarg.h>

//eigen
#include <Eigen/Geometry>

//opencv for textre
#include "opencv2/opencv.hpp"

//better enums
#include <enum.h>



BETTER_ENUM(MeshColorType, int, Solid = 0, PerVertColor, Texture, SemanticPred, SemanticGT, NormalVector, SSAO, Height, Intensity )


class MeshGL; //we forward declare this so we can have from here a pointer to the gpu stuff
class LabelMngr;
class RandGenerator;


class Mesh;

struct VisOptions{
     //visualization params (it's nice to have here so that the various algorithms that run in different threads can set them)
    bool m_is_visible=true;
    bool m_show_points=false;
    bool m_show_lines=false;
    bool m_show_mesh=true;
    bool m_show_wireframe=false;
    bool m_show_surfels=false;
    bool m_show_vert_ids=false;
    bool m_show_vert_coords=false;


    float m_point_size=4.0;
    float m_line_width=1.0; //specified the width of of both line rendering and the wireframe rendering
    MeshColorType m_color_type=MeshColorType::Solid;
    Eigen::Vector3f m_point_color = Eigen::Vector3f(1.0, 215.0/255.0, 85.0/255.0); 
    Eigen::Vector3f m_line_color = Eigen::Vector3f(1.0, 0.0, 0.0);   //used for lines and wireframes
    Eigen::Vector3f m_solid_color = Eigen::Vector3f(1.0, 206.0/255.0, 143.0/255.0);
    Eigen::Vector3f m_label_color = Eigen::Vector3f(1.0, 160.0/255.0, 0.0);
    float m_metalness=0.0;
    float m_roughness=0.35;

    //we define some functions for settings colors both for convenicence and easily calling then from python with pybind
    void set_color_solid(){
        m_color_type=MeshColorType::Solid;
    }
    void set_color_pervertcolor(){
        m_color_type=MeshColorType::PerVertColor;
    }
    void set_color_texture(){
        m_color_type=MeshColorType::Texture;
    }
    void set_color_semanticpred(){
        m_color_type=MeshColorType::SemanticPred;
    }
    void set_color_semanticgt(){
        m_color_type=MeshColorType::SemanticGT;
    }
    void set_color_normalvector(){
        m_color_type=MeshColorType::NormalVector;
    }


};


class Mesh : public std::enable_shared_from_this<Mesh>{ //enable_shared_from is required due to pybind https://pybind11.readthedocs.io/en/stable/advanced/smart_ptrs.html
public:
    EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    template <class ...Args>
    static std::shared_ptr<Mesh> create( Args&& ...args ){
        return std::shared_ptr<Mesh>( new Mesh(std::forward<Args>(args)...) );
    }
    Mesh();
    Mesh(const std::string file_path);
    ~Mesh()=default; 

    Mesh clone();
    void add(const Mesh& new_mesh); //Adds another mesh to this one and combines it into one
    void clear();
    void assign_mesh_gpu(std::shared_ptr<MeshGL> mesh_gpu); //assigns the pointer to the gpu implementation of this mesh
    void load_from_file(const std::string file_path);
    void save_to_file(const std::string file_path);
    bool is_empty()const;
    // void apply_transform(Eigen::Affine3d& trans, const bool transform_points_at_zero=false ); //transforms the vertices V and the normals. A more efficient way would be to just update the model matrix and let the GPU do it but I like having the V here and on the GPU in sync so I rather transform on CPU and then send all the data to GPU
    // void transform_model_matrix(const Eigen::Affine3d& trans); //updates the model matrix but does not change the vertex data V on the CPU
    // void apply_transform(const Eigen::Affine3d& tf, const bool update_cpu_data, const bool transform_points_at_zero=false); //if we update the CPU data we move directly the V vertices but the model matrix does not get updates and it will stay as whatever it was set before. That means that the origin around which rotations will be applied subsequently may not lie anymore where you expected. If we have update_cpu_data to false, we only modify the model matrix therefore the model matrix 

    void transform_vertices_cpu(const Eigen::Affine3d& trans, const bool transform_points_at_zero=false); //modifyed the vertices on the cpu but does not update the model matrix
    void transform_model_matrix(const Eigen::Affine3d& trans); //just affects how the model is displayed when rendered by modifying the model matrix but does not change the vertices themselves
    void translate_model_matrix(const Eigen::Vector3d& translation); //easier acces to transform of model matrix by just translation. Easier to call from python
    void rotate_model_matrix(const Eigen::Vector3d& axis, const float angle_degrees);
    void rotate_model_matrix_local(const Eigen::Vector3d& axis, const float angle_degrees);

    void clear_C();
    void color_from_label_indices(Eigen::MatrixXi label_indices);
    Eigen::Vector3d centroid();
    void sanity_check() const; //check that all the data inside the mesh is valid, there are enough normals for each face, faces don't idx invalid points etc.
    //create certain meshes
    void create_full_screen_quad();
    void create_box_ndc(); //makes a 1x1x1 vox in NDC. which has z going into the screen
    void create_grid(const int nr_segments, const float y_pos, const float scale);
    void create_floor(const float y_pos, const float scale);
    void add_child(std::shared_ptr<Mesh>& mesh); //add a child into the transformation hierarchy. Therefore when this object moves or rotates the children also do.

    //lots of mesh ops 
    void remove_marked_vertices(const std::vector<bool>& mask, const bool keep);
    void set_marked_vertices_to_zero(const std::vector<bool>& mask, const bool keep); //useful for when the actual removal of verts will destroy the organized structure
    void remove_vertices_at_zero(); // zero is used to denote the invalid vertex, we can remove them and rebuild F, E and the rest of indices with this function
    void remove_unreferenced_verts();
    void remove_duplicate_vertices();
    void set_duplicate_verts_to_zero();
    void decimate(const int nr_target_faces);
    bool compute_non_manifold_edges(std::vector<bool>& is_face_non_manifold, std::vector<bool>& is_vertex_non_manifold,  const Eigen::MatrixXi& F_in);
    void rotate_90_x_axis();
    void worldGL2worldROS();
    void worldROS2worldGL();
    // void rotate_x_axis(const float degrees);
    // void rotate_y_axis(const float degrees);
    void random_subsample(const float percentage_removal); 
    void recalculate_normals(); //recalculates NF and NV
    void flip_normals();
    void normalize_size(); //normalize the size of the mesh between [-1,1]
    void normalize_position(); //calculate the bounding box of the object and put it at 0.0.0
    Eigen::VectorXi fix_oversplit_due_to_blender_uv();
    void color_connected_components();
    void remove_small_uv_charts();
    void apply_D(); //given the D which is the distance of the vertices from the origin of the senser, applies it to the vertices
    void to_image();
    void to_image(Eigen::Affine3d tf_currframe_alg);
    void to_mesh();
    void to_mesh(Eigen::Affine3d tf_currframe_alg);
    void to_3D();   //from a matrix with 2 columns creates one with 3 columns (requiered to go from the delaunay triangulation into a 3d mesh representable in libigl)
    void to_2D(); //from a matrix V with 3 columns, it discards te last one and creates one with 2 columns (in order for an image to be passed to the triangle library)
    void restrict_around_azimuthal_angle(const float angle, const float range); //for an organized point cloud, sets to zero the points that are not around a certain azimuthal angle when viewed from the algorithm frame
    void as_uv_mesh_paralel_to_axis(const int axis, const float size_modifier);
    Mesh interpolate(const Mesh& target_mesh, const float factor);
    float get_scale();

    //some convenience functions and also useful for calling from python using pybind
    // void move_in_x(const float amount);
    // void move_in_y(const float amount);
    // void move_in_z(const float amount);
    void random_translation(const float translation_strength);
    void random_rotation(const float rotation_strength); //applies a rotation to all axes, of maximum rotation_strength degrees
    void random_stretch(const float stretch_strength); //stretches the x,y,z axis by a random number
    void random_noise(const float noise_stddev); //adds random gaussian noise with some strength to each points

    //return the min y and min max flor plotting. May not neceserally be the min max of the actual data. The min_max_y_plotting is controlable in the gui
    float min_y();
    float max_y();



    friend std::ostream &operator<<(std::ostream&, const Mesh& m);

    bool m_is_dirty; // if it's dirty then we need to upload this data to the GPU

    VisOptions m_vis;
    bool m_force_vis_update; //sometimes we want the m_vis stored in the this MeshCore to go into the MeshGL, sometimes we don't. The default is to not propagate, setting this flag to true will force the update of m_vis inside the MeshGL

    Eigen::Affine3d m_model_matrix;  //transform from object coordiantes to the world coordinates, esentially putting the model somewhere in the world. 
    Eigen::Affine3d m_cur_pose; //the current pose, which describest the transformation that the cpu vertices underwent in order to appear centered nicelly for the gpu. The gpu then puts the vertices into the world with the model matrix. The cur pose is not used by opengl

    Eigen::MatrixXd V; 
    Eigen::MatrixXi F;
    Eigen::MatrixXd C;
    Eigen::MatrixXi E;
    Eigen::MatrixXd D;  //distances of points to the sensor
    Eigen::MatrixXd NF; //normals of each face
    Eigen::MatrixXd NV; //normals of each vertex
    Eigen::MatrixXd UV; //UV for each vertex
    Eigen::MatrixXd V_tangent_u; //for surfel rendering each vertex has a 2 vectors that are tangent defining the span of the elipsoid. For memory usage we don't store the 2 vectors directly because we alreayd have a normal vector, rather we store one tangent vector in full (vec3) and the other one we store only the norm of it because it's dirrection can be inferred as the cross product between the normal and the first tangent vector
    Eigen::MatrixXd V_length_v;
    Eigen::MatrixXi L_pred; //predicted labels for each point, useful for semantic segmentation 
    Eigen::MatrixXi L_gt; //ground truth labels for each point, useful for semantic segmentation 
    Eigen::MatrixXd I; //intensity value of each point in the cloud. Useful for laser scanner


    int m_seg_label_pred; // for classification we will have a lable for the whole cloud
    int m_seg_label_gt;

    cv::Mat m_rgb_tex_cpu;

    std::weak_ptr<MeshGL> m_mesh_gpu; // a pointer to the gpu implementation of this mesh, needs ot be weak because the mesh already has a shared ptr to the MeshCore
    std::shared_ptr<LabelMngr> m_label_mngr;
    std::shared_ptr<RandGenerator> m_rand_gen;
    std::vector<std::shared_ptr<Mesh>> m_child_meshes;

    //oher stuff that may or may not be needed depending on the application
    uint64_t t; //timestamp or scan nr which will be monotonically increasing
    int id; //id number that can be used to identity meshes of the same type (vegation, people etc)
    int m_height;
    int m_width;
    float m_view_direction; //direction in which the points have been removed from the velodyne cloud so that it can be unwrapped easier into 2D
    Eigen::Vector2f m_min_max_y; //the min and the max coordinate in y direction. useful for plotting color based on height
    Eigen::Vector2f m_min_max_y_for_plotting; //sometimes we want the min and max to be a bit different (controlable through the gui)


    //identification
    std::string name;

  

private:

    std::string named(const std::string msg) const{
        return name.empty()? msg : name + ": " + msg;    
    }


    //We use this for reading ply files because the readPLY from libigl has a memory leak https://github.com/libigl/libigl/issues/919
    void read_ply(const std::string file_path);
    void write_ply(const std::string file_path);
    void read_obj(const std::string file_path);



};

typedef std::shared_ptr<Mesh> MeshSharedPtr;

