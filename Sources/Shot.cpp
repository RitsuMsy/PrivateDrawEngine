#include "Shot.h"
#include "Judge.h"
#include "Player.h"
#include "Enemy.h"

#include "XMFLOAT_Helper.h"


void Shot::Initialize()
{
	if(!Model)	Model = std::make_unique<Skinned_Mesh>(".\\Resources\\Shots\\Shot.fbx");
	Sphere = std::make_unique<Geometric_Sphere>();

	SkinnedShader = std::make_unique<ShaderEx>();
	SkinnedShader->Create(L"Shaders\\skinned_mesh_vs", L"Shaders\\skinned_mesh_ps");

	Parameters = std::make_unique<Object3d>();

	LifeTimer = 0;
	Exist = false;
}

void Shot::Update()
{
	// とりあえず前に進む動きを作る
	static const float MOVE_SPEED = 0.5f;
	Parameters->Velocity = DirectX::SimpleMath::Vector3{ 0.0f,0.0f,0.0f };	// 例のごとく毎フレーム加速度リセット
	Parameters->Velocity += Model->getWorld().Backward() * MOVE_SPEED;
	Parameters->Position += Parameters->Velocity;

	// 自然消滅 適当なので要修正
	LifeTimer += 0.1f;
	if (LifeTimer >= 20.0f) {
		LifeTimer = 0.0f;
		Exist = false;
	}

	// モデルに描画系パラメーターを渡す
	Sphere->Parameters->CopyParam(Parameters.get());
	Sphere->Parameters->Color = DirectX::SimpleMath::Vector4{ 1.0f,LifeTimer/20.0f,1.0f,1.0f };
	//Sphere->Parameters->CopyParam(Model->getParameters());
	Model->getParameters()->CopyParam(Parameters.get());
}

void Shot::Render()
{
	if (Exist) {
		Model->Render(SkinnedShader.get());
		//Sphere->Render();
	}
}

void Shot::set(const Object3d* parent)
{
	Parameters->CopyParam(parent);
	Exist = true;
}

//----------------------------------------------------//
void ShotManager::Update()
{
	// 存在フラグの立っていない要素は削除する
	for (auto shots = Shots.begin(); shots != Shots.end();)
	{
		if (!shots->get()->getExist())	// こいつ…存在していないぞ!?
		{
			shots = this->Shots.erase(shots);	// 存在していないところを消す
		}
		else {	// 存在が確認されたので処理します
			shots->get()->Update();
			for (auto enem = EnemyManager::getInstance().getEnemys()->begin(); enem != EnemyManager::getInstance().getEnemys()->end(); ++enem)
			{
				// 大前提判定を取る者の存在判定 両方とも存在している且つ
				if (shots->get()->getExist() && enem->get()->getExist())
				{
					// 当たり判定、戻り値trueであれば
					if (isHit(enem->get()->Parameters.get()))
					{
						enem->get()->setExist(false);
						//shots->get()->setExist(false);	//  TODO: 存在消す処理をコメントアウトしているのはデバッグ用 一度の弾で複数体消すため
						//break;
					}
				}
			}
			++shots;	// 次へ
		}
	}
}

void ShotManager::Render()
{
	for (auto it = Shots.begin(); it != Shots.end(); ++it)
	{
		// 弾の描画
		it->get()->Render();
	}
}

void ShotManager::newSet(const Object3d* initData)
{
	std::unique_ptr<Shot>Shots = std::make_unique<Shot>();
	Shots->Initialize();
	Shots->set(initData);
	push(std::move(Shots));
}

bool ShotManager::isHit(const Object3d* Capcule)
{
	for (auto it = Shots.begin(); it != Shots.end(); ++it)
	{
		if (Judge::getInstance()->c_b(*Capcule, 1.0f, *it->get()->Parameters))
		{
			return true;
		}
	}
		return false;
}