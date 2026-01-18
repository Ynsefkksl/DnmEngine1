#pragma once

#include "Utility/EngineDefines.hpp"
#include "Graphics/RawBuffer.hpp"
#include "Entity/Transform.hpp"

#undef near //win api macro
#undef far //win api macro

struct CameraGPU {
    glm::mat4 projectionMtx;
    glm::mat4 viewMtx;
};

class ENGINE_API Camera : public Transform, public PtrFactory<Camera> {
public:
    Camera(glm::vec2 size, float fov, float near, float far, bool isOrthographic = false)
    : m_size(size),
        m_fov(fov),
        m_near(near),
        m_far(far),
        m_isOrthographic(isOrthographic) {
            CalcProjectionMtx();
        }

    Camera(Camera&& other)
    : Transform(std::move(other)),
        m_projectionMtx(other.m_projectionMtx),
        m_viewMtx(other.m_viewMtx),
        m_direction(other.m_direction),
        m_fov(other.m_fov),
        m_near(other.m_near),
        m_far(other.m_far),
        m_isOrthographic(other.m_isOrthographic) {}
    ~Camera() {}

    void CalcProjectionMtx();

    const glm::mat4& GetViewMtx() const { return m_viewMtx; }
    const glm::mat4& GetProjectionMtx() const { return m_projectionMtx; }
    const glm::vec3& GetDirection() const { return m_direction; }
    const glm::vec2& GetSize() const { return m_size; }
    float GetFov() const { return m_fov; }
    float GetNear() const { return m_near; }
    float GetFar() const { return m_far; }
    bool IsOrthographic() const { return m_isOrthographic; }
    
    void SetDirection(const glm::vec3& dir) { m_direction = dir; }
    void SetSize(const glm::vec2& size) { m_size = size; }
    void SetFov(float fov) { m_fov = fov; }
    void SetNear(float nearPlane) { m_near = nearPlane; }
    void SetFar(float farPlane) { m_far = farPlane; }
    void SetOrthographic(bool isOrtho) { m_isOrthographic = isOrtho; }

    Camera(const Camera &) = delete;
    Camera &operator=(const Camera &) = delete;
    Camera &operator=(Camera &&) = delete;
private:    
    glm::mat4 m_projectionMtx{};
    glm::mat4 m_viewMtx{};
    glm::vec3 m_direction{};
    glm::vec2 m_size;
    float m_fov;
    float m_near;
    float m_far;
    bool m_isOrthographic;
};