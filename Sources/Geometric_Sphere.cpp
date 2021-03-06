#include "framework.h"
#include "geometric_primitive.h"

#include <vector>

Geometric_Sphere::Geometric_Sphere(u_int slices, u_int stacks) {
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	static const FLOAT RADIAN = 0.5f;
	this->Radian = RADIAN;
	this->slices = slices;
	this->stacks = stacks;

	float degree = 2.0f * 3.14159265358989f / static_cast<float>(slices);	// 奇数*Pi/分割数=三角 偶数であれば平行四辺形になる(???)

	//
	// 一番上の頂点から順に頂点を計算していく．
	//

	// 上下頂点：マッピング時に最上下頂点に割り当てるテクスチャマップ上の一意の点がないため、テクスチャ座標の歪みが発生することに注意
	// 長方形のテクスチャを球体にマッピングする際、最上下頂点に割り当てるテクスチャマップ上の一意の点がないため、テクスチャ座標の歪みが生じることに注意
	// 長方形のテクスチャを球体にマッピングする場合、最上下頂点に割り当てるテクスチャマップ上の一意の点がないため、テクスチャ座標の歪みが生じることに注意

	Vertex top_vertex;		// 上の頂点(上向き)
	top_vertex.position = DirectX::SimpleMath::Vector3(0.0f, +RADIAN, 0.0f);
	top_vertex.normal = DirectX::SimpleMath::Vector3(0.0f, +1.0f, 0.0f);

	Vertex bottom_vertex;	// 下の頂点(下向き)
	bottom_vertex.position = DirectX::SimpleMath::Vector3(0.0f, -RADIAN, 0.0f);
	bottom_vertex.normal = DirectX::SimpleMath::Vector3(0.0f, -1.0f, 0.0f);

	vertices.push_back(top_vertex);	// 上に頂点を[0]に登録

	float phi_step = DirectX::XM_PI / stacks;			// Φ=黄金比？のステップ?
	float theta_step = 2.0f * DirectX::XM_PI / slices;	// θのステップ?

	// 各スタック円の頂点を計算する
	for (u_int i = 1; i <= stacks - 1; ++i)
	{
		float phi = i * phi_step;

		// 円の頂点.
		for (u_int j = 0; j <= slices; ++j)
		{
			float theta = j * theta_step;

			Vertex v;

			// 球面からカルテシアン(直交座標系)へ
			v.position.x = RADIAN * sinf(phi) * cosf(theta);
			v.position.y = RADIAN * cosf(phi);
			v.position.z = RADIAN * sinf(phi) * sinf(theta);

			DirectX::XMVECTOR p = DirectX::XMLoadFloat3(&v.position);	// positionをvector型に
			DirectX::XMStoreFloat3(&v.normal, DirectX::XMVector3Normalize(p));	// 法線の計算

			vertices.push_back(v);	// 順番に格納
		}
	}
	vertices.push_back(bottom_vertex);

	// 上の円の頂点割当
	{
		for (UINT i = 1; i <= slices; ++i)
		{
			indices.push_back(0);
			indices.push_back(i + 1);
			indices.push_back(i);
		}
	}

	//
	// 内側のスタック（最上下頂点に接続されていない）のインデックスを計算します。
	//

	// 最初の円の最初の頂点のインデックス分ずらす
	// これは一番上の頂点をスキップしているだけ
	u_int base_index = 1;
	u_int ring_vertex_count = slices + 1;
	for (u_int i = 0; i < stacks - 2; ++i)	// 上から下の順
	{
		for (u_int j = 0; j < slices; ++j)	// 後から前の順
		{
			// 各面の左側三角形
			indices.push_back(base_index + i * ring_vertex_count + j);
			indices.push_back(base_index + i * ring_vertex_count + j + 1);
			indices.push_back(base_index + (i + 1) * ring_vertex_count + j);

			// 各面の右側三角形
			indices.push_back(base_index + (i + 1) * ring_vertex_count + j);
			indices.push_back(base_index + i * ring_vertex_count + j + 1);
			indices.push_back(base_index + (i + 1) * ring_vertex_count + j + 1);
		}
	}

	//
	// 下のスタックのインデックスを計算する 下のスタックは頂点バッファに最後に書き込まれたもの
	// そして一番下の頂点と下の円を接続する
	//

	// 一番下の頂点を最後に追加

	u_int south_pole_index = (u_int)vertices.size() - 1;

	// 最後の円の最初の頂点のインデックス分ずらす
	// これは一番下の頂点をスキップしているだけ

	// 底の円の頂点割当
	{
		base_index = south_pole_index - ring_vertex_count;

		for (u_int i = 0; i < slices; ++i)
		{
			indices.push_back(south_pole_index);
			indices.push_back(base_index + i);
			indices.push_back(base_index + i + 1);
		}
	}

	Create_com_buffers(vertices.data(), vertices.size(), indices.data(), indices.size());
}