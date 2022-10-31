#ifndef MYOBJECT
#define MYOBJECT

#define BOARDSIZE 200
#define MAX_VELOCITY 5
#define MIN_VELOCITY 1
#define CAR_INITIAL_X -75
#define CAR_INITIAL_Y 0.2
#define CAR_INITIAL_Z 0
#define MAX_ORANGES 5
#define MIN_ORANGES 8
#define MAX_CHEERIO_RADIUS 2
#define MIN_CHEERIO_RADIUS 0.5
#define MAX_COIN_RADIUS 0.5
#define MIN_COIN_RADIUS 0.25
#define CHEERIO_SPACE 5
#define frand()			((float)rand()/RAND_MAX)
#define M_PI			3.14159265
#define MAX_PARTICULAS  1500

// assimp include files. These three are usually needed.
#include "assimp/Importer.hpp"	//OO version Header!
#include "assimp/scene.h"
#include "flare.h"
#include "Texture_Loader.h"



class My3DVector {
public:
	float x;
	float y;
	float z;
	float angle;

	My3DVector();
	My3DVector(float x, float y, float z);
	My3DVector(float angle, float x, float y, float z);

	void normalize2D();

	My3DVector rotatearoundX(float angle);
	My3DVector rotatearoundY(float angle);
	My3DVector rotatearoundZ(float angle);
	My3DVector inverseRotationZ(float angle);
};

class MyObject {
public:
	MyMesh mesh;
	My3DVector position;
	My3DVector scaleSize;
	std::vector<My3DVector> center;
	std::vector<My3DVector> rotations;
	float angle = 0;

	MyObject();
	MyObject(MyMesh objectMesh, My3DVector pos, My3DVector scale, std::vector<My3DVector> rotationsList);

	My3DVector updateVertice(My3DVector vertice);

	void render(VSShaderLib& shader, std::string objectType, float* cam, bool bumpmap);

	//void render(VSShaderLib& shader, std::string objectType, float* cam);

	//void render(VSShaderLib& shader, std::string objectType);

	//void render(VSShaderLib& shader);

};

class MyParticles {
public:
	float	life;		// vida
	float	fade;		// fade
	float	r, g, b;    // color
	My3DVector velocity;
	My3DVector aceleration;


	MyObject particle;
	MyParticles();
	void render(VSShaderLib& shader);

	void iniParticles(float X, float Y, float Z);
	void updateParticles();


};

class MyTable {
public:
	MyObject table;

	MyTable();
	MyTable(My3DVector position);

	void render(VSShaderLib& shader, bool bumpmap);

	//void render(VSShaderLib& shader);

};

class MyOrange {
public:
	My3DVector direction;
	MyObject orange;
	float carVelocity;


	MyOrange();
	MyOrange(My3DVector position, float velocity);

	void render(VSShaderLib& shader, bool bumpmap);


	bool outOfLimits();
	void updatePosition(float deltaTime);

};

class MyButter {
public:
	MyObject butter;
	float velocity;
	My3DVector direction;
	MyButter();
	MyButter(My3DVector position);

	void updatePosition(float deltaTime);

	void collision(My3DVector carDirection, float carVelocity);

	void updatePosition(My3DVector direction, float velocity);

	void render(VSShaderLib& shader);
	std::vector<My3DVector> AABB();
};

class MyTree {
public:
	MyObject tree;
	My3DVector positionTree;
	MyTree();
	MyTree(My3DVector position);


	//void render(VSShaderLib& shader);
	void render(VSShaderLib& shader, float* cam);
	bool outOfLimits();
};

class MyCheerio {
public:
	float velocity;
	My3DVector direction;

	MyObject cheerio;

	MyCheerio();
	MyCheerio(My3DVector position);

	void collision(My3DVector carDirection, float carVelocity);

	void updatePosition(float deltaTime);


	std::vector<My3DVector> AABB();
	void updatePosition(My3DVector direction, float velocity);

	void render(VSShaderLib& shader, bool bumpmap);

};

class MyCandle {
public:
	MyObject candle;
	float light_position[4] = { 0.0 };

	MyCandle();
	MyCandle(My3DVector position);

	void render(VSShaderLib& shader, bool bumpmap);


};

class MyRoad {
public:
	std::vector<MyCheerio> cheerios;

	MyRoad();
	MyRoad(float radius, float innerRadius);

	void updatePosition(float deltaTime);

	void render(VSShaderLib& shader, bool bumpmap);



};


class MyLife {
public:
	MyObject life;
	bool active = false;

	MyLife();
	MyLife(My3DVector pos);

	void render(VSShaderLib& shader);


};

class MyCoin {
public:
	MyObject coin;
	bool catched = false;

	MyCoin();
	MyCoin(My3DVector pos);

	void updatePosition();

	std::vector<My3DVector> AABB();

	void render(VSShaderLib& shader);


};

class MyAssimp {
public:

	std::vector<MyMesh> myMeshes;
	My3DVector position;
	My3DVector scaleSize;
	std::vector<My3DVector> center;
	std::vector<My3DVector> rotations;
	My3DVector translation;


	MyAssimp();

	MyAssimp(std::string modelDir, My3DVector pos, My3DVector scale, std::vector<My3DVector> rotationsList);

	void render(VSShaderLib& shader, const aiScene* sc, const aiNode* nd);

};

class MyCar {
public:
	My3DVector direction;
	MyObject body;
	MyObject window;
	MyObject wheels[4];
	MyObject lights[2];
	My3DVector lights_position[2];
	float rotation = 0.0;
	float velocity = 0.0f;

	MyAssimp carAssimp;


	MyCar();
	MyCar(My3DVector position);

	void render(VSShaderLib& shader);
	void up();
	void down();
	void turnLeft();
	void turnRight();
	void updatePosition();
	void accelerate(float deltaTime);
	void deccelerate(float deltaTime);
	void breaks(bool acceleration);
	void restart();
	std::vector<My3DVector> AABB();
	void stop();
	void updateDirection();
};


class MyFlare {
public:
	std::vector<char*> flareTextureNames = { "crcl", "flar", "hxgn", "ring", "sun" };
	MyFlare();
	MyFlare(char* file_path);
	double clamp(const double x, const double min, const double max);
	int clampi(const int x, const int min, const int max);
	int getTextureId(char* name);
	void loadFlareFile(FLARE_DEF* flare, char* filename);
	void render_flare(VSShaderLib& shader, FLARE_DEF* flare, int lx, int ly, int* m_viewport);
	void renderSceneFlare(VSShaderLib& shader, My3DVector lighPos);
	MyMesh myMeshFlare;
	void render(VSShaderLib& shader);

};

class MySkyBox {
public:
	MyObject skyBox;
	MySkyBox();
	MySkyBox(My3DVector pos);

	void render(VSShaderLib& shader);
};

class MyEnvCube {
public:
	MyObject envCube;
	MyEnvCube();
	MyEnvCube(My3DVector pos);

	void render(VSShaderLib& shader);
};


class MyMirror {
public:
	MyObject mirror;
	MyMirror();
	MyMirror(My3DVector pos);

	void render(VSShaderLib& shader);
};


#endif 