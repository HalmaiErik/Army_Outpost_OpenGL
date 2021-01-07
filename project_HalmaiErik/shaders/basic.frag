#version 410 core

in vec3 fNormal;
in vec2 fTexCoords;
in vec4 fPosEye;
in vec4 fPosLightSpace;
in vec4 fPos;

out vec4 fColor;

//matrices
uniform mat3 normalMatrix;
//lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform mat3 lightDirMatrix;
uniform vec3 pointLightPos;
// textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

//components
vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 64.0f;

//point light
float constant = 1.0f;
float linear = 0.09f;
float quadratic = 0.032f;

vec3 viewDirN;

void computeLightComponents()
{       
    vec3 cameraPosEye = vec3(0.0f);//in eye coordinates, the viewer is situated at the origin
    
    //transform normal
    vec3 normalEye = normalize(normalMatrix * fNormal);  
    
    //compute light direction
    vec3 lightDirN = normalize(lightDirMatrix * lightDir);  

    //compute view direction 
    viewDirN = normalize(cameraPosEye - fPosEye.xyz);
    
    //compute half vector
    vec3 halfVector = normalize(lightDirN + viewDirN);
        
    //compute ambient light
    ambient = ambientStrength * lightColor;
    
    //compute diffuse light
    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;
    
    //compute specular light
    float specCoeff = pow(max(dot(halfVector, normalEye), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;
}

vec3 computePointLight() {
	vec3 posLightColor = vec3(1.0f, 1.0f, 0.0f);

	vec3 lightDirN = normalize(pointLightPos - fPos.xyz);
	float diff = max(dot(fNormal, lightDirN), 0.0f);
	vec3 reflectDir = reflect(-lightDirN, fNormal);
	float spec = pow(max(dot(viewDirN, reflectDir), 0.0f), shininess);

	float distance = length(pointLightPos - fPos.xyz);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * (distance * distance)); 

    vec3 ambientPoint = (ambient * vec3(texture(diffuseTexture, fTexCoords).rgb) * posLightColor) * attenuation;
    vec3 diffusePoint = (diffuse * diff * vec3(texture(diffuseTexture, fTexCoords).rgb) * posLightColor) * attenuation;
    vec3 specularPoint = (specular * spec * vec3(texture(specularTexture, fTexCoords).rgb) * posLightColor) * attenuation;

    return (ambientPoint + diffusePoint + specularPoint);
}

float computeShadow()
{   
    // perform perspective divide
    vec3 normalizedCoords = fPosLightSpace.xyz / fPosLightSpace.w;
    if(normalizedCoords.z > 1.0f)
        return 0.0f;

    // Transform to [0,1] range
    normalizedCoords = normalizedCoords * 0.5f + 0.5f;
    // Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;    
    // Get depth of current fragment from light's perspective
    float currentDepth = normalizedCoords.z;

    // Check whether current frag pos is in shadow
    
    /*
    float bias = 0.005f;
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    */
    
    float bias = max(0.018f * (1.0f - dot(fNormal, lightDir)), 0.005f);
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    

    return shadow;  
}

float computeFog()
{
    float fogDensity = 0.02f; 
    float fragmentDistance = length(fPosEye.z / fPosEye.w); 
    float fogFactor = exp(-pow(fragmentDistance * fogDensity, 2)); 

    return clamp(fogFactor, 0.0f, 1.0f); 
}

void main() 
{
    computeLightComponents();
    computePointLight();

    float shadow = computeShadow();

    // modulate with diffuse map
    ambient *= vec3(texture(diffuseTexture, fTexCoords));
    diffuse *= vec3(texture(diffuseTexture, fTexCoords));
    // modulate with specular map
    specular *= vec3(texture(specularTexture, fTexCoords));

    //modulate with shadow
    vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular, 1.0f);

    color += computePointLight();

    vec4 fogColor = vec4(0.5f, 0.5f, 0.5f, 1.0f);
    float fogFactor = computeFog();
    
    fColor = fogColor * (1 - fogFactor) + vec4(color, 1.0f) * fogFactor;
}
