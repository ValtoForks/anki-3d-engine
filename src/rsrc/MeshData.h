#ifndef MESH_DATA_H
#define MESH_DATA_H

#include <string>
#include <boost/array.hpp>
#include "m/Math.h"
#include "util/StdTypes.h"
#include "util/Accessors.h"
#include "util/Vec.h"


/// Mesh data. This class loads the mesh file and the Mesh class loads it to
/// the GPU
///
/// Binary file format:
///
/// @code
/// // Header
/// <magic:ANKIMESH>
/// <string:meshName>
///
/// // Verts
/// uint: verts number
/// float: vert 0 x, float vert 0 y, float: vert 0 z
/// ...
///
/// // Faces
/// uint: faces number
/// uint: tri 0 vert ID 0, uint: tri 0 vert ID 1, uint: tri 0 vert ID 2
/// ...
///
/// // Tex coords
/// uint: tex coords number
/// float: tex coord for vert 0 x, float: tex coord for vert 0 y
/// ...
///
/// // Bone weights
/// uint: bone weights number (equal to verts number)
/// uint: bones number for vert 0, uint: bone id for vert 0 and weight 0,
///       float: weight for vert 0 and weight 0, ...
/// ...
/// @endcode
class MeshData
{
	public:
		/// Vertex weight for skeletal animation
		class VertexWeight
		{
			public:
				/// Dont change this or prepare to change the skinning code in
				/// shader
				static const uint MAX_BONES_PER_VERT = 4;

				/// @todo change the vals to uint when change drivers
				float bonesNum;
				boost::array<float, MAX_BONES_PER_VERT> boneIds;
				boost::array<float, MAX_BONES_PER_VERT> weights;
		};

		/// Triangle
		class Triangle
		{
			public:
				/// An array with the vertex indexes in the mesh class
				uint vertIds[3];
				Vec3 normal;
		};

		MeshData(const char* filename) {load(filename);}
		~MeshData() {}

		/// @name Accessors
		/// @{
		GETTER_R(Vec<Vec3>, vertCoords, getVertCoords)
		GETTER_R(Vec<Vec3>, vertNormals, getVertNormals)
		GETTER_R(Vec<Vec4>, vertTangents, getVertTangents)
		GETTER_R(Vec<Vec2>, texCoords, getTexCoords)
		GETTER_R(Vec<VertexWeight>, vertWeights, getVertWeights)
		GETTER_R(Vec<Triangle>, tris, getTris)
		GETTER_R(Vec<ushort>, vertIndeces, getVertIndeces)
		/// @}

	private:
		/// @name Data
		/// @{
		Vec<Vec3> vertCoords; ///< Loaded from file
		Vec<Vec3> vertNormals; ///< Generated
		Vec<Vec4> vertTangents; ///< Generated
		/// Optional. One for every vert so we can use vertex arrays & VBOs
		Vec<Vec2> texCoords;
		Vec<VertexWeight> vertWeights; ///< Optional
		Vec<Triangle> tris; ///< Required
		Vec<ushort> vertIndeces; ///< Generated. Used for vertex arrays & VBOs
		/// @}

		/// Load the mesh data from a binary file
		/// @exception Exception
		void load(const char* filename);

		void createFaceNormals();
		void createVertNormals();
		void createAllNormals();
		void createVertTangents();
		void createVertIndeces();

		/// This method does some sanity checks and creates normals,
		/// tangents, VBOs etc
		/// @exception Exception
		void doPostLoad();
};


inline void MeshData::createAllNormals()
{
	createFaceNormals();
	createVertNormals();
}


#endif