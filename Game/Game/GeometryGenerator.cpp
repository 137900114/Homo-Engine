#include "GeometryGenerator.h"
#include "Math.h"

Game::Mesh Game::GeometryGenerator::generateCube(bool anticlockwise,Vector3 scaling) {
	Mesh result;
	//for cubes we don't use index list,or we will get strange normal
	result.useIndexList = false;

	result.vertexList.resize(36 * result.vertexSize);
	result.vertexNum = 36;
	
	Mesh::Vertex* vert = reinterpret_cast<Mesh::Vertex*>(result.vertexList.data);
	
	Vector3 VPos[8] = {
		{-1.f, 1.f, 1.f},{ 1.f, 1.f, 1.f},{ 1.f, 1.f,-1.f},{-1.f, 1.f,-1.f},
		{-1.f,-1.f, 1.f},{ 1.f,-1.f, 1.f},{ 1.f,-1.f,-1.f},{-1.f,-1.f,-1.f}
	};

	for (int i = 0; i != 8; i++) {
		VPos[i] = VPos[i] * scaling;
	}

	Vector3 Normals[6] = {
		{ 0.f, 1.f, 0.f},{ 0.f, 0.f, 1.f},{ 1.f, 0.f, 0.f},
		{ 0.f, 0.f,-1.f},{-1.f, 0.f, 0.f},{ 0.f,-1.f, 0.f}
	};

	//if the winding order is anticlockwise
	if (!anticlockwise) {
		std::swap(VPos[1], VPos[3]);
		std::swap(VPos[5], VPos[7]);

		std::swap(Normals[1],Normals[4]);
		std::swap(Normals[2],Normals[3]);
	}

	vert[0] =  { Vector2(),Vector3(),Normals[0],VPos[0] ,Vector4() };
	vert[1] =  { Vector2(),Vector3(),Normals[0],VPos[1] ,Vector4() };
	vert[2] =  { Vector2(),Vector3(),Normals[0],VPos[2] ,Vector4() };

	vert[3] =  { Vector2(),Vector3(),Normals[0],VPos[0] ,Vector4() };
	vert[4] =  { Vector2(),Vector3(),Normals[0],VPos[2] ,Vector4() };
	vert[5] =  { Vector2(),Vector3(),Normals[0],VPos[3] ,Vector4() };



	vert[6] =  { Vector2(),Vector3(),Normals[1],VPos[0] ,Vector4() };
	vert[7] =  { Vector2(),Vector3(),Normals[1],VPos[3] ,Vector4() };
	vert[8] =  { Vector2(),Vector3(),Normals[1],VPos[7] ,Vector4() };

	vert[9] =  { Vector2(),Vector3(),Normals[1],VPos[0] ,Vector4() };
	vert[10] = { Vector2(),Vector3(),Normals[1],VPos[7] ,Vector4() };
	vert[11] = { Vector2(),Vector3(),Normals[1],VPos[4] ,Vector4() };



	vert[12] = { Vector2(),Vector3(),Normals[2],VPos[3] ,Vector4() };
	vert[13] = { Vector2(),Vector3(),Normals[2],VPos[2] ,Vector4() };
	vert[14] = { Vector2(),Vector3(),Normals[2],VPos[6] ,Vector4() };

	vert[15] = { Vector2(),Vector3(),Normals[2],VPos[3] ,Vector4() };
	vert[16] = { Vector2(),Vector3(),Normals[2],VPos[6] ,Vector4() };
	vert[17] = { Vector2(),Vector3(),Normals[2],VPos[7] ,Vector4() };


	vert[18] = { Vector2(),Vector3(),Normals[3],VPos[2] ,Vector4() };
	vert[19] = { Vector2(),Vector3(),Normals[3],VPos[1] ,Vector4() };
	vert[20] = { Vector2(),Vector3(),Normals[3],VPos[5] ,Vector4() };

	vert[21] = { Vector2(),Vector3(),Normals[3],VPos[2] ,Vector4() };
	vert[22] = { Vector2(),Vector3(),Normals[3],VPos[5] ,Vector4() };
	vert[23] = { Vector2(),Vector3(),Normals[3],VPos[6] ,Vector4() };



	vert[24] = { Vector2(),Vector3(),Normals[4],VPos[1] ,Vector4() };
	vert[25] = { Vector2(),Vector3(),Normals[4],VPos[0] ,Vector4() };
	vert[26] = { Vector2(),Vector3(),Normals[4],VPos[4] ,Vector4() };

	vert[27] = { Vector2(),Vector3(),Normals[4],VPos[1] ,Vector4() };
	vert[28] = { Vector2(),Vector3(),Normals[4],VPos[4] ,Vector4() };
	vert[29] = { Vector2(),Vector3(),Normals[4],VPos[5] ,Vector4() };



	vert[30] = { Vector2(),Vector3(),Normals[5],VPos[4] ,Vector4() };
	vert[31] = { Vector2(),Vector3(),Normals[5],VPos[7] ,Vector4() };
	vert[32] = { Vector2(),Vector3(),Normals[5],VPos[6] ,Vector4() };

	vert[33] = { Vector2(),Vector3(),Normals[5],VPos[4] ,Vector4() };
	vert[34] = { Vector2(),Vector3(),Normals[5],VPos[6] ,Vector4() };
	vert[35] = { Vector2(),Vector3(),Normals[5],VPos[5] ,Vector4() };

	return std::move(result);
}

#define STEP(index,bound) ((index) + 1) % (bound)

template<typename T>
void loadSphereIndexBuffer(T* index,size_t faceNum,bool anticlockwise) {
	for (int i = 0; i != faceNum * 2; i++) {
		index[i * 3] = 0;
		index[i * 3 + 2] = i + 1;
		index[i * 3 + 1] = STEP(i, faceNum * 2) + 1;

		if (!anticlockwise) {
			std::swap(index[i * 3 + 1], index[i * 3 + 2]);
		}
	}
	index += faceNum * 2 * 3;
	T vIndex = 1;
	for (int y = 1; y != faceNum - 1; y++) {
		for (int x = 0; x != faceNum * 2; x++) {
			index[x * 6] = vIndex + x;
			index[x * 6 + 2] = vIndex + faceNum * 2 + x;
			index[x * 6 + 1] = vIndex + faceNum * 2 + STEP(x, faceNum * 2);

			index[x * 6 + 3] = vIndex + x;
			index[x * 6 + 5] = vIndex + faceNum * 2 + STEP(x, faceNum * 2);
			index[x * 6 + 4] = vIndex + STEP(x, faceNum * 2);

			if (!anticlockwise) {
				std::swap(index[x * 6 + 1], index[x * 6 + 2]);
				std::swap(index[x * 6 + 4], index[x * 6 + 5]);
			}
		}
		vIndex += faceNum * 2;
		index += faceNum * 12;
	}

	for (int i = 0; i != faceNum * 2; i++) {
		index[i * 3] = vIndex + i;
		index[i * 3 + 2] = vIndex + faceNum * 2;
		index[i * 3 + 1] = vIndex + STEP(i, faceNum * 2);

		if (!anticlockwise) {
			std::swap(index[i * 3 + 1], index[i * 3 + 2]);
		}
	}
}

Game::Mesh Game::GeometryGenerator::generateSphere(bool anticlockwise, float radius, uint32_t faceNum) {
	uint32_t vertNum = 2 + (faceNum - 1) * faceNum * 2;
	uint32_t indexNum = faceNum * (faceNum - 1) * 12;
	
	Mesh result;
	result.indexType = vertNum > 65535 ? Mesh::I32 : Mesh::I16;
	result.indexNum = indexNum;
	result.indexList.resize(result.indexSize * indexNum);
	result.useIndexList = true;

	result.vertexList.resize(vertNum * result.vertexSize);
	result.vertexNum = vertNum;

	uint16_t* index16 = reinterpret_cast<uint16_t*>(result.indexList.data);
	uint32_t* index32 = reinterpret_cast<uint32_t*>(result.indexList.data);

	Mesh::Vertex* vertex = reinterpret_cast<Mesh::Vertex*>(result.vertexList.data);

	//fill the vertex buffer
	vertex[0] = { Vector2(0.f,0.f),Vector3(),Vector3(0.f,1.f,0.f),Vector3(0.f,radius,0.f),Vector4() };
	vertex[vertNum - 1] = {Vector2(1.f,1.f),Vector3(),Vector3(0.f,-1.f,0.f),Vector3(0.f,-radius,0.f),Vector4()};

	if (!anticlockwise) {
		vertex[0].Normal = vertex[0].Normal * (-1.f);
		vertex[vertNum - 1].Normal = vertex[vertNum - 1].Normal * (-1.f);
	}

	float theta = PI / static_cast<float>(faceNum);
	float invPI2 =  1.f / (PI * 2.f);
	float invPI = 1.f / PI;
	for(int x  = 0;x != faceNum * 2;x++){
		for (int y = 1; y != faceNum; y++) {
			float u = theta * x;
			float v = theta * y;

			Vector3 Normal = Vector3(sin(v) * cos(u), cos(v), sin(v) * sin(u));
			if (!anticlockwise) {
				Normal = Vector3() - Normal;
			}
			Vector3 Position = Normal * radius;
			Vector2 Texcoord = Vector2(u * invPI2 ,v * invPI);

			vertex[x + faceNum * 2 * (y - 1) + 1] = {Texcoord,Vector3(),Normal,Position,Vector4()};
		}
	}
	
	//fill the index buffer
	if (result.indexType == Mesh::I16) {
		loadSphereIndexBuffer(index16,faceNum,anticlockwise);
	}
	else {
		loadSphereIndexBuffer(index32, faceNum, anticlockwise);
	}

	return std::move(result);
}