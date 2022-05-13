#pragma once
#include "Scenemanager.h"

class SceneTitle :public SceneBase {

	// 変数 //
private:
public:
	// Sprite型 画像描画用
	std::unique_ptr<Sprite> TitleImage;
	std::unique_ptr<ShaderEx> SpriteShader = nullptr;


	// シーン定数バッファ
	struct SceneConstants {
		DirectX::SimpleMath::Matrix view_projection;	// VP変換行列
		DirectX::SimpleMath::Vector4 light_direction;	// ライトの向き
		DirectX::SimpleMath::Vector4 camera_position;	// カメラの位置
	};

	Microsoft::WRL::ComPtr<ID3D11Buffer> ConstantBuffers[8];

	// Geometric_primitiveの変数やつ
	std::unique_ptr< Geometric_Cube> Grid;	// グリッド線もどき
	std::unique_ptr<ShaderEx> GeomtricShader = nullptr;

	// Skkined_Mesh用
	std::unique_ptr<Skinned_Mesh> skinned_mesh;


	// 個人 ImGuiで数値を編集、格納して関数に渡す変数
	float light_dir[3]{ 0.5f,-1.0f,1.0f };	// ライトの向かう方向
	bool focus_zero = true;	// 焦点が(0,0,0)に向けるかどうか
	DirectX::SimpleMath::Vector3 eyePos = DirectX::SimpleMath::Vector3(0.0f, 0.0f, -10.0f);	// カメラの位置

	std::unique_ptr<Camera> camera = nullptr;

	// 関数 //
private:
public:
	bool Initialize();
	void Update();
	void Render();

	void imguiUpdate();	// Scene内で表示させるimguiを管理

};
