#pragma once

#include "Object3d.h"
#include "skinned_mesh.h"
#include "geometric_primitive.h"

#include <memory>

// 弾クラス
class Shot
{
	// 変数
private:
	//static std::unique_ptr<Skinned_Mesh> Model;	// staticにしたかったけど上手く行かないので一旦削除
	std::unique_ptr<Skinned_Mesh> Model;	// そのままモデル
	std::unique_ptr<Geometric_Sphere> test;	// テストモデル まる

	// デフォルトのシェーダー
	std::unique_ptr<ShaderEx> SkinnedShader = nullptr;

	float LifeTimer = 0;	// 射出時間の保有
	bool Exist = false;		// 発射済みフラグ
public:
	std::unique_ptr<Object3d> Parameters;
	bool HitConfirmation = false;	// 当たり判定

	// 関数
private:
public:

	Shot() {};
	Shot(Shot& shot)
	{
		this->Model = std::move(shot.Model);
		this->test = std::move(shot.test);
		this->LifeTimer = shot.LifeTimer;
	};
	~Shot() {};

	void Initialize();
	void Update();
	void Render();

	void set(const Object3d* parent);	// オブジェクトは全てObject3dクラスを保有している前提で各種パラメータをコピー
	void setExist(bool flg) { Exist = flg; }
	bool getExist() { return Exist; }
};

class ShotManager
{
private:
	std::vector<std::unique_ptr<Shot>> Shots;

public:

	void Initialize() { Shots.clear(); }
	void Update();	// 存在していない弾は内部で削除している
	void Render();

	// 生成、格納系
	void newSet(const Object3d* initData);
	void push(std::unique_ptr<Shot> shot) { Shots.emplace_back(std::move(shot)); };	// 配列に格納する

	size_t getSize() { return Shots.size(); };
	std::vector<std::unique_ptr<Shot>>* getShots() { return &Shots; }	// 格納コンテナを返す 外部から参照したいときに

	bool isHit(const Object3d* Capcule);
};