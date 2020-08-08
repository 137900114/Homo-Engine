#include "Scene.h"
#include "FileLoader.h"
#include "GraphicModule.h"
#include "Common.h"
#include "Memory.h"
#include "Vector.h"
#include <sstream>

namespace Game {
	extern GraphicModule* gGraphic;
	extern FileLoader* gFileLoader;

	extern MemoryModule* gMemory;
}


Game::Scene::Scene(const char* name):
name(name){
	root = gMemory->New<SceneRootNode>(this);
}

Game::Scene::~Scene() {
	gMemory->Delete(root);

	for (auto& iter : meshs) {
		iter.second.vertexList.release();
		iter.second.indexList.release();
	}

	for (auto& iter : images) {
		iter.second.data.release();
	}

	for (auto& iter : buffers) {
		iter.second.buffer.release();
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
		if (!gFileLoader->FileReadAndClose(filename, filedata,LOAD_FILE_MODE::READ_CHARACTERS)) {
			Log("SceneLoader : fail to open file %s\n",filename);
			return nullptr;
		}
		_newScene = ObjPraser(filedata,file_name.c_str(),useIndex);
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

	gMemory->Delete(iter->second);
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
	Scene* scene = gMemory->New<Scene>(name);
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