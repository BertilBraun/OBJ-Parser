#include "OBJLoader.h"

#include <fstream>
#include <algorithm>

static inline uint FindNextChar(uint start, const char* str, uint length, char token);
static inline float ParseOBJFloatValue(const String& token, uint start, uint end);
static inline uint ParseOBJIndexValue(const String& token);

void IndexedModel::CalcNormals()
{
	for(unsigned int i = 0; i < indices.size(); i += 3)
	{
		int i0 = indices[i];
		int i1 = indices[i + 1];
		int i2 = indices[i + 2];

		vec3 v1 = vertices[i1] - vertices[i0];
		vec3 v2 = vertices[i2] - vertices[i0];
		
		vec3 normal = normalize(cross(v1, v2));
			
		normals[i0] += normal;
		normals[i1] += normal;
		normals[i2] += normal;
	}
	
	for(unsigned int i = 0; i < vertices.size(); i++)
		normals[i] = normalize(normals[i]);
}

OBJLoader::OBJLoader(const String& fileName)
{
	std::ifstream file(fileName.c_str());

	if (!file.is_open() || fileName.substr(fileName.length(), fileName.length() - 4) == ".obj") {
		printf("UNABLE TO LOAD MESH : %s", fileName.c_str());
		return;
	}

	printf("LOADING MESH : %s", fileName.c_str());

	String line;
	folder = String::folder(fileName);
	while (getline(file, line))
	{
		if ((uint)line.length() < 2)
			continue;

		switch (line[0]) {
		case 'o':
		case 'g':
			if (loadingModel.vertices.size() && OBJIndices.size()) {

				loadedModels.push_back(ToIndexedModel());
				OBJIndices.clear();
				printf("LOADED MESH WITH : %i VERTECIES", loadedModels.back().vertices.size());
			}

			hasUVs = false;
			hasNormals = false;
			loadingModel.name = String::tail(line);
			break;

		case 'v':
			if (line[1] == 't')
				loadingModel.textures.push_back(ParseOBJVec2(line));

			else if (line[1] == 'n')
				loadingModel.normals.push_back(ParseOBJVec3(line));

			else if (line[1] == ' ' || line[1] == '\t')
				loadingModel.vertices.push_back(ParseOBJVec3(line));

			break;

		case 'f':
			CreateOBJFace(line);
			break;
		};

		if (String::firstToken(line) == "usemtl")
			loadingModel.material = materials[String::tail(line)];

		if (String::firstToken(line) == "mtllib")
			materials = LoadMaterials(folder, String::tail(line));
	}

	if (loadingModel.vertices.size() && OBJIndices.size())
		loadedModels.push_back(ToIndexedModel());

	printf("LOADED MESH WITH : %i VERTECIES", loadedModels.back().vertices.size());
}

Vector<IndexedModel> OBJLoader::GetModels()
{
	return loadedModels;
}

IndexedModel OBJLoader::ToIndexedModel()
{
	IndexedModel result;
	IndexedModel normalModel;

	uint numIndices = (uint)OBJIndices.size();

	Vector<OBJIndex*> indexLookup;

	for (uint i = 0; i < numIndices; i++)
		indexLookup.push_back(&OBJIndices[i]);

	std::sort(indexLookup.begin(), indexLookup.end(), [](const OBJIndex* a, const OBJIndex* b) { return a->vertexIndex < b->vertexIndex; });

	std::map<OBJIndex, uint> normalModelIndexMap;
	std::map<uint, uint> indexMap;

	for (uint i = 0; i < numIndices; i++)
	{
		OBJIndex* currentIndex = &OBJIndices[i];

		vec3 currentPosition = loadingModel.vertices[currentIndex->vertexIndex];
		vec2 currentTexCoord = (hasUVs) ? loadingModel.textures[currentIndex->uvIndex] : vec2(0, 0);
		vec3 currentNormal = (hasNormals) ? loadingModel.normals[currentIndex->normalIndex] : vec3(0, 0, 0);

		uint normalModelIndex;
		uint resultModelIndex;

		//Create model to properly generate normals on
		std::map<OBJIndex, uint>::iterator it = normalModelIndexMap.find(*currentIndex);
		if (it == normalModelIndexMap.end())
		{
			normalModelIndex = (uint)normalModel.vertices.size();

			normalModelIndexMap.insert(std::pair<OBJIndex, uint>(*currentIndex, normalModelIndex));
			normalModel.vertices.push_back(currentPosition);
			normalModel.textures.push_back(currentTexCoord);
			normalModel.normals.push_back(currentNormal);
		}
		else
			normalModelIndex = it->second;

		//Create model which properly separates texture coordinates
		uint previousVertexLocation = FindLastVertexIndex(indexLookup, currentIndex, result);

		if (previousVertexLocation == (uint)-1)
		{
			resultModelIndex = (uint)result.vertices.size();

			result.vertices.push_back(currentPosition);
			result.textures.push_back(currentTexCoord);
			result.normals.push_back(currentNormal);
		}
		else
			resultModelIndex = previousVertexLocation;

		normalModel.indices.push_back(normalModelIndex);
		result.indices.push_back(resultModelIndex);
		indexMap.insert(std::pair<uint, uint>(resultModelIndex, normalModelIndex));
	}

	if (!hasNormals)
	{
		normalModel.CalcNormals();

		for (uint i = 0; i < result.vertices.size(); i++)
			result.normals[i] = normalModel.normals[indexMap[i]];
	}

	result.name = loadingModel.name;
	result.material = loadingModel.material;

	return result;
}

std::map<String, Material> OBJLoader::LoadMaterials(const String & folder, const String & path)
{
	std::map<String, Material> materials;

	if (path.substr(path.size() - 4, path.size()) != ".mtl")
		return materials;

	std::ifstream file(folder + path);

	if (!file.is_open()) {
		printf("UNABLE TO LOAD MATERIAL : %s IN FOLDER : ", path.c_str(), folder.c_str());
		return materials;
	}

	Material tempMaterial;
	bool listening = false;

	String curline, firstToken, tailToken;
	while (getline(file, curline))
	{
		firstToken = String::firstToken(curline);
		tailToken = String::tail(curline);

		if (firstToken == "newmtl")
		{
			if (!listening)
				listening = true;
			else
				materials[tempMaterial.MaterialName] = tempMaterial;

			tempMaterial.MaterialName = (curline.size() > 7) ? tailToken : "none";
		}

		else if (firstToken == "Ka")
		{
			std::vector<String> temp = String::split(tailToken, ' ');
			tempMaterial.AmbientColor = vec3(std::stof(temp[0]), std::stof(temp[1]), std::stof(temp[2]));
		}

		else if (firstToken == "Kd")
		{
			std::vector<String> temp = String::split(tailToken, ' ');
			tempMaterial.DiffuseColor = vec3(std::stof(temp[0]), std::stof(temp[1]), std::stof(temp[2]));
		}

		else if (firstToken == "Ks")
		{
			std::vector<String> temp = String::split(tailToken, ' ');
			tempMaterial.SpecularColor = vec3(std::stof(temp[0]), std::stof(temp[1]), std::stof(temp[2]));
		}

		else if (firstToken == "Ns")
			tempMaterial.SpecularExponent = std::stof(tailToken);

		else if (firstToken == "Ni")
			tempMaterial.OpticalDensity = std::stof(tailToken);

		else if (firstToken == "d")
			tempMaterial.Dissolve = std::stof(tailToken);

		else if (firstToken == "illum")
			tempMaterial.Illumination = std::stoi(tailToken);

		else if (firstToken == "map_Ka")
			tempMaterial.AmbientTexture = folder + tailToken;

		else if (firstToken == "map_Kd")
			tempMaterial.DiffuseTexture = folder + tailToken;

		else if (firstToken == "map_Ks")
			tempMaterial.SpecularTexture = folder + tailToken;

		else if (firstToken == "map_Ns")
			tempMaterial.SpecularHightlight = folder + tailToken;

		else if (firstToken == "map_d")
			tempMaterial.AlphaTexture = folder + tailToken;

		else if (firstToken == "map_Bump" || firstToken == "map_bump" || firstToken == "bump")
			tempMaterial.BumpTexture = folder + tailToken;
	}

	materials[tempMaterial.MaterialName] = tempMaterial;
	return materials;
}

uint OBJLoader::FindLastVertexIndex(const Vector<OBJIndex*>& indexLookup, const OBJIndex* currentIndex, const IndexedModel& result)
{
	uint start = 0;
	uint end = (uint)indexLookup.size();
	uint current = (end - start) / 2 + start;
	uint previous = start;

	while (current != previous)
	{
		OBJIndex* testIndex = indexLookup[current];

		if (testIndex->vertexIndex == currentIndex->vertexIndex)
		{
			uint countStart = current;

			for (uint i = 0; i < current; i++)
			{
				OBJIndex* possibleIndex = indexLookup[current - i];

				if (possibleIndex == currentIndex)
					continue;

				if (possibleIndex->vertexIndex != currentIndex->vertexIndex)
					break;

				countStart--;
			}

			for (uint i = countStart; i < indexLookup.size() - countStart; i++)
			{
				OBJIndex* possibleIndex = indexLookup[current + i];

				if (possibleIndex == currentIndex)
					continue;

				if (possibleIndex->vertexIndex != currentIndex->vertexIndex)
					break;

				else if ((!hasUVs || possibleIndex->uvIndex == currentIndex->uvIndex)
					&& (!hasNormals || possibleIndex->normalIndex == currentIndex->normalIndex))
				{
					vec3 currentPosition = loadingModel.vertices[currentIndex->vertexIndex];
					vec2 currentTexCoord = (hasUVs) ? loadingModel.textures[currentIndex->uvIndex] : vec2(0, 0);
					vec3 currentNormal = (hasNormals) ? loadingModel.normals[currentIndex->normalIndex] : vec3(0, 0, 0);

					for (uint j = 0; j < result.vertices.size(); j++)
						if (currentPosition == result.vertices[j] && ((!hasUVs || currentTexCoord == result.textures[j]) && (!hasNormals || currentNormal == result.normals[j])))
							return j;
				}
			}

			return -1;
		}
		else
		{
			if (testIndex->vertexIndex < currentIndex->vertexIndex)
				start = current;
			else
				end = current;
		}

		previous = current;
		current = (end - start) / 2 + start;
	}

	return -1;
}

void OBJLoader::CreateOBJFace(const String& line)
{
	std::vector<String> tokens = String::split(line, ' ');

	OBJIndices.push_back(ParseOBJIndex(tokens[1]));
	OBJIndices.push_back(ParseOBJIndex(tokens[2]));
	OBJIndices.push_back(ParseOBJIndex(tokens[3]));

	if ((int)tokens.size() > 4)
	{
		OBJIndices.push_back(ParseOBJIndex(tokens[1]));
		OBJIndices.push_back(ParseOBJIndex(tokens[3]));
		OBJIndices.push_back(ParseOBJIndex(tokens[4]));
	}
}

OBJLoader::OBJIndex OBJLoader::ParseOBJIndex(const String& token)
{
	OBJIndex result;
	String substr;
	uint tokenLength = (uint)token.length();

	uint vertIndexStart = 0, vertIndexEnd = FindNextChar(vertIndexStart, token.c_str(), tokenLength, '/');

	//VERTICIES

	result.vertexIndex = ParseOBJIndexValue(token.substr(vertIndexStart, vertIndexEnd - vertIndexStart));
	result.uvIndex = 0;
	result.normalIndex = 0;

	if (vertIndexEnd >= tokenLength)
		return result;

	//TEXTURE COORDS

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, token.c_str(), tokenLength, '/');

	hasUVs = true;
	substr = token.substr(vertIndexStart, vertIndexEnd - vertIndexStart);

	if (substr != "")
		result.uvIndex = ParseOBJIndexValue(substr);
	else
		hasUVs = false;

	if (vertIndexEnd >= tokenLength)
		return result;

	//NORMALS

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, token.c_str(), tokenLength, '/');

	hasNormals = true;
	substr = token.substr(vertIndexStart, vertIndexEnd - vertIndexStart);

	if (substr != "")
		result.normalIndex = ParseOBJIndexValue(substr);
	else
		hasNormals = false;

	return result;
}

vec3 OBJLoader::ParseOBJVec3(const String& line)
{
	uint tokenLength = (uint)line.length();
	const char* tokenString = line.c_str();

	uint vertIndexStart = 2;

	while (vertIndexStart < tokenLength)
	{
		if (tokenString[vertIndexStart] != ' ')
			break;
		vertIndexStart++;
	}

	uint vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float x = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float y = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float z = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	return vec3(x, y, z);
}

vec2 OBJLoader::ParseOBJVec2(const String& line)
{
	uint tokenLength = (uint)line.length();
	const char* tokenString = line.c_str();

	uint vertIndexStart = 3;

	while (vertIndexStart < tokenLength)
	{
		if (tokenString[vertIndexStart] != ' ')
			break;
		vertIndexStart++;
	}

	uint vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float x = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	vertIndexStart = vertIndexEnd + 1;
	vertIndexEnd = FindNextChar(vertIndexStart, tokenString, tokenLength, ' ');

	float y = ParseOBJFloatValue(line, vertIndexStart, vertIndexEnd);

	return vec2(x, y);
}

static inline uint FindNextChar(uint start, const char* str, uint length, char token)
{
	uint result = start;

	for (; result < length; result++)
		if (str[result] == token)
			break;

	return result;
}

static inline uint ParseOBJIndexValue(const String& substr)
{
	return atoi(substr.c_str()) - 1;
}

static inline float ParseOBJFloatValue(const String& token, uint start, uint end)
{
	return (float)atof(token.substr(start, end - start).c_str());
}
