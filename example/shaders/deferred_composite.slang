struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct CompositeParams
{
    int displayMode;
    float nearPlane;
    float farPlane;
    float padding;
};

struct CompositeTextures
{
    Sampler2D litScene;
    Sampler2D albedo;
    Sampler2D normals;
    Sampler2D depth;
};

ParameterBlock<CompositeParams> params;
ParameterBlock<CompositeTextures> textures;

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

float3 ACESFilmic(float3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float linearizeDepth(float depth, float near, float far)
{
    return near * far / (far - depth * (far - near));
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float2 uv = input.uv;

    switch (params.displayMode)
    {
        case 0:
        {
            float3 hdr = textures.litScene.Sample(uv).rgb;
            float3 mapped = ACESFilmic(hdr);
            float3 gamma = pow(mapped, float3(1.0 / 2.2));
            return float4(gamma, 1.0);
        }
        case 1:
        {
            float3 hdr = textures.litScene.Sample(uv).rgb;
            return float4(saturate(hdr), 1.0);
        }
        case 2:
        {
            return float4(textures.albedo.Sample(uv).rgb, 1.0);
        }
        case 3:
        {
            return float4(textures.normals.Sample(uv).rgb * 0.5 + 0.5, 1.0);
        }
        case 4:
        {
            float d = linearizeDepth(textures.depth.Sample(uv).r, params.nearPlane, params.farPlane);
            return float4(float3(d / params.farPlane), 1.0);
        }
        case 5:
        {
            return float4(textures.albedo.Sample(uv).aaa, 1.0);
        }
        case 6:
        {
            return float4(textures.normals.Sample(uv).aaa, 1.0);
        }
        default:
        {
            return float4(textures.litScene.Sample(uv).rgb, 1.0);
        }
    }
}
