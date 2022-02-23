#pragma once

#include <SM_Vector.h>
#include <SM_Matrix.h>

namespace globegen
{

class Camera3D
{
public:
    Camera3D();
    Camera3D(const sm::vec3& pos, const sm::vec3& target, const sm::vec3& up);

    void Roll(float angle);
    void Yaw(float angle);
    void Pitch(float angle);
    void SetUpDir(const sm::vec3& up);

    void Translate(float dx, float dy);
    void MoveToward(float offset, bool fix_z = false);
    void Move(const sm::vec3& dir, float offset);
    void AimAtTarget();

    sm::mat4 GetViewMat() const;
    sm::mat4 GetRotateMat() const;

    auto& GetPosition() const { return m_pos; }
    auto& GetTarget() const { return m_target; }
    float GetDistance() const { return m_distance; }

    float GetNear() const { return m_znear; }
    float GetFar() const { return m_zfar; }
    float GetAngleOfView() const { return m_angle_of_view; }

    void SetAspect(float aspect) { m_aspect = aspect; }
    float GetAspect() const { return m_aspect; }

private:
    void CalcUVN(const sm::vec3& up);

private:
    // pos
    sm::vec3 m_pos, m_target;
    float m_distance;

    // angle
    sm::vec3 m_u, m_v, m_n;

    // store for reset
    sm::vec3 m_init_pos;
    sm::vec3 m_init_target;
    sm::vec3 m_init_up;

    // projection
    float m_znear, m_zfar;
    float m_aspect;
    float m_angle_of_view;

}; // Camera3D

}