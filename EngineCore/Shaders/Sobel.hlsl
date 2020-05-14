Texture2D resource : register(t0);
RWTexture2D<float4> target : register(u0);

float CalLumination(float3 color){
    return dot(color,float3(0.299f,0.587f,0.114f));
}

[numthreads(16,16,1)]
void SobelC(int2 tid : SV_DispatchThreadID){
    float4 c[3][3];

    for(int i = 0;i != 3;i++){
        for(int j = 0;j != 3;j++){
            int2 id = tid.xy + int2(i - 1,j - 1);
            c[i][j] = resource[id];
        }
    }

    float4 Gx = -1.f * c[0][0] - 2.f* c[1][0] - 1.f * c[2][0] +
        1.f * c[0][2] + 2.f * c[1][2] + 1.f * c[2][2];
    float4 Gy = -1.f * c[2][0] -2.f * c[2][1] - 1.f * c[2][2] +
        1.f * c[0][0] + 2.f * c[0][1] + 1.f * c[0][2];
    
    float4 dv = sqrt(Gx * Gx + Gy * Gy);

    dv = 1.f - saturate(CalLumination(dv.rgb));
    target[tid.xy] = dv * resource[tid.xy];
}