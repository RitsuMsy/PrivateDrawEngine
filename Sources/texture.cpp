
#pragma once

#include <WICTextureLoader.h>

#include <wrl.h>

#include <string>
#include <map>


#include <memory>

#include "framework.h"
#include "texture.h"
#include "misc.h"
static std::map <std::wstring, Microsoft::WRL::ComPtr <ID3D11ShaderResourceView>> resources;


HRESULT load_texture_from_file(const wchar_t* filename, ID3D11ShaderResourceView** shader_resource_view, D3D11_TEXTURE2D_DESC* texture2d_desc) {
	ID3D11Device* device = FRAMEWORK->GetDevice();

	HRESULT hr{ S_OK };
	Microsoft::WRL::ComPtr<ID3D11Resource> resource{};

	// 画像ファイルのロードとSRVオブジェクトの生成
	auto it = resources.find(filename);	// クラスのテンプレートパラメータkey_type型のキーを受け取って検索する。見つからなかったらend()を返す
	if (it != resources.end()) {		// end()じゃない＝filenameが見つかったら
		*shader_resource_view = it->second.Get();	// secondはyオブジェクト、ここではSRV。ちなみにxオブジェクトはfirst
		(*shader_resource_view)->AddRef();			// カプセル化されたインターフェイスポインタを呼び出す
		(*shader_resource_view)->GetResource(resource.GetAddressOf());
	}
	else {
		hr = DirectX::CreateWICTextureFromFile(device, filename, resource.GetAddressOf(), shader_resource_view);	// resourceとsrvが作成される
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
		resources.insert(std::make_pair(filename, *shader_resource_view));	// insert：mapコンテナの拡張
	}

	// テクスチャ情報の取得
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d{};
	hr = resource.Get()->QueryInterface<ID3D11Texture2D>(texture2d.GetAddressOf());	// 特定のインターフェイスをサポートしているかを判別する
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
	texture2d->GetDesc(texture2d_desc);

	return hr;
}

// ダミーテクスチャの作成
HRESULT make_dummy_texture(ID3D11ShaderResourceView** shader_resource_view, DWORD value/*0xAABBGGRR*/, UINT dimension) {
	ID3D11Device* device = FRAMEWORK->GetDevice();

	HRESULT hr{ S_OK };

	D3D11_TEXTURE2D_DESC texture2d_desc{};
	texture2d_desc.Width = dimension;
	texture2d_desc.Height= dimension;
	texture2d_desc.MipLevels = 1;
	texture2d_desc.ArraySize= 1;
	texture2d_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texture2d_desc.SampleDesc.Count = 1;
	texture2d_desc.SampleDesc.Quality = 0;
	texture2d_desc.Usage = D3D11_USAGE_DEFAULT;
	texture2d_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	size_t texels = dimension * dimension;
	std::unique_ptr<DWORD[]>system{std::make_unique<DWORD[]>(texels) };
	for (size_t i = 0; i < texels; i++) {
		system[i] = value;
	}

	D3D11_SUBRESOURCE_DATA subresource_data{};
	subresource_data.pSysMem = system.get();
	subresource_data.SysMemPitch = sizeof(DWORD) * dimension;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2d;
	hr = device->CreateTexture2D(&texture2d_desc, &subresource_data, &texture2d);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	D3D11_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc{};
	shader_resource_view_desc.Format = texture2d_desc.Format;
	shader_resource_view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shader_resource_view_desc.Texture2D.MipLevels = 1;
	hr = device->CreateShaderResourceView(texture2d.Get(), &shader_resource_view_desc, shader_resource_view);
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

	return hr;

}

void rerease_all_textures() {
	resources.clear();
}