// Copyright (C) 2014, Panagiotis Christopoulos Charitos.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#include "anki/gl/GlClientBufferHandle.h"
#include "anki/gl/GlClientBuffer.h"
#include "anki/gl/GlCommandBufferHandle.h"
#include "anki/gl/GlDevice.h"
#include "anki/core/Counters.h"

namespace anki {

//==============================================================================
GlClientBufferHandle::GlClientBufferHandle()
{}

//==============================================================================
GlClientBufferHandle::GlClientBufferHandle(
	GlCommandBufferHandle& commands, PtrSize size, void* preallocatedMem)
{
	ANKI_ASSERT(!isCreated());

	Error err = ErrorCode::NONE;

	auto alloc = commands._getAllocator();

	using Deleter = GlHandleDefaultDeleter<
		GlClientBuffer, GlCommandBufferAllocator<GlClientBuffer>>;

	*static_cast<Base*>(this) = Base(nullptr, alloc, Deleter());

	if(preallocatedMem != nullptr)
	{
		err = _get().create(preallocatedMem, size);
	}
	else
	{
		err = _get().create(alloc, size);

		ANKI_COUNTER_INC(GL_CLIENT_BUFFERS_SIZE, U64(size));
	}

	ANKI_ASSERT(!err);
}

//==============================================================================
GlClientBufferHandle::~GlClientBufferHandle()
{}

//==============================================================================
void* GlClientBufferHandle::getBaseAddress()
{
	return _get().getBaseAddress();
}

//==============================================================================
PtrSize GlClientBufferHandle::getSize() const
{
	return _get().getSize();
}

} // end namespace anki

