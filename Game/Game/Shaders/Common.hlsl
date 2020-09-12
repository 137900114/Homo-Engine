
struct StandardVertexIn{
    float2 Uv : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Normal  : NORMAL;
    float3 Position : POSITION;
    float4 Color    : COLOR;
};

struct ObjectBuffer : register(b0){
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

struct StanderdLight : register(b2){
    int lightType;       //stands for the type of the light
    float3 lightVec;     //if the light's type is directional light light vector stands for the direction of the light,
                         //if the light type is point light then the light vector stands for the position of the light
    float lightAttuation;//if the only the point light will use this attribute
    float3 lightIntensity;//the light intensity rgb value 
};

