#include <sstream>
#include <functional>
#include <filesystem>
#include <fstream>
#include <SimpleMath.h>

#include "framework.h"

#include "misc.h"
#include "shader.h"
#include "texture.h"
#include "instance_Skinned_Mesh.h"


inline DirectX::SimpleMath::Matrix ConvertToXmfloat4x4(const FbxAMatrix& fbxamatrix) {
	DirectX::SimpleMath::Matrix value;
	// 2重for文 4x4だからね
	for (int row = 0; row < 4; row++) {
		for (int column = 0; column < 4; column++) {
			value.m[row][column] = static_cast<float>(fbxamatrix[row][column]);
		}
	}
	return value;
}

inline DirectX::SimpleMath::Vector3 ConvertToXmfloat3(const FbxDouble3& fbxdouble3) {
	DirectX::SimpleMath::Vector3 value;
	value.x = static_cast<float>(fbxdouble3[0]);
	value.y = static_cast<float>(fbxdouble3[1]);
	value.z = static_cast<float>(fbxdouble3[2]);
	return value;
}

inline DirectX::SimpleMath::Vector4 ConvertToXmfloat4(const FbxDouble4& fbxdouble4) {
	DirectX::SimpleMath::Vector4 value;
	value.x = static_cast<float>(fbxdouble4[0]);
	value.y = static_cast<float>(fbxdouble4[1]);
	value.z = static_cast<float>(fbxdouble4[2]);
	value.w = static_cast<float>(fbxdouble4[3]);
	return value;
}

InstanceSkinnedMesh::InstanceSkinnedMesh(const char* fbx_filename, int draw_amount, int cstNo, bool triangulate) {
	FbxManager* fbx_manager{ FbxManager::Create() };	// マネージャの生成

	// 引数ファイル名.fbx拡張子を.cerealに変換、ファイル名.cerealが存在している場合は.cerealをロード、存在しない場合には.fbxをロードする
	// 名前でファイル有無を判断をするので、中身が違っても同名ファイルであれば新規作成はされないので手動で削除する必要がある
	std::filesystem::path cereal_filename(fbx_filename);
	cereal_filename.replace_extension("cereal");

	//既に.cerealがあった場合はそれを読み込む
	if (std::filesystem::exists(cereal_filename.c_str()))
	{
		std::ifstream ifs(cereal_filename.c_str(), std::ios::binary);
		cereal::BinaryInputArchive deserialization(ifs);
		deserialization(scene_view, meshes, materials);
	}
	// なければfbxを読みこみ、シリアライズさせる
	else
	{
		FbxScene* fbx_scene{ FbxScene::Create(fbx_manager,"") };	// sceneにfbx内ファイル内の情報を流し込む

		FbxImporter* fbx_importer{ FbxImporter::Create(fbx_manager,"") };	// インポーターの生成
		bool import_status{ false };
		import_status = fbx_importer->Initialize(fbx_filename);
		_ASSERT_EXPR_A(import_status, fbx_importer->GetStatus().GetErrorString());

		import_status = fbx_importer->Import(fbx_scene);
		_ASSERT_EXPR_A(import_status, fbx_importer->GetStatus().GetErrorString());

		FbxGeometryConverter fbx_converter(fbx_manager);
		if (triangulate) {
			fbx_converter.Triangulate(fbx_scene, true, false);	// 四角ポリゴンを三角ポリゴンに変換 作り直しなので非常に重い
			fbx_converter.RemoveBadPolygonsFromMeshes(fbx_scene);
		}

		// 関数作成
		std::function<void(FbxNode*)> traverse{ [&](FbxNode* fbx_node) {
			scene::node& node{scene_view.nodes.emplace_back()};
			// 引数のfbx_nodeの情報をコピー
			node.attribute = fbx_node->GetNodeAttribute() ? fbx_node->GetNodeAttribute()->GetAttributeType() : FbxNodeAttribute::EType::eUnknown;
			node.name = fbx_node->GetName();
			node.unique_id = fbx_node->GetUniqueID();
			node.parent_index = scene_view.indexof(fbx_node->GetParent() ? fbx_node->GetParent()->GetUniqueID() : 0);	// 親が存在していれば番号を取得
			for (int child_index = 0; child_index < fbx_node->GetChildCount(); ++child_index) {
				traverse(fbx_node->GetChild(child_index));
			}
		}
		};
		traverse(fbx_scene->GetRootNode());

		Fetch_Meshes(fbx_scene, meshes);
		Fetch_Materials(fbx_scene, materials);

#if 0	// デバッグウィンドウに出力
		for (const scene::node& node : scene_view.nodes) {
			FbxNode* fbx_node{ fbx_scene->FindNodeByName(node.name.c_str()) };
			// ノードデータをデバッグとして出力ウィンドウに表示する
			std::string node_name = fbx_node->GetName();
			uint64_t uid = fbx_node->GetUniqueID();
			uint64_t parent_uid = fbx_node->GetParent() ? fbx_node->GetParent()->GetUniqueID() : 0;	// 親が存在していれば番号を取得
			int32_t type = fbx_node->GetNodeAttribute() ? fbx_node->GetNodeAttribute()->GetAttributeType() : 0;

			// 情報アウトプット用デバッグ
			std::stringstream debug_string;
			debug_string << node_name << ":" << uid << ":" << parent_uid << ":" << type << "\n";
			OutputDebugStringA(debug_string.str().c_str());
		}
#endif

		// std::ofstream ofs(ファイル名,オープンモード)
		std::ofstream ofs(cereal_filename.c_str(), std::ios::binary);
		cereal::BinaryOutputArchive serialization(ofs);
		serialization(scene_view, meshes, materials);
	}
	fbx_manager->Destroy();

	// マテリアル情報がない場合に備え予めダミーテクスチャをセット
	// 静的宣言なので一回だけ 中身がnullじゃなかったら作成、それ以外は作らない
	if (dummyTexture) { Texture::MakeDummyTexture(dummyTexture.GetAddressOf(), 0xFFFFFFFF, 16); }
	Create_com_buffers(fbx_filename);

	CstNo = cstNo;
	if (!InstanceShader)	// 一回だけ生成
	{
		InstanceShader = std::make_unique<ShaderEx>();
		InstanceShader->CreateVS(L"Shaders\\Instance_Mesh_vs");
		InstanceShader->CreatePS(L"Shaders\\Instance_Mesh_ps");
	}

	//インスタンス描画準備----------------------------------------------------------------------------------
	{
		ID3D11Device* device = FRAMEWORK->GetDevice();
		HRESULT hr = {S_OK};

		D3D11_BUFFER_DESC BufferDesc;
		ZeroMemory(&BufferDesc, sizeof(D3D11_BUFFER_DESC));
		BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		BufferDesc.ByteWidth = sizeof(InstanceData) * draw_amount;			// バッファサイズ
		BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;		// 構造化バッファ
		BufferDesc.StructureByteStride = sizeof(InstanceData);			// 構造化バッファーのサイズ (バイト単位)
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;							// 動的
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;				// CPUからの書き込みを許可
		hr = device->CreateBuffer(&BufferDesc, nullptr, InputBuffer.GetAddressOf());
		if (FAILED(hr))_ASSERT_EXPR_A(false, "FAILED CreateStructuredBuffer_for_InstanceDraw");


		// 構造化バッファーからシェーダーリソースビューを作成する
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
			ZeroMemory(&SRVDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
			SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;   // SRVであることを指定する
			SRVDesc.BufferEx.FirstElement = 0;
			SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
			SRVDesc.BufferEx.NumElements = draw_amount;              // リソース内の要素の数

			// 構造化バッファーをもとにシェーダーリソースビューを作成する
			hr = device->CreateShaderResourceView(InputBuffer.Get(), &SRVDesc, SRV.GetAddressOf());
			if (FAILED(hr))	_ASSERT_EXPR_A(false, "FAILED CreateShaderResourceView");
		}
	}	//インスタンス描画末-------------------------------------------------------------------------------

	// 各種パラメータの初期化
	Parameters = std::make_unique<Object3d>();
	Parameters->Position = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 0.0f);
	Parameters->Scale = DirectX::SimpleMath::Vector3(1.0f, 1.0f, 1.0f);
	Parameters->Orientation = DirectX::SimpleMath::Quaternion(0.0f, 0.0f, 0.0f, 1.0f);
	Parameters->Color = DirectX::SimpleMath::Vector4(1.0f, 1.0f, 1.0f, 1.0f);

}

DirectX::SimpleMath::Matrix InstanceSkinnedMesh::CulcWorldMatrix(const float scale_factor)
{
	DirectX::XMMATRIX C{ XMLoadFloat4x4(&coordinate_system_transforms[CstNo]) * DirectX::XMMatrixScaling(scale_factor,scale_factor,scale_factor) };

	DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(Parameters->Scale.x,Parameters->Scale.y,Parameters->Scale.z) };
	DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(Parameters->Orientation);
	DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(Parameters->Position.x,Parameters->Position.y,Parameters->Position.z) };

	XMStoreFloat4x4(&world, C * S * R * T);	// ワールド変換行列作成
	return world;
}

void InstanceSkinnedMesh::addWorldData()
{
	WorldData d;
	d.size = Parameters->Scale;
	d.position = Parameters->Position;
	d.orientation = Parameters->Orientation;
	d.color = Parameters->Color;
	worldData.emplace_back(d);
}

void InstanceSkinnedMesh::Render(ID3D11DeviceContext* device_context, UINT drawInstance) {
	// 単位をセンチメートルからメートルに変更するため、scale_factorを0.01に設定する
	static constexpr  float SCALE_FACTOR = 1.0f;
	{
		instanceData.clear();	// リセット

		DirectX::XMMATRIX C{ XMLoadFloat4x4(&coordinate_system_transforms[CstNo]) * DirectX::XMMatrixScaling(SCALE_FACTOR,SCALE_FACTOR,SCALE_FACTOR) };
		// 保存したデータ分ワールド行列を作成する
		for (auto& d : worldData) {
			DirectX::XMMATRIX S{ DirectX::XMMatrixScaling(d.size.x,d.size.y,d.size.z) };
			DirectX::XMMATRIX R = DirectX::XMMatrixRotationQuaternion(d.orientation);
			DirectX::XMMATRIX T{ DirectX::XMMatrixTranslation(d.position.x,d.position.y,d.position.z) };

			InstanceData w;
			XMStoreFloat4x4(&w.world, C * S * R * T);	// ワールド変換行列作成
			for (const Mesh& mesh : meshes) {
				DirectX::XMStoreFloat4x4(&w.world,
					DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&mesh.default_global_transform), DirectX::XMLoadFloat4x4(&w.world)));	// グローバルのTransformとworld行列を掛けてworld座標に変換している
			}
			w.color = d.color;
			instanceData.emplace_back(w);		// ワールド行列ストック
		}
		// SRV指定のバッファにデータを入れる
		D3D11_MAPPED_SUBRESOURCE subRes;
		device_context->Map(InputBuffer.Get(), NULL, D3D11_MAP_WRITE_DISCARD, NULL, &subRes);	// subResにInputBufferをマップ
		{
			memcpy(subRes.pData, instanceData.data(), sizeof(InstanceData) * worldData.size());	// SRVのバッファに初期化情報をコピー
		}
		device_context->Unmap(InputBuffer.Get(), 0);
		// セットする
		device_context->VSSetShaderResources(0, 1, SRV.GetAddressOf());	// (t0)
	}

	for (const Mesh& mesh : meshes) {
		Constants data{ world,Parameters->Color };
		DirectX::XMStoreFloat4x4(&data.world,
			DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&mesh.default_global_transform), DirectX::XMLoadFloat4x4(&world)));	// グローバルのTransformとworld行列を掛けてworld座標に変換している

		uint32_t stride{ sizeof(Vertex) };	// stride:刻み幅
		uint32_t offset{ 0 };
		device_context->IASetVertexBuffers(0, 1, mesh.VertexBuffer.GetAddressOf(), &stride, &offset);
		device_context->IASetIndexBuffer(mesh.index_buffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		device_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// シェーダの設定
		InstanceShader->Activate(device_context);
		// ラスタライザステートの設定
		device_context->RSSetState(FRAMEWORK->GetRasterizerState(FRAMEWORK->RS_SOLID_NONE));

		for (const Mesh::Subset& subset : mesh.subsets) {	// マテリアル別メッシュの数回すよ
			if (subset.material_unique_id != 0)	// unique_idの確認
			{
				const Material& material = materials.at(subset.material_unique_id);
				if (materials.size() > 0)	// マテリアル情報があるか確認
				{
					device_context->PSSetShaderResources(0, 1, material.srv[0].GetAddressOf());
					DirectX::XMStoreFloat4(&data.material_color, DirectX::XMVectorMultiply(DirectX::XMLoadFloat4(&Parameters->Color), DirectX::XMLoadFloat4(&material.Kd)));	// マテリアルとカラーを合成
				}
			}
			else
			{
				device_context->PSSetShaderResources(0, 1, dummyTexture.GetAddressOf());	// ダミーテクスチャを使用する
			}

			device_context->VSSetConstantBuffers(0, 1, ConstantBuffers.GetAddressOf());
			device_context->UpdateSubresource(ConstantBuffers.Get(), 0, 0, &data, 0, 0);
			device_context->DrawIndexedInstanced(subset.index_count, drawInstance, subset.start_index_location, 0 ,0);
			//immediate_context->DrawIndexedInstancedIndirect(引数分からん);	// 描画するインデックスの数,最初のインデックスの場所,頂点バッファから読み取る前に追加する値
		}
	}	// autpFor末
	// シェーダの無効化
	InstanceShader->Inactivate(device_context);
}

void InstanceSkinnedMesh::Create_com_buffers(const char* fbx_filename) {
	ID3D11Device* device = FRAMEWORK->GetDevice();

	HRESULT hr{ S_OK };

	for (Mesh& mesh : meshes) {
		D3D11_BUFFER_DESC buffer_desc{};
		buffer_desc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * mesh.vertices.size());
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		buffer_desc.CPUAccessFlags = 0;
		buffer_desc.MiscFlags = 0;
		buffer_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA subresource_data;
		subresource_data.pSysMem = mesh.vertices.data();	// どの情報で初期化するか
		subresource_data.SysMemPitch = 0;
		subresource_data.SysMemSlicePitch = 0;

		hr = device->CreateBuffer(&buffer_desc, &subresource_data, mesh.VertexBuffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));

		buffer_desc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * mesh.indices.size());
		buffer_desc.Usage = D3D11_USAGE_DEFAULT;
		buffer_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		subresource_data.pSysMem = mesh.indices.data();		// どの情報で初期化するか

		hr = device->CreateBuffer(&buffer_desc, &subresource_data, mesh.index_buffer.ReleaseAndGetAddressOf());
		_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
#if 1
		mesh.vertices.clear();
		mesh.indices.clear();
#endif
	}

	// マテリアル数に応じてテクスチャをセット
	for (std::unordered_map<uint64_t, Material>::iterator iterator = materials.begin(); iterator != materials.end(); ++iterator) {
		if (iterator->second.texture_filenames[0].size() > 0) {	// secondは値にアクセスするために使用する
			std::filesystem::path path(fbx_filename);
			path.replace_filename(iterator->second.texture_filenames[0]);
			D3D11_TEXTURE2D_DESC texture2d_desc;
			Texture::LoadTextureFromFile(path.c_str(), iterator->second.srv[0].GetAddressOf(), &texture2d_desc);
		}
		else {
			Texture::MakeDummyTexture(iterator->second.srv[0].GetAddressOf(), 0xFFFFFFFF, 16);
		}
	}

	D3D11_BUFFER_DESC buffer_desc{};
	ZeroMemory(&buffer_desc, sizeof(D3D11_BUFFER_DESC));
	buffer_desc.ByteWidth = sizeof(Constants);	// Constantsの型を使用
	buffer_desc.Usage = D3D11_USAGE_DEFAULT;
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	// ConstantBufferとして使用することをきめる
	hr = device->CreateBuffer(&buffer_desc, nullptr, ConstantBuffers.ReleaseAndGetAddressOf());
	_ASSERT_EXPR(SUCCEEDED(hr), hr_trace(hr));
}

void InstanceSkinnedMesh::Fetch_Meshes(FbxScene* fbx_scene, std::vector<Mesh>& meshes) {
	for (const scene::node& node : scene_view.nodes) {
		if (node.attribute != FbxNodeAttribute::EType::eMesh) {	// Mesh属性じゃなかったら
			continue;
		}

		FbxNode* fbx_node{ fbx_scene->FindNodeByName(node.name.c_str()) };	//ノード名取得
		FbxMesh* fbx_mesh{ fbx_node->GetMesh() };	// fbx_nodeのメッシュを取得

		Mesh& mesh{ meshes.emplace_back() };
		mesh.unique_id = fbx_mesh->GetNode()->GetUniqueID();
		mesh.name = fbx_mesh->GetName();
		mesh.node_index = scene_view.indexof(mesh.unique_id);
		mesh.default_global_transform = ConvertToXmfloat4x4(fbx_mesh->GetNode()->EvaluateGlobalTransform());

		std::vector <Mesh::Subset>& subsets{ mesh.subsets };
		const int material_count{ fbx_mesh->GetNode()->GetMaterialCount() };
		subsets.resize(material_count > 0 ? material_count : 1);
		for (int material_index = 0; material_index < material_count; ++material_index) {
			const FbxSurfaceMaterial* fbx_material{ fbx_mesh->GetNode()->GetMaterial(material_index) };
			subsets.at(material_index).material_name = fbx_material->GetName();
			subsets.at(material_index).material_unique_id = fbx_material->GetUniqueID();
		}
		if (material_count > 0) {
			const int polygon_count{ fbx_mesh->GetPolygonCount() };
			for (int polygon_index = 0; polygon_index < polygon_count; ++polygon_index) {
				const int material_index{ fbx_mesh->GetElementMaterial()->GetIndexArray().GetAt(polygon_index) };
				subsets.at(material_index).index_count += 3;
			}
			uint32_t offset{ 0 };
			for (Mesh::Subset& subset : subsets) {
				subset.start_index_location = offset;
				offset += subset.index_count;
				// これは次の手順でカウンタとして使用され、ゼロにリセットされる
				subset.index_count = 0;
			}
		}

		const int polygon_count{ fbx_mesh->GetPolygonCount() };
		mesh.vertices.resize(polygon_count * 3LL);
		mesh.indices.resize(polygon_count * 3LL);

		FbxStringList uv_names;
		fbx_mesh->GetUVSetNames(uv_names);
		const FbxVector4* control_points{ fbx_mesh->GetControlPoints() };
		for (int polygon_index = 0; polygon_index < polygon_count; ++polygon_index) {
			const int mterial_index{ material_count > 0 ? fbx_mesh->GetElementMaterial()->GetIndexArray().GetAt(polygon_index) : 0 };
			Mesh::Subset& subset{ subsets.at(mterial_index) };
			const uint32_t offset{ subset.start_index_location + subset.index_count };
			for (int position_in_polygon = 0; position_in_polygon < 3; ++position_in_polygon) {
				const int vertex_index{ polygon_index * 3 + position_in_polygon };

				Vertex vertex;
				const int polygon_vertex{ fbx_mesh->GetPolygonVertex(polygon_index, position_in_polygon) };
				vertex.position.x = static_cast<float>(control_points[polygon_vertex][0]);
				vertex.position.y = static_cast<float>(control_points[polygon_vertex][1]);
				vertex.position.z = static_cast<float>(control_points[polygon_vertex][2]);

				if (fbx_mesh->GetElementNormalCount() > 0) {
					FbxVector4 normal;
					fbx_mesh->GetPolygonVertexNormal(polygon_index, position_in_polygon, normal);
					vertex.normal.x = static_cast<float>(normal[0]);
					vertex.normal.y = static_cast<float>(normal[1]);
					vertex.normal.z = static_cast<float>(normal[2]);
				}
				if (fbx_mesh->GetElementUVCount() > 0) {
					FbxVector2 uv;
					bool unmapped_uv;
					fbx_mesh->GetPolygonVertexUV(polygon_index, position_in_polygon,
						uv_names[0], uv, unmapped_uv);
					vertex.texcoord.x = static_cast<float>(uv[0]);
					vertex.texcoord.y = 1.0f - static_cast<float>(uv[1]);
				}

				mesh.vertices.at(vertex_index) = std::move(vertex);
				//mesh.indices.at(vertex_index) = vertex_index;
				mesh.indices.at(static_cast<size_t>(offset) + position_in_polygon) = vertex_index;
				subset.index_count++;
			}
		}
	}
}

void InstanceSkinnedMesh::Fetch_Materials(FbxScene* fbx_scene, std::unordered_map<uint64_t, Material>& materials) {
	const size_t node_count{ scene_view.nodes.size() };	// ノードのサイズ
	for (size_t node_index = 0; node_index < node_count; ++node_index) {
		const scene::node& node{ scene_view.nodes.at(node_index) };	// 指定番号のノード取得
		const FbxNode* fbx_node{ fbx_scene->FindNodeByName(node.name.c_str()) };	// ノードを検索、情報取得

		const int material_count{ fbx_node->GetMaterialCount() };	// マテリアル数の取得
		for (int material_index = 0; material_index < material_count; ++material_index) {
			const FbxSurfaceMaterial* fbx_material{ fbx_node->GetMaterial(material_index) };	// FbxSurfaceMaterial の取得

			Material material;
			material.name = fbx_material->GetName();
			material.unique_id = fbx_material->GetUniqueID();
			FbxProperty fbx_property;
			// Kdの取得
			{
				fbx_property = fbx_material->FindProperty(FbxSurfaceMaterial::sDiffuse);	// Diffuseの取得
				if (fbx_property.IsValid()) {	// 有効かどうかのチェック
					const FbxDouble3 color{ fbx_property.Get<FbxDouble3>() };
					material.Kd.x = static_cast<float>(color[0]);
					material.Kd.y = static_cast<float>(color[1]);
					material.Kd.z = static_cast<float>(color[2]);
					material.Kd.w = 1.0f;

					const FbxFileTexture* fbx_texture{ fbx_property.GetSrcObject<FbxFileTexture>() };	// テクスチャ情報の取得
					material.texture_filenames[0] = fbx_texture ? fbx_texture->GetRelativeFileName() : "";	// テクスチャ名の取得
				}
			}
			// Ksの取得
			{
				fbx_property = fbx_material->FindProperty(FbxSurfaceMaterial::sSpecular);	// Specularの取得
				if (fbx_property.IsValid()) {	// 有効かどうかのチェック
					const FbxDouble3 color{ fbx_property.Get<FbxDouble3>() };
					material.Ks.x = static_cast<float>(color[0]);
					material.Ks.y = static_cast<float>(color[1]);
					material.Ks.z = static_cast<float>(color[2]);
					material.Ks.w = 1.0f;
				}
			}
			// Kaの取得
			{
				fbx_property = fbx_material->FindProperty(FbxSurfaceMaterial::sAmbient);	// Ambientの取得
				if (fbx_property.IsValid()) {	// 有効かどうかのチェック
					const FbxDouble3 color{ fbx_property.Get<FbxDouble3>() };
					material.Ka.x = static_cast<float>(color[0]);
					material.Ka.y = static_cast<float>(color[1]);
					material.Ka.z = static_cast<float>(color[2]);
					material.Ka.w = 1.0f;
				}
			}
			materials.emplace(material.unique_id, std::move(material));	// unique番目にmaterialを格納する
		}
	}
}

void InstanceSkinnedMesh::imguiWindow(const char* beginname) {
	//float pos[3]{ Parameters->Position.x ,Parameters->Position.y ,Parameters->Position.z };
	//float size[3]{ Parameters->Scale.x ,Parameters->Scale.y ,Parameters->Scale.z };
	//float angle[3]{ Parameters->Rotate.x,Parameters->Rotate.y,Parameters->Rotate.z };
	//float Color[4]{ Parameters->Color.x ,Parameters->Color.y,Parameters->Color.z,Parameters->Color.w };

	//ImGui::Begin(beginname);	// 識別ID 同じIDだと一緒のウィンドウにまとめられる

	//ImGui::SliderFloat3(u8"Position", pos, -5, 5);
	//ImGui::SliderFloat3(u8"Size", size, 0, 5);
	//ImGui::SliderFloat3(u8"angle", angle, -360, 360);
	//ImGui::ColorEdit4(u8"Color", (float*)&Color);
	//ImGui::Checkbox(u8"WireFrame", &wireframe);

	//ImGui::End();	// ウィンドウ終了
	//// パラメータ代入
	//setPos(DirectX::SimpleMath::Vector3(pos[0], pos[1], pos[2]));
	//setScale(DirectX::SimpleMath::Vector3(size[0], size[1], size[2]));
	//setOrientation(DirectX::SimpleMath::Vector3(angle[0], angle[1], angle[2]));
	//setColor(DirectX::SimpleMath::Vector4(Color[0], Color[1], Color[2], Color[3]));
}