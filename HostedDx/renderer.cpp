#include "stdafx.h"
#include "renderer.hpp"
#include "effect_wrapper.hpp"
#include "graphics.hpp"

void Renderer::render_at_time(EffectWrapper *effect, ID3D11DeviceContext *context, const uint32_t start_ms, const uint32_t end_ms)
{
	std::vector<TimeSlice*> slices;
	// find all the slices contained in the span
	for (auto it = _slices.begin(); it != _slices.end(); ++it) {
		if (it->_start_ms >= end_ms)
			break;
		if (it->_start_ms >= start_ms)
			slices.push_back(&(*it));
	}


	UINT ofs = 0;
	UINT strides = sizeof(D3DXVECTOR3);

	for (auto it = slices.begin();it != slices.end(); ++it) {
		TimeSlice* cur = *it;

		D3DXMATRIX mtx, mtx2;
		D3DXMatrixTranslation(&mtx, -1, 0, 0);
		D3DXMatrixScaling(&mtx2, 2 / ((cur->_end_ms - cur->_start_ms) / 1000.0f), 1, 1);
		D3DXMatrixTranspose(&mtx, &(mtx2 * mtx));
		effect->set_variable("mtx", mtx);
		effect->unmap_buffers();
		effect->set_cbuffer();

		ID3D11Buffer* bufs[] = { cur->_vb_left };
		context->IASetVertexBuffers(0, 1, &bufs[0], &strides, &ofs);
		context->Draw(cur->_vertex_count, 0);
	}

}
