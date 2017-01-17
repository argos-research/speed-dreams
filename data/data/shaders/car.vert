varying vec3 normal;
varying vec3 pos;

void main()
{
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = gl_MultiTexCoord1;
	normal = normalize(gl_NormalMatrix * gl_Normal);
	pos = vec3(gl_ModelViewMatrix*gl_Vertex);

	gl_Position = ftransform();
}

