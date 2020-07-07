#ifdef STD_VERTEX_IN 

struct VertexIn{
    float2 Uv : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Normal  : NORMAL;
    float3 Position : POSITION;
    float4 Color    : COLOR;
};

#endif


#ifdef STD_LIGHT_PASS

#ifndef MAX_LIGHT_NUM 
#define MAX_LIGHT_NUM 1
#endif

struct Light{
    float3 direction;
    float4 intensity;
};

cbuffer LightPass : register(b2){
    Light lights[MAX_LIGHT_NUM];
};

#endif

#ifdef STD_CAMERA_PASS

cbuffer CameraPass : register(b1){
    float4x4 proj;
    float4x4 invProj;

    float timeLine;

};

#endif
