#include "stdafx.h"
#include "renderer.hpp"
#include "effect_wrapper.hpp"
#include "graphics.hpp"

Renderer::~Renderer()
{
  container_delete(_slices);
}

// given a range, [start_x, end_x), calc where ms would end up if the
// range was scaled from -1..1
float shifted_pos(const int start_ms, const int end_ms, const int ms)
{
	const float scale_x = 2 / ((end_ms - start_ms) / 1000.0f);

	// Calc how much to offset the first slice
	const float t = (start_ms - ms) / 1000.0f;
	return -1 - (t * scale_x);
}

void Renderer::render_at_time(EffectWrapper *vs, EffectWrapper *ps, ID3D11DeviceContext *context, const int32_t start_ms, const int32_t end_ms, const int32_t cur_pos)
{
	if (start_ms == end_ms)
		return;

	std::vector<TimeSlice*> slices;
	// find all the slices contained in the span
	for (auto it = _slices.begin(); it != _slices.end(); ++it) {
    const int32_t s = (*it)->_start_ms;
    const int32_t e = (*it)->_end_ms;
    if ((s >= start_ms && s < end_ms) || (e >= start_ms && e < end_ms) || (s <= start_ms && e >= end_ms))
      slices.push_back(*it);
	}

	if (slices.empty())
		return;

	uint32_t cur = 0;
	float start_ofs = 0;

	// We want to scale end-ms - start-ms to 2 units
	const float scale_x = 2 / ((end_ms - start_ms) / 1000.0f);

  // Calc how much to offset the first slice
	float offset_x = shifted_pos(start_ms, end_ms, slices[0]->_start_ms);

  D3DXMATRIX mtx, mtx2;
	for (auto it = slices.begin();it != slices.end(); ++it) {
		TimeSlice* t = *it;

		D3DXMatrixTranslation(&mtx, offset_x, 0, 0);
		D3DXMatrixScaling(&mtx2, scale_x, 1, 1);
		D3DXMatrixTranspose(&mtx, &(mtx2 * mtx));
		vs->set_variable("mtx", mtx);
		vs->unmap_buffers();
		vs->set_cbuffer();

    // left
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);
    ps->set_variable("color", D3DXCOLOR(1,1,0,1));
    ps->unmap_buffers();
    ps->set_cbuffer();

    set_vb(context, t->_vb_left, sizeof(D3DXVECTOR3));
		context->Draw(t->_vertex_count, 0);

    // right
    ps->set_variable("color", D3DXCOLOR(1,0,0,1));
    ps->unmap_buffers();
    ps->set_cbuffer();

    set_vb(context, t->_vb_right, sizeof(D3DXVECTOR3));
    context->Draw(t->_vertex_count, 0);

		if (t->_cutoff_vertex_count > 0) {
			// cut off
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
			ps->set_variable("color", D3DXCOLOR(1,1,1,1));
			ps->unmap_buffers();
			ps->set_cbuffer();

			set_vb(context, t->_vb_cutoff, sizeof(D3DXVECTOR3));
			context->Draw(t->_cutoff_vertex_count, 0);
		}

		offset_x += scale_x * ((t->_end_ms - t->_start_ms) / 1000.0f);
	}

	// draw the now-playing
	D3DXMatrixTranslation(&mtx, shifted_pos(start_ms, end_ms, cur_pos), 0, 0);
	D3DXMatrixScaling(&mtx2, scale_x, 1, 1);
	D3DXMatrixTranspose(&mtx, &(mtx2 * mtx));
	vs->set_variable("mtx", mtx);
	vs->unmap_buffers();
	vs->set_cbuffer();
	ps->set_variable("color", D3DXCOLOR(0,0.1f,0.7f,1));
	ps->unmap_buffers();
	ps->set_cbuffer();

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	set_vb(context, _vb_current_pos, sizeof(D3DXVECTOR3));
	context->Draw(4, 0);

}
