// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

namespace anki {

/// @addtogroup util_patterns
/// @{

/// Makes a derived class non copyable
struct NonCopyable
{
	NonCopyable()
	{}

	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;

	~NonCopyable()
	{}
};
/// @}

} // end namespace anki

