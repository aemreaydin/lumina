struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD0;
};

Sampler2D sceneTexture;

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    float2 uv = float2((vertexID << 1) & 2, vertexID & 2);
    VertexOutput output;
    output.position = float4(uv * 2.0 - 1.0, 0.0, 1.0);
    #ifdef VULKAN
    output.uv = float2(uv.x, 1.0 - uv.y);  // flip Y for correct orientation
    #else
    output.uv = uv;
    #endif
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return sceneTexture.Sample(input.uv);
}
