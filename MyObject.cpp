#include <string>
#include <assert.h>
#include <stdlib.h>
#include <vector>

#include <iostream>
#include <sstream>
#include <fstream>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>

// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/scene.h"

#include "AVTmathLib.h"
#include "meshFromAssimp.h"
#include "VertexAttrDef.h"
#include "geometry.h"

#include "VSShaderlib.h"
#include "MyObject.h"

extern float mMatrix[COUNT_MATRICES][16];
extern float mCompMatrix[COUNT_COMPUTED_MATRICES][16];
extern float mNormal3x3[9];


//FLARE STUFF
FLARE_DEF AVTflare;
GLuint FlareTextureArray[5];



/* IMPORTANT: Use the next data to make this Assimp demo to work*/

// Created an instance of the Importer class in the meshFromAssimp.cpp file
extern Assimp::Importer importer;
// the global Assimp scene object
extern const aiScene* scene;
char model_dir[50];  //initialized by the user input at the console
// scale factor for the Assimp model to fit in the window
extern float scaleFactor;
bool normalMapKey = TRUE; // by default if there is a normal map then bump effect is implemented. press key "b" to enable/disable normal mapping 



My3DVector::My3DVector() {}

My3DVector::My3DVector(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

// Creates a vector with an angle
My3DVector::My3DVector(float angle, float x, float y, float z) {
	this->angle = angle;
	this->x = x;
	this->y = y;
	this->z = z;
}

// Normalizes the vector
void My3DVector::normalize2D() {
	float normalize_constant = sqrt(pow(this->x, 2) + pow(this->z, 2));
	this->x = this->x / normalize_constant;
	this->z = this->z / normalize_constant;
}

My3DVector My3DVector::rotatearoundX(float angle) {
	float new_angle = fmod(angle, 360.0f) * (3.14f / 180.f);
	float new_y = cos(new_angle) * this->y - sin(new_angle) * this->z;
	float new_z = sin(new_angle) * this->y + cos(new_angle) * this->z;
	return My3DVector(this->x, new_y, new_z);
}

My3DVector My3DVector::rotatearoundY(float angle) {
	float new_angle = fmod(angle, 360.0f) * (3.14f / 180.f);
	float new_x = cos(new_angle) * this->y + sin(new_angle) * this->z;
	float new_z = -1 * sin(new_angle) * this->y + cos(new_angle) * this->z;
	return My3DVector(new_x, this->y, new_z);
}

My3DVector My3DVector::rotatearoundZ(float angle) {
	float new_angle = fmod(angle, 360.0f) * (3.14f / 180.f);
	float new_x = cos(new_angle) * this->x - sin(new_angle) * this->y;
	float new_y = sin(new_angle) * this->x + cos(new_angle) * this->y;
	return My3DVector(new_x, new_y, this->z);
}

My3DVector My3DVector::inverseRotationZ(float angle) {
	float new_angle = fmod(angle, 360.0f) * (3.14f / 180.f);
	float new_x = cos(new_angle) * this->x + sin(new_angle) * this->y;
	float new_y = -1 * sin(new_angle) * this->x + cos(new_angle) * this->y;
	return My3DVector(new_x, new_y, this->z);
}

MyObject::MyObject() {
}

/*
An Object has:
A Mesh that represents the shape
Pos that represents the position
Scale that represents the scale
A rotationList that represents the rotations.

*/

MyFlare::MyFlare()
{
}

MyFlare::MyFlare(char* file_path) {
	myMeshFlare = createQuad(1, 1);

	//Load flare from file
	loadFlareFile(&AVTflare, file_path);
}

inline double MyFlare::clamp(const double x, const double min, const double max) {
	return (x < min ? min : (x > max ? max : x));
}

inline int MyFlare::clampi(const int x, const int min, const int max) {
	return (x < min ? min : (x > max ? max : x));
}

int MyFlare::getTextureId(char* name) {
	int i;

	for (i = 0; i < flareTextureNames.size(); ++i)
	{
		if (strncmp(name, flareTextureNames[i], strlen(name)) == 0)
			return i;
	}
	return -1;
}

void MyFlare::loadFlareFile(FLARE_DEF* flare, char* filename) {
	int     n = 0;
	FILE* f;
	char    buf[256];
	int fields;

	memset(flare, 0, sizeof(FLARE_DEF));

	f = fopen(filename, "r");
	if (f)
	{
		fgets(buf, sizeof(buf), f);
		sscanf(buf, "%f %f", &flare->fScale, &flare->fMaxSize);

		while (!feof(f))
		{
			char            name[8] = { '\0', };
			double          dDist = 0.0, dSize = 0.0;
			float			color[4];
			int				id;

			fgets(buf, sizeof(buf), f);
			fields = sscanf(buf, "%4s %lf %lf ( %f %f %f %f )", name, &dDist, &dSize, &color[3], &color[0], &color[1], &color[2]);
			if (fields == 7)
			{
				for (int i = 0; i < 4; ++i) color[i] = clamp(color[i] / 255.0f, 0.0f, 1.0f);
				id = getTextureId(name);
				if (id < 0) printf("Texture name not recognized\n");
				else
					flare->element[n].textureId = id;
				flare->element[n].fDistance = (float)dDist;
				flare->element[n].fSize = (float)dSize;
				memcpy(flare->element[n].matDiffuse, color, 4 * sizeof(float));
				++n;
			}
		}

		flare->nPieces = n;
		fclose(f);
	}
	else printf("Flare file opening error\n");
}

void MyFlare::render_flare(VSShaderLib& shader, FLARE_DEF* flare, int lx, int ly, int* m_viewport) {  //lx, ly represent the projected position of light on viewport

	int     dx, dy;          // Screen coordinates of "destination"
	int     px, py;          // Screen coordinates of flare element
	int		cx, cy;
	float    maxflaredist, flaredist, flaremaxsize, flarescale, scaleDistance;
	int     width, height, alpha;    // Piece parameters;
	int     i;
	float	diffuse[4];

	GLint loc;

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Flare elements textures------------------------------------------------------------------------------------------
	glGenTextures(5, FlareTextureArray);
	Texture2D_Loader(FlareTextureArray, "./Texture_Materials/crcl.tga", 0);
	Texture2D_Loader(FlareTextureArray, "./Texture_Materials/flar.tga", 1);
	Texture2D_Loader(FlareTextureArray, "./Texture_Materials/hxgn.tga", 2);
	Texture2D_Loader(FlareTextureArray, "./Texture_Materials/ring.tga", 3);
	Texture2D_Loader(FlareTextureArray, "./Texture_Materials/sun.tga", 4);

	int screenMaxCoordX = m_viewport[0] + m_viewport[2] - 1;
	int screenMaxCoordY = m_viewport[1] + m_viewport[3] - 1;

	//viewport center
	cx = m_viewport[0] + (int)(0.5f * (float)m_viewport[2]) - 1;
	cy = m_viewport[1] + (int)(0.5f * (float)m_viewport[3]) - 1;

	// Compute how far off-center the flare source is.
	maxflaredist = sqrt(cx * cx + cy * cy);
	flaredist = sqrt((lx - cx) * (lx - cx) + (ly - cy) * (ly - cy));
	scaleDistance = (maxflaredist - flaredist) / maxflaredist;
	flaremaxsize = (int)(m_viewport[2] * flare->fMaxSize);
	flarescale = (int)(m_viewport[2] * flare->fScale);

	// Destination is opposite side of centre from source
	dx = clampi(cx + (cx - lx), m_viewport[0], screenMaxCoordX);
	dy = clampi(cy + (cy - ly), m_viewport[1], screenMaxCoordY);

	// Render each element. To be used Texture Unit 0
	GLint texMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "texMode");
	GLint tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");

	glUniform1i(tex_loc, 0);  //use TU 0
	glUniform1i(texMode_uniformId, 25); // draw modulated textured particles 


	GLint pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	GLint vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	GLint normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");

	for (i = 0; i < flare->nPieces; ++i)
	{
		// Position is interpolated along line between start and destination.
		px = (int)((1.0f - flare->element[i].fDistance) * lx + flare->element[i].fDistance * dx);
		py = (int)((1.0f - flare->element[i].fDistance) * ly + flare->element[i].fDistance * dy);
		px = clampi(px, m_viewport[0], screenMaxCoordX);
		py = clampi(py, m_viewport[1], screenMaxCoordY);

		// Piece size are 0 to 1; flare size is proportion of screen width; scale by flaredist/maxflaredist.
		width = (int)(scaleDistance * flarescale * flare->element[i].fSize);

		// Width gets clamped, to allows the off-axis flaresto keep a good size without letting the elements get big when centered.
		if (width > flaremaxsize)  width = flaremaxsize;

		height = (int)((float)m_viewport[3] / (float)m_viewport[2] * (float)width);
		memcpy(diffuse, flare->element[i].matDiffuse, 4 * sizeof(float));
		diffuse[3] *= scaleDistance;   //scale the alpha channel

		if (width > 1)
		{
			// send the material - diffuse color modulated with texture
			loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
			glUniform4fv(loc, 1, diffuse);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, FlareTextureArray[flare->element[i].textureId]);
			pushMatrix(MODEL);
			translate(MODEL, (float)(px - width * 0.0f), (float)(py - height * 0.0f), 0.0f);
			scale(MODEL, (float)width, (float)height, 1);
			computeDerivedMatrix(PROJ_VIEW_MODEL);
			glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
			glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
			computeNormalMatrix3x3();
			glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

			glBindVertexArray(myMeshFlare.vao);
			glDrawElements(myMeshFlare.type, myMeshFlare.numIndexes, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
			popMatrix(MODEL);
		}
	}
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glDisable(GL_BLEND);
}

void MyFlare::renderSceneFlare(VSShaderLib& shader, My3DVector lightPosVec) {

	int flarePos[2];
	int m_viewport[4];

	float lightPosFloat[4] = { lightPosVec.x, lightPosVec.y, lightPosVec.z, 1.0f };
	float lightEyePos[4];
	float lightScreenPos[3];

	glGetIntegerv(GL_VIEWPORT, m_viewport);

	pushMatrix(MODEL);
	loadIdentity(MODEL);
	computeDerivedMatrix(PROJ_VIEW_MODEL);  //pvm to be applied to lightPost. pvm is used in project function

	if (!project(lightPosFloat, lightScreenPos, m_viewport))
		printf("Error in getting projected light in screen\n");  //Calculate the window Coordinates of the light position: the projected position of light on viewport

	flarePos[0] = clampi((int)lightScreenPos[0], m_viewport[0], m_viewport[0] + m_viewport[2] - 1);
	flarePos[1] = clampi((int)lightScreenPos[1], m_viewport[1], m_viewport[1] + m_viewport[3] - 1);
	popMatrix(MODEL);


	//viewer looking down at  negative z direction
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);
	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	render_flare(shader, &AVTflare, flarePos[0], flarePos[1], m_viewport);
	popMatrix(PROJECTION);
	popMatrix(VIEW);
}

MyAssimp::MyAssimp() {}

MyAssimp::MyAssimp(std::string modelDir, My3DVector pos, My3DVector scale, std::vector<My3DVector> rotationsList) {

	std::string filepath;

	std::strncpy(model_dir, modelDir.c_str(), sizeof(modelDir));

	std::ostringstream oss;
	oss << model_dir << "/" << model_dir << ".obj";
	filepath = oss.str();   //path of OBJ file in the VS project

	strcat(model_dir, "/");  //directory path in the VS project

	//check if file exists
	std::ifstream fin(filepath.c_str());
	if (!fin.fail()) {
		fin.close();
	}
	else {
		printf("Couldn't open file: %s\n", filepath.c_str());
		exit(1);
	}

	//import 3D file into Assimp scene graph
	if (!Import3DFromFile(filepath)) exit(1);

	//creation of Mymesh array with VAO Geometry and Material
	myMeshes = createMeshFromAssimp(scene);

	position = pos;
	scaleSize = scale;
	rotations = rotationsList;
}

void MyAssimp::render(VSShaderLib& shader, const aiScene* sc, const aiNode* nd) {

	bool normalMapKey = TRUE;

	GLint loc;

	// Get node transformation matrix
	aiMatrix4x4 m = nd->mTransformation;
	// OpenGL matrices are column major
	m.Transpose();

	// save model matrix and apply node transformation
	pushMatrix(MODEL);

	translate(MODEL, position.x, position.y, position.z);

	for (My3DVector MyRotation : rotations) {
		rotate(MODEL, MyRotation.angle, MyRotation.x, MyRotation.y, MyRotation.z);
	}
	scale(MODEL, scaleSize.x, scaleSize.y, scaleSize.z);
	for (My3DVector MyCenter : center) {
		translate(MODEL, MyCenter.x, MyCenter.y, MyCenter.z);
	}

	float aux[16];
	memcpy(aux, &m, sizeof(float) * 16);
	multMatrix(MODEL, aux);

	// draw all meshes assigned to this node
	for (unsigned int n = 0; n < nd->mNumMeshes; ++n) {

		// send the material
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
		glUniform4fv(loc, 1, myMeshes[nd->mMeshes[n]].mat.ambient);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
		glUniform4fv(loc, 1, myMeshes[nd->mMeshes[n]].mat.diffuse);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
		glUniform4fv(loc, 1, myMeshes[nd->mMeshes[n]].mat.specular);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.emissive");
		glUniform4fv(loc, 1, myMeshes[nd->mMeshes[n]].mat.emissive);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
		glUniform1f(loc, myMeshes[nd->mMeshes[n]].mat.shininess);
		loc = glGetUniformLocation(shader.getProgramIndex(), "mat.texCount");
		glUniform1i(loc, myMeshes[nd->mMeshes[n]].mat.texCount);

		GLint texMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "texMode");
		glUniform1i(texMode_uniformId, 100);

		unsigned int  diffMapCount = 0;  //read 2 diffuse textures

		//devido ao fragment shader suporta 2 texturas difusas simultaneas, 1 especular e 1 normal map
		GLint normalMap_loc = glGetUniformLocation(shader.getProgramIndex(), "normalMap");
		GLint specularMap_loc = glGetUniformLocation(shader.getProgramIndex(), "specularMap");
		GLint diffMapCount_loc = glGetUniformLocation(shader.getProgramIndex(), "diffMapCount");
		GLint pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
		GLint vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
		GLint normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");
		
		glUniform1i(normalMap_loc, false);
		glUniform1i(specularMap_loc, false);
		glUniform1ui(diffMapCount_loc, 0);


		if (myMeshes[nd->mMeshes[n]].mat.texCount != 0) {

			for (unsigned int i = 0; i < myMeshes[nd->mMeshes[n]].mat.texCount; ++i) {
				if (myMeshes[nd->mMeshes[n]].texTypes[i] == DIFFUSE) {
					if (diffMapCount == 0) {
						diffMapCount++;
						loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitDiff");
						glUniform1i(loc, myMeshes[nd->mMeshes[n]].texUnits[i] + 14); //+ 14 because of the num of textures
						glUniform1ui(diffMapCount_loc, diffMapCount);
					}
					else if (diffMapCount == 1) {
						diffMapCount++;
						loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitDiff1");
						glUniform1i(loc, myMeshes[nd->mMeshes[n]].texUnits[i] + 14);
						glUniform1ui(diffMapCount_loc, diffMapCount);
					}
					else printf("Only supports a Material with a maximum of 2 diffuse textures\n");
				}
				else if (myMeshes[nd->mMeshes[n]].texTypes[i] == SPECULAR) {
					loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitSpec");
					glUniform1i(loc, myMeshes[nd->mMeshes[n]].texUnits[i] + 14);
					glUniform1i(specularMap_loc, true);
				}
				else if (myMeshes[nd->mMeshes[n]].texTypes[i] == NORMALS) {
					loc = glGetUniformLocation(shader.getProgramIndex(), "texUnitNormalMap");
					if (normalMapKey)
						glUniform1i(normalMap_loc, normalMapKey);
					glUniform1i(loc, myMeshes[nd->mMeshes[n]].texUnits[i] + 14);

				}
				else printf("Texture Map not supported\n");
			}

		}

		computeDerivedMatrix(PROJ_VIEW_MODEL);
		glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
		glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
		computeNormalMatrix3x3();
		glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);
		// bind VAO
		glBindVertexArray(myMeshes[nd->mMeshes[n]].vao);

		if (!shader.isProgramValid()) {
			printf("Program Not Valid!\n");
			exit(1);
		}
		// draw
		glDrawElements(myMeshes[nd->mMeshes[n]].type, myMeshes[nd->mMeshes[n]].numIndexes, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}


	// draw all children
	popMatrix(MODEL);

	for (unsigned int n = 0; n < nd->mNumChildren; ++n) {
		render(shader, sc, nd->mChildren[n]);
	}


}

MyObject::MyObject(MyMesh objectMesh, My3DVector pos, My3DVector scale, std::vector<My3DVector> rotationsList) {
	mesh = objectMesh;
	position = pos;
	scaleSize = scale;
	rotations = rotationsList;

}

My3DVector MyObject::updateVertice(My3DVector vertice) {
	pushMatrix(MODEL);
	translate(MODEL, position.x, position.y, position.z);

	for (My3DVector MyRotation : rotations) {
		rotate(MODEL, MyRotation.angle, MyRotation.x, MyRotation.y, MyRotation.z);
	}

	scale(MODEL, scaleSize.x, scaleSize.y, scaleSize.z);

	for (My3DVector MyCenter : center) {
		translate(MODEL, MyCenter.x, MyCenter.y, MyCenter.z);
	}


	//UPDATE VERTICES
	float transformVertice[4] = { vertice.x,vertice.y,vertice.z, 1.0 };

	float updatedVertice[4];

	// multiply vec4 vertex by the model matrix (mul. order is important)
	multMatrixPoint(MODEL, transformVertice, updatedVertice);

	popMatrix(MODEL);

	// return vector with new vertices
	return My3DVector{ updatedVertice[0],updatedVertice[1],updatedVertice[2] };
}

MyParticles::MyParticles() {

	MyMesh particleMesh = createQuad(2, 2);

	float particleshininess = 500.0;
	int particletexcount = 0;

	particleMesh.mat.shininess = particleshininess;
	particleMesh.mat.texCount = particletexcount;

	particle = MyObject(particleMesh, My3DVector(0, 0, 0), My3DVector(0.01, 0.01, 0.01), {});
}

void MyParticles::render(VSShaderLib& shader) {
	particle.render(shader, "particles", {}, false);
}

void MyParticles::iniParticles(float X, float Y, float Z) {

	GLfloat v, theta, phi;

	v = 0.8 * frand() + 0.2;
	phi = frand() * M_PI;
	theta = 2.0 * frand() * M_PI;

	particle.position.x = X;
	particle.position.y = Y + 5.0f;
	particle.position.z = Z;
	velocity.x = v * cos(theta) * sin(phi);
	velocity.y = v * cos(phi) * 5;
	velocity.z = v * sin(theta) * sin(phi);
	//aceleration.x = 0.1f; /* simular um pouco de vento 
	aceleration.x = 0.0f;
	aceleration.y = -0.15f; // simular a aceleração da gravidade 
	aceleration.z = 0.0f;

	// tom amarelado que vai ser multiplicado pela textura que varia entre branco e preto 
		//r = 0.882f;
		//g = 0.552f;
		//b = 0.211f;
	r = float((rand() % 100) / 100.0f);
	g = float((rand() % 100) / 100.0f);
	b = float((rand() % 100) / 100.0f);

	life = 1.0f; // vida inicial 
	fade = 0.0025f; // step de decréscimo da vida para cada iteração 

}

void MyParticles::updateParticles() {
	int i;
	float h;

	/* Método de Euler de integração de eq. diferenciais ordinárias
	h representa o step de tempo; dv/dt = a; dx/dt = v; e conhecem-se os valores iniciais de x e v */

	//h = 0.125f;
	h = 0.033;

	particle.position.x += (h * velocity.x);
	particle.position.y += (h * velocity.y);
	particle.position.z += (h * velocity.z);
	velocity.x += (h * aceleration.x);
	velocity.y += (h * aceleration.y);
	velocity.z += (h * aceleration.z);
	life -= fade;

}

MyCar::MyCar() {

}

/*
A car receieves it's position
*/

MyCar::MyCar(My3DVector position) {

	// O nosso carro começa a olhar para cima.
	direction = My3DVector(0, 0, -1);
	// -1 no Z = Para cima em top view
	// 1 no Z = Para baixo em top view.
	// -1 no x = Para esquerda em top view.
	// 1 no x = Para direita em top view.


	//Body
	MyMesh bodyMesh = createCube();


	// Defines the materials for the car's body mesh
	float bodyamb[] = { 0.3f, 0.0f, 0.0f, 1.0f };
	float bodydiff[] = { 0.9f, 0.1f, 0.0f, 1.0f };
	float bodyspec[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	float bodyemissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float bodyshininess = 500.0;
	int bodytexcount = 0;

	// Binds the materials to the mesh.
	memcpy(bodyMesh.mat.ambient, bodyamb, 4 * sizeof(float));
	memcpy(bodyMesh.mat.diffuse, bodydiff, 4 * sizeof(float));
	memcpy(bodyMesh.mat.specular, bodyspec, 4 * sizeof(float));
	memcpy(bodyMesh.mat.emissive, bodyemissive, 4 * sizeof(float));
	bodyMesh.mat.shininess = bodyshininess;
	bodyMesh.mat.texCount = bodytexcount;

	//bodyPosition = My3DVector(position.x, position.y, position.z);

	// Create the body's object

	body = MyObject(bodyMesh, position, My3DVector(2.5f, 1.0f, 2.5f), { My3DVector(0, 0.0, 1.0, 0.0) });
	// Normalmente seria -0.5, 0, -1, mas como tamos a usar o scale = 2, fica em -0.5
	My3DVector centerVector = My3DVector(-0.5, 0, -0.5);
	// Body's center
	body.center = { centerVector };
	//Window
	MyMesh windowMesh = createCube();


	// Defines the materials for the windows's body mesh
	float windowamb[] = { 0.0f, 0.0f, 0.9f, 1.0f };
	float windowdiff[] = { 0.0f, 0.1f, 0.9f, 1.0f };
	float windowspec[] = { 0.9f, 0.9f, 0.9f, 1.0f };
	float windowemissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float windowshininess = 500.0;
	int windowtexcount = 0;

	// Binds the materials to the mesh.
	memcpy(windowMesh.mat.ambient, windowamb, 4 * sizeof(float));
	memcpy(windowMesh.mat.diffuse, windowdiff, 4 * sizeof(float));
	memcpy(windowMesh.mat.specular, windowspec, 4 * sizeof(float));
	memcpy(windowMesh.mat.emissive, windowemissive, 4 * sizeof(float));
	windowMesh.mat.shininess = windowshininess;
	windowMesh.mat.texCount = windowtexcount;


	My3DVector windowposition = My3DVector(position.x, position.y + 1, position.z);
	//Creates the windows object
	window = MyObject(windowMesh, windowposition, My3DVector(1, 0.5, 1), { My3DVector(0, 0, 1, 0) });
	My3DVector centerWindowVector = My3DVector(-0.5, 0, -0.5);
	window.center = { centerWindowVector };


	//Wheels
	MyMesh wheelMesh;

	// Defines the materials for the wheels's body mesh
	float wheelamb[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float wheeldiff[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float wheelspec[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float wheelemissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float wheelshininess = 1.0f;
	int wheeltexcount = 0;
	// Top Right Wheel, Bottom Right Wheel, Bottom Left Wheel, Top Left Wheel
	//float wheelx[] = {0.5, 0.5, -0.5, -0.5};
	//float wheelz[] = { -0.5, 0.5, 0.5, -0.5 };

	float wheelx[] = { position.x,position.x,position.x,position.x };
	float wheely[] = { 0.5,0.5,0.5,0.5 };
	float wheelz[] = { position.z, position.z, position.z,position.z };


	// -1 no Z = Para cima em top view
	// 1 no Z = Para baixo em top view.
	// -1 no x = Para esquerda em top view.
	// 1 no x = Para direita em top view.
	//float wheelz[] = { -0.5, 0.5, 0.5, -0.5 };
	std::vector<My3DVector> wheelcenters;
	wheelcenters.push_back(My3DVector(-0.5, 0.5, 0.5));
	wheelcenters.push_back(My3DVector(-0.5, 0.5, -0.5));
	wheelcenters.push_back(My3DVector(-0.5, -0.5, -0.5));
	wheelcenters.push_back(My3DVector(-0.5, -0.5, 0.5));

	for (int i = 0; i < 4; i++) {

		wheelMesh = createTorus(0.25, 0.5, 20, 20);
		memcpy(wheelMesh.mat.ambient, wheelamb, 4 * sizeof(float));
		memcpy(wheelMesh.mat.diffuse, wheeldiff, 4 * sizeof(float));
		memcpy(wheelMesh.mat.specular, wheelspec, 4 * sizeof(float));
		memcpy(wheelMesh.mat.emissive, wheelemissive, 4 * sizeof(float));
		wheelMesh.mat.shininess = wheelshininess;
		wheelMesh.mat.texCount = wheeltexcount;


		wheels[i] = MyObject(wheelMesh, My3DVector(wheelx[i], wheely[i], wheelz[i]), My3DVector(1, 1, 1), { My3DVector(float(90), 0, 0, 1), My3DVector(0, 1, 0, 0) });
		wheels[i].center = { wheelcenters[i] };
	}

	//Lights
	MyMesh lightsMesh;
	// Defines the materials for the wheels's body mesh
	float lightamb[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	float lightdiff[] = { 1.0f, 1.0f, 0.0f, 1.0f };
	float lightspec[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float lightemissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float lightshininess = 1.0f;
	int lighttexcount = 0;
	My3DVector lightsScale = My3DVector(0.25, 0.25, 0.25);

	//True Values
	std::vector<My3DVector> lightcenter = { My3DVector(-2,0,-5), My3DVector(1,0,-5) };
	std::vector<My3DVector> lightspos = { My3DVector(position.x, position.y + 0.75,position.z), My3DVector(position.x, position.y + 0.75, position.z) };


	//std::vector<My3DVector> lightcenter = { My3DVector(-0.5,0,-5), My3DVector(-0.5,0,-5) };
	//std::vector<My3DVector> lightspos = { My3DVector(position.x - 0.375, position.y + 0.75,position.z), My3DVector(position.x + 0.375, position.y + 0.75, position.z) };

	lights_position[0] = My3DVector(position.x + 0.375, position.y + 0.75, position.z - 1.25);
	lights_position[1] = My3DVector(position.x - 0.375, position.y + 0.75, position.z - 1.25);

	for (int i = 0; i < 2; i++) {

		lightsMesh = createCube();
		memcpy(lightsMesh.mat.ambient, lightamb, 4 * sizeof(float));
		memcpy(lightsMesh.mat.diffuse, lightdiff, 4 * sizeof(float));
		memcpy(lightsMesh.mat.specular, lightspec, 4 * sizeof(float));
		memcpy(lightsMesh.mat.emissive, lightemissive, 4 * sizeof(float));
		lightsMesh.mat.shininess = lightshininess;
		lightsMesh.mat.texCount = lighttexcount;

		lights[i] = MyObject(lightsMesh, lightspos[i], lightsScale, { My3DVector(0, 0, 1, 0) });
		lights[i].center = { lightcenter[i] };
	}

	//carAssimp.center = {My3DVector(327.1f, 0, -272.8f)};
	float scalefactor = 0.05f;
	My3DVector translatetoCenter = My3DVector(0, -5, 0);
	carAssimp = MyAssimp("UFO", My3DVector(position.x + translatetoCenter.x * scalefactor, position.y + translatetoCenter.y * scalefactor, position.z - translatetoCenter.z * scalefactor), My3DVector(1.0f * scalefactor, 1.0f * scalefactor, 1.0f * scalefactor), { My3DVector(0, 0.0, 1.0, 0.0) });
	carAssimp.center = { translatetoCenter };
}

void MyCar::render(VSShaderLib& shader) {
	
	//body.render(shader,"body", {}, false);
	/*
	window.render(shader, "window", {}, false);
	for (int i = 0; i < 4; i++) {
		wheels[i].render(shader, "wheels", {}, false);
	}
	for (int i = 0; i < 2; i++) {
		lights[i].render(shader,"wheels", {}, false);
	}*/
	carAssimp.render(shader, scene, scene->mRootNode);
}

void MyCar::up() {
	body.position.y++;
	window.position.y++;
	for (int i = 0; i < 4; i++) wheels[i].position.y++;
	for (int i = 0; i < 2; i++) lights[i].position.y++;
	carAssimp.position.y++;
}

void MyCar::down() {
	if (body.position.y > CAR_INITIAL_Y) {
		body.position.y--;
		if (body.position.y < CAR_INITIAL_Y) {
			body.position.y = CAR_INITIAL_Y;
		}
		window.position.y--;
		for (int i = 0; i < 4; i++) wheels[i].position.y--;
		for (int i = 0; i < 2; i++) lights[i].position.y--;
		carAssimp.position.y--;
	}
}


void MyCar::turnRight() {
	body.angle -= 10 * velocity;
	body.angle = fmod(body.angle, 360);
	body.rotations[0].angle = body.angle;
	window.rotations[0].angle = body.angle;
	for (int i = 0; i < 4; i++) {
		wheels[i].angle -= 10 * velocity;
		wheels[i].rotations[1].angle = body.angle;
	}
	for (int i = 0; i < 2; i++) {
		lights[i].angle -= 10 * velocity;
		lights[i].rotations[0].angle = body.angle;
	}

	carAssimp.rotations[0].angle = body.angle;
	carAssimp.position = body.position;
	updateDirection();

}

void MyCar::turnLeft() {
	body.angle += 10 * velocity;
	body.angle = fmod(body.angle, 360);
	body.rotations[0].angle = body.angle;
	window.rotations[0].angle = body.angle;
	for (int i = 0; i < 4; i++) {
		wheels[i].angle += 10 * velocity;
		wheels[i].rotations[1].angle = body.angle;
	}
	for (int i = 0; i < 2; i++) {
		lights[i].angle += 10 * velocity;
		lights[i].rotations[0].angle = body.angle;
	}
	carAssimp.rotations[0].angle = body.angle;
	carAssimp.position = body.position;
	updateDirection();

}

void MyCar::updateDirection() {
	// cos needs radians so we convert degrees into radians
	// sen needs radians so we convert degrees into radians

	direction.x = 0 * cos(body.angle * 3.14f / 180.0f) - -1 * sin(body.angle * 3.14f / 180.0f);
	direction.z = 0 * sin(body.angle * 3.14f / 180.0f) + -1 * cos(body.angle * 3.14f / 180.0f);

	direction.normalize2D();
}

void MyCar::updatePosition() {

	body.position.x += -1 * direction.x * velocity;
	body.position.z += direction.z * velocity;
	window.position.x += -1 * direction.x * velocity;
	window.position.z += direction.z * velocity;
	for (int i = 0; i < 4; i++) {
		wheels[i].position.x += -1 * direction.x * velocity;
		wheels[i].position.z += direction.z * velocity;
	}
	for (int i = 0; i < 2; i++) {
		lights[i].position.x += -1 * direction.x * velocity;
		lights[i].position.z += direction.z * velocity;
		lights_position[i].x += -1 * direction.x * velocity;
		lights_position[i].z += direction.z * velocity;
	}
	carAssimp.position = body.position;

}

void MyCar::accelerate(float deltaTime) {
	if (velocity <= MAX_VELOCITY) {
		velocity += 0.0005f * deltaTime;
	}
}

void MyCar::deccelerate(float deltaTime) {
	if (velocity >= -MAX_VELOCITY) {
		velocity -= 0.0005f * deltaTime;
	}
}

void MyCar::breaks(bool acceleration) {

	if (acceleration && velocity >= 0.0f) {
		velocity -= 0.01f;
		if (velocity < 0.0f)
			velocity = 0.0f;
	}
	else if (!acceleration && abs(velocity) >= 0.0f) {
		velocity += 0.01f;
		if (velocity > 0.0f)
			velocity = 0.0f;
	}

}

void MyCar::stop() {
	velocity = 0.0f;

}


void MyCar::restart() {
	velocity = 0.0f;
	direction = My3DVector{ 0, 0, -1 };
	body.position = My3DVector{ CAR_INITIAL_X, CAR_INITIAL_Y, CAR_INITIAL_Z };
	body.angle = 0.0f;
	body.rotations[0].angle = 0.0f;
	window.position = My3DVector{ CAR_INITIAL_X, CAR_INITIAL_Y + 1, CAR_INITIAL_Z };
	window.angle = 0.0f;
	window.rotations[0].angle = 0.0f;
	carAssimp.position = My3DVector{ CAR_INITIAL_X, CAR_INITIAL_Y - 5 * 0.25, CAR_INITIAL_Z };
	carAssimp.rotations[0].angle = 0.0f;

	for (int i = 0; i < 4; i++) {
		//wheels[i].position = My3DVector(wheelx[i], wheely[i], wheelz[i]);
		wheels[i].position = My3DVector(CAR_INITIAL_X, CAR_INITIAL_Y + 0.5, CAR_INITIAL_Z);
		wheels[i].angle = 0.0f;
		wheels[i].rotations[1].angle = 0.0f;

	}

	for (int i = 0; i < 2; i++) {
		lights[i].position = My3DVector{ CAR_INITIAL_X, CAR_INITIAL_Y + 0.75, CAR_INITIAL_Z };
		lights[i].angle = 0.0f;
		lights[i].rotations[0].angle = 0.0f;
	}

	lights_position[0] = My3DVector{ CAR_INITIAL_X + 0.375, CAR_INITIAL_Y, CAR_INITIAL_Z - 1.25 };
	lights_position[1] = My3DVector{ CAR_INITIAL_X - 0.375, CAR_INITIAL_Y, CAR_INITIAL_Z - 1.25 };
}

std::vector<My3DVector> MyCar::AABB() {
	//GATHER ALL VERTICES
	std::vector<My3DVector> vertices = { My3DVector{0.0, 1.0, 1.0}, My3DVector{0.0, 0.0, 1.0},
										My3DVector{1.0, 0.0, 1.0}, My3DVector{1.0, 1.0, 1.0},
										My3DVector{1.0, 1.0, 0.0}, My3DVector{1.0, 0.0, 0.0},
										My3DVector{0.0, 0.0, 0.0}, My3DVector{0.0, 1.0, 0.0} };

	My3DVector minPos = body.updateVertice(vertices[0]);
	My3DVector maxPos = body.updateVertice(vertices[0]);

	for (My3DVector vertices : vertices) {
		My3DVector updatedVertices = body.updateVertice(vertices);

		minPos = My3DVector{ std::min(minPos.x, updatedVertices.x), std::min(minPos.y, updatedVertices.y), std::min(minPos.z, updatedVertices.z) };
		maxPos = My3DVector{ std::max(maxPos.x, updatedVertices.x), std::max(maxPos.y, updatedVertices.y), std::max(maxPos.z, updatedVertices.z) };

	}

	//return min and max position of vertices
	return { minPos, maxPos };
}


MyTable::MyTable() {}

MyTable::MyTable(My3DVector position) {

	MyMesh tableMesh;

	float amb[] = { 0.2f, 0.15f, 0.1f, 1.0f };
	float diff[] = { 0.8f, 0.6f, 0.4f, 1.0f };
	float spec[] = { 0.8f, 0.8f, 0.8f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 500.0;
	int texcount = 0;


	tableMesh = createCube();
	memcpy(tableMesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(tableMesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(tableMesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(tableMesh.mat.emissive, emissive, 4 * sizeof(float));
	tableMesh.mat.shininess = shininess;
	tableMesh.mat.texCount = texcount;

	table = MyObject(tableMesh, position, My3DVector(BOARDSIZE, 0.005f, BOARDSIZE), {});
	table = MyObject(tableMesh, position, My3DVector(BOARDSIZE, 0.005f, BOARDSIZE), {});

}

void MyTable::render(VSShaderLib& shader, bool bumpmap) {
	table.render(shader, "table", {}, bumpmap);
}
MyOrange::MyOrange() {
}

MyOrange::MyOrange(My3DVector position, float velocity) {


	MyMesh orangeMesh;

	float amb[] = { 1.0f, 0.65f, 0.0f, 0.7f };
	float diff[] = { 1.0f, 0.65f, 0.0f, 0.7f };
	float spec[] = { 1.0f, 0.65f, 0.0f, 0.7f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 0.7f };
	float shininess = 500.0;
	int texcount = 0;

	orangeMesh = createSphere(2, 20);
	//orangeMesh = createCube();
	memcpy(orangeMesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(orangeMesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(orangeMesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(orangeMesh.mat.emissive, emissive, 4 * sizeof(float));
	orangeMesh.mat.shininess = shininess;
	orangeMesh.mat.texCount = texcount;

	My3DVector orangeDirection = My3DVector((rand() % BOARDSIZE) - BOARDSIZE / 2, 0, (rand() % BOARDSIZE) - BOARDSIZE / 2);
	orangeDirection.normalize2D();
	direction = orangeDirection;
	carVelocity = velocity;
	position = My3DVector((rand() % BOARDSIZE) - BOARDSIZE / 2, 2, (rand() % BOARDSIZE) - BOARDSIZE / 2);

	orange = MyObject(orangeMesh, position, My3DVector(1, 1, 1), { My3DVector(0, -orangeDirection.x, 1, orangeDirection.z) });
}

void MyOrange::render(VSShaderLib& shader, bool bumpmap) {
	orange.render(shader, "orange", {}, bumpmap);
}

bool MyOrange::outOfLimits() {
	if (abs(orange.position.x) > BOARDSIZE / 2 || abs(orange.position.z) > BOARDSIZE / 2) {
		return true;
	}
	return false;

}

void MyOrange::updatePosition(float deltaTime) {

	orange.angle = fmod(orange.angle + 10, 360);
	orange.rotations[0].angle = orange.angle;
	carVelocity += 0.005 * deltaTime;
	orange.position.x += 0.1 * direction.x * carVelocity;
	orange.position.z += 0.1 * direction.z * carVelocity;
}

//My3DVector MyOrange::boundingBox() {

//}

MyTree::MyTree() {

}

MyTree::MyTree(My3DVector positionTree) {

	MyMesh treeMesh;

	float treeamb[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float treediff[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float treespec[] = { 0.5f, 0.5f, 0.5f, 1.0f };
	float treeemissive[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float treeshininess = 500.0;
	int treetexcount = 0;

	treeMesh = createCube();
	memcpy(treeMesh.mat.ambient, treeamb, 4 * sizeof(float));
	memcpy(treeMesh.mat.diffuse, treediff, 4 * sizeof(float));
	memcpy(treeMesh.mat.specular, treespec, 4 * sizeof(float));
	memcpy(treeMesh.mat.emissive, treeemissive, 4 * sizeof(float));
	treeMesh.mat.shininess = treeshininess;
	treeMesh.mat.texCount = treetexcount;


	tree.position = positionTree;
	tree = MyObject(treeMesh, My3DVector(tree.position.x, tree.position.y, tree.position.z), My3DVector(10, 10, 0.01), {});


}


void MyTree::render(VSShaderLib& shader, float* cam) {
	tree.render(shader, "tree", cam, false);
}

bool MyTree::outOfLimits() {
	if (abs(tree.position.x) > BOARDSIZE / 2 || abs(tree.position.z) > BOARDSIZE / 2) {
		return true;
	}
	return false;

}

MySkyBox::MySkyBox() {}

MySkyBox::MySkyBox(My3DVector position) {
	MyMesh skyBoxMesh;

	float skyBoxamb[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	float skyBoxdiff[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	float skyBoxspec[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	float skyBoxemissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float skyBoxshininess = 1.0;
	int skyBoxtexcount = 0;

	skyBoxMesh = createCube();
	memcpy(skyBoxMesh.mat.ambient, skyBoxamb, 4 * sizeof(float));
	memcpy(skyBoxMesh.mat.diffuse, skyBoxdiff, 4 * sizeof(float));
	memcpy(skyBoxMesh.mat.specular, skyBoxspec, 4 * sizeof(float));
	memcpy(skyBoxMesh.mat.emissive, skyBoxemissive, 4 * sizeof(float));
	skyBoxMesh.mat.shininess = skyBoxshininess;
	skyBoxMesh.mat.texCount = skyBoxtexcount;

	skyBox = MyObject(skyBoxMesh, My3DVector(position.x, position.y, position.z), My3DVector(3, 1.5, 2), {});

	My3DVector centerVector = My3DVector(-0.5f, -0.5f, -0.5f);
	skyBox.center = { centerVector };

}

void MySkyBox::render(VSShaderLib& shader) {

	glDepthMask(GL_FALSE);
	glFrontFace(GL_CW);

	pushMatrix(MODEL);
	pushMatrix(VIEW);//se quiser anular a translação

	mMatrix[VIEW][12] = 0.0f;
	mMatrix[VIEW][13] = 0.0f;
	mMatrix[VIEW][14] = 0.0f;

	skyBox.render(shader, "skyBox", {}, false);

	popMatrix(MODEL);
	popMatrix(VIEW);

	glFrontFace(GL_CCW);
	glDepthMask(GL_TRUE);
}

MyEnvCube::MyEnvCube() {}

MyEnvCube::MyEnvCube(My3DVector position) {
	MyMesh envCubeMesh;

	float envCubeamb[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	float envCubediff[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	float envCubespec[] = { 1.0f, 1.0f, 0.0f, 0.0f };
	float envCubeemissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float envCubeshininess = 1.0;
	int envCubetexcount = 0;

	envCubeMesh = createCube();
	memcpy(envCubeMesh.mat.ambient, envCubeamb, 4 * sizeof(float));
	memcpy(envCubeMesh.mat.diffuse, envCubediff, 4 * sizeof(float));
	memcpy(envCubeMesh.mat.specular, envCubespec, 4 * sizeof(float));
	memcpy(envCubeMesh.mat.emissive, envCubeemissive, 4 * sizeof(float));
	envCubeMesh.mat.shininess = envCubeshininess;
	envCubeMesh.mat.texCount = envCubetexcount;

	envCube = MyObject(envCubeMesh, My3DVector(position.x, position.y, position.z), My3DVector(20, 20, 20), {});
}

void MyEnvCube::render(VSShaderLib& shader) {

	envCube.render(shader, "envCube", {}, false);
}

MyButter::MyButter() {

}

MyButter::MyButter(My3DVector position) {

	MyMesh butterMesh;
	float butteramb[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float butterdiff[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float butterspec[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float butteremissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float buttershininess = 500.0;
	int buttertexcount = 0;

	butterMesh = createCube();
	memcpy(butterMesh.mat.ambient, butteramb, 4 * sizeof(float));
	memcpy(butterMesh.mat.diffuse, butterdiff, 4 * sizeof(float));
	memcpy(butterMesh.mat.specular, butterspec, 4 * sizeof(float));
	memcpy(butterMesh.mat.emissive, butteremissive, 4 * sizeof(float));
	butterMesh.mat.shininess = buttershininess;
	butterMesh.mat.texCount = buttertexcount;

	butter = MyObject(butterMesh, My3DVector(position.x, position.y, position.z), My3DVector(3, 1.5, 2), {});

}


void MyButter::updatePosition(float deltaTime) {
	butter.position.x += -1 * direction.x * velocity;
	butter.position.z += direction.z * velocity;
	velocity -= 0.0005 * deltaTime;
	if (velocity < 0) {
		velocity = 0;
	}
}


void MyButter::collision(My3DVector carDirection, float carVelocity) {
	if (carVelocity != 0) {
		direction = carDirection;
		velocity = carVelocity;
	}
}





void MyButter::render(VSShaderLib& shader) {
	butter.render(shader, "butter", {}, false);
}

std::vector<My3DVector> MyButter::AABB() {
	//GATHER ALL VERTICES OF BUTTER CUBE
	std::vector<My3DVector> vertices = { My3DVector{0.0, 1.0, 1.0}, My3DVector{0.0, 0.0, 1.0},
										My3DVector{1.0, 0.0, 1.0}, My3DVector{1.0, 1.0, 1.0},
										My3DVector{1.0, 1.0, 0.0}, My3DVector{1.0, 0.0, 0.0},
										My3DVector{0.0, 0.0, 0.0}, My3DVector{0.0, 1.0, 0.0} };

	My3DVector minPos = butter.updateVertice(vertices[0]);
	My3DVector maxPos = butter.updateVertice(vertices[0]);

	for (My3DVector vertices : vertices) {
		My3DVector updatedVertices = butter.updateVertice(vertices);

		minPos = My3DVector{ std::min(minPos.x, updatedVertices.x), std::min(minPos.y, updatedVertices.y), std::min(minPos.z, updatedVertices.z) };
		maxPos = My3DVector{ std::max(maxPos.x, updatedVertices.x), std::max(maxPos.y, updatedVertices.y), std::max(maxPos.z, updatedVertices.z) };
	}

	//return min and max position of vertices
	return { minPos, maxPos };
}


MyCheerio::MyCheerio() {
}

MyCheerio::MyCheerio(My3DVector position) {

	MyMesh cheerioMesh;

	direction = My3DVector(0.0f, 0.0f, 0.0f);

	float amb[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float diff[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float spec[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 500.0;
	int texcount = 0;

	cheerioMesh = createTorus(MIN_CHEERIO_RADIUS, MAX_CHEERIO_RADIUS, 20, 20);
	memcpy(cheerioMesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(cheerioMesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(cheerioMesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(cheerioMesh.mat.emissive, emissive, 4 * sizeof(float));
	cheerioMesh.mat.shininess = shininess;
	cheerioMesh.mat.texCount = texcount;

	cheerio = MyObject(cheerioMesh, position, My3DVector(1, 1, 1), {});

}

void MyCheerio::collision(My3DVector carDirection, float carVelocity) {
	printf("Collision Happened!");
	if (carVelocity != 0) {
		direction = carDirection;
		velocity = carVelocity;
	}
}

void MyCheerio::updatePosition(float deltaTime) {
	
	if (velocity > 0) {
		printf("%f\n", velocity);
		printf("cheerio:x %f cheerio:y %f\n", cheerio.position.x, cheerio.position.z);
	}
	cheerio.position.x += -1 * direction.x * velocity;
	cheerio.position.z += direction.z * velocity;

	if (velocity > 0) {
		printf("cheerio:x %f cheerio:y %f\n", cheerio.position.x, cheerio.position.z);
	}
	velocity -= 0.0005 * deltaTime;
	if (velocity < 0) {
		velocity = 0;
	}

}

void MyCheerio::render(VSShaderLib& shader, bool bumpmap) {
	cheerio.render(shader, "cheerio", {}, bumpmap);
}


std::vector<My3DVector> MyCheerio::AABB() {
	//GATHER ALL VERTICES OF BUTTER CUBE
	std::vector<My3DVector> vertices = { My3DVector{ MAX_CHEERIO_RADIUS, (MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, MAX_CHEERIO_RADIUS },
											My3DVector{ MAX_CHEERIO_RADIUS, (MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, -MAX_CHEERIO_RADIUS },
											My3DVector{ MAX_CHEERIO_RADIUS, -(MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, MAX_CHEERIO_RADIUS },
											My3DVector{ MAX_CHEERIO_RADIUS, -(MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, -MAX_CHEERIO_RADIUS },
											My3DVector{ -MAX_CHEERIO_RADIUS, (MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, MAX_CHEERIO_RADIUS },
											My3DVector{ -MAX_CHEERIO_RADIUS, (MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, -MAX_CHEERIO_RADIUS },
											My3DVector{ -MAX_CHEERIO_RADIUS, -(MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, MAX_CHEERIO_RADIUS },
											My3DVector{ -MAX_CHEERIO_RADIUS, -(MAX_CHEERIO_RADIUS - MIN_CHEERIO_RADIUS) / 2, -MAX_CHEERIO_RADIUS } };

	My3DVector minPos = cheerio.updateVertice(vertices[0]);
	My3DVector maxPos = cheerio.updateVertice(vertices[0]);

	for (My3DVector vertices : vertices) {
		My3DVector updatedVertices = cheerio.updateVertice(vertices);

		minPos = My3DVector{ std::min(minPos.x, updatedVertices.x), std::min(minPos.y, updatedVertices.y), std::min(minPos.z, updatedVertices.z) };
		maxPos = My3DVector{ std::max(maxPos.x, updatedVertices.x), std::max(maxPos.y, updatedVertices.y), std::max(maxPos.z, updatedVertices.z) };
	}

	//return min and max position of vertices
	return { minPos, maxPos };
}

MyRoad::MyRoad() {

}

MyRoad::MyRoad(float radius, float innerRadius) {
	MyCheerio newcheerio;
	My3DVector cheerioposition;
	int cheerioY = 1;
	for (int i = -radius; i <= radius; i += CHEERIO_SPACE) {
		cheerioposition = My3DVector(i, cheerioY, radius);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
		cheerioposition = My3DVector(i, cheerioY, -radius);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
	}

	for (int j = -radius; j <= radius; j += CHEERIO_SPACE) {
		cheerioposition = My3DVector(radius, cheerioY, j);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
		cheerioposition = My3DVector(-radius, cheerioY, j);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
	}

	for (int i = -innerRadius; i <= innerRadius; i += CHEERIO_SPACE) {
		cheerioposition = My3DVector(i, cheerioY, innerRadius);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
		cheerioposition = My3DVector(i, cheerioY, -innerRadius);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
	}

	for (int j = -innerRadius; j <= innerRadius; j += CHEERIO_SPACE) {
		cheerioposition = My3DVector(innerRadius, cheerioY, j);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
		cheerioposition = My3DVector(-innerRadius, cheerioY, j);
		newcheerio = MyCheerio(cheerioposition);
		cheerios.push_back(newcheerio);
	}

}

void MyRoad::updatePosition(float deltaTime) {
	for (int i = 0; i < cheerios.size(); i++) {
		cheerios[i].updatePosition(deltaTime);
	}
}

void MyRoad::render(VSShaderLib& shader, bool bumpmap) {
	for (MyCheerio current_cheerio : cheerios) {
		current_cheerio.render(shader, bumpmap);
	}
}


MyCandle::MyCandle() {
}

MyCandle::MyCandle(My3DVector position) {

	MyMesh candleMesh;

	float candleamb[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float candlediff[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float candlespec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float candleemissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float candleshininess = 100.0f;
	int candletexcount = 0;

	candleMesh = createCylinder(5, 2, 20);
	memcpy(candleMesh.mat.ambient, candleamb, 4 * sizeof(float));
	memcpy(candleMesh.mat.diffuse, candlediff, 4 * sizeof(float));
	memcpy(candleMesh.mat.specular, candlespec, 4 * sizeof(float));
	memcpy(candleMesh.mat.emissive, candleemissive, 4 * sizeof(float));
	candleMesh.mat.shininess = candleshininess;
	candleMesh.mat.texCount = candletexcount;

	candle = MyObject(candleMesh, My3DVector(position.x, position.y + 2.5, position.z), My3DVector(1, 1, 1), {});

	light_position[0] = position.x;
	light_position[1] = 5 + 2;
	light_position[2] = position.z;
	light_position[3] = 1.0;
}

void MyCandle::render(VSShaderLib& shader, bool bumpmap) {
	candle.render(shader, "candle", {}, bumpmap);
}

MyCoin::MyCoin() {

}


MyCoin::MyCoin(My3DVector position) {

	MyMesh coinMesh;

	//direction = My3DVector(0.0f, 0.0f, 0.0f);

	float amb[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float diff[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float spec[] = { 1.0f, 0.65f, 0.0f, 1.0f };
	float emissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float shininess = 500.0;
	int texcount = 0;

	coinMesh = createTorus(MIN_COIN_RADIUS, MAX_COIN_RADIUS, 20, 20);
	memcpy(coinMesh.mat.ambient, amb, 4 * sizeof(float));
	memcpy(coinMesh.mat.diffuse, diff, 4 * sizeof(float));
	memcpy(coinMesh.mat.specular, spec, 4 * sizeof(float));
	memcpy(coinMesh.mat.emissive, emissive, 4 * sizeof(float));
	coinMesh.mat.shininess = shininess;
	coinMesh.mat.texCount = texcount;

	coin = MyObject(coinMesh, position, My3DVector(1, 1, 1), { My3DVector(float(90), 0, 0, 1), My3DVector(0, 1, 0, 0) });

}

void MyCoin::updatePosition() {
	coin.rotations[1].angle += 5;
	fmod(coin.rotations[1].angle, 360.0f);
}


std::vector<My3DVector> MyCoin::AABB() {
	//GATHER ALL VERTICES OF BUTTER CUBE
	std::vector<My3DVector> vertices = { My3DVector{ MAX_COIN_RADIUS, (MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, MAX_COIN_RADIUS },
											My3DVector{ MAX_COIN_RADIUS, (MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, -MAX_COIN_RADIUS },
											My3DVector{ MAX_COIN_RADIUS, -(MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, MAX_COIN_RADIUS },
											My3DVector{ MAX_COIN_RADIUS, -(MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, -MAX_COIN_RADIUS },
											My3DVector{ -MAX_COIN_RADIUS, (MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, MAX_COIN_RADIUS },
											My3DVector{ -MAX_COIN_RADIUS, (MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, -MAX_COIN_RADIUS },
											My3DVector{ -MAX_COIN_RADIUS, -(MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, MAX_COIN_RADIUS },
											My3DVector{ -MAX_COIN_RADIUS, -(MAX_COIN_RADIUS - MIN_COIN_RADIUS) / 2, -MAX_COIN_RADIUS } };

	My3DVector minPos = coin.updateVertice(vertices[0]);
	My3DVector maxPos = coin.updateVertice(vertices[0]);

	for (My3DVector vertices : vertices) {
		My3DVector updatedVertices = coin.updateVertice(vertices);

		minPos = My3DVector{ std::min(minPos.x, updatedVertices.x), std::min(minPos.y, updatedVertices.y), std::min(minPos.z, updatedVertices.z) };
		maxPos = My3DVector{ std::max(maxPos.x, updatedVertices.x), std::max(maxPos.y, updatedVertices.y), std::max(maxPos.z, updatedVertices.z) };
	}

	//return min and max position of vertices
	return { minPos, maxPos };
}

void MyCoin::render(VSShaderLib& shader) {
	coin.render(shader, "coin", {}, false);
}

MyMirror::MyMirror() {

}


MyMirror::MyMirror(My3DVector pos) {
	MyMesh mirrorMesh;

	float mirroramb[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float  mirrordiff[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float  mirrorspec[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float  mirroremissive[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float  mirrorshininess = 100.0f;
	int  mirrortexcount = 0;

	mirrorMesh = createCube();
	memcpy(mirrorMesh.mat.ambient, mirroramb, 4 * sizeof(float));
	memcpy(mirrorMesh.mat.diffuse, mirrordiff, 4 * sizeof(float));
	memcpy(mirrorMesh.mat.specular, mirrorspec, 4 * sizeof(float));
	memcpy(mirrorMesh.mat.emissive, mirroremissive, 4 * sizeof(float));
	mirrorMesh.mat.shininess = mirrorshininess;
	mirrorMesh.mat.texCount = mirrortexcount;

	mirror = MyObject(mirrorMesh, My3DVector(pos.x, pos.y, pos.z), My3DVector(50, 0, 50), {});

}

void MyMirror::render(VSShaderLib& shader) {
	mirror.render(shader, "mirror", {}, false);
}

//BILLBOARD
std::vector<My3DVector> l3dBillboardSphericalBegin(float* cam, float* worldPos) {

	std::vector<My3DVector> rotations = {};

	float lookAt[3] = { 0,0,1 }, objToCamProj[3], objToCam[3], upAux[3], angleCosine;

	// objToCamProj is the vector in world coordinates from the local origin to the camera
	// projected in the XZ plane
	objToCamProj[0] = cam[0] - worldPos[0];
	objToCamProj[1] = 0;
	objToCamProj[2] = cam[2] - worldPos[2];

	// normalize both vectors to get the cosine directly afterwards
	normalize(objToCamProj);

	// easy fix to determine wether the angle is negative or positive
	// for positive angles upAux will be a vector pointing in the 
	// positive y direction, otherwise upAux will point downwards
	// effectively reversing the rotation.

	crossProduct(lookAt, objToCamProj, upAux);

	// compute the angle
	angleCosine = dotProduct(lookAt, objToCamProj);

	// perform the rotation. The if statement is used for stability reasons
	// if the lookAt and v vectors are too close together then |aux| could
	// be bigger than 1 due to lack of precision
	if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
		rotate(MODEL, acos(angleCosine) * 180 / 3.14, upAux[0], upAux[1], upAux[2]);


	// The second part tilts the object so that it faces the camera

	// objToCam is the vector in world coordinates from the local origin to the camera
	objToCam[0] = cam[0] - worldPos[0];
	objToCam[1] = cam[1] - worldPos[1];
	objToCam[2] = cam[2] - worldPos[2];

	// Normalize to get the cosine afterwards
	normalize(objToCam);

	// Compute the angle between v and v2, i.e. compute the
	// required angle for the lookup vector
	angleCosine = dotProduct(objToCamProj, objToCam);


	// Tilt the object. The test is done to prevent instability when objToCam and objToCamProj have a very small
	// angle between them
	if ((angleCosine < 0.99990) && (angleCosine > -0.9999))
		if (objToCam[1] < 0)
			rotations.push_back(My3DVector{ float(acos(angleCosine) * 180 / 3.14), 1, 0, 0 });
		else
			rotations.push_back(My3DVector{ float(acos(angleCosine) * 180 / 3.14), -1, 0, 0 });

	return rotations;
}

void MyObject::render(VSShaderLib& shader, std::string objectType, float* cam, bool bumpmap) {


	GLint loc;

	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.ambient");
	glUniform4fv(loc, 1, mesh.mat.ambient);
	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.diffuse");
	glUniform4fv(loc, 1, mesh.mat.diffuse);
	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.specular");
	glUniform4fv(loc, 1, mesh.mat.specular);
	loc = glGetUniformLocation(shader.getProgramIndex(), "mat.shininess");
	glUniform1f(loc, mesh.mat.shininess);

	pushMatrix(MODEL);
	translate(MODEL, position.x, position.y, position.z);
	if (objectType.compare("tree") == 0) {
		float pos[3];

		pos[0] = position.x; pos[1] = position.y; pos[2] = position.z;

		l3dBillboardSphericalBegin(cam, pos);
	}

	for (My3DVector MyRotation : rotations) {
		rotate(MODEL, MyRotation.angle, MyRotation.x, MyRotation.y, MyRotation.z);
	}
	scale(MODEL, scaleSize.x, scaleSize.y, scaleSize.z);
	for (My3DVector MyCenter : center) {
		translate(MODEL, MyCenter.x, MyCenter.y, MyCenter.z);
	}

	GLint pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	GLint vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	GLint normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");
	GLint texMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "texMode");// different modes of texturing
	GLint model_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_Model");
	GLint view_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_View");

	glUniformMatrix4fv(model_uniformId, 1, GL_FALSE, mMatrix[MODEL]);
	glUniformMatrix4fv(view_uniformId, 1, GL_FALSE, mMatrix[VIEW]);
	// send matrices to OGL
	computeDerivedMatrix(PROJ_VIEW_MODEL);
	glUniformMatrix4fv(vm_uniformId, 1, GL_FALSE, mCompMatrix[VIEW_MODEL]);
	glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);
	computeNormalMatrix3x3();
	glUniformMatrix3fv(normal_uniformId, 1, GL_FALSE, mNormal3x3);

	//TEXTURES FOR OBJECT
	if (objectType.compare("table") == 0) glUniform1i(texMode_uniformId, 0); //road + grass 0+1

	else if (objectType.compare("cheerio") == 0) {
		if (bumpmap) {
			glUniform1i(texMode_uniformId, 14);
		}
		else glUniform1i(texMode_uniformId, 2);
	
	}
	else if (objectType.compare("orange") == 0) {
		if (bumpmap) {
			glUniform1i(texMode_uniformId, 13);
		}
		else glUniform1i(texMode_uniformId, 3);
	}
	else if (objectType.compare("tree") == 0) glUniform1i(texMode_uniformId, 4);
	else if (objectType.compare("butter") == 0) glUniform1i(texMode_uniformId, 6);
	else if (objectType.compare("candle") == 0) {
		if (bumpmap) {
			glUniform1i(texMode_uniformId, 12);
		}
		else glUniform1i(texMode_uniformId, 7);
	}
	else if (objectType.compare("particles") == 0) glUniform1i(texMode_uniformId, 9);
	else if (objectType.compare("skyBox") == 0) glUniform1i(texMode_uniformId, 10);
	else if (objectType.compare("envCube") == 0) glUniform1i(texMode_uniformId, 11);
	else glUniform1i(texMode_uniformId, 5);

	// Render mesh
	glBindVertexArray(mesh.vao);

	if (!shader.isProgramValid()) {
		printf("Program Not Valid!\n");
		exit(1);
	}

	glDrawElements(mesh.type, mesh.numIndexes, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	popMatrix(MODEL);

}