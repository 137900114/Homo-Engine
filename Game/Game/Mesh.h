#pragma once
#include "Buffer.h"
#include "uuid.h"
#include "Vector.h"

namespace Game{

	
	//a vertex contains texcoord(2) , tangent(3) , normal(3) , position(3) , color(4)
	//a index is a 16 bit data or 32 bit data
	struct Mesh {
		struct {
			UUID index;
			UUID vertex;
		} gpuDataToken;
		const static size_t vertexSize = sizeof(float) * 15;
		struct Vertex{
			Vector2 TexCoord;
			Vector3 Tangent;
			Vector3 Normal;
			Vector3 Position;
			Vector4 Color;
		};
		Buffer vertexList;
		size_t vertexNum;

		enum IndexType { I16 = 2,I32 = 4};
		Buffer indexList;
		size_t indexNum = 0;

		union {
			IndexType indexType;
			uint32_t  indexSize;
		};

		bool useIndexList;


		Mesh(IndexType type = I16):
			vertexList(),indexList(),useIndexList(false),vertexNum(0),indexNum(0),
			indexType(type){}

		Mesh(Mesh&& mesh) noexcept {
			moveMesh(std::move(mesh));
		}

		const Mesh& operator=(Mesh&& mesh) noexcept {
			moveMesh(std::move(mesh));
			return *this;
		}

		//it is not allowed to copy a mesh.
		Mesh(const Mesh& m) = delete;
		const Mesh& operator=(const Mesh& m) = delete;

		inline Vertex* resizeVertexList() { vertexList.resize(vertexNum * vertexSize); return reinterpret_cast<Vertex*>(vertexList.data); }
		inline void* resizeIndexList() { indexList.resize(indexNum * indexSize); return indexList.data; }

		inline Vertex* getVertexData() { return reinterpret_cast<Vertex*>(vertexList.data); }
		inline uint16_t* getIndex16() { return reinterpret_cast<uint16_t*>(indexList.data); }
		inline uint32_t* getIndex32() { return reinterpret_cast<uint32_t*>(indexList.data); }

	private:
		void moveMesh(Mesh&& mesh) {
			vertexNum = mesh.vertexNum;
			mesh.vertexNum = 0;
			useIndexList = mesh.useIndexList;
			mesh.useIndexList = false;
			indexNum = mesh.indexNum;
			mesh.indexNum = 0;
			indexType = mesh.indexType;
			mesh.indexType = I16;

			gpuDataToken.index = std::move(gpuDataToken.index);
			gpuDataToken.vertex = std::move(gpuDataToken.vertex);
			indexList = std::move(mesh.indexList);
			vertexList = std::move(mesh.vertexList);
		}
	};
	
	std::string to_string(Mesh& mesh);
}