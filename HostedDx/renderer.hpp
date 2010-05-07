#pragma once

class EffectWrapper;

struct TimeSlice
{
	TimeSlice() : _start_ms(0), _end_ms(0), _vertex_count(0), _stride(0) {}
	uint32_t  _start_ms;
	uint32_t  _end_ms;

	uint32_t	_vertex_count;
	uint32_t	_stride;
	CComPtr<ID3D11Buffer>	_vb_left;
	CComPtr<ID3D11Buffer>	_vb_right;
};

struct Renderer
{
	void render_at_time(EffectWrapper *effect, ID3D11DeviceContext *context, const uint32_t start_ms, const uint32_t end_ms);
	std::vector<TimeSlice> _slices;
};
