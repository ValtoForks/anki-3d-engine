// Copyright (C) 2009-2016, Panagiotis Christopoulos Charitos and contributors.
// All rights reserved.
// Code licensed under the BSD License.
// http://www.anki3d.org/LICENSE

#pragma once

#include <anki/scene/SceneNode.h>
#include <anki/Math.h>

namespace anki
{

/// @addtogroup scene
/// @{

/// Occluder scene node.
class OccluderNode : public SceneNode
{
	friend class OccluderMoveFeedbackComponent;

public:
	OccluderNode(SceneGraph* scene)
		: SceneNode(scene)
	{
	}

	~OccluderNode();

	ANKI_USE_RESULT Error init(const CString& name, const CString& meshFname);

private:
	DynamicArray<Vec3> m_vertsL; ///< Verts in local space.
	DynamicArray<Vec3> m_vertsW; ///< Verts in world space.

	void onMoveComponentUpdate(MoveComponent& movec);
};
/// @}

} // end namespace anki