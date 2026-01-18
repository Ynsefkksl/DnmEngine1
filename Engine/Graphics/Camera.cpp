#include "Graphics/Camera.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/fwd.hpp>

void Camera::CalcProjectionMtx() {
    if (m_isOrthographic)
        m_projectionMtx = glm::ortho(-m_size.x/2, m_size.x/2, -m_size.y/2, m_size.y/2, m_near, m_far );
    else
        m_projectionMtx = glm::perspective(glm::radians(m_fov), m_size.x / m_size.y, m_near, m_far);

    m_projectionMtx[1][1] *= -1;
}