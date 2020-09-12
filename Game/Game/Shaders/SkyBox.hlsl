//the camera pass of will always be placed on register b1
cbuffer StandardCamera : register(b0){
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

struct StandardVertexIn{
    float2 Uv : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Normal  : NORMAL;
    float3 Position : POSITION;
    float4 Color    : COLOR;
};

struct VertexOut{
    float3 relaPos : POSITION;
    float4 Position : SV_Position;
};

TextureCube skyBox : register(t0);
SamplerState skySampler : register(s0);


VertexOut VS(StandardVertexIn input){
    VertexOut output;
    
    float3 worldPos = input.Position + cameraPosition;
    //output.Position = float4(world,1.)
    output.Position = mul(viewProj,float4(worldPos,1.f)).xyzz;
    output.Position.z -= 1e-4f;

    output.relaPos = -input.Position;

    return output;
}

float4 PS(VertexOut input) : SV_TARGET {
    float3 target = skyBox.Sample(skySampler,input.relaPos);

    return float4(target,1.);
}