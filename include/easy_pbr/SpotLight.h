#pragma once 

#include <memory>

#include "Shader.h"
#include "GBuffer.h"

#include <Eigen/Geometry>

#include "easy_pbr/Camera.h"

#define CONFIGURU_WITH_EIGEN 1
#define CONFIGURU_IMPLICIT_CONVERSIONS 1
#include <configuru.hpp>

//to his the fact that it is inheriting from Camera and both inherit from enabled_shared_from https://www.codeproject.com/Articles/286304/Solution-for-multiple-enable-shared-from-this-in-i
#include "shared_ptr/EnableSharedFromThis.h"
#include "shared_ptr/SmartPtrBuilder.h"

namespace easy_pbr{

class MeshGL;

// class SpotLight : public std::enable_shared_from_this<SpotLight>, public Camera
class SpotLight : public Camera, public Generic::EnableSharedFromThis< SpotLight >
{
public:
    // EIGEN_MAKE_ALIGNED_OPERATOR_NEW
    SpotLight(const configuru::Config& config);

    // void render_to_shadow_map(const MeshCore& mesh);
    void set_power_for_point(const Eigen::Vector3f& point, const float power);
    void render_mesh_to_shadow_map(std::shared_ptr<MeshGL>& mesh);
    void render_points_to_shadow_map(std::shared_ptr<MeshGL>& mesh);
    void clear_shadow_map();
    void set_shadow_map_resolution(const int shadow_map_resolution);
    int shadow_map_resolution();
    bool has_shadow_map();
    gl::Texture2D& get_shadow_map_ref();
    gl::GBuffer& get_shadow_map_fbo_ref();

    void print_ptr();

    float m_power;
    Eigen::Vector3f m_color; 
    bool m_create_shadow;

private:

    void init_params(const configuru::Config& config_file);
    void init_opengl();

    gl::Shader m_shadow_map_shader;
    gl::GBuffer m_shadow_map_fbo; //fbo that contains only depth maps for usage as a shadow map
    int m_shadow_map_resolution;
  
};

} //namespace easy_pbr