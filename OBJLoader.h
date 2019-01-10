#pragma once

#include <map>
#include "Vector.h"
#include "String.h"
#include "glm/glm.h"

namespace {

	typedef unsigned int uint;

	using namespace glm;

	struct Material
	{
		String MaterialName = "";

		vec3 AmbientColor = vec3(1);
		vec3 DiffuseColor = vec3(1);
		vec3 SpecularColor = vec3(1);

		float SpecularExponent = 0.0f;
		float OpticalDensity = 0.0f;
		float Dissolve = 0.0f;
		int Illumination = 0;
	
		String AmbientTexture = "";
		String DiffuseTexture = "";
		String SpecularTexture = "";
		String SpecularHightlight = "";
		String AlphaTexture = "";
		String BumpTexture = "";
	};


	struct IndexedModel {
		String name;

		Material material;

		Vector<vec3> vertices;
		Vector<vec2> textures;
		Vector<vec3> normals;
		Vector<uint> indices;

		void CalcNormals();
	};

	class OBJLoader {
	public:
		OBJLoader(const String& fileName);

		Vector<IndexedModel> GetModels();
	private:
		struct OBJIndex {
			unsigned int vertexIndex, uvIndex, normalIndex;
			bool operator<(const OBJIndex& o) const { return vertexIndex < o.vertexIndex; }
		};

		String folder;
		bool hasUVs = false;
		bool hasNormals = false;

		Vector<OBJIndex> OBJIndices;
		Vector<IndexedModel> loadedModels;
		std::map<String, Material> materials;
		IndexedModel loadingModel;

		IndexedModel ToIndexedModel();

		std::map<String, Material> LoadMaterials(const String& folder, const String& path);

		unsigned int FindLastVertexIndex(const Vector<OBJIndex*>& indexLookup, const OBJIndex* currentIndex, const IndexedModel& result);
		void CreateOBJFace(const String& line);

		vec2 ParseOBJVec2(const String& line);
		vec3 ParseOBJVec3(const String& line);
		OBJIndex ParseOBJIndex(const String& token);
	};
}
