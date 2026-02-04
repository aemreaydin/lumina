struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct DebugParams
{
    int displayMode;
    float nearPlane;
    float farPlane;
    float padding;
};

ParameterBlock<DebugParams> params;
Sampler2D debugTexture;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    VertexOutput output;
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    #ifdef VULKAN
    output.uv = float2(uv.x, 1.0 - uv.y);
    #else
    output.uv = uv;
    #endif
    return output;
}

float linearizeDepth(float depth, float near, float far)
{
    return near * far / (far - depth * (far - near));
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 texel = debugTexture.Sample(input.uv);
    switch (params.displayMode)
    {
        case 0: return float4(texel.rgb, 1.0);              // Albedo
        case 1: return float4(texel.rgb * 0.5 + 0.5, 1.0);  // Normals
        case 2:                                               // Depth
        {
            float d = linearizeDepth(texel.r, params.nearPlane, params.farPlane);
            return float4(float3(d / params.farPlane), 1.0);
        }
        case 3: return float4(texel.aaa, 1.0);              // Metallic
        case 4: return float4(texel.aaa, 1.0);              // Roughness
        default: return float4(texel.rgb, 1.0);
    }
}
