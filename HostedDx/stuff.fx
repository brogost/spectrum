float4x4 mtx;

struct vsInput
{
	float3 position : POSITION;
};

struct vsOutput
{
	float4 position : SV_POSITION;
};

vsOutput vsMain( in vsInput v )
{
	vsOutput o = (vsOutput)0;
	o.position = mul(float4(v.position,1), mtx);
	return o;
}

float4 psMain(float4 v : SV_POSITION) : SV_TARGET
{
	return float4(1,1,1,1);
}
