#include "Mesh.h"
#include "FileLoader.h"
#include "Common.h"

namespace Game {
	extern FileLoader* gFileLoader;
	std::string to_string(Game::Vector2 vec) {
		return "( " + std::to_string(vec.x) + " , " + std::to_string(vec.y) + " )";
	}

	std::string to_string(Game::Vector3 vec) {

		return "( " + std::to_string(vec.x) + " , " + std::to_string(vec.y) +
		" , "+ std::to_string(vec.z) + " )";
	}

	std::string to_string(Game::Vector4 vec) {

		return "( " + std::to_string(vec.x) + " , " + std::to_string(vec.y) +
			" , " + std::to_string(vec.z)  + " , " + std::to_string(vec.w) + " )";
	}

	std::string to_string(Game::Mesh::Vertex vert) {
		return " Color" + to_string(vert.Color) + "\tPosition" + to_string(vert.Position)
			+ "\tNormal" + to_string(vert.Normal) + "\tTangent" + to_string(vert.Tangent)
			+ "\tTexCoord" + to_string(vert.TexCoord) + "\n";
	}

	std::string to_string(Game::Mesh& mesh) {
		std::string result = "vertex data:\n"; 
		for (int i = 0; i != mesh.vertexNum; i++) {
			Game::Mesh::Vertex* curr = reinterpret_cast<Game::Mesh::Vertex*>(mesh.vertexList.data) + i;
			result.append(std::to_string(i) + ":" + to_string(*curr));
		}
		if (mesh.useIndexList) {
			result.append("\nindex data:\n");
			for (int i = 0; i != mesh.indexNum; i++) {
				if (i % 3 == 0)
					result.append("|\t");
				if (mesh.indexType == Mesh::I16) {
					result.append(std::to_string(*(reinterpret_cast<uint16_t*>(mesh.indexList.data) + i)) + " ");
				}
				else {
					result.append(std::to_string(*(reinterpret_cast<uint32_t*>(mesh.indexList.data) + i)) + " ");
				}
			}
		}
		return result;
	}
}

