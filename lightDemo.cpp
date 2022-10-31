#include <math.h>
#include <iostream>
#include <sstream>
#include <string>
#include <Windows.h>
#include <iostream>
#include <Mmsystem.h>
#include <mciapi.h>
#pragma comment(lib, "Winmm.lib")
#include <mmsystem.h>

// include GLEW to access OpenGL 3.3 functions
#include <GL/glew.h>


// GLUT is the toolkit to interface with the OS
#include <GL/freeglut.h>

#include <IL/il.h>

// Use Very Simple Libs
#include "VSShaderlib.h"
#include "AVTmathLib.h"
#include "VertexAttrDef.h"
#include "geometry.h"

#include "avtFreeType.h"

#include "MyObject.h"

#define M_PI			3.14159265

using namespace std;

#define CAPTION "CGJ Micromachines"
int WindowHandle = 0;
int WinX = 640, WinY = 480;

unsigned int FrameCount = 0;

//shaders
VSShaderLib shader;  //geometry
VSShaderLib shaderText;  //render bitmap text

//File with the font
const string font_name = "fonts/arial.ttf";

//Vector with meshes
vector<struct MyMesh> myMeshes;


//External array storage defined in AVTmathLib.cpp

/// The storage for matrices
extern float mMatrix[COUNT_MATRICES][16];
extern float mCompMatrix[COUNT_COMPUTED_MATRICES][16];

/// The normal matrix
extern float mNormal3x3[9];

GLint pvm_uniformId;
GLint vm_uniformId;
GLint normal_uniformId;
GLint lPos_uniformId;
GLint normalMap_loc;
GLint specularMap_loc;
GLint diffMapCount_loc;
GLint shadowMode_uniformId;

//Texture stuff
GLint tex_loc, tex_loc1, tex_loc2, tex_loc3, tex_loc4, tex_loc5, tex_loc6, tex_loc7, tex_loc8, tex_loc9, tex_loc25, tex_cube_loc, tex_grassNormal, tex_candleNormal, tex_cerealNormal;
GLint texMode_uniformId;
unsigned int texture;
GLuint TextureArray[25];


//FLARE STUFF
bool flareEffect = false;
bool flag = false;


//fog
GLint fogActivation_uniformId;
bool fogActivation = false;

bool music = false;
bool pause = false;

float x, y, z;
float cam[3] = { x,y,z };

//lights
bool directionalLight = false;
bool pointLight = true;
bool spotlights = false;

GLint spotlight_mode;
GLint directional_mode;
GLint pointlight_mode;

GLint ldDirection_uniformId;
GLint reflect_perFragment_uniformId;


// Mouse Tracking Variables
int startX, startY, tracking = 0;

//Bumpmap
bool bumpmap = false;


// Camera Spherical Coordinates
float alpha = 0.0f, beta = 15;
float r = 8.0f;
float xUp = 0, yUp = 1, zUp = 0;
My3DVector lookAtRearview;
int deltaX, deltaY;
float alphaAux = alpha, betaAux = beta;
float rAux;

// Camera Position
float camX, camY, camZ;
float orthoHeight = BOARDSIZE / 2;
float ratio;

string camType = "main";
bool mouseControlActive = true;



// Frame counting and FPS computation
long myTime, timebase = 0, frame = 0;
char s[32];
float lightPos[4] = { 0.0f, 5.0f, 0.0f, 1.0f };


//MyObjects
MyCar car;
MyTable table;
MyFlare flare;
std::vector<MyOrange> oranges;
std::vector<MyTree> trees;
std::vector<MyButter> butters;
std::vector<MyCandle> candles;
std::vector<MyCoin> coins;
std::vector<MyParticles> particles;
MyRoad road;
MyTree tree;
MySkyBox skyBox;
MyEnvCube envCube;
MyMirror mirror;



int numLife = 5;
int points = 0;
string txt;
string txt1;
string txt2;
string txt3;
string txtFinish1;
string txtFinish2;
string txtSound = "OFF";

float numOranges;
float numTree = 10;
float numButter = 4;
float numCandles = 6;
float numCoins = 50;
bool acceleration;
bool breaks = false;
bool stopRender = false;
bool help = false;

int lastTime = 0;
int deltaTime = 0;

int dead_num_particles = 0;
bool fireworks;
float particle_color[4];


void timer(int value)
{
	std::ostringstream oss;
	oss << CAPTION << ": " << FrameCount << " FPS @ (" << WinX << "x" << WinY << ")";
	std::string s = oss.str();
	glutSetWindow(WindowHandle);
	glutSetWindowTitle(s.c_str());
	FrameCount = 0;
	glutTimerFunc(1000, timer, 0);

}

void refresh(int value)
{
	glutPostRedisplay();
	glutTimerFunc(1000 / 60, refresh, 0);
}

void catchCoin(int coinID) {
	//create car AABB
	std::vector<My3DVector> carAABBPosition = car.AABB(); //first pos = min, second pos = max

	//Coins
	//AABB vs AABB 
	std::vector<My3DVector> CoinAABBPosition = coins[coinID].AABB(); //first pos = min, second pos = max

	if ((carAABBPosition[0].x <= CoinAABBPosition[1].x && carAABBPosition[1].x >= CoinAABBPosition[0].x) &&
		(carAABBPosition[0].y <= CoinAABBPosition[1].y && carAABBPosition[1].y >= CoinAABBPosition[0].y) &&
		(carAABBPosition[0].z <= CoinAABBPosition[1].z && carAABBPosition[1].z >= CoinAABBPosition[0].z)) {
		coins[coinID].catched = true;
		points++;
	}

}


void lights() {

	GLint loc;

	float res_cone_dir[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float coneDir[4] = { -cos((car.body.angle - 90.0f) * 3.14 / 180), 0 , sin((car.body.angle - 90.0f) * 3.14 / 180), 0.0f };

	multMatrixPoint(VIEW, coneDir, res_cone_dir);
	loc = glGetUniformLocation(shader.getProgramIndex(), "coneDir");
	glUniform4fv(loc, 1, res_cone_dir);

	loc = glGetUniformLocation(shader.getProgramIndex(), "spotCosCutOff");
	glUniform1f(loc, 0.93f);

	float res_pos[8] = { 0.0 };

	My3DVector updated_position0 = car.lights[0].updateVertice(car.lights[0].position);
	My3DVector updated_position1 = car.lights[1].updateVertice(car.lights[1].position);


	float diff_pos0[4] = { updated_position0.x - car.lights[0].position.x, 0, updated_position0.z - car.lights[0].position.z, 1.0f };
	float diff_pos1[4] = { updated_position1.x - car.lights[1].position.x, 0, updated_position1.z - car.lights[1].position.z, 1.0f };

	float average_diff[4] = { (diff_pos0[0] + diff_pos1[0]) / 2, (diff_pos0[1] + diff_pos1[1]) / 2, (diff_pos0[2] + diff_pos1[2]) / 2, (diff_pos0[3] + diff_pos1[3]) / 2 };

	float light_pos0[4] = { updated_position0.x - average_diff[0] , updated_position0.y, updated_position0.z - average_diff[2] , 1.0f };
	float light_pos1[4] = { updated_position1.x - average_diff[0] , updated_position1.y, updated_position1.z - average_diff[2] , 1.0f };

	multMatrixPoint(VIEW, light_pos0, res_pos);   //lightPos definido em World Coord so is converted to eye space
	multMatrixPoint(VIEW, light_pos1, res_pos + 4);   //lightPos definido em World Coord so is converted to eye space

	// Spotlights
	GLint sl_pos_uniformId = glGetUniformLocation(shader.getProgramIndex(), "sl_pos");
	glUniform4fv(sl_pos_uniformId, 2, res_pos);

	float res_dl_dir[4] = { 0.0 };

	//Directional Light
	for (int i = 0; i < 1; i++) {
		float light_pos[4] = { 0.0f, -1.0f, 0.0f, 0.0f };
		multMatrixPoint(VIEW, light_pos, res_dl_dir);
	}

	GLint dl_dir_uniformId = glGetUniformLocation(shader.getProgramIndex(), "dl_dir");
	glUniform4fv(dl_dir_uniformId, 1, res_dl_dir);

	// Pointlights
	float res_pl_pos[4 * 6] = { 0.0 };

	for (int i = 0; i < numCandles; i++) {
		float light_pos[4] = { candles[i].light_position[0] , candles[i].light_position[1], candles[i].light_position[2] , 1.0f };
		multMatrixPoint(VIEW, light_pos, res_pl_pos + 4 * i);
	}

	GLint pp_pos_unfiromId = glGetUniformLocation(shader.getProgramIndex(), "pl_pos");
	glUniform4fv(pp_pos_unfiromId, 6, res_pl_pos);

}

void renderObjects() {

	envCube.render(shader);
	if (!pause) road.updatePosition(deltaTime);
	road.render(shader, bumpmap);
	if (!pause) car.updatePosition();
	//table.render(shader, bumpmap);
	for (int i = 0; i < numOranges; i++) {
		if (oranges[i].outOfLimits()) {
			float velocity = rand() % (MAX_VELOCITY - MIN_VELOCITY + 1) + MIN_VELOCITY;
			oranges[i] = MyOrange::MyOrange(My3DVector(10, 0.5, 10), velocity);
		}
		if (!pause) oranges[i].updatePosition(deltaTime);
		oranges[i].render(shader, bumpmap);
	}
	for (int i = 0; i < numCandles; i++) candles[i].render(shader, bumpmap);
	for (int i = 0; i < coins.size(); i++) {
		if (!coins[i].catched) {
			if (!pause) coins[i].updatePosition();
			coins[i].render(shader);
		}
	}
	for (int i = 0; i < numButter; i++) {
		if (!pause) butters[i].updatePosition(deltaTime);
		butters[i].render(shader);
	}
	for (int i = 0; i < numTree; i++) {
		if (trees[i].outOfLimits()) trees[i] = MyTree::MyTree(My3DVector(10, 0, 10));
		float sendCam[3] = { x,y,z };
		trees[i].render(shader, sendCam);
	}
	if (fireworks) {
		for (int i = 0; i < MAX_PARTICULAS; i++)
		{
			particles[i].updateParticles();
			if (particles[i].life > 0.0f) {
				/* A vida da partícula representa o canal alpha da cor. Como o blend está activo a cor final é a soma da cor rgb do fragmento multiplicada pelo
				alpha com a cor do pixel destino */
				particle_color[0] = particles[i].r;
				particle_color[1] = particles[i].g;
				particle_color[2] = particles[i].b;
				particle_color[3] = particles[i].life;
				// send the material - diffuse color modulated with texture
				memcpy(particles[i].particle.mesh.mat.diffuse, particle_color, 4 * sizeof(float));

				particles[i].render(shader);
			}
			else dead_num_particles++;
			

		}
		if (dead_num_particles == MAX_PARTICULAS) {
			fireworks = false;
			dead_num_particles = 0;
			printf("All particles dead\n");
		}
	}
	car.render(shader);
	

}


// Reshape Callback Function
void changeProjection() {
	loadIdentity(PROJECTION);
	if (camType.compare("orthogonal") == 0) ortho(-orthoHeight * ratio, orthoHeight * ratio, -orthoHeight, orthoHeight, -0.1f, 1000.0f);
	else perspective(53.13f, ratio, 0.1f, 1000.0f);
}

void changeSize(int w, int h) {
	// Prevent a divide by zero, when window is too short
	if (h == 0) h = 1;
	// set the viewport to be the entire window
	glViewport(0, 0, w, h);
	// set the projection matrix
	ratio = (1.0f * w) / h;
	WinX = w;
	WinY = h;
	changeProjection();

}

void checkFinish() {
	if (car.carAssimp.position.z > 60 && car.carAssimp.position.z < 90 && car.carAssimp.position.x < 0 && car.carAssimp.position.x > -5) {
		flag = true;
	}

	if (flag == true) {
		if (car.carAssimp.position.z > -3 && car.carAssimp.position.z < 3 && car.carAssimp.position.x > -90 && car.carAssimp.position.x < -64) {
			flag = false;
			fireworks = true;
			txtFinish1 = "YOU WON";
			txtFinish2 = "'R' restart";
			pause = true;
			for (int i = 0; i < MAX_PARTICULAS; i++) particles[i].iniParticles(car.carAssimp.position.x, car.carAssimp.position.y, car.carAssimp.position.z);
		}
	}
}

//CHECK FOR CAR COLLISIONS
void carCollisions() {
	//create car AABB
	std::vector<My3DVector> carAABBPosition = car.AABB(); //first pos = min, second pos = max

	//Oranges
	//AABB vs Sphere 
	for (int i = 0; i < numOranges; i++) {
		//idea: var x = Math.max(box.minX, Math.min(sphere.x, box.maxX));
		float x = max(carAABBPosition[0].x, min(oranges[i].orange.position.x, carAABBPosition[1].x));
		float y = max(carAABBPosition[0].y, min(oranges[i].orange.position.y, carAABBPosition[1].y));
		float z = max(carAABBPosition[0].z, min(oranges[i].orange.position.z, carAABBPosition[1].z));

		float distance = sqrt(pow((x - oranges[i].orange.position.x), 2) +
			pow((y - oranges[i].orange.position.y), 2) +
			pow((z - oranges[i].orange.position.z), 2));

		if (distance < 1) { //distance < sphere.radius; COLLISION
			numLife--;
			car.stop();
			car.restart();
		}
	}

	//Butter
	//AABB vs AABB 
	for (int i = 0; i < numButter; i++) {
		std::vector<My3DVector> ButterAABBPosition = butters[i].AABB(); //first pos = min, second pos = max
		if ((carAABBPosition[0].x <= ButterAABBPosition[1].x && carAABBPosition[1].x >= ButterAABBPosition[0].x) &&
			(carAABBPosition[0].y <= ButterAABBPosition[1].y && carAABBPosition[1].y >= ButterAABBPosition[0].y) &&
			(carAABBPosition[0].z <= ButterAABBPosition[1].z && carAABBPosition[1].z >= ButterAABBPosition[0].z)) {
			butters[i].collision(car.direction, car.velocity);
			car.stop();
			acceleration = false;

		}
	}



	bool has_colided = false;
	for (int i = 0; i < road.cheerios.size(); i++) {
		std::vector<My3DVector> CheerioAABBPosition = road.cheerios[i].AABB(); //first pos = min, second pos = max

		if ((carAABBPosition[0].x <= CheerioAABBPosition[1].x && carAABBPosition[1].x >= CheerioAABBPosition[0].x) &&
			(carAABBPosition[0].y <= CheerioAABBPosition[1].y && carAABBPosition[1].y >= CheerioAABBPosition[0].y) &&
			(carAABBPosition[0].z <= CheerioAABBPosition[1].z && carAABBPosition[1].z >= CheerioAABBPosition[0].z)) {
			road.cheerios[i].collision(car.direction, car.velocity);
			has_colided = true;
		}
	}

	if (has_colided) {
		car.stop();
		acceleration = false;
		has_colided = false;
	}

	for (int i = 0; i < coins.size(); i++) {
		if(!coins[i].catched) catchCoin(i);
	}

}


// ------------------------------------------------------------
//
// Model loading and OpenGL setup
//

void init()
{
	/* Initialization of DevIL */
	if (ilGetInteger(IL_VERSION_NUM) < IL_VERSION) {
		printf("wrong DevIL version \n");
		exit(0);
	}
	ilInit();

	/// Initialization of freetype library with font_name file
	freeType_init(font_name);
	srand(time(NULL));

	//Texture Object definition------------------------------------------------------------------------------------------
	glGenTextures(25, TextureArray);
	Texture2D_Loader(TextureArray, "./Texture_Materials/grass.jpg", 0);
	Texture2D_Loader(TextureArray, "./Texture_Materials/road.png", 1);
	Texture2D_Loader(TextureArray, "./Texture_Materials/cereal.jpg", 2);
	Texture2D_Loader(TextureArray, "./Texture_Materials/orange.jpg", 3);
	Texture2D_Loader(TextureArray, "./Texture_Materials/tree.png", 4);
	Texture2D_Loader(TextureArray, "./Texture_Materials/finish.png", 5);
	Texture2D_Loader(TextureArray, "./Texture_Materials/butter.jpg", 6);
	Texture2D_Loader(TextureArray, "./Texture_Materials/candle.jpg", 7);
	Texture2D_Loader(TextureArray, "./Texture_Materials/candle.jpg", 8);
	Texture2D_Loader(TextureArray, "./Texture_Materials/particle.tga", 9);
	Texture2D_Loader(TextureArray, "./Texture_Materials/orangeNormal.png", 11);
	Texture2D_Loader(TextureArray, "./Texture_Materials/candleNormal.png", 12);
	Texture2D_Loader(TextureArray, "./Texture_Materials/cerealNormal.png", 13);

	//Sky Box Texture Object

	const char* filenames[] = { "./Texture_Materials/skyBox/posx.png", "./Texture_Materials/skyBox/negx.png", "./Texture_Materials/skyBox/posy.png", "./Texture_Materials/skyBox/negy.png", "./Texture_Materials/skyBox/posz.png", "./Texture_Materials/skyBox/negz.png" };
	TextureCubeMap_Loader(TextureArray, filenames, 10);

	
	//init flare
	flare = MyFlare::MyFlare("./Texture_Materials/flare.txt");

	//init objects
	skyBox = MySkyBox::MySkyBox(My3DVector(0,0,0));
	envCube = MyEnvCube::MyEnvCube(My3DVector(-10, 10, -10));
	car = MyCar::MyCar(My3DVector(CAR_INITIAL_X, CAR_INITIAL_Y, CAR_INITIAL_Z));
	table = MyTable::MyTable(My3DVector(-BOARDSIZE / 2, 0, -BOARDSIZE / 2));
	numOranges = rand() % (MAX_ORANGES - MIN_ORANGES + 1) + MIN_ORANGES;
	road = MyRoad::MyRoad(BOARDSIZE / 2 - 10, BOARDSIZE / 2 - 40);
	mirror = MyMirror::MyMirror(My3DVector(0, 0, 0));
	oranges = {};
	for (int i = 0; i < numOranges; i++) {
		float velocity = rand() % (MAX_VELOCITY - MIN_VELOCITY + 1) + MIN_VELOCITY;
		oranges.push_back(MyOrange::MyOrange(My3DVector(10, 2, 10), velocity));
	}
	trees = {};
	trees.push_back(MyTree::MyTree(My3DVector(-60, 0, 20)));
	trees.push_back(MyTree::MyTree(My3DVector(-60, 0, 0)));
	trees.push_back(MyTree::MyTree(My3DVector(-60, 0, -20)));
	trees.push_back(MyTree::MyTree(My3DVector(-60, 0, -40)));
	trees.push_back(MyTree::MyTree(My3DVector(-60, 0, 40)));
	trees.push_back(MyTree::MyTree(My3DVector(60, 0, -40)));
	trees.push_back(MyTree::MyTree(My3DVector(60, 0, 40)));
	trees.push_back(MyTree::MyTree(My3DVector(60, 0, 0)));
	trees.push_back(MyTree::MyTree(My3DVector(60, 0, 20)));
	trees.push_back(MyTree::MyTree(My3DVector(60, 0, -20)));
	butters = { MyButter::MyButter(My3DVector(-70, 0, 50)), MyButter::MyButter(My3DVector(-40, 0, 80)), MyButter::MyButter(My3DVector(-40, 0, -80)), MyButter::MyButter(My3DVector(70, 0, 10)) };
	candles = {};
	candles.push_back(MyCandle::MyCandle(My3DVector(40, 0, 0)));
	candles.push_back(MyCandle::MyCandle(My3DVector(-40, 0, 0)));
	candles.push_back(MyCandle::MyCandle(My3DVector(40, 0, -40)));
	candles.push_back(MyCandle::MyCandle(My3DVector(-40, 0, -40)));
	candles.push_back(MyCandle::MyCandle(My3DVector(40, 0, 40)));
	candles.push_back(MyCandle::MyCandle(My3DVector(-40, 0, 40)));
	coins = {};

	for (int j = 0; j <= 20; j += 10) {
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(-77 + i, 1, 20 + i + j)));
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(-72 - i, 1, 20 + i + j + 5)));
	}
	for (int j = 0; j <= 20; j += 10) {
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(-77 + i, 1, -20 - i - j)));
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(-72 - i, 1, -20 - i - j - 5)));
	}
	for (int j = 0; j <= 20; j += 10) {
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(77 - i, 1, 20 + i + j)));
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(72 + i, 1, 20 + i + j + 5)));
	}
	for (int j = 0; j <= 20; j += 10) {
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(77 - i, 1, -20 - i - j)));
		for (int i = 0; i < 5; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(72 + i, 1, -20 - i - j - 5)));
	}

	for (int i = 0; i < numCoins / 2; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(10 + i, 1, -72)));
	for (int i = 0; i < numCoins / 2; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(-10 - i, 1, -72)));
	for (int i = 0; i < numCoins / 2; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(10 + i, 1, 72)));
	for (int i = 0; i < numCoins / 2; i += 1) coins.push_back(MyCoin::MyCoin(My3DVector(-10 - i, 1, 72)));


	for (int i = 0; i < MAX_PARTICULAS; i++) particles.push_back(MyParticles::MyParticles());

	// some GL settings
	//glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_BLEND);

}

//RESTART VARIABLES
void restartGame() {
	fireworks = false;
	init();
	pause = false;
	txt = "";
	txt1 = "";
	txt2 = "";
	txtFinish1 = "";
	txtFinish2 = "";
	txt3 = "";
	points = 0;
	numLife = 5;
	flag = false;
	if (music == true) {
		mciSendString("close \"music.mp3\"", NULL, 0, 0);
		mciSendString("play \"music.mp3\" repeat", NULL, 0, 0);
	}
	

}

//RENDER REARVIEW MIRROR
void rearviewMirror() {

	x = 0;
	y = 0;
	z = 0;

	x += car.body.position.x - 1.0f * car.direction.x;
	y += car.body.position.y + 2.0f;
	z += car.body.position.z - 0.01f * car.direction.z;

	lookAtRearview.x = car.body.position.x + car.direction.x * 10.0f;
	lookAtRearview.y = car.body.position.y - 3.0f;
	lookAtRearview.z = car.body.position.z - car.direction.z * 10.0f;

	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);
	pushMatrix(MODEL);
	
	
	//size of clipping
	ortho(-1, 2, 0, 2);

	glUseProgram(shader.getProgramIndex());

	translate(MODEL, 0, 1.8, 0);
	
	computeDerivedMatrix(PROJ_VIEW_MODEL);
	glUniformMatrix4fv(pvm_uniformId, 1, GL_FALSE, mCompMatrix[PROJ_VIEW_MODEL]);


	glStencilFunc(GL_NEVER, 0x1, 0x1);
	glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

	MyMesh mesh = createCube();
	glBindVertexArray(mesh.vao);
	glDrawElements(mesh.type, mesh.numIndexes, GL_UNSIGNED_INT, 0);

	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);
}


// ------------------------------------------------------------
//
// Render stufff
//

void renderScene(void) {

	float particle_color[4];

	int m_viewport[4];
	glGetIntegerv(GL_VIEWPORT, m_viewport);

	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	deltaTime = currentTime - lastTime;
	lastTime = currentTime;

	GLint loc;
	float res[4];

	FrameCount++;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// load identity matrices
	loadIdentity(VIEW);
	loadIdentity(MODEL);

	// set the camera using a function similar to gluLookAt
	if (camType == "main" || camType == "rearview") {
		// set the camera position based on its spherical coordinates
		camX = r * sin((alphaAux + car.body.angle) * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
		camZ = r * cos((alphaAux + car.body.angle) * 3.14f / 180.0f) * cos(betaAux * 3.14f / 180.0f);
		camY = r * sin(betaAux * 3.14f / 180.0f);

		x = camX + car.body.position.x;
		y = camY + car.body.position.y;
		z = camZ + car.body.position.z;

		//update lookat
		lookAt(x, y, z, car.body.position.x, car.body.position.y, car.body.position.z, xUp, yUp, zUp);
		
	} else if (camType == "perspective" || camType=="orthogonal") { //perspective e orthogonal
		camX = r * sin(0 * 3.14f / 180.0f) * cos(90 * 3.14f / 180.0f);
		camZ = r * cos(0 * 3.14f / 180.0f) * cos(90 * 3.14f / 180.0f);

		x = camX;
		y = BOARDSIZE;
		z = camZ;

		lookAt(x, y, z, 0, 0, 0, xUp, yUp, zUp);
	}


	// use our shader
	glUseProgram(shader.getProgramIndex());

	//fog
	fogActivation_uniformId = glGetUniformLocation(shader.getProgramIndex(), "fogActivation");
	if (fogActivation) glUniform1i(fogActivation_uniformId, 1);
	else glUniform1i(fogActivation_uniformId, 0);

	//headlights/spotlights
	loc = glGetUniformLocation(shader.getProgramIndex(), "spotlight_mode");
	if (spotlights) glUniform1i(loc, 1);
	else glUniform1i(loc, 0);

	//pointlight
	loc = glGetUniformLocation(shader.getProgramIndex(), "pointlight_mode");
	if (pointLight) glUniform1i(loc, 1);
	else glUniform1i(loc, 0);

	//Directional Light
	loc = glGetUniformLocation(shader.getProgramIndex(), "directional_mode");

	if (directionalLight) glUniform1i(loc, 1);
	else glUniform1i(loc, 0);

	lights();

	//Associar os Texture Units aos Objects Texture
	//road loaded in TU0;
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, TextureArray[0]);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, TextureArray[1]);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, TextureArray[2]);

	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, TextureArray[3]);

	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_2D, TextureArray[4]);

	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, TextureArray[5]);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_2D, TextureArray[6]);

	glActiveTexture(GL_TEXTURE7);
	glBindTexture(GL_TEXTURE_2D, TextureArray[7]);

	glActiveTexture(GL_TEXTURE8);
	glBindTexture(GL_TEXTURE_2D, TextureArray[8]);

	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, TextureArray[9]);

	glActiveTexture(GL_TEXTURE10);
	glBindTexture(GL_TEXTURE_CUBE_MAP, TextureArray[10]);

	glActiveTexture(GL_TEXTURE11);
	glBindTexture(GL_TEXTURE_2D, TextureArray[11]);

	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_2D, TextureArray[12]);

	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, TextureArray[13]);


	//Indicar ao sampler do GLSL quais os Texture Units a serem usados (1 por textura)
	glUniform1i(tex_loc, 0);
	glUniform1i(tex_loc1, 1);
	glUniform1i(tex_loc2, 2);
	glUniform1i(tex_loc3, 3);
	glUniform1i(tex_loc4, 4);
	glUniform1i(tex_loc5, 5);
	glUniform1i(tex_loc6, 6);
	glUniform1i(tex_loc7, 7);
	glUniform1i(tex_loc8, 8);
	glUniform1i(tex_loc9, 9);
	glUniform1i(tex_cube_loc, 10);
	glUniform1i(tex_grassNormal, 11);
	glUniform1i(tex_candleNormal, 12);
	glUniform1i(tex_cerealNormal, 13);


	//render obj
	skyBox.render(shader);

	checkFinish();	
	carCollisions();
	if (breaks) car.breaks(acceleration);
	if (numLife == 0) restartGame();
	

	float mat[16];
	GLfloat plano_chao[4] = { 0,1,0,0 };

	//glEnable(GL_DEPTH_TEST);

	if (y > 0.0f) {  //camera in front of the floor so render reflections and shadows. Inner product between the viewing direction and the normal of the ground

		glEnable(GL_STENCIL_TEST);        // Escrever 1 no stencil buffer onde se for desenhar a reflexão e a sombra
		glStencilFunc(GL_NEVER, 0x1, 0x1);
		glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

		// Fill stencil buffer with Ground shape; never rendered into color buffer
		table.render(shader, bumpmap);
		glUniform1i(shadowMode_uniformId, 0);  //iluminação phong

		// Desenhar apenas onde o stencil buffer esta a 1
		glStencilFunc(GL_EQUAL, 0x1, 0x1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

		
		pushMatrix(MODEL);
		scale(MODEL, 1.0f, -1.0f, 1.0f);
		glCullFace(GL_FRONT);
		renderObjects();
		glCullFace(GL_BACK);
		popMatrix(MODEL);


		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);		// Blend specular Ground with reflected geometry
		table.render(shader, bumpmap);
		// Render the Shadows
		float lightPos[4] = { candles[3].light_position[0], candles[3].light_position[1], candles[3].light_position[2], 1.0f };//position of point light in World coordinates
		glUniform1i(shadowMode_uniformId, 1);  //Render with constant color
		shadow_matrix(mat, plano_chao, lightPos);

		glDisable(GL_DEPTH_TEST); //To force the shadow geometry to be rendered even if behind the floor

		//Dark the color stored in color buffer
		glBlendFunc(GL_DST_COLOR, GL_ZERO);
		glStencilFunc(GL_EQUAL, 0x1, 0x1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

		pushMatrix(MODEL);
		multMatrix(MODEL, mat);
		renderObjects();
		popMatrix(MODEL);

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);

		//render the geometry
		glUniform1i(shadowMode_uniformId, 0);
	}
	else {  //Camera behind the floor so render only the opaque objects
		lights();
		glUniform1i(shadowMode_uniformId, 0);
		table.render(shader, bumpmap);
		renderObjects();
	}



	//REARVIEW STUFF
	glClear(GL_STENCIL_BUFFER_BIT);
	glEnable(GL_STENCIL_TEST);
	if (camType == "rearview") 	rearviewMirror();
	glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	renderObjects();
	glDisable(GL_STENCIL_TEST);

	//rearview camera
	if (camType == "rearview"){
		//need to keep the ortho in front of everything and blend
		glClear(GL_STENCIL_BUFFER_BIT);
		glEnable(GL_STENCIL_TEST);
		rearviewMirror();
		pushMatrix(VIEW);
		pushMatrix(MODEL);
		loadIdentity(VIEW);
		loadIdentity(MODEL);
		glStencilFunc(GL_EQUAL, 0x1, 0x1);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);


		lookAt(x, y, z, lookAtRearview.x, lookAtRearview.y, lookAtRearview.z, xUp, yUp, zUp);

		//render obj or else everthing is missing
		skyBox.render(shader);

		scale(PROJECTION, -1, 1, 1);
		lights();
		renderObjects();
		table.render(shader, bumpmap);

		popMatrix(VIEW);
		popMatrix(MODEL);

		glDisable(GL_STENCIL_TEST);
	}
	else {
		scale(PROJECTION, -1, 1, 1);
		glClearStencil(0x0);
		glClear(GL_STENCIL_BUFFER_BIT);
	}

	scale(PROJECTION, -1, 1, 1);

	if (flareEffect && !spotlight_mode) {
		//float eyeLightPos[4];  //position of light in eye coordinates
		float lightPos[4] = { candles[3].light_position[0], 10, candles[3].light_position[2], 1.0f };//position of point light in World coordinates
		//glUniform1i(loc, 0);
		//multMatrixPoint(VIEW, lightPos, eyeLightPos);
		//glUniform4fv(lPos_uniformId, 1, eyeLightPos);
		My3DVector lightPosVec = { lightPos[0], lightPos[1], lightPos[2] };
		flare.renderSceneFlare(shader, lightPosVec);
	}

	glStencilFunc(GL_ALWAYS, 0x1, 0x1);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	//Render text (bitmap fonts) in screen coordinates. So use ortoghonal projection with viewport coordinates.
	glDisable(GL_DEPTH_TEST);
	//the glyph contains background colors and non-transparent for the actual character pixels. So we use the blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	//HUD
	pushMatrix(MODEL);
	loadIdentity(MODEL);
	pushMatrix(PROJECTION);
	loadIdentity(PROJECTION);
	pushMatrix(VIEW);
	loadIdentity(VIEW);
	ortho(m_viewport[0], m_viewport[0] + m_viewport[2] - 1, m_viewport[1], m_viewport[1] + m_viewport[3] - 1, -1, 1);
	if (!help) {
		RenderText(shaderText, txt, (WinX / 2) - 140, WinY / 2 + 10, 1.5f, 0.5f, 0.8f, 0.2f); //paused
		RenderText(shaderText, txt1, (WinX / 2) - 120, (WinY / 2) - 50, 1.0f, 0.5f, 0.8f, 0.2f); //unpause
		RenderText(shaderText, txt2, (WinX / 2) - 100, (WinY / 2) - 100, 1.0f, 0.5f, 0.8f, 0.2f); //restart
		RenderText(shaderText, txt3, (WinX / 2) - 80, (WinY / 2) - 160, 1.0f, 0.5f, 0.8f, 0.2f); //help
	}else {
		RenderText(shaderText, txt, (WinX / 2) - 110, WinY / 2 + 10, 1.5f, 0.5f, 0.8f, 0.2f); //h
		RenderText(shaderText, txt1, (WinX / 2) - 160, (WinY / 2) - 50, 1.0f, 0.5f, 0.8f, 0.2f); //
		RenderText(shaderText, txt2, (WinX / 2) - 220, (WinY / 2) - 100, 1.0f, 0.5f, 0.8f, 0.2f); //
		RenderText(shaderText, txt3, (WinX / 2) - 160, (WinY / 2) - 160, 1.0f, 0.5f, 0.8f, 0.2f); //
	}

	if (fireworks) {
		RenderText(shaderText, txtFinish1, (WinX / 2) - 160, WinY / 2 + 10, 1.5f, 0.5f, 0.8f, 0.2f); //paused
		RenderText(shaderText, txtFinish2, (WinX / 2) - 90, (WinY / 2) - 50, 1.0f, 0.5f, 0.8f, 0.2f); //unpause
	}
	
	RenderText(shaderText, "Points", 120, WinY - 30, 0.40f, 0.45f, 0.8f, 0.2f); //points
	RenderText(shaderText, std::to_string(points), 140, WinY - 60, 0.5f, 0.5f, 0.8f, 0.2f); //points
	RenderText(shaderText, "Lifes", 50, WinY - 30, 0.40f, 0.45f, 0.8f, 0.2f); //lifes
	RenderText(shaderText, std::to_string(numLife), 65, WinY-60, 0.5f, 0.5f, 0.8f, 0.2f); //num lifes
	RenderText(shaderText, "Sound "+ txtSound,WinX-140, WinY - 30, 0.40f, 0.45f, 0.8f, 0.2f); //sound

	popMatrix(PROJECTION);
	popMatrix(VIEW);
	popMatrix(MODEL);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);



	glBindTexture(GL_TEXTURE_2D, 0);
	glutSwapBuffers();
}


// ------------------------------------------------------------
//
// Mouse Events
//

void processMouseButtons(int button, int state, int xx, int yy)
{

	if (!pause) {

		// start tracking the mouse
		if (state == GLUT_DOWN) {
			startX = xx;
			startY = yy;
			if (button == GLUT_LEFT_BUTTON) tracking = 1;
			else if (button == GLUT_RIGHT_BUTTON) tracking = 2;
		}

		//stop tracking the mouse
		else if (state == GLUT_UP) {
			if (tracking == 1) {
				alpha -= (xx - startX);
				beta += (yy - startY);
			}
			else if (tracking == 2) {
				r += (yy - startY) * 0.01f;
				if (r < 0.1f) r = 0.1f;
			}
			tracking = 0;
		}

	}
}

// Track mouse motion while buttons are pressed

void processMouseMotion(int xx, int yy)
{
	if (!pause) {

		if (mouseControlActive == true) {
			deltaX = -xx + startX;
			deltaY = yy - startY;

			// left mouse button: move camera
			if (tracking == 1) {

				alphaAux = alpha + deltaX;
				betaAux = beta + deltaY;

				if (betaAux > 85.0f) betaAux = 85.0f;
				else if (betaAux < -85.0f) betaAux = -85.0f;
				rAux = r;

			}

			// right mouse button: zoom
			else if (tracking == 2) {
				alphaAux = alpha;
				betaAux = beta;
				rAux = r + (deltaY * 0.01f);
				if (rAux < 0.1f) rAux = 0.1f;
			}
		}
	}
}


void mouseWheel(int wheel, int direction, int x, int y) {
	if (!pause) {
		r += direction * 0.1f;
		if (r < 0.1f) r = 0.1f;
	}

}

// --------------------------------------------------------
//
// Shader Stuff
//


GLuint setupShaders() {

	// Shader for models
	shader.init();
	//shader.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/pointlight.vert");
	//shader.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/pointlight.frag");
	shader.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/texture_demo.vert");
	shader.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/texture_demo.frag");

	// set semantics for the shader variables
	glBindFragDataLocation(shader.getProgramIndex(), 0, "colorOut");
	glBindAttribLocation(shader.getProgramIndex(), VERTEX_COORD_ATTRIB, "position");
	glBindAttribLocation(shader.getProgramIndex(), NORMAL_ATTRIB, "normal");
	glBindAttribLocation(shader.getProgramIndex(), TEXTURE_COORD_ATTRIB, "texCoord");
	glBindAttribLocation(shader.getProgramIndex(), TANGENT_ATTRIB, "tangent");
	glBindAttribLocation(shader.getProgramIndex(), BITANGENT_ATTRIB, "bitangent");

	glLinkProgram(shader.getProgramIndex());

	ldDirection_uniformId = glGetUniformLocation(shader.getProgramIndex(), "ld_directions");

	pvm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_pvm");
	vm_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_viewModel");
	normal_uniformId = glGetUniformLocation(shader.getProgramIndex(), "m_normal");
	lPos_uniformId = glGetUniformLocation(shader.getProgramIndex(), "l_pos");
	specularMap_loc = glGetUniformLocation(shader.getProgramIndex(), "specularMap");
	diffMapCount_loc = glGetUniformLocation(shader.getProgramIndex(), "diffMapCount");
	reflect_perFragment_uniformId = glGetUniformLocation(shader.getProgramIndex(), "reflect_perFrag");
	shadowMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "shadowMode");


	// Textures Get UniformID
	texMode_uniformId = glGetUniformLocation(shader.getProgramIndex(), "texMode");
	tex_loc = glGetUniformLocation(shader.getProgramIndex(), "texmap");
	tex_loc1 = glGetUniformLocation(shader.getProgramIndex(), "texmap1");
	tex_loc2 = glGetUniformLocation(shader.getProgramIndex(), "texmap2");
	tex_loc3 = glGetUniformLocation(shader.getProgramIndex(), "texmap3");
	tex_loc4 = glGetUniformLocation(shader.getProgramIndex(), "texmap4");
	tex_loc5 = glGetUniformLocation(shader.getProgramIndex(), "texmap5");
	tex_loc6 = glGetUniformLocation(shader.getProgramIndex(), "texmap6");
	tex_loc7 = glGetUniformLocation(shader.getProgramIndex(), "texmap7");
	tex_loc8 = glGetUniformLocation(shader.getProgramIndex(), "texmap8");
	tex_loc9 = glGetUniformLocation(shader.getProgramIndex(), "texmap9");
	tex_cube_loc = glGetUniformLocation(shader.getProgramIndex(), "cubeMap");
	tex_grassNormal = glGetUniformLocation(shader.getProgramIndex(), "grassNormal");
	tex_candleNormal = glGetUniformLocation(shader.getProgramIndex(), "candleNormal");
	tex_cerealNormal = glGetUniformLocation(shader.getProgramIndex(), "cerealNormal");

	tex_loc25 = glGetUniformLocation(shader.getProgramIndex(), "texmap25");

	printf("InfoLog for Shader\n%s\n\n", shader.getAllInfoLogs().c_str());

	// Shader for bitmap Text
	shaderText.init();
	shaderText.loadShader(VSShaderLib::VERTEX_SHADER, "shaders/text.vert");
	shaderText.loadShader(VSShaderLib::FRAGMENT_SHADER, "shaders/text.frag");

	glLinkProgram(shaderText.getProgramIndex());
	printf("InfoLog for Text Rendering Shader\n%s\n\n", shaderText.getAllInfoLogs().c_str());

	return(shader.isProgramLinked() && shaderText.isProgramLinked());
}


// ------------------------------------------------------------
//
// Events from the Keyboard
//


void processKeysPressed(unsigned char key, int xx, int yy)
{

	switch (key) {
	case 'S':
	case 's':
		if (!help) {
			if (!pause) {
				mciSendString("pause \"music.mp3\"", NULL, 0, NULL);
				pause = true;
				txt = "PAUSED";
				txt1 = "'S' unpause";
				txt2 = "'R' restart";
				txt3 = "'H' help";
				txtSound = "OFF";

			}
			else {
				if (music == true) {
					mciSendString("resume \"music.mp3\"", NULL, 0, NULL);
					txtSound = "ON";

				}
				pause = false;
				txt = "";
				txt1 = "";
				txt2 = "";
				txt3 = "";

			}
		}
		break;

	}

	if (!pause) {
		switch (key) {
		case 27:
			glutLeaveMainLoop();
			break;
		case '0':
			//sound
			if (!music) music = true;
			else music = false;
			if (music == true) {
				mciSendString("play \"music.mp3\" repeat", NULL, 0, 0);
				txtSound = "ON";
			}
			else {
				mciSendString("pause \"music.mp3\" ", NULL, 0, NULL);
				txtSound = "OFF";
			}
			break;
		case 'W':
		case 'w':
			car.up();
			break;
		case 'E':
		case 'e':
			car.down();
			break;
		case 'O':
		case 'o':
			//check key to simulate it
			if (car.velocity != 0) {
				if (acceleration) car.accelerate(deltaTime);
				else car.deccelerate(deltaTime);

				car.turnLeft();
			}
			break;
		case 'P':
		case 'p':
			//check key to simulate it
			if (car.velocity != 0) {
				if (acceleration) car.accelerate(deltaTime);
				else car.deccelerate(deltaTime);

				car.turnRight();
			}
			break;

		case 'Q':
		case 'q':
			car.accelerate(deltaTime);
			breaks = false;
			acceleration = true;
			break;

		case 'A':
		case 'a':
			car.deccelerate(deltaTime);
			breaks = false;
			acceleration = false;
			break;

		case 'F':
		case 'f':
			fogActivation = !fogActivation;
			break;
			//directional light
		case 'N':
		case 'n':
			if (!directionalLight) directionalLight = true;
			else directionalLight = false;
			break;
			//Point Lights
		case 'C':
		case 'c':
			if (!pointLight) pointLight = true;
			else pointLight = false;
			break;
			//bumpmapping
		case 'B':
		case 'b':
			bumpmap = !bumpmap;
			break;

			//Spotlights
		case 'H':
		case 'h':
			if (!spotlights) spotlights = true;
			else spotlights = false;
			break;
			//FLARE EFFECT
		case 'R':
		case 'r':
			if (spotlights) flareEffect = false;
			else
				if (flareEffect) flareEffect = false;
				else {
					flareEffect = true;
				}
			break;
			//cams
		case '1':
			mouseControlActive = false;
			camType = "orthogonal";
			changeProjection();
			break;

		case '2':
			mouseControlActive = false;
			camType = "perspective";
			changeProjection();
			break;

		case '3':
			mouseControlActive = true;
			camType = "main";
			changeProjection();
			break;

		case '4':
			camType = "rearview";
			break;
		}
	}
	else {
		switch (key) {
			case 'R':
			case 'r':
				restartGame();
				break;
			case 'H':
			case 'h':
				if (!help) {
					help = true;
					txt = "HELP";
					txt1 = "'H' to go back";
					txt2 = "'0' to play/stop music";
					txt3 = "'W' | 'E' to float";
				}
				else {
					help = false;
					txt = "PAUSED";
					txt1 = "'S' unpause";
					txt2 = "'R' restart";
					txt3 = "'H' help";
				}
				break;
		}
	}

}

void processKeysReleased(unsigned char key, int xx, int yy) {
	switch (key) {
	case 'Q':
	case 'q':
	case 'A':
	case 'a':
		breaks = true;
		break;
	}

}

// ------------------------------------------------------------
//
// Main function
//


int main(int argc, char** argv) {
	
	//  GLUT initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE | GLUT_STENCIL);

	glutInitContextVersion(4, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE | GLUT_DEBUG);

	glutInitWindowPosition(100, 100);
	glutInitWindowSize(WinX, WinY);
	WindowHandle = glutCreateWindow(CAPTION);


	//Mouse and Keyboard Callbacks
	glutKeyboardFunc(processKeysPressed);
	glutKeyboardUpFunc(processKeysReleased);

	//  Callback Registration
	glutDisplayFunc(renderScene);
	glutReshapeFunc(changeSize);

	glutTimerFunc(0, timer, 0);
	//glutIdleFunc(renderScene);  // Use it for maximum performance
	glutTimerFunc(0, refresh, 0);    //use it to get 60 FPS whatever

	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glutMouseWheelFunc(mouseWheel);


	//	return from main loop
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	//	Init GLEW
	glewExperimental = GL_TRUE;
	glewInit();
	printf("Vendor: %s\n", glGetString(GL_VENDOR));
	printf("Renderer: %s\n", glGetString(GL_RENDERER));
	printf("Version: %s\n", glGetString(GL_VERSION));
	printf("GLSL: %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	if (!setupShaders()) return(1);

	init();

	//  GLUT main loop
	glutMainLoop();

	return(0);

}