#include "Scene.h"
#include "FileLoader.h"
#include "GraphicModule.h"
#include "Common.h"
#include "Memory.h"
#include "Vector.h"
#include "Timer.h"
#include "tinyxml2.h"
#include "MeshRenderer.h"
#include <sstream>
#include <queue>
using namespace tinyxml2;

namespace Game {
	extern GraphicModule* gGraphic;
	extern FileLoader gFileLoader;

	extern MemoryModule gMemory;
	extern Timer gTimer;

	SceneLoader gSceneLoader;
}

Game::Scene::Scene(const char* name):
name(name){
	root = gMemory.New<SceneRootNode>("root",this);
	skybox = nullptr;
}

Game::Scene::~Scene() {
	gMemory.Delete(root);

	for (auto& iter : meshs) {
		iter.second.vertexList.release();
		iter.second.indexList.release();
	}

	for (auto& iter : textures) {
		iter.second.data.release();
	}
}

Game::Scene* Game::SceneLoader::loadScene(const char* filename,bool useIndex) {

	Buffer filedata;
	Scene* _newScene;

	std::string file_extension, file_name;
	getExtensionName(filename, file_extension, file_name);

	auto iter = scenesMap.find(file_name);
	if (iter != scenesMap.end()) {
		Log("SceneLoader : Scene %s have been loaded\n",filename);
		return iter->second;
	}
	
	if (file_extension == "obj") {
		if (!gFileLoader.FileReadAndClose(filename, filedata,LOAD_FILE_MODE::READ_CHARACTERS)) {
			Log("SceneLoader : fail to open file %s\n",filename);
			return nullptr;
		}
		_newScene = ObjPraser(filedata,file_name.c_str(),useIndex);
	}
	else if (file_extension == "dae") {
		if (!gFileLoader.FileReadAndClose(filename,filedata,LOAD_FILE_MODE::READ_CHARACTERS)) {
			Log("SceneLoader : fail to open file %s\n",filename);
			return nullptr;
		}
		_newScene = DaePraser(filedata,file_name.c_str(),useIndex);
	}
	else {
		return nullptr;
	}

	//if the scene parser fails
	if (!_newScene) return nullptr;

	scenesMap[file_name] = _newScene;

	gGraphic->uploadScene(*_newScene);
	return  _newScene;
}


Game::Scene* Game::SceneLoader::getScene(const char* scene_name) {
	auto iter = scenesMap.find(std::string(scene_name));
	if (iter == scenesMap.end()) {
		Log("SceneLoader : fail to find scene whose name is %s\n",scene_name);
		return nullptr;
	}

	return iter->second;
}

void Game::SceneLoader::destroyScene(const char* scene_name) {
	auto iter = scenesMap.find(std::string(scene_name));
	if (iter == scenesMap.end()) {
		Log("ScenesLoader : the scene to destroy %s doesn't exists!\n",scene_name);
		return;
	}

	//maybe some data have uploaded to gpu in graphic module,we need to destroy them!
	gGraphic->releaseScene(*iter->second);

	gMemory.Delete(iter->second);
}



//currently we only read some mesh into the scene
Game::Scene* Game::SceneLoader::ObjPraser(Game::Buffer& buf,const char* name,bool useIndex) {
	std::string data = reinterpret_cast<char*>(buf.data);

	std::istringstream strstr(data);
	std::string line;

	struct FaceType{
		struct {
			uint32_t ip, it, in;
		} Vert[4];
		//three or four
		int vertNum;
	};
	
	//std::vector<std::string> materialList;
	std::vector<Game::Vector3> Position;
	std::vector<Game::Vector3> Normal;
	std::vector<Game::Vector2> Texcoord;
	std::vector<FaceType> face;
	Mesh* currentMesh = nullptr;
	size_t vertexNum = 0;
	Scene* scene = gMemory.New<Scene>(name);
	int line_counter = 0;
	while (std::getline(strstr, line)) {
		line_counter++;
		if (line[0] == '#') continue;//skip the commet

		std::vector<std::string> strLis;
		str_split(line, strLis);

		/*
		now we just ignore the material
		if (strLis[0].compare("mtllib") == 0 || strLis[0].compare("newmtl") == 0) {
			Buffer buf;
			if (!gFileLoader.FileReadAndClose(strLis[1].c_str(),buf,READ_CHARACTERS)) {
				Log("fileloader : fail to parse object file %s,the material file %s doesn't exists!\n",
					name,strLis[1].c_str());
				gMemory->Delete(newScene);
			}
			MtlParse(newScene,buf);
		}else if(strLis[0].compare("usemtl")){
			materialList.push_back();
		}
		*/
		if (strLis[0].compare("o") == 0) {

			vertexNum = 0;
			scene->meshs[strLis[1]] = std::move(Mesh());
			currentMesh = &(scene->meshs[strLis[1]]);
		}
		else if (strLis[0].compare("v") == 0) {
			if (strLis.size() < 4) {
				Log("fileloader : invalid data in file %s : the vertex position data must have at least 3 dimension at line %d\n",
					name,line_counter);
				Position.push_back(Vector3());
				continue;
			}
			Position.push_back(Vector3(std::stof(strLis[1]), std::stof(strLis[2]), std::stof(strLis[3])));
		}
		else if (strLis[0].compare("vn") == 0) {
			if (strLis.size() < 4) {
				Log("fileloader : invalid data in file %s: the normal data must have at least 3 dimension at line %d\n",
					name,line_counter);
				Normal.push_back(Vector3());
				continue;
			}
			
			Normal.push_back(Vector3(std::stof(strLis[1]), std::stof(strLis[2]), std::stof(strLis[3])));
		}
		else if (strLis[0].compare("vt") == 0) {
			if (strLis.size() < 3) {
				Log("fileloader : invalid object file %s:invalid texture coordinate data at least 2 dimension at line %d\n",
					name,line_counter);
				Texcoord.push_back(Vector2());
				continue;
			}
			Texcoord.push_back(Vector2(std::stof(strLis[1]), std::stof(strLis[2])));
		}
		else if (strLis[0].compare("f") == 0) {
			std::vector<std::string> indexs;
			FaceType _face;
			for (int j = 1; j != strLis.size(); j++) {
				str_split(strLis[j],indexs,'/');
				_face.Vert[j - 1].ip = std::stoi(indexs[0]);
				_face.Vert[j - 1].it = std::stoi(indexs[1]);
				_face.Vert[j - 1].in = std::stoi(indexs[2]);
			}

			_face.vertNum = strLis.size() - 1;
			vertexNum += (_face.vertNum - 2) * 3;
			face.push_back(_face);
		}
	}

	if (currentMesh == nullptr) return nullptr;

	if (useIndex) {
		//Create the vertex buffer and set initial value to 0
		currentMesh->vertexList.resize(Mesh::vertexSize * Position.size());
		Mesh::Vertex* vert = reinterpret_cast<Mesh::Vertex*>(currentMesh->vertexList.data);
		currentMesh->vertexNum = Position.size();
		memset(vert,0,currentMesh->vertexList.size);

		currentMesh->indexType = vertexNum < (2 << 16) ? Mesh::I16 : Mesh::I32;
		currentMesh->indexNum = vertexNum;
		currentMesh->useIndexList = true;
		currentMesh->indexList.resize(currentMesh->indexSize * vertexNum);

		//sum up the times the indeices appear , and do average for normals
		std::vector<uint32_t> indexCount(vertexNum,0);

#define CalcuVertexAndIterateIndex(type)	type* currI = reinterpret_cast<type*>(currentMesh->indexList.data);\
									for (auto& item : face) {\
										for (int i = 0; i != item.vertNum; i++) {\
											Mesh::Vertex* currVert = vert + (item.Vert[i].ip - 1);\
											\
											currVert->Position = Position[item.Vert[i].ip - 1];\
											currVert->Normal = currVert->Normal + Normal[item.Vert[i].in - 1];\
											currVert->TexCoord = Texcoord[item.Vert[i].it - 1];\
											indexCount[item.Vert[i].ip - 1]++;\
										}\
											\
										*currI = item.Vert[0].ip - 1;\
										currI++;\
										*currI = item.Vert[1].ip - 1;\
										currI++;\
										*currI = item.Vert[2].ip - 1;\
										currI++;\
										if (item.vertNum == 4) {\
											*currI = item.Vert[0].ip - 1;\
											currI++;\
											*currI = item.Vert[2].ip - 1;\
											currI++;\
											*currI = item.Vert[3].ip - 1;\
											currI++;\
										}\
									}

		if (currentMesh->indexType == Mesh::I16) {
			CalcuVertexAndIterateIndex(uint16_t);
		}
		else {
			CalcuVertexAndIterateIndex(uint32_t);
		}
		//the normal of every vertex will be average of every face
		for (int i = 0; i != Position.size(); i++) {
			(vert + i)->Normal = normalize((vert + i)->Normal / (float)indexCount[i]);
		}
	}else {
		currentMesh->indexNum = 0;
		currentMesh->useIndexList = false;

		currentMesh->vertexList.resize(vertexNum * Mesh::vertexSize);
		Mesh::Vertex* vertex = reinterpret_cast<Mesh::Vertex*>(currentMesh->vertexList.data);
		for (auto& item : face) {
			Mesh::Vertex newVert[4];
			for (int i = 0; i != item.vertNum; i++) {
				newVert[i].Normal = Normal[item.Vert[i].in - 1];
				newVert[i].Position = Position[item.Vert[i].ip - 1];
				newVert[i].TexCoord = Texcoord[item.Vert[i].it - 1];
			}
			*vertex = newVert[0];
			vertex++;
			*vertex = newVert[1];
			vertex++;
			*vertex = newVert[2];
			vertex++;
			if (item.vertNum == 4) {
				*vertex = newVert[0];
				vertex++;
				*vertex = newVert[2];
				vertex++;
				*vertex = newVert[3];
				vertex++;

			}
		}
		currentMesh->vertexNum = vertexNum;
	}

	return scene;
}


struct ArrayList{
	Game::Buffer buffer;
	int stride;
	int count;

	ArrayList() {}
	ArrayList(ArrayList&& other) { buffer = std::move(other.buffer); }
	ArrayList& operator=(ArrayList&& other) {
		buffer = std::move(other.buffer);
		stride = other.stride;
		count = other.count;
		return *this;
	}

	template<typename T>
	T get(size_t index) {
		return *(reinterpret_cast<T*>(buffer.data) + index);
	}

	template<typename T>
	bool set(size_t index,T data) {
		if (index * sizeof(T) < buffer.size) {
			*(reinterpret_cast<T*>(buffer.data) + index) = data;
			return true;
		}

		return false;
	}
};


inline void dae_cast_url_to_id(const char** url) {
	if(**url == '#')*url = (*url) + 1;
}

struct DaeSceneParser {

	std::map<std::string, ArrayList> arrayDataList;
	XMLElement* collada;

	Game::Scene* target;
	bool theUpAxisisY = true;
	bool ParseGeometry(const char** error_str);
	bool ParseCamera(const char** error_str);
	bool ParseLight(const char** error_str);
	bool ParseScene(const char** error_str);
	bool ParseMaterial(const char** error_str);

	std::map<std::string, Game::Camera> cameras;
	std::map<std::string, Game::Light> lights;
};


Game::Scene* Game::SceneLoader::DaePraser(Game::Buffer& buf, const char* name, bool /*useIndex*/) {
	XMLDocument file;

	XMLError error = file.Parse(reinterpret_cast<char*>(buf.data), buf.size);
	if (error) {
		Log("fail to parse xml file %s,fail code %d", name, error);
		return nullptr;
	}

	XMLElement* collada = file.FirstChildElement("COLLADA");
	if (!collada) {
		Log("fail to parse dea file %s,can't find node COLLADA\n", name);
		return nullptr;
	}

	DaeSceneParser parser;
	Scene* target = gMemory.New<Scene>(name);
	parser.target = target;
	parser.collada = collada;

	const char* error_str;


	XMLElement* asset = collada->FirstChildElement("asset");
	if (asset) {
		const char* axis = asset->FirstChildElement("up_axis")->FirstChild()->Value();
		if (!strcmp(axis, "Z_UP")) {
			parser.theUpAxisisY = false;
		}
	}
	else {
		Log("warning: collada file doesn't contains the assert node,this may cause the api will not act as you expected\n");
	}

	if (!parser.ParseCamera(&error_str) ||
		!parser.ParseLight(&error_str) ||
		!parser.ParseMaterial(&error_str) ||
		!parser.ParseGeometry(&error_str) ||
		!parser.ParseScene(&error_str)) {

		Log("fail to parse dea file,reason %s", error_str);
		gMemory.Delete(target);
		return nullptr;
	}

	//gGraphic->uploadScene(*target);
	this->scenesMap[name] = target;

	return target;
}

bool DaeSceneParser::ParseMaterial(const char** error) {
	return true;
}

//currently we don't consider generate tangent
bool DaeSceneParser::ParseGeometry(const char** error) {
	XMLElement* geometry_library = collada->FirstChildElement("library_geometries");

	if (!geometry_library) {
		*error = "fail to parse geomtry library , can't find node library_geometries";
		return false;
	}

	std::map<std::string, ArrayList> source;
	std::map<std::string, std::string> vertices;
	XMLElement* curr_geo = geometry_library->FirstChildElement("geometry");
	while (curr_geo) {
		const char* geometry_name;
		curr_geo->QueryStringAttribute("id", &geometry_name);
		Game::Mesh geometry;

		XMLElement* geo_ele = curr_geo->FirstChildElement()->FirstChildElement();
		while (geo_ele) {
			const char* element_name = geo_ele->Value();
			if (!strcmp(element_name, "source")) {
				XMLElement* float_array_element = geo_ele->FirstChildElement("float_array");
				if (!float_array_element) continue;

				const char* source_id;
				int count;
				geo_ele->QueryStringAttribute("id", &source_id);
				float_array_element->QueryIntAttribute("count", &count);

				std::string source_data = float_array_element->FirstChild()->Value();
				std::vector<std::string> source_data_array;
				str_split(source_data, source_data_array);

				ArrayList arrayData;
				arrayData.buffer.resize(sizeof(float) * count);
				for (int i = 0; i != count; i++) {
					arrayData.set(i, std::stof(source_data_array[i]));
				}

				float_array_element->NextSiblingElement()->FirstChildElement()->QueryIntAttribute("stride",
					&arrayData.stride);
				float_array_element->NextSiblingElement()->FirstChildElement()->QueryIntAttribute("count",
					&arrayData.count);

				source[source_id] = std::move(arrayData);
			}
			else if (!strcmp(element_name, "vertices")) {
				const char* vertice_id, * array_id;
				geo_ele->QueryStringAttribute("id", &vertice_id);
				geo_ele->FirstChildElement()->QueryStringAttribute("source", &array_id);
				
				dae_cast_url_to_id(&array_id);
				vertices[vertice_id] = array_id;
			}
			else if (!strcmp(element_name, "triangles")) {
				//currently we ignore material
				enum VERTEX_TARGET {
					TARGET_TEXCOORD,
					//TARGET_TANGENT,
					TARGET_NORMAL,
					TARGET_POSITION,
					TARGET_COLOR
				};
				//parse input element
				XMLElement* input_ele = geo_ele->FirstChildElement();
				int vertex_num, normal_num;
				std::vector<VERTEX_TARGET> write_targets;
				std::vector<ArrayList*> write_source;
				while (input_ele) {
					if (strcmp(input_ele->Value(), "input")) {
						break;
					}

					const char* name, * url;
					input_ele->QueryStringAttribute("semantic", &name);
					input_ele->QueryStringAttribute("source", &url);
					dae_cast_url_to_id(&url);
					if (!strcmp(name, "VERTEX")) {
						auto find_vertices = vertices.find(url);
						if (find_vertices == vertices.end()) {
							*error = "invaild vertex";
							return false;
						}
						url = find_vertices->second.c_str();
						dae_cast_url_to_id(&url);
					}

					auto find_source = source.find(url);
					if (find_source == source.end()) {
						*error = "invaild source,the file may be damaged";
						return false;
					}
					write_source.push_back(&find_source->second);

					if (!strcmp(name, "VERTEX")) {
						write_targets.push_back(TARGET_POSITION);
						vertex_num = write_source[write_source.size() - 1]->count;
					}
					else if (!strcmp(name, "NORMAL")) {
						write_targets.push_back(TARGET_NORMAL);
						normal_num = write_source[write_source.size() - 1]->count;
					}
					else if (!strcmp(name, "TEXCOORD")) {
						write_targets.push_back(TARGET_TEXCOORD);
					}
					input_ele = input_ele->NextSiblingElement();
				}

				int count,vertex_stride = write_source.size();
				geo_ele->QueryIntAttribute("count", &count);
				std::string index_data_str = geo_ele->FirstChildElement("p")->FirstChild()->Value();
				std::vector<std::string> index_data_array;
				str_split(index_data_str, index_data_array);

				if (index_data_array.size() != count * 3 * vertex_stride) {
					*error = "invaild geometry triangle data,the index data count doesn't match";
					return false;
				}
				bool theUpAxisisY = this->theUpAxisisY;
				auto access_vertex = [write_targets, write_source, index_data_array,theUpAxisisY](int index)->Game::Mesh::Vertex {
					Game::Mesh::Vertex result;

					for (int i = 0; i != write_targets.size(); i++) {
						int offset = index * write_targets.size() + i;
						int _index = stoi(index_data_array[offset]);
						switch (write_targets[i]) {
						case TARGET_POSITION:
							result.Position = write_source[i]->get<Game::Vector3>(_index);
							if (!theUpAxisisY) {
								std::swap(result.Position.y,result.Position.z);
								result.Position.x = -result.Position.x;
							}
							break;
						case TARGET_TEXCOORD:
							result.TexCoord = write_source[i]->get<Game::Vector2>(_index);
							break;
						case TARGET_NORMAL:
							result.Normal = write_source[i]->get<Game::Vector3>(_index);
							if (!theUpAxisisY) {
								std::swap(result.Normal.y,result.Normal.z);
							}
							break;
						case TARGET_COLOR:
							result.Color = write_source[i]->get<Game::Vector4>(_index);
							break;
						}
					}

					return result;
				};
				
				if (vertex_num == normal_num) {
					geometry.useIndexList = true;
					geometry.indexNum = count * 3;
					if (geometry.indexNum < 1 << 16) {
						geometry.indexType = Game::Mesh::I16;
					}
					else {
						geometry.indexType = Game::Mesh::I32;
					}

					geometry.indexList.resize(geometry.indexSize * geometry.indexNum);
					geometry.vertexNum = vertex_num;
					geometry.vertexList.resize(geometry.vertexNum * geometry.vertexSize);

					Game::Mesh::Vertex* vertex_ptr = reinterpret_cast<Game::Mesh::Vertex*>(geometry.vertexList.data);

					for (int i = 0; i != count * 3; i++) {
						int vertex_index = stoi(index_data_array[i * vertex_stride]);

						if (geometry.indexType == Game::Mesh::I16) {
							*(reinterpret_cast<uint16_t*>(geometry.indexList.data) + i)
								= static_cast<uint16_t>(vertex_index);
						}
						else {
							*(reinterpret_cast<uint32_t*>(geometry.indexList.data) + i)
								= static_cast<uint32_t>(vertex_index);
						}

						vertex_ptr[vertex_index] = access_vertex(i);
					}
				}
				else {
					geometry.useIndexList = false;
					geometry.vertexNum = count * 3;
					geometry.vertexList.resize(geometry.vertexNum * geometry.vertexSize);

					Game::Mesh::Vertex* vertex_ptr = reinterpret_cast<Game::Mesh::Vertex*>(geometry.vertexList.data);

					for (int i = 0; i != count * 3; i++) {
						vertex_ptr[i] = access_vertex(i);
					}
				}

			}

			geo_ele = geo_ele->NextSiblingElement();
		}

		target->meshs[geometry_name] = std::move(geometry);
		curr_geo = curr_geo->NextSiblingElement("geometry");
	}

	return true;
}


bool DaeSceneParser::ParseCamera(const char** error) {

	XMLElement* camera_library = collada->FirstChildElement("library_cameras");
	if (!camera_library) {
		*error = "fail to parse camera data , can't find node library_cameras";
		return false;
	}

	XMLElement* camera_ele = camera_library->FirstChildElement("camera");
	while (camera_ele) {

		Game::Camera camera;
		const char * url;
		camera_ele->QueryStringAttribute("id",&url);

		XMLElement* perspective = camera_ele->FirstChildElement()->FirstChildElement("technique_common")->FirstChildElement("perspective");
		
		float fovx, aspect_ratio, znear, zfar;
		fovx = std::stof(std::string(perspective->FirstChildElement("xfov")->FirstChild()->Value()));
		aspect_ratio = std::stof(std::string(perspective->FirstChildElement("aspect_ratio")->FirstChild()->Value()));
		znear = std::stof(std::string(perspective->FirstChildElement("znear")->FirstChild()->Value()));
		zfar = std::stof(std::string(perspective->FirstChildElement("zfar")->FirstChild()->Value()));

		//convert radian coordiate
		fovx = fovx * PI / 180.;
		float tangx = tanf(fovx / 2.);
		float tangy = tangx / aspect_ratio;
		float fovy = atanf(tangy) * 2.;

		camera.SetProjection(znear, zfar, aspect_ratio, fovy);
		cameras[url] = camera;
		camera_ele = camera_ele->NextSiblingElement("camera");
	}

	return true;
}

bool DaeSceneParser::ParseLight(const char** error) {

	XMLElement* light_library = collada->FirstChildElement("library_lights");
	if (!light_library) {
		*error = "fail to parse light library,no light library found";
		return false;
	}

	XMLElement* light_ele = light_library->FirstChildElement("light");
	while (light_ele) {
		const char* id;
		light_ele->QueryStringAttribute("id" , &id);
		
		XMLElement* light_node;
		if (light_node = light_ele->FirstChildElement()->FirstChildElement("point")) {
			std::vector<std::string> color;
			str_split(std::string(light_node->FirstChildElement("color")->FirstChild()->Value()),color);

			Game::Light light;
			light.lightType = Game::ShaderLightType::LIGHT_TYPE_POINT;
			light.lightIntensity = Game::Vector3(stof(color[0]),stof(color[1]),stof(color[2]));

			lights[id] = light;
		}
		else if(light_node = light_ele->FirstChildElement()->FirstChildElement("directional")){
			std::vector<std::string> color;
			str_split(light_node->FirstChildElement("color")->FirstChild()->Value(), color);

			Game::Light light;
			light.lightType = Game::ShaderLightType::LIGHT_TYPE_DIRECTIONAL;
			light.lightIntensity = Game::Vector3(stof(color[0]),stof(color[1]),stof(color[2]));

			lights[id] = light;
		}

		light_ele = light_ele->NextSiblingElement();
	}
	return true;
}

bool parse_scene_element(Game::SceneRootNode* root,XMLElement* scene_nodes, Game::Scene* target, const char** error, bool UpIsY,
	std::map<std::string, Game::Light>& lightMap,std::map<std::string,Game::Camera>& camera) {
	const char* name;
	scene_nodes->QueryStringAttribute("name", &name);

	std::string transfrom_str = scene_nodes->FirstChildElement("matrix")->FirstChild()->Value();
	std::vector<std::string> transfrom_str_array;
	str_split(transfrom_str, transfrom_str_array);

	float transfrom_array[16];
	for (int i = 0; i != 16; i++) {
		transfrom_array[i] = std::stof(transfrom_str_array[i]);
	}


	
	Game::Mat4x4 world = Game::Mat4x4(transfrom_array);
	if (!UpIsY) {
		Game::Mat4x4 transCoord = Game::Mat4x4(-1.,0.,0.,0.,
												0.,0.,1.,0.,
												0.,1.,0.,0.,
												0.,0.,0.,1.);

		world = mul(transCoord,mul(world,transCoord));
	}

	Game::SceneTransform transform;
	transform.UnpackTransform(world,root->transform.GetWorld());
	Game::SceneRootNode* curr_node = nullptr;

	XMLElement* ele = nullptr;
	if (ele = scene_nodes->FirstChildElement("instance_geometry")) {

		const char* url;
		ele->QueryStringAttribute("url", &url);
		dae_cast_url_to_id(&url);

		auto query_mesh = target->meshs.find(url);
		if (query_mesh == target->meshs.end()) {
			*error = "fail to load geometry invaild geometry url";
			return false;
		}

		Game::SceneObject* object = Game::gMemory.New<Game::SceneObject>(name, target);
		object->transform = transform;
		object->geoMesh = &query_mesh->second;
		target->root->childs.push_back(object);
		curr_node = object;


		Game::MeshRenderer* renderer = Game::gMemory.New<Game::MeshRenderer>(object);
		object->components.push_back(renderer);

	}
	else if (ele = scene_nodes->FirstChildElement("instance_camera")) {

		const char* url;
		ele->QueryStringAttribute("url", &url);
		dae_cast_url_to_id(&url);

		auto query_camera = camera.find(url);

		if (query_camera == camera.end()) {
			*error = "fail to load camera invaild caemra url";
			return false;
		}

		Game::SceneCamera* object = Game::gMemory.New<Game::SceneCamera>(name, target, query_camera->second);
		object->transform = transform;

		target->root->childs.push_back(object);
		if (!target->mainCamera) {
			target->mainCamera = object;
		}
		curr_node = object;
	}
	else if (ele = scene_nodes->FirstChildElement("instance_light")) {

		const char* url;
		ele->QueryStringAttribute("url", &url);
		dae_cast_url_to_id(&url);

		auto query_light = lightMap.find(url);

		if (query_light == lightMap.end()) {
			*error = "fail to load light invaild light url";
			return false;
		}

		query_light->second.lightIntensity = Game::Vector3(1.,1.,1.);
		Game::SceneLight* object = Game::gMemory.New<Game::SceneLight>(name, target, query_light->second);
		object->transform = transform;

		target->root->childs.push_back(object);
		if (!target->mainLight) {
			target->mainLight = object;
		}
		curr_node = object;
	}
	else {
		Game::SceneRootNode* object = Game::gMemory.New<Game::SceneRootNode>(name,target);
		object->transform = transform;

		target->root->childs.push_back(object);
		curr_node = object;
	}

	ele = scene_nodes->FirstChildElement("node");
	while (ele && curr_node) {
		if (!parse_scene_element(curr_node,ele,target,error,UpIsY,lightMap,
			camera)) {
			return false;
		}
		ele = ele->NextSiblingElement("node");
	}

	return true;
}

bool DaeSceneParser::ParseScene(const char** error) {

	XMLElement* scene_library = collada->FirstChildElement("library_visual_scenes");
	if (!scene_library) {
		*error = "fail to parse scene library,no scene library found";
		return false;
	}

	XMLElement* scene = scene_library->FirstChildElement("visual_scene");
	if (!scene) {
		*error = "no scene founded in scene library";
		return false;
	}

	XMLElement* scene_nodes = scene->FirstChildElement("node");
	if (scene_nodes == nullptr) {
		Log("the scene %s is empty no node inside",target->name.c_str());
		return true;
	}

	while (scene_nodes) {
		if (!parse_scene_element(target->root, scene_nodes, target, error,this->theUpAxisisY,this->lights,
			this->cameras)) {
			return false;
		}
		scene_nodes = scene_nodes->NextSiblingElement("node");
	}
	
	return true;
}

bool Game::SceneManager::initialize() {

	return true;
}

void Game::SceneManager::go_through_all_nodes(Game::SceneRootNode* root,void (Game::SceneRootNode::* opera)()) {
	(root->*opera)();
	for (int i = 0; i != root->childs.size(); i++) {	
		go_through_all_nodes(root->childs[i], opera);
	}
}


void Game::SceneManager::go_through_all_nodes(Game::SceneRootNode* root,float deltaTime) {
	
	for (int i = 0; i != root->childs.size(); i++) {
		root->childs[i]->update(deltaTime);
		go_through_all_nodes(root->childs[i], deltaTime);
	}
}


void Game::SceneManager::tick() {
	if (!currentScene) return;

	for (auto& item : currentScene->meshs) {
		if (item.second.gpuDataToken.vertex != UUID::invalid) {
			item.second.vertexList.release();
			if (item.second.useIndexList) {
				item.second.indexList.release();
			}
		}
	}

	float deltaTime = gTimer.DeltaTime();
	go_through_all_nodes(currentScene->root,deltaTime);
}


void Game::SceneManager::finalize() {
	//gSceneLoader.destroyScene(currentScene->name.c_str());
	//gMemory->Delete(currentScene);
}

bool Game::SceneManager::getScene(const char* scenename,bool distroy_last_scene) {
	if (distroy_last_scene && currentScene != nullptr) {
		go_through_all_nodes(currentScene->root,&SceneRootNode::finalize);
		gSceneLoader.destroyScene(currentScene->name.c_str());
	}
	
	currentScene = gSceneLoader.getScene(scenename);
	return currentScene != nullptr;
}

bool Game::SceneManager::loadScene(const char* filename,bool switch_to_current_scene,bool distroy_last_scene) {
	if (distroy_last_scene && currentScene != nullptr) {
		go_through_all_nodes(currentScene->root,&SceneRootNode::finalize);
		gSceneLoader.destroyScene(currentScene->name.c_str());
	}
	Scene* newScene = gSceneLoader.loadScene(filename);
	if (switch_to_current_scene && newScene != nullptr) {
		currentScene = newScene;
		go_through_all_nodes(currentScene->root,&SceneRootNode::initialize);
	}

	return currentScene != nullptr;
}

Game::SceneRootNode* Game::SceneManager::QueryNode(std::string name,bool depth_first) {
	if (depth_first) {
		return dfs_find_node(name, currentScene->root);
	}
	else {
		return bfs_find_node(name);
	}
}

Game::SceneRootNode* Game::SceneManager::dfs_find_node(const std::string& name,Game::SceneRootNode* root) {
	for (auto& item : root->childs) {
		Game::SceneRootNode* node =	dfs_find_node(name, item);
		if (node != nullptr) return node;
	}

	if (root->name.compare(name) == 0) {
		return root;
	}

	return nullptr;
}

Game::SceneRootNode* Game::SceneManager::bfs_find_node(const std::string& name) {
	std::queue<Game::SceneRootNode*> next;
	next.push(currentScene->root);
	
	while (!next.empty()) {
		Game::SceneRootNode* node = next.front();
		next.pop();
		if (node->name.compare(name) == 0) {
			return node;
		}

		for (auto& item : node->childs) {
			next.push(item);
		}
	}
	
	return nullptr;
}