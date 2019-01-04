// Copyright (C) 2009-2018, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

// Defines it needs:
// WORKGROUP_SIZE (only for compute)

#pragma once

#pragma anki mutator ORIENTATION 0 1 2 // 0: VERTICAL, 1: HORIZONTAL, 2: BOX
#pragma anki mutator KERNEL_SIZE 3 5 7 9 11 13 15
#pragma anki mutator COLOR_COMPONENTS 4 3 1

#pragma anki input const UVec2 TEXTURE_SIZE

#include <shaders/GaussianBlurCommon.glsl>

#if defined(ANKI_COMPUTE_SHADER)
#	define USE_COMPUTE 1
#else
#	define USE_COMPUTE 0
#endif

// Determine color type
#if COLOR_COMPONENTS == 4
#	define COL_TYPE Vec4
#	define TEX_FETCH rgba
#	define TO_VEC4(x_) x_
#elif COLOR_COMPONENTS == 3
#	define COL_TYPE Vec3
#	define TEX_FETCH rgb
#	define TO_VEC4(x_) Vec4(x_, 0.0)
#elif COLOR_COMPONENTS == 1
#	define COL_TYPE F32
#	define TEX_FETCH r
#	define TO_VEC4(x_) Vec4(x_, 0.0, 0.0, 0.0)
#else
#	error See file
#endif

layout(ANKI_TEX_BINDING(0, 0)) uniform sampler2D u_tex; ///< Input texture

#if USE_COMPUTE
layout(local_size_x = WORKGROUP_SIZE.x, local_size_y = WORKGROUP_SIZE.y, local_size_z = 1) in;
layout(ANKI_IMAGE_BINDING(0, 0)) writeonly uniform image2D u_outImg;
#else
layout(location = 0) in Vec2 in_uv;
layout(location = 0) out COL_TYPE out_color;
#endif

void main()
{
	// Set UVs
#if USE_COMPUTE
	ANKI_BRANCH if(gl_GlobalInvocationID.x >= TEXTURE_SIZE.x || gl_GlobalInvocationID.y >= TEXTURE_SIZE.y)
	{
		// Out of bounds
		return;
	}

	Vec2 uv = (Vec2(gl_GlobalInvocationID.xy) + 0.5) / Vec2(TEXTURE_SIZE);
#else
	Vec2 uv = in_uv;
#endif

	const Vec2 TEXEL_SIZE = 1.0 / Vec2(TEXTURE_SIZE);

#if !defined(BOX)
	// Do seperable

#	if defined(HORIZONTAL)
#		define X_OR_Y x
#	else
#		define X_OR_Y y
#	endif

	COL_TYPE color = textureLod(u_tex, uv, 0.0).TEX_FETCH * WEIGHTS[0u];

	Vec2 uvOffset = Vec2(0.0);
	uvOffset.X_OR_Y = 1.5 * TEXEL_SIZE.X_OR_Y;

	ANKI_UNROLL for(U32 i = 0u; i < STEP_COUNT; ++i)
	{
		COL_TYPE col =
			textureLod(u_tex, uv + uvOffset, 0.0).TEX_FETCH + textureLod(u_tex, uv - uvOffset, 0.0).TEX_FETCH;
		color += WEIGHTS[i + 1u] * col;

		uvOffset.X_OR_Y += 2.0 * TEXEL_SIZE.X_OR_Y;
	}
#else
	// Do box

	const Vec2 OFFSET = 1.5 * TEXEL_SIZE;

	COL_TYPE color = textureLod(u_tex, uv, 0.0).TEX_FETCH * BOX_WEIGHTS[0u];

	COL_TYPE col;
	col = textureLod(u_tex, uv + Vec2(OFFSET.x, 0.0), 0.0).TEX_FETCH;
	col += textureLod(u_tex, uv + Vec2(0.0, OFFSET.y), 0.0).TEX_FETCH;
	col += textureLod(u_tex, uv + Vec2(-OFFSET.x, 0.0), 0.0).TEX_FETCH;
	col += textureLod(u_tex, uv + Vec2(0.0, -OFFSET.y), 0.0).TEX_FETCH;
	color += col * BOX_WEIGHTS[1u];

	col = textureLod(u_tex, uv + Vec2(+OFFSET.x, +OFFSET.y), 0.0).TEX_FETCH;
	col += textureLod(u_tex, uv + Vec2(+OFFSET.x, -OFFSET.y), 0.0).TEX_FETCH;
	col += textureLod(u_tex, uv + Vec2(-OFFSET.x, +OFFSET.y), 0.0).TEX_FETCH;
	col += textureLod(u_tex, uv + Vec2(-OFFSET.x, -OFFSET.y), 0.0).TEX_FETCH;
	color += col * BOX_WEIGHTS[2u];
#endif

	// Write value
#if USE_COMPUTE
	imageStore(u_outImg, IVec2(gl_GlobalInvocationID.xy), TO_VEC4(color));
#else
	out_color = color;
#endif
}
