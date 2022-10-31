#include <string>
#include <vector>
#include <assimp/scene.h>
bool Import3DFromFile(const std::string& pFile);
std::vector<struct MyMesh> createMeshFromAssimp(const aiScene* sc);