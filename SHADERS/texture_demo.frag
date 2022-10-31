#version 330

uniform sampler2D texmap;
uniform sampler2D texmap1;
uniform sampler2D texmap2;
uniform sampler2D texmap3;
uniform sampler2D texmap4;
uniform sampler2D texmap5;
uniform sampler2D texmap6;
uniform sampler2D texmap7;
uniform sampler2D texmap8;
uniform sampler2D texmap9;
uniform sampler2D texmap25;


uniform mat4 m_View;

uniform samplerCube cubeMap;
uniform sampler2D normalMap;
uniform sampler2D sphereMap;
uniform sampler2D candleNormal;
uniform sampler2D grassNormal;
uniform sampler2D cerealNormal;

uniform bool shadowMode;

uniform int texMode;
uniform int fogActivation;
uniform int directionalLight;
uniform int reflect_perFrag;

//ASSIMP OBJ STUFF
uniform	sampler2D texUnitDiff;
uniform	sampler2D texUnitDiff1;
uniform	sampler2D texUnitSpec;
uniform	sampler2D texUnitNormalMap;

uniform bool specularMap;
uniform uint diffMapCount;


uniform bool spotlight_mode;
uniform bool pointlight_mode;
uniform bool directional_mode;

uniform vec4 coneDir;
uniform float spotCosCutOff;


out vec4 FragColor;
out vec4 colorOut;

struct Materials {
    vec3 direction;
	vec4 diffuse;
	vec4 ambient;
	vec4 specular;
	vec4 emissive;
	float shininess;
	int texCount;
};
uniform Materials mat;


in Data {

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


} DataIn;


vec4 applyFog( in vec3 rgb, in float distance) {

	float fogAmount = exp( -distance*0.05 );
	clamp(fogAmount, 0, 1.0);
	vec3 fogColor = vec3(0.5, 0.6, 0.7);
	vec3 final_color = mix(fogColor, rgb, fogAmount);
	return vec4(final_color, 1.0f);
}

void main() {
	vec4 texel, texel1, texel2, texel3, texel4, texel5, texel6, texel7, texel8, texel9, texel10, texel25, cube_texel; 
	vec4 diff, auxSpec;
	bool assimp = false;

	vec4 spec = vec4(0.0);
	float intensity = 0.0f;
	float intSpec = 0.0f;

	float att = 0.0;
	float spotExp = 80.0;
	vec3 n;

	//BELOW
	//vec3 n = normalize(DataIn.normal);

	vec3 l = normalize(DataIn.lightDir);
	vec3 e = normalize(DataIn.eye); 
	vec3 eBump = normalize(DataIn.eyeBump); 
	vec3 sd = normalize(vec3(-coneDir));

	int numberOfLights = 0;
	vec4 tmpColorOut = vec4(0.0);
	vec4 finalColorOut = vec4(0.0);

	if(texMode == 12)  // lookup normal from normal map, move from [0,1] to [-1, 1] range, normalize
		n = normalize(2.0 * texture(candleNormal, DataIn.tex_coord).rgb - 1.0); //candle normal
	else if(texMode == 13)
		n = normalize(2.0 * texture(grassNormal, DataIn.tex_coord).rgb - 1.0); //orange normal
	else if(texMode == 14)
		n = normalize(2.0 * texture(cerealNormal, DataIn.tex_coord).rgb - 1.0); //cereal normal
	else
		n = normalize(DataIn.normal);

	if (texMode != 5){
		texel = texture(texmap, DataIn.tex_coord);   //texel from grass        		
		texel1 = texture(texmap1, DataIn.tex_coord); //texel from road 
		texel2 = texture(texmap2, DataIn.tex_coord); //texel from cheerio 
		texel3 = texture(texmap3, DataIn.tex_coord); //texel from orange
		texel4 = texture(texmap4, DataIn.tex_coord); //texel from tree
		texel6 = texture(texmap6, DataIn.tex_coord);//texel from butter
		texel7 = texture(texmap7, DataIn.tex_coord);//texel from candle
		texel9 = texture(texmap9, DataIn.tex_coord);//texel from particles
	}

	// ASSIMP OBJ STUFF
	diff = mat.diffuse;
	auxSpec = mat.specular;
	
	// ASSIMP OBJ STUFF
	if (mat.texCount != 0 && texMode == 100) {
		assimp =true;
		if(diffMapCount == 0.0) diff = mat.diffuse;
		else if(diffMapCount == 1.0) diff = mat.diffuse * texture(texUnitDiff, DataIn.tex_coord);
		else diff = mat.diffuse * texture(texUnitDiff, DataIn.tex_coord) * texture(texUnitDiff1, DataIn.tex_coord);

		if(specularMap) auxSpec = mat.specular * texture(texUnitSpec, DataIn.tex_coord);
		else auxSpec = mat.specular;
	}


	if(spotlight_mode == true)  {  //Scene iluminated by a spotlight
		for(int i = 0; i < 2; i++){

			numberOfLights += 1;

			vec3 l = normalize(DataIn.sl_dir[i]);

			float spotCos = dot(sd, l);

			if(spotCos > spotCosCutOff)  {	//inside cone
				//att = pow(spotCos, spotExp);
				intensity = max(dot(n,l), 0.0);
				if (intensity > 0.0) {
					vec3 h = normalize(l + e);
					intSpec = max(dot(h,n), 0.0);
					spec = auxSpec * pow(intSpec, mat.shininess);
				}
			}

			if(texMode == 0) tmpColorOut += max(intensity*mix(texel, texel1, texel1.a) + spec, 0.07* mix(texel, texel1, texel1.a));
			else if(texMode == 2 || texMode == 14) tmpColorOut += vec4((max(intensity*texel2 + spec, 0.2*texel2)).rgb, 1.0);
			else if(texMode == 3 || texMode == 13) tmpColorOut += vec4((max(intensity*texel3 + spec, 0.2*texel3)).rgb, 1.0);
			else if(texMode == 4) tmpColorOut += max(intensity*texel4 + spec, 0.07*texel4); 
			else if(texMode == 6) tmpColorOut += max(intensity*texel6 + spec, 0.07*texel6);
			else if(texMode == 7 || texMode == 12) tmpColorOut += vec4((max(intensity*texel7 + spec, 0.2*texel7)).rgb, 1.0);
			else if(texMode == 100) tmpColorOut += max(intensity * diff, diff * 0.15) + spec; //assimp
			else tmpColorOut += max(intensity * mat.diffuse, mat.diffuse * 0.15) + spec;
		}
		if(numberOfLights > 0 && texMode == 0 ) finalColorOut += tmpColorOut;
		else if (numberOfLights > 0) finalColorOut +=  (tmpColorOut / numberOfLights);

	}


	numberOfLights = 0;
	tmpColorOut = vec4(0.0);

	if(pointlight_mode == true){//Scene iluminated by a pointlight
		for(int i = 0; i < 6; i++){

			numberOfLights += 1;

			vec3 l = normalize(DataIn.pl_dir[i]);
			intensity = max(dot(n,l), 0.0);
				if (intensity > 0.0) {
					vec3 h = normalize(l + e);
					intSpec = max(dot(h,n), 0.0);
					spec = auxSpec * pow(intSpec, mat.shininess);
				}
			if(texMode == 0) tmpColorOut += max(intensity*mix(texel, texel1, texel1.a) + spec, 0.07* mix(texel, texel1, texel1.a));
			else if(texMode == 2 || texMode == 14) tmpColorOut += vec4((max(intensity*texel2 + spec, 0.2*texel2)).rgb, 1.0);
			else if(texMode == 3 || texMode == 13) tmpColorOut += vec4((max(intensity*texel3 + spec, 0.2*texel3)).rgb, 1.0);
			else if(texMode == 4) tmpColorOut += max(intensity*texel4 + spec, 0.07*texel4); 
			else if(texMode == 6) tmpColorOut += max(intensity*texel6 + spec, 0.07*texel6);
			else if(texMode == 7 || texMode == 12) tmpColorOut += vec4((max(intensity*texel7 + spec, 0.2*texel7)).rgb, 1.0);
			else if(texMode == 9)  {	// modulated texture for particle
				if((texel9.a == 0.0)  || (mat.diffuse.a == 0.0) ) discard;
				else tmpColorOut += mat.diffuse * texel9;
			}
			else if(texMode == 100) tmpColorOut += max(intensity * diff, diff * 0.15) + spec; //assimp
			else tmpColorOut += max(intensity * mat.diffuse, mat.diffuse * 0.15) + spec;

		}

		if(numberOfLights > 0 && texMode == 0 ) finalColorOut += tmpColorOut;
		else if (numberOfLights > 0) finalColorOut +=  (tmpColorOut / numberOfLights);

	} 

	numberOfLights = 0;
	tmpColorOut = vec4(0.0);

	if(directional_mode == true) { //Scene iluminated by a directionalLight
		vec3 l = normalize(DataIn.dl_dir);

		numberOfLights += 1;

		intensity = max(dot(n,l), 0.0);
			if (intensity > 0.0) {
				vec3 h = normalize(l + e);
				intSpec = max(dot(h,n), 0.0);
				spec = auxSpec * pow(intSpec, mat.shininess);
			}

			if(texMode == 0) tmpColorOut += max(intensity*mix(texel, texel1, texel1.a) + spec, 0.07* mix(texel, texel1, texel1.a));
			else if(texMode == 2 || texMode == 14) tmpColorOut += vec4((max(intensity*texel2 + spec, 0.2*texel2)).rgb, 1.0);
			else if(texMode == 3 || texMode == 13) tmpColorOut += vec4((max(intensity*texel3 + spec, 0.2*texel3)).rgb, 1.0);
			else if(texMode == 4) tmpColorOut += max(intensity*texel4 + spec, 0.07*texel4);
			else if(texMode == 6) tmpColorOut += max(intensity*texel6 + spec, 0.07*texel6);
			else if(texMode == 7 || texMode == 12) tmpColorOut += vec4((max(intensity*texel7 + spec, 0.2*texel7)).rgb, 1.0);
			else if(texMode == 9)  {	// modulated texture for particle
				if((texel9.a == 0.0)  || (mat.diffuse.a == 0.0) ) discard;
				else tmpColorOut += mat.diffuse * texel9;
			}
			else if(texMode == 100) tmpColorOut += max(intensity * diff, diff * 0.15) + spec; //assimp
			else tmpColorOut += max(intensity * mat.diffuse, mat.diffuse * 0.15) + spec;
			
			if (numberOfLights > 0) finalColorOut +=  (tmpColorOut / numberOfLights);

	}
     
    colorOut = finalColorOut;
      
	if (texMode == 4) {
        //TRANSPARENCY
		if(texel4.a < 0.5)
			discard;
    } else if(texMode==25){ //lens flare
			if((texel.a == 0.0)  || (mat.diffuse.a == 0.0) ) discard;
			else colorOut += diff * texel;
	} else if (texMode==10){ //Skybox
		colorOut = texture(cubeMap, DataIn.skyboxTexCoord);
	} else if (texMode==11){
		//if(reflect_perFrag == 1) {  //reflected vector calculated here
			//vec3 reflected1 = vec3 (transpose(m_View) * vec4 (vec3(reflect(-e, n)), 0.0)); //reflection vector in world coord
			//reflected1.x= -reflected1.x;   
			//cube_texel = texture(cubeMap, reflected1);
		//}
		//else
			cube_texel = texture(cubeMap, DataIn.reflected); //use interpolated reflected vector calculated in vertex shader

		colorOut = vec4(cube_texel.rgb, 1.0);

	}

 
	if (fogActivation == 1) {
		float dist = length(DataIn.pos);
		colorOut = applyFog(vec3(colorOut), dist);
	}

	if (shadowMode) {
		colorOut = vec4(0.8, 0.8, 0.8, 1.0);
		//		colorOut = vec4(0.5, 0.5, 0.5, 1.0); !!!!!!!!!! testar

	}

}