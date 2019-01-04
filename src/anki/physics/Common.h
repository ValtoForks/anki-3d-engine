// Copyright (C) 2009-2018, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/util/StdTypes.h>
#include <anki/util/Enum.h>
#include <anki/util/Ptr.h>
#include <anki/Math.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#define BT_THREADSAFE 0
#define BT_NO_PROFILE 1
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletDynamics/Character/btKinematicCharacterController.h>
#include <BulletCollision/Gimpact/btGImpactShape.h>
#pragma GCC diagnostic pop

namespace anki
{

#define ANKI_PHYS_LOGI(...) ANKI_LOG("PHYS", NORMAL, __VA_ARGS__)
#define ANKI_PHYS_LOGE(...) ANKI_LOG("PHYS", ERROR, __VA_ARGS__)
#define ANKI_PHYS_LOGW(...) ANKI_LOG("PHYS", WARNING, __VA_ARGS__)
#define ANKI_PHYS_LOGF(...) ANKI_LOG("PHYS", FATAL, __VA_ARGS__)

// Forward
class PhysicsObject;
class PhysicsFilteredObject;
class PhysicsWorld;
class PhysicsCollisionShape;
class PhysicsBody;
class PhysicsPlayerController;
class PhysicsJoint;
class PhysicsTrigger;

/// @addtogroup physics
/// @{

/// PhysicsPtr custom deleter.
class PhysicsPtrDeleter
{
public:
	void operator()(PhysicsObject* ptr);
};

/// Smart pointer for physics objects.
template<typename T>
using PhysicsPtr = IntrusivePtr<T, PhysicsPtrDeleter>;

using PhysicsObjectPtr = PhysicsPtr<PhysicsObject>;
using PhysicsFilteredObjectPtr = PhysicsPtr<PhysicsFilteredObject>;
using PhysicsCollisionShapePtr = PhysicsPtr<PhysicsCollisionShape>;
using PhysicsBodyPtr = PhysicsPtr<PhysicsBody>;
using PhysicsPlayerControllerPtr = PhysicsPtr<PhysicsPlayerController>;
using PhysicsJointPtr = PhysicsPtr<PhysicsJoint>;
using PhysicsTriggerPtr = PhysicsPtr<PhysicsTrigger>;

/// Material types.
enum class PhysicsMaterialBit : U64
{
	NONE = 0,
	STATIC_GEOMETRY = 1 << 0,
	DYNAMIC_GEOMETRY = 1 << 1,
	TRIGGER = 1 << 2,
	PLAYER = 1 << 3,
	PARTICLE = 1 << 4,

	ALL = MAX_U64
};
ANKI_ENUM_ALLOW_NUMERIC_OPERATIONS(PhysicsMaterialBit, inline)

ANKI_USE_RESULT inline Vec3 toAnki(const btVector3& v)
{
	return Vec3(v.getX(), v.getY(), v.getZ());
}

ANKI_USE_RESULT inline btVector3 toBt(const Vec3& v)
{
	return btVector3(v.x(), v.y(), v.z());
}

ANKI_USE_RESULT inline btTransform toBt(const Transform& a)
{
	Mat4 mat(a);
	mat.transpose();
	btTransform out;
	out.setFromOpenGLMatrix(&mat(0, 0));
	return out;
}

ANKI_USE_RESULT inline Mat3x4 toAnki(const btMatrix3x3& m)
{
	Mat3x4 m3;
	m3.setRows(Vec4(toAnki(m[0]), 0.0f), Vec4(toAnki(m[1]), 0.0f), Vec4(toAnki(m[2]), 0.0f));
	return m3;
}

ANKI_USE_RESULT inline Transform toAnki(const btTransform& t)
{
	Transform out;
	out.setRotation(toAnki(t.getBasis()));
	out.setOrigin(Vec4(toAnki(t.getOrigin()), 0.0f));
	out.setScale(1.0f);
	return out;
}

/// A wrapper template to compensate for of the fact that Bullet classes get initialized in the constructor.
template<typename TBtClass>
class BtClassWrapper
{
public:
	BtClassWrapper()
	{
	}

	template<typename... TArgs>
	void init(TArgs&&... args)
	{
		::new(&m_data[0]) TBtClass(std::forward<TArgs>(args)...);
	}

	void destroy()
	{
		reinterpret_cast<TBtClass*>(&m_data[0])->~TBtClass();
	}

	TBtClass* operator->()
	{
		return reinterpret_cast<TBtClass*>(&m_data[0]);
	}

	const TBtClass* operator->() const
	{
		return reinterpret_cast<const TBtClass*>(&m_data[0]);
	}

	TBtClass& operator*()
	{
		return *reinterpret_cast<TBtClass*>(&m_data[0]);
	}

	const TBtClass& operator*() const
	{
		return *reinterpret_cast<const TBtClass*>(&m_data[0]);
	}

	TBtClass* get()
	{
		return reinterpret_cast<TBtClass*>(&m_data[0]);
	}

	const TBtClass* get() const
	{
		return reinterpret_cast<const TBtClass*>(&m_data[0]);
	}

private:
	alignas(alignof(TBtClass)) Array<U8, sizeof(TBtClass)> m_data;
};
/// @}

} // end namespace anki
