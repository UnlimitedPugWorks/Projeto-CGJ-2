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
#include "VertexAttrDef.h"
#include "geometry.h"

#include "VSShaderlib.h"
#include "MyLights.h"



MyLights::MyLights() {
}

/*MyLights::MyLights(My3DVector lightPos) {

}

My3DVector MyLights::updateLightsPos(My3DVector vertice)
{
	return My3DVector();
}*/



