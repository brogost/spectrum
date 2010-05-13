#pragma once

class EffectWrapper;

struct TimeSlice
{
	TimeSlice() : _start_ms(0), _end_ms(0), _vertex_count(0), _stride(0), _cutoff_vertex_count(0) {}
	int32_t  _start_ms;
	int32_t  _end_ms;

	uint32_t	_vertex_count;
	uint32_t	_stride;
  std::vector<D3DXVECTOR3> _data_left;
  std::vector<D3DXVECTOR3> _data_right;
	CComPtr<ID3D11Buffer>	_vb_left;
	CComPtr<ID3D11Buffer>	_vb_right;
  CComPtr<ID3D11Buffer>	_vb_cutoff;
  uint32_t _cutoff_vertex_count;
};

struct Renderer
{
  ~Renderer();
	void render_at_time(EffectWrapper *vs, EffectWrapper *ps, ID3D11DeviceContext *context, const int32_t start_ms, const int32_t end_ms, const int32_t cur_pos);
  typedef std::vector<TimeSlice*> Slices;
	Slices _slices;

	CComPtr<ID3D11DepthStencilState> _dss_current_pos;
	CComPtr<ID3D11Buffer> _vb_current_pos;

};
