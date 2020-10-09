struct StandardVertexIn{
    float2 Uv : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Normal  : NORMAL;
    float3 Position : POSITION;
    float4 Color    : COLOR;
};

cbuffer ObjectBuffer : register(b0){
    float4x4 world;
    float4x4 transInvWorld;
    
}

//the camera pass of will always be placed on register b1
cbuffer StandardCamera : register(b1){
    float4x4 view;
    float4x4 invView;
    float4x4 proj;
    float4x4 invProj;
    float4x4 viewProj;
    float4x4 invViewProj;

    float3 cameraPosition;
    float timeLine;
    
    float nearPlane;
    float farPlane;
}


#define LIGHT_TYPE_POINT 0
#define LIGHT_TYPE_DIRECTIONAL 1

#define Ambient float3(0.25,0.25,0.25)
#define Fresnel float3(0.02,0.02,0.02)
#define DiffuseAlbedo  float3(0.25,0.25,0.25)
#define Rougthness  float(.25f)
#define Shininess     float(.75f)

cbuffer StanderdLight : register(b2){
    int lightType;       //stands for the type of the light
    float3 lightVec;     //if the light's type is directional light light vector stands for the direction of the light,
                         //if the light type is point light then the light vector stands for the position of the light
    float lightAttuation;//if the only the point light will use this attribute
    float3 lightIntensity;//the light intensity rgb value 
};


struct VertexOut{
    float3 Normal : TEXCOORD0;
    float4 worldPos : TEXCOORD1;
    float4 Position : SV_POSITION;
};

//we only use normal and position information in this shader
VertexOut VS(StandardVertexIn input){
    VertexOut output;
    output.Normal = mul((float3x3)transInvWorld ,input.Normal);
    //output.Normal = input.Normal;
    output.worldPos = mul(world,float4(input.Position,1.f));
    //output.Position = float4(world,1.);
    output.Position = mul(viewProj,output.worldPos);

    return output;
}

float3 SchlickFresnel(float3 R0,float3 normal,float3 LightDir){

    float f0 = 1. - saturate(dot(normal,LightDir));

    float3 reflection = R0 + (1. - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflection;
}


float3 BinlingPhone(float3 LightIntensity,float3 LightDir,float3 normal,float3 viewDir){
    float m = Shininess * 256.f;
    float3 halfVec = normalize(normal + viewDir);

    float roughFactor = (m + 8.f) * .125f * pow(saturate(dot(halfVec,normal)),m);
    float3 specular = SchlickFresnel(Fresnel,halfVec,LightDir);

    specular = specular * roughFactor;
    //specular = specular / (specular + 1.f);

    return (specular + DiffuseAlbedo) * LightIntensity;
}



float3 DirectionalLight(float3 LightDir,float3 normal,float3 ViewDir,float3 LightIntensity){
    LightDir = - LightDir;
    float ndotl = saturate(dot(LightDir,normal));

    float3 intensity = LightIntensity * ndotl;
    return BinlingPhone(intensity,LightDir,normal,ViewDir);
}

float3 PointLight(float3 LightPos,float3 normal,float3 ViewDir,float3 WorldPos,float3 LightIntensity){
    float3 LightDir = normalize(LightPos - WorldPos);
    float ndotl = saturate(dot(LightDir,normal));

    float3 intensity = LightIntensity * ndotl;
    return BinlingPhone(intensity,LightDir,normal,ViewDir);

}




float4 PS(VertexOut input) : SV_TARGET {
    float3 target = Ambient;
    input.Normal = normalize(input.Normal);
    //we don't want to add negative value to the result
    float3 ViewDir = normalize(cameraPosition - input.worldPos);

    if(lightType == LIGHT_TYPE_DIRECTIONAL){
        target += DirectionalLight(lightVec,input.Normal,ViewDir,lightIntensity);
    }else if(lightType == LIGHT_TYPE_POINT){
        target += PointLight(lightVec,input.Normal,ViewDir,input.worldPos,lightIntensity);//(float3)lightIntensity * clamp(dot(lightDir,input.Normal),0.f,1.f);
    }

    return float4(target,1.);
}

