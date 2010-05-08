#include "stdafx.h"
#include "renderer.hpp"
#include "effect_wrapper.hpp"
#include "graphics.hpp"

void Renderer::render_at_time(EffectWrapper *effect, ID3D11DeviceContext *context, const uint32_t start_ms, const uint32_t end_ms)
{
	if (start_ms == end_ms)
		return;

	std::vector<TimeSlice*> slices;
	// find all the slices contained in the span
	for (auto it = _slices.begin(); it != _slices.end(); ++it) {
		if (it->_start_ms >= end_ms)
			break;
		if (it->_start_ms >= start_ms)
			slices.push_back(&(*it));
	}

	if (slices.empty())
		return;

	UINT ofs = 0;
	UINT strides = sizeof(D3DXVECTOR3);

	uint32_t cur = 0;
	float start_ofs = 0;

	// We want to scale end-ms - start-ms to 2 units
	const float scale_x = 2 / ((end_ms - start_ms) / 1000.0f);
	const float t = ((int32_t)start_ms - (int32_t)slices[0]->_start_ms) / 1000.0f;
	float offset_x = -1 - (t * scale_x);

	for (auto it = slices.begin();it != slices.end(); ++it) {
		TimeSlice* t = *it;

		D3DXMATRIX mtx, mtx2;
		D3DXMatrixTranslation(&mtx, offset_x, 0, 0);
		D3DXMatrixScaling(&mtx2, scale_x, 1, 1);
		D3DXMatrixTranspose(&mtx, &(mtx2 * mtx));
		effect->set_variable("mtx", mtx);
		effect->unmap_buffers();
		effect->set_cbuffer();

		ID3D11Buffer* bufs[] = { t->_vb_left };
		context->IASetVertexBuffers(0, 1, &bufs[0], &strides, &ofs);
		context->Draw(t->_vertex_count, 0);

		offset_x += scale_x * ((t->_end_ms - t->_start_ms) / 1000.0f);
	}

}
