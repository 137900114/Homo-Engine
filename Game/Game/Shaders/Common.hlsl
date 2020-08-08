
struct StandardVertex{
    float2 Uv : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Normal  : NORMAL;
    float3 Position : POSITION;
    float4 Color    : COLOR;
};

#define USE_STANDARD_CAMERA(reg,spc) \
cbuffer StandardCamera : register(b##reg,space##spc){\
    float4x4 view;\
    float4x4 invView;\
    float4x4 proj;\
    float4x4 invProj;\
    float4x4 viewProj;\
    float4x4 invViewProj;\
    \
    float3 cameraPosition;\
    float timeLine;\
    \
    float nearPlane;\
    float farPlane;\
}
