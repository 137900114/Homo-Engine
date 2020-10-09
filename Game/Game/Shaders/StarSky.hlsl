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


static const float3 background = float3(0.,0.,0.);
static const float3 horizonLight = float3(.35,.35,.5);
static const float exponent = 32;

static const float starSpeed = 1.5;

TextureCube skyBox : register(t0);
SamplerState skySampler : register(s0);

VertexOut VS(StandardVertexIn input){
    
    VertexOut output;
    
    float3 worldPos = input.Position + cameraPosition;
    //output.Position = float4(world,1.)
    output.Position = mul(viewProj,float4(worldPos,1.f)).xyzz;

    output.relaPos = -input.Position;

    return output;
}

float hash2d(float2 seed){
    float p = dot(seed,float2(19.11,51.423));
    return frac(sin(p) * 51414.1234);
}


float star(float2 st){

    st.x += timeLine * starSpeed;

    float2 i = floor(st);
    float2 f = frac(st);

    f = smoothstep(0.,1.,f);

    float a00 = hash2d(i);
    float a01 = hash2d(i + float2(0.,1.));
    float a10 = hash2d(i + float2(1.,0.));
    float a11 = hash2d(i + float2(1.,1.));
    
    return lerp(
        lerp(a00,a01,f.x),
        lerp(a10,a11,f.x),
        f.y
    );
}

float4 PS(VertexOut input) : SV_TARGET{

    float3 target = float3(0.,0.,0.);
    float3 worldDir = normalize(input.relaPos);

    float y = abs(worldDir.y);

    float p0 = pow(max(1. - y *.5 ,0.),exponent);

    target += lerp(background,horizonLight,p0);


    float2 st = worldDir.xy * 64;
    float starNoise = star(st);

    target = starNoise;

    return float4(target,1.);
}