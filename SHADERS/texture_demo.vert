#version 330

uniform mat4 m_pvm;
uniform mat4 m_viewModel;
uniform mat3 m_normal;
uniform mat4 m_Model;   //por causa do cubo para a skybox
uniform mat4 m_View;


uniform int texMode;
uniform int reflect_perFrag;

//Temporário
uniform vec4 l_pos;

// 6 Point Lights
uniform vec4 pl_pos[6];

// 1 Directional Light
uniform vec4 dl_dir;

// 2 Spot Lights
uniform vec4 sl_pos[2];
uniform vec4 sl_l_dir[2];

in vec4 position;
in vec4 normal;    //por causa do gerador de geometria
in vec4 texCoord;
in vec4 tangent;


out Data {
	vec4 pos;
	vec3 normal;
	vec3 eye;
	vec3 eyeBump;
	vec3 lightDir;
	vec2 tex_coord;
	//Directions for Point Lights
	vec3 pl_dir[6];
	// Direction for Directional Lights
	vec3 dl_dir;
	//Directions for Spot Lights
	vec3 sl_dir[2];
	vec3 skyboxTexCoord;
	vec3 reflected;

} DataOut;


void main () {
	vec3 lightDir, eyeDir;
	vec3 n, t, b;
	vec3 aux;


	DataOut.skyboxTexCoord = vec3(m_Model * position);	//Transformação de modelação do cubo unitário 
	DataOut.skyboxTexCoord.x = - DataOut.skyboxTexCoord.x; //Texturas mapeadas no interior logo negar a coordenada x

	vec4 pos = m_viewModel * position;
	DataOut.pos = pos;

	DataOut.normal = normalize(m_normal * normal.xyz);
	n = normalize(m_normal * normal.xyz);
	// eye and light vectors in eye space
	eyeDir =  vec3(-pos);
	lightDir = vec3(l_pos - pos);


	if(texMode == 7 || texMode == 3 || texMode == 2)  {  //convert eye and light vectors to tangent space

			//Calculate components of TBN basis in eye space
			t = normalize(m_normal * tangent.xyz);  
			b = tangent.w * cross(n,t);

			aux.x = dot(lightDir, t);
			aux.y = dot(lightDir, b);
			aux.z = dot(lightDir, n);
			lightDir = normalize(aux);

			aux.x = dot(eyeDir, t);
			aux.y = dot(eyeDir, b);
			aux.z = dot(eyeDir, n);
			eyeDir = normalize(aux);
	}

	
	//Point Lights
	for(int i = 0; i < 6 ; i++){
		DataOut.pl_dir[i] = vec3(pl_pos[i] - pos);
	}

	//Directional Lights
	DataOut.dl_dir = vec3(-dl_dir);
	
	//Spot Lights
	for(int i = 0; i < 2; i++){
		DataOut.sl_dir[i] = vec3(sl_pos[i] - pos);
	}

	DataOut.eye = eyeDir;
	DataOut.tex_coord = texCoord.st;
	DataOut.lightDir = lightDir;
	DataOut.eyeBump = eyeDir;

	if((texMode == 11) && (reflect_perFrag == 0)) {  //calculate here the reflected vector
			DataOut.reflected = vec3 (transpose(m_View) * vec4 (vec3(reflect(-DataOut.eye, DataOut.normal)), 0.0)); //reflection vector in world coord
			DataOut.reflected.x= -DataOut.reflected.x; // as texturas foram mapeadas no interior da skybox 
	}

	gl_Position = m_pvm * position;	
}