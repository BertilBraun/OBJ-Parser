# OBJ-Parser
OBJ File parser loading Indexed Models and Materials

usage:

```c++
  OBJLoader loader("PATH TO LOAD");
  
  for (IndexedModel& model : loader.GetModels()) {
    
    //process Model
    
  }
}
```
OBJLoader:
```c++
class OBJLoader {
		OBJLoader(const String& fileName);  //Loades models from file : warning : large models can take a second or two

		Vector<IndexedModel> GetModels();
  };
``` 
IndexedModel:
```c++
  struct IndexedModel {
		String name;

		Material material;

		Vector<vec3> vertices;
		Vector<vec2> textures;
		Vector<vec3> normals;
		Vector<uint> indices;
	};
``` 

Material:
```c++
  struct Material
	{
		String MaterialName;

		vec3 AmbientColor;
		vec3 DiffuseColor;
		vec3 SpecularColor;

		float SpecularExponent;
		float OpticalDensity;
		float Dissolve;
		int Illumination;
	
		String AmbientTexture;
		String DiffuseTexture;
		String SpecularTexture;
		String SpecularHightlight;
		String AlphaTexture;
		String BumpTexture;
	};
``` 
dependancie:
  requires glm vor vec3, vec2 and optimized calculations

additional:

```c++
  template<typename T>
  class Vector : std::vector<T>
``` 
features:

```c++
  void swapRemove(int index);
  void insertEnd(Vector<T>& toInsert, const Vector<T>& data);
  bool containsAll(const std::vector<T>& o);
  bool contains(const T& val);
``` 

additional:

```c++
  class String : public std::string
``` 
features:

```c++
	static String Trim(const String& str, char c = ' ');
	static String TrimTail(const String& str, char c);

	static String ToString(glm::vec3 val);
	static String ToString(glm::vec2 val);

	static String ToUpper(const String& str);
	static String ToLower(const String& str);

	static Vector<String> getFileContent(const String& path);
  
	static Vector<String> split(const String& str, const String& delim);
	static Vector<String> split(const String& str, char delim);

	static String tail(const String &in);
	static String folder(const String &in);
	static String firstToken(const String &in);

	static bool isWhitespace(char c);
	static bool isWhitespace(const String & c);
```
