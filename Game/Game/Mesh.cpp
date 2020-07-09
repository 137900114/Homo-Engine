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
		return " Color" + to_string(vert.Color) + "\nPosition" + to_string(vert.Position)
			+ "\nNormal" + to_string(vert.Normal) + "\nTangent" + to_string(vert.Tangent)
			+ "\nTexCoord" + to_string(vert.TexCoord) + "\n";
	}

	std::string to_string(Game::Mesh& mesh) {
		std::string result;
		for (int i = 0; i != mesh.vertexNum; i++) {
			Game::Mesh::Vertex* curr = reinterpret_cast<Game::Mesh::Vertex*>(mesh.vertexList.data) + i;
			result.append(to_string(*curr));
		}
		return result;
	}
}

