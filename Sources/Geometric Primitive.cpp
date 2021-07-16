#include "Geometric_Primitive.h"
#include "shader.h"
#include "misc.h"

#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#include <vector>


Geometric_Primitive::Geometric_Primitive(ID3D11Device* device, const char* vs_cso_name, const char* ps_cso_name) {

	HRESULT hr{ S_OK };

	D3D11_INPUT_ELEMENT_DESC input_element_desc[]{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL"	, 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// �V�F�[�_�쐬
	create_vs_from_cso(device, vs_cso_name, vertex_shader.GetAddressOf(), input_layout.GetAddressOf(), input_element_desc, ARRAYSIZE(input_element_desc));
	create_ps_from_cso(device, ps_cso_name, pixel_shader.GetAddressOf());

	// �R���X�^���g�o�b�t�@�쐬
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(Constants);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	hr = device->CreateBuffer(&buffer_desc, nullptr, constant_buffer.GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// ���X�^���C�U�I�u�W�F�N�g�̐���
	D3D11_RASTERIZER_DESC rasterizer_desc{};
	/*-----�h��Ԃ� �O�ʕ`��-----*/
	rasterizer_desc.FillMode = D3D11_FILL_SOLID;	// �����_�����O�Ɏg���h��Ԃ����[�h D3D11_FILL_SOLID|D3D11_FILL_WIREFRAME
	rasterizer_desc.CullMode = D3D11_CULL_BACK;	// �`�悷��@������ D3D11_CULL_NONE(���ʕ`��)|D3D11_CULL_FRONT(��ʕ`��)|D3D11_CULL_BACK(�O�ʕ`��)
	rasterizer_desc.FrontCounterClockwise = FALSE;	// �O�p�`���O�ʂ��w�ʂ������肷�� TRUE�̎��A���_�����Ύ��肾�ƑO�����Ƃ݂Ȃ����
	rasterizer_desc.DepthBias = 0;					// �[�x�o�C�A�X ����[�x��ɕ\������Ƃ��ɗD��x�����߂�̂Ɏg�p�����肷��
	rasterizer_desc.DepthBiasClamp = 0;			// ��L���l     �s�N�Z���̍ő�[�x�o�C�A�X
	rasterizer_desc.SlopeScaledDepthBias = 0;		// ��L���l     ����̃s�N�Z���̌X���̃X�J���[
	rasterizer_desc.DepthClipEnable = TRUE;		// �����Ɋ�Â��ăN���b�s���O��L���ɂ��邩
	rasterizer_desc.ScissorEnable = FALSE;			// �V�U�[��`�J�����O���g�p���邩 �V�U�[��`�F�`��̈�̎w��ɂ悭�g����
	rasterizer_desc.MultisampleEnable = FALSE;		// �}���`�T���v�����O�A���`�G�C���A�X(MSAA)��RTV���g�p���Ă��鎞�Ature�Ŏl�ӌ`���C���A���`�G�C���A�X�Afalse�ŃA���t�@���C���A���`�G�C���A�X���g�p
													// MSAA���g�p����ɂ̓��\�[�X��������DX11_SAMPLE_DESC::Count��1����̒l��ݒ肷��K�v������
	rasterizer_desc.AntialiasedLineEnable = FALSE;	// MSAA��RTV���g�p���Ă��鎞�A�����`���MultisampleEnable��false�̎��ɃA���`�G�C���A�X��L���ɂ���
	hr = device->CreateRasterizerState(&rasterizer_desc, rasterizer_states[0].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	/*-----���C���[�t���[�� �O�ʕ`��-----*/
	rasterizer_desc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizer_desc.CullMode = D3D11_CULL_BACK;
	rasterizer_desc.AntialiasedLineEnable = TRUE;
	hr = device->CreateRasterizerState(&rasterizer_desc, rasterizer_states[1].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	/*-----���C���[�t���[�� ���ʕ`��-----*/
	rasterizer_desc.FillMode = D3D11_FILL_WIREFRAME;
	rasterizer_desc.CullMode = D3D11_CULL_NONE;
	rasterizer_desc.AntialiasedLineEnable = TRUE;
	hr = device->CreateRasterizerState(&rasterizer_desc, rasterizer_states[2].GetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	// �e��p�����[�^�̏�����
	param.Pos   = XMFLOAT3(0.0f, 0.0f,0.0f);
	param.Size  = XMFLOAT3(1.0f,1.0f,1.0f);
	param.Angle = XMFLOAT3(0.0f, 0.0f, 0.0f);
	param.Color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

}

void Geometric_Primitive::Create_com_buffers(ID3D11Device* device, Vertex* vertices, size_t vertex_count, uint32_t* indices, size_t index_count){
	HRESULT hr{ S_OK };

	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * vertex_count);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	buffer_desc.CPUAccessFlags = 0;
	buffer_desc.MiscFlags = 0;
	buffer_desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA subresource_data;
	subresource_data.pSysMem = vertices;	// �ǂ̏��ŏ��������邩
	subresource_data.SysMemPitch = 0;
	subresource_data.SysMemSlicePitch = 0;

	hr = device->CreateBuffer(&buffer_desc, &subresource_data, vertex_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


	buffer_desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * index_count);
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	subresource_data.pSysMem = indices;		// �ǂ̏��ŏ��������邩

	hr = device->CreateBuffer(&buffer_desc, &subresource_data, index_buffer.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));


}

void Geometric_Primitive::Render(ID3D11DeviceContext* immediate_context, const XMFLOAT4X4& world, const XMFLOAT4& material_color, bool WireFrame) {
	uint32_t stride{ sizeof(Vertex) };
	uint32_t offset{ 0 };

	// ���_�o�b�t�@�̃o�C���h
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	// �C���f�b�N�X�o�b�t�@�̃o�C���h
	immediate_context->IASetIndexBuffer(index_buffer.Get(),		// �C���f�b�N�X���i�[�����I�u�W�F�N�g�̃|�C���^
		DXGI_FORMAT_R32_UINT,		// �C���f�b�N�X�o�b�t�@���̃f�[�^�̃t�H�[�}�b�g(16bit��32bit�̂ǂ��炩)
		0);							// �I�t�Z�b�g

	//�v���~�e�B�u�^�C�v�y�уf�[�^�̏����Ɋւ�����̃o�C���h
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���̓��C�A�E�g�I�u�W�F�N�g�̃o�C���h
	immediate_context->IASetInputLayout(input_layout.Get());

	// �V�F�[�_�̃o�C���h
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	Constants data{ world,material_color };
	// ����������}�b�v�s�\�ȃ������ɍ쐬���ꂽ�T�u���\�[�X�Ƀf�[�^���R�s�[
	immediate_context->UpdateSubresource(constant_buffer.Get(),	// ���惊�\�[�X�ւ̃|�C���^
		0,	// ����T�u���\�[�X�����ʂ���C���f�b�N�X
		0, &data, 0, 0);

	// �萔�o�b�t�@�̐ݒ�
	immediate_context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());

	// ���X�^���C�U�X�e�[�g�̐ݒ�
	wireframe = WireFrame;
	immediate_context->RSSetState(rasterizer_states[wireframe].Get());

	D3D11_BUFFER_DESC buffer_desc{};
	index_buffer->GetDesc(&buffer_desc);
	immediate_context->DrawIndexed(buffer_desc.ByteWidth / sizeof(uint32_t), 0, 0);	// �`�悷��C���f�b�N�X�̐�,�ŏ��̃C���f�b�N�X�̏ꏊ,���_�o�b�t�@����ǂݎ��O�ɒǉ�����l
}

void Geometric_Primitive::Render(ID3D11DeviceContext* immediate_context) {
	XMMATRIX S{ XMMatrixScaling(param.Size.x,param.Size.y,param.Size.z) }	;				// �g�k
	XMMATRIX R{ XMMatrixRotationRollPitchYaw(param.Angle.x,param.Angle.y,param.Angle.z) };	// ��]
	XMMATRIX T{ XMMatrixTranslation(param.Pos.x,param.Pos.y,param.Pos.z) };					// ���s�ړ�

	XMFLOAT4X4 world;
	XMStoreFloat4x4(&world, S * R * T);	// ���[���h�ϊ��s��쐬

	uint32_t stride{ sizeof(Vertex) };
	uint32_t offset{ 0 };

	// ���_�o�b�t�@�̃o�C���h
	immediate_context->IASetVertexBuffers(0, 1, vertex_buffer.GetAddressOf(), &stride, &offset);

	// �C���f�b�N�X�o�b�t�@�̃o�C���h
	immediate_context->IASetIndexBuffer(
		index_buffer.Get(),			// �C���f�b�N�X���i�[�����I�u�W�F�N�g�̃|�C���^
		DXGI_FORMAT_R32_UINT,		// �C���f�b�N�X�o�b�t�@���̃f�[�^�̃t�H�[�}�b�g(16bit��32bit�̂ǂ��炩)
		0);							// �I�t�Z�b�g

	//�v���~�e�B�u�^�C�v�y�уf�[�^�̏����Ɋւ�����̃o�C���h
	immediate_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���̓��C�A�E�g�I�u�W�F�N�g�̃o�C���h
	immediate_context->IASetInputLayout(input_layout.Get());

	// �V�F�[�_�̃o�C���h
	immediate_context->VSSetShader(vertex_shader.Get(), nullptr, 0);
	immediate_context->PSSetShader(pixel_shader.Get(), nullptr, 0);

	Constants data{ world,param.Color };
	// ����������}�b�v�s�\�ȃ������ɍ쐬���ꂽ�T�u���\�[�X�Ƀf�[�^���R�s�[
	immediate_context->UpdateSubresource(constant_buffer.Get(),	// ���惊�\�[�X�ւ̃|�C���^
		0,	// ����T�u���\�[�X�����ʂ���C���f�b�N�X
		0, &data, 0, 0);

	// �萔�o�b�t�@�̐ݒ�
	immediate_context->VSSetConstantBuffers(0, 1, constant_buffer.GetAddressOf());

	// ���X�^���C�U�X�e�[�g�̐ݒ�
	immediate_context->RSSetState(rasterizer_states[wireframe].Get());

	D3D11_BUFFER_DESC buffer_desc{};
	index_buffer->GetDesc(&buffer_desc);
	immediate_context->DrawIndexed(buffer_desc.ByteWidth / sizeof(uint32_t), 0, 0);	// �`�悷��C���f�b�N�X�̐�,�ŏ��̃C���f�b�N�X�̏ꏊ,���_�o�b�t�@����ǂݎ��O�ɒǉ�����l


}

void Geometric_Primitive::imguiWindow(const char* beginname) {

	float pos[3]{ param.Pos.x ,param.Pos.y ,param.Pos.z };
	float size[3]{ param.Size.x ,param.Size.y ,param.Size.z };
	float angle[3]{ param.Angle.x,param.Angle.y,param.Angle.z };
	float Color[4]{ param.Color.x ,param.Color.y,param.Color.z,param.Color.w };

	ImGui::Begin(beginname);	// ����ID ����ID���ƈꏏ�̃E�B���h�E�ɂ܂Ƃ߂���

	ImGui::SliderFloat3(u8"Position", pos, -5, 5);
	ImGui::SliderFloat3(u8"Size", size, 0, 5);
	ImGui::SliderFloat3(u8"angle", angle, -360, 360);
	ImGui::ColorEdit4(u8"Color", (float*)&Color);
	ImGui::Checkbox(u8"WireFrame" ,&wireframe);

	ImGui::End();
	setPos(DirectX::XMFLOAT3(pos[0], pos[1], pos[2]));
	setSize(DirectX::XMFLOAT3(size[0], size[1], size[2]));
	setAngle(DirectX::XMFLOAT3(XMConvertToRadians(angle[0]), XMConvertToRadians(angle[1]), XMConvertToRadians(angle[2])));
	setColor(DirectX::XMFLOAT4(Color[0], Color[1], Color[2], Color[3]));
}