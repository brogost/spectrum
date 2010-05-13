// Shader that just outputs a full screen quad

// 0-1
// 2-3

Texture2D fs_texture;
extern sampler linear_sampler;

struct psInput
{
	float4	pos : SV_Position;
	float2 	tex : TexCoord;
};

psInput vsMain(in uint v : SV_VertexID)
{
	psInput o = (psInput)0;
	if (v == 0) {
		o.pos = float4(-1, +1, 0, 1);
		o.tex = float2(0,0);
	} else if (v == 1) {
		o.pos = float4(+1, +1, 0, 1);
		o.tex = float2(1,0);
	} else if (v == 2) {
		o.pos = float4(-1, -1, 0, 1);
		o.tex = float2(0,1);
	} else if (v == 3) {
		o.pos = float4(+1, -1, 0, 1);
		o.tex = float2(1,1);
	}
	return o;
}

float4 psMain(in psInput v) : SV_Target
{
	return fs_texture.Sample(linear_sampler, v.tex);
}
