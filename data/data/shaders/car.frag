varying vec3 normal;
varying vec3 pos;

uniform sampler2D diffusemap;

uniform vec4 specularColor;
uniform float smoothness;
uniform vec3 lightvector;
uniform vec4 lightpower;
uniform vec4 ambientColor;

uniform int reflectionMappingMethod;
uniform samplerCube reflectionMapCube;
uniform sampler2D reflectionMap2DSampler;
uniform vec3 reflectionMapStaticOffsetCoords;

vec4 reflectionMappingContribution(vec3 reflectDirection)
{
     vec3 envcolor = vec3(textureCube(reflectionMapCube, reflectDirection));
     return vec4(envcolor, 1.0);
}

void main()
{
    //vec3 delta_normal = texture2D(normalmap, vec2(gl_TexCoord[0]));
    //vec3 norma = normalize(normal+delta_normal);
    vec3 norma = normal;
    vec3 v = normalize(-pos);
    vec3 l = normalize(lightvector);
    vec3 h = normalize(l+v);
    vec3 reflectDirection = reflect(v, normal);

    float cosTh = max(0.0, dot(norma, h));
    float cosTi = max(0.0, dot(norma, l));

    vec4 diffuse =  texture2D(diffusemap, vec2(gl_TexCoord[0]));
    vec4 fcolor  =  ambientColor*diffuse+(diffuse+ smoothness*specularColor*pow(cosTh,smoothness))*lightpower*cosTi;
    vec4 final_color;

    if(reflectionMappingMethod!=0)
	{
        final_color = mix(reflectionMappingContribution(reflectDirection), fcolor , 0.70);
    }else
	{
        final_color = fcolor;
    }
    gl_FragColor  = final_color;
}
