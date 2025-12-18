struct VertexInput
{
    float3 position : POSITION;
    float2 uv: TEXCOORD0;
};

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

struct Transforms
{
    float4x4 MVP;
};

ConstantBuffer<Transforms> transforms;

Sampler2D tex;

[shader("vertex")]
VertexOutput vertexMain(VertexInput input)
{
    VertexOutput output;
    output.position = mul(transforms.MVP, float4(input.position, 1.0));
    output.uv = input.uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float4 color = tex.Sample(input.uv);
    return color;
}
