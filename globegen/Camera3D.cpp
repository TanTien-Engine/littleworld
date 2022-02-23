#include "Camera3D.h"

namespace
{

static const float ZNEAR = 0.1f;
static const float ZFAR  = 100;
static const float FOVY  = 45;

}

namespace globegen
{

Camera3D::Camera3D()
	: m_distance(0)
	, m_znear(ZNEAR)
	, m_zfar(ZFAR)
	, m_aspect(1)
	, m_angle_of_view(FOVY)
{
}

Camera3D::Camera3D(const sm::vec3& pos, const sm::vec3& target, const sm::vec3& up)
	: m_pos(pos)
	, m_target(target)
	, m_distance(0)
	, m_init_pos(pos)
	, m_init_target(target)
	, m_init_up(up)
	, m_znear(ZNEAR)
	, m_zfar(ZFAR)
	, m_aspect(1)
	, m_angle_of_view(FOVY)
{
	m_distance = (pos - target).Length();
	CalcUVN(up);
}

void Camera3D::Roll(float angle)
{
	float cs = cos(angle * SM_DEG_TO_RAD);
	float sn = sin(angle * SM_DEG_TO_RAD);
	sm::vec3 t(m_u);
	sm::vec3 s(m_v);

	m_u.x = cs * t.x - sn * s.x;
	m_u.y = cs * t.y - sn * s.y;
	m_u.z = cs * t.z - sn * s.z;

	m_v.x = sn * t.x + cs * s.x;
	m_v.y = sn * t.y + cs * s.y;
	m_v.z = sn * t.z + cs * s.z;
}

void Camera3D::Yaw(float angle)
{
	float cs = cos(angle * SM_DEG_TO_RAD);
	float sn = sin(angle * SM_DEG_TO_RAD);
	sm::vec3 t(m_n);
	sm::vec3 s(m_u);

	m_n.x = cs * t.x - sn * s.x;
	m_n.y = cs * t.y - sn * s.y;
	m_n.z = cs * t.z - sn * s.z;

	m_u.x = sn * t.x + cs * s.x;
	m_u.y = sn * t.y + cs * s.y;
	m_u.z = sn * t.z + cs * s.z;
}

void Camera3D::Pitch(float angle)
{
	float cs = cos(angle * SM_DEG_TO_RAD);
	float sn = sin(angle * SM_DEG_TO_RAD);
	sm::vec3 t(m_v);
	sm::vec3 s(m_n);

	m_v.x = cs * t.x - sn * s.x;
	m_v.y = cs * t.y - sn * s.y;
	m_v.z = cs * t.z - sn * s.z;

	m_n.x = sn * t.x + cs * s.x;
	m_n.y = sn * t.y + cs * s.y;
	m_n.z = sn * t.z + cs * s.z;
}

void Camera3D::SetUpDir(const sm::vec3& up)
{
	m_v = up;
	m_u = m_v.Cross(m_n).Normalized();
    m_v = m_n.Cross(m_u).Normalized();
}

void Camera3D::Translate(float dx, float dy)
{
	sm::vec3 tx = - m_u * dx,
		      ty = m_v * dy;
	m_pos += tx;
	m_pos += ty;
	m_target += tx;
	m_target += ty;
}

void Camera3D::MoveToward(float offset, bool fix_z)
{
	if (fix_z) {
		auto dir = m_n;
		dir.z = 0;
		m_pos += dir * offset;
	} else {
		m_pos += m_n * offset;
	}
	m_distance = (m_target - m_pos).Length();
}

void Camera3D::Move(const sm::vec3& dir, float offset)
{
	m_pos += dir * offset;
	m_distance = (m_target - m_pos).Length();
	m_target = m_pos + m_n * m_distance;

	CalcUVN(m_init_up);
}

void Camera3D::AimAtTarget()
{
	m_pos = m_target - m_n * m_distance;

	CalcUVN(m_init_up);
}

sm::mat4 Camera3D::GetViewMat() const
{
    sm::mat4 mat;
    float* m = &mat.c[0][0];
    m[0] = m_u.x;  m[4] = m_u.y;  m[8] = m_u.z;   m[12] = -m_pos.Dot(m_u);
    m[1] = m_v.x;  m[5] = m_v.y;  m[9] = m_v.z;   m[13] = -m_pos.Dot(m_v);
    m[2] = -m_n.x; m[6] = -m_n.y; m[10] = -m_n.z; m[14] = m_pos.Dot(m_n);
    m[3] = 0;      m[7] = 0;      m[11] = 0;      m[15] = 1.0;
    return mat;
}

sm::mat4 Camera3D::GetRotateMat() const
{
    sm::mat4 mat;
    float* m = &mat.c[0][0];
    m[0] = m_u.x; m[4] = m_u.y; m[8] = m_u.z; m[12] = 0;
    m[1] = m_v.x; m[5] = m_v.y; m[9] = m_v.z; m[13] = 0;
    m[2] = -m_n.x; m[6] = -m_n.y; m[10] = -m_n.z; m[14] = 0;
    m[3] = 0;     m[7] = 0;     m[11] = 0;     m[15] = 1.0;
    return mat;
}

void Camera3D::CalcUVN(const sm::vec3& up)
{
    m_n = (m_target - m_pos).Normalized();
	m_u = m_n.Cross(up).Normalized();
	m_v = m_u.Cross(m_n);
}

}