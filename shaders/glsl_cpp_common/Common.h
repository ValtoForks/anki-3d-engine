// Copyright (C) 2009-2018, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

//
// Macros & functions
//
#if defined(__cplusplus)
#	define ANKI_BEGIN_NAMESPACE \
		namespace anki \
		{
#	define ANKI_END_NAMESPACE }
#	define ANKI_SHADER_FUNC_INLINE inline

#	define ANKI_SHADER_INOUT(type_) type_&
#	define ANKI_SHADER_IN(type_) const type_&
#	define ANKI_SHADER_OUT(type_) type_&

#	define ANKI_SHADER_STATIC_ASSERT(cond_) static_assert(cond_, "See file");

ANKI_BEGIN_NAMESPACE
template<typename T>
inline F32 dot(const T& a, const T& b)
{
	return a.dot(b);
}
ANKI_END_NAMESPACE

#else
#	define x() x
#	define y() y
#	define z() z
#	define w() w

#	define xyz() xyz

#	define ANKI_BEGIN_NAMESPACE
#	define ANKI_END_NAMESPACE
#	define ANKI_SHADER_FUNC_INLINE

#	define ANKI_SHADER_INOUT(type_) inout type_
#	define ANKI_SHADER_IN(type_) in type_
#	define ANKI_SHADER_OUT(type_) out type_

#	define ANKI_SHADER_STATIC_ASSERT(cond_)
#endif

//
// Consts
//
ANKI_BEGIN_NAMESPACE

const U32 SIZEOF_VEC4 = 4u * 4u;
const U32 SIZEOF_MAT4 = 4u * SIZEOF_VEC4;

ANKI_END_NAMESPACE
