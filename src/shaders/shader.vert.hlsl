struct VSOutput {
    float4 Position : SV_Position;
    float3 Color : COLOR0;
};

static const float3 positions[3] = {
    float3( 0.0, -0.5, 0.0),
    float3(-0.5,  0.5, 0.0),
    float3( 0.5,  0.5, 0.0)
};

static const float3 colors[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0)
};

VSOutput main(uint VertexIndex : SV_VertexID) {
    VSOutput output;
    output.Position = float4(positions[VertexIndex], 1.0);
    output.Color = colors[VertexIndex];
    return output;
  }
