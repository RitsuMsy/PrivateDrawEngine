#pragma once

#include "Object3d.h"
#include "skinned_mesh.h"
#include "geometric_primitive.h"

#include <memory>

// �e�N���X
class Shot
{
	// �ϐ�
private:
	//static std::unique_ptr<Skinned_Mesh> Model;	// static�ɂ������������Ǐ�肭�s���Ȃ��̂ň�U�폜
	std::unique_ptr<Skinned_Mesh> Model;	// ���̂܂܃��f��
	std::unique_ptr<Geometric_Sphere> test;	// �e�X�g���f�� �܂�

	// �f�t�H���g�̃V�F�[�_�[
	std::unique_ptr<ShaderEx> SkinnedShader = nullptr;

	float LifeTimer = 0;	// �ˏo���Ԃۗ̕L
	bool Exist;		// ���ˍς݃t���O
public:
	std::unique_ptr<Object3d> Parameters;
	bool HitConfirmation;	// �����蔻��

	// �֐�
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

	void set(const Object3d* parent);	// �I�u�W�F�N�g�͑S��Object3d�N���X��ۗL���Ă���O��Ŋe��p�����[�^���R�s�[
	bool getExist() { return Exist; }
};

class ShotManager
{
private:
	std::vector<std::unique_ptr<Shot>> Shots;

public:

	void Initialize() { Shots.clear(); }
	void Update();	// ���݂��Ă��Ȃ��e�͓����ō폜���Ă���
	void Render();

	// �����A�i�[�n
	void newSet(const Object3d* initData);
	void push(std::unique_ptr<Shot> shot) { Shots.emplace_back(std::move(shot)); };	// �z��Ɋi�[����

	size_t getSize() { return Shots.size(); };
	std::vector<std::unique_ptr<Shot>>* getShots() { return &Shots; }	// �i�[�R���e�i��Ԃ� �O������Q�Ƃ������Ƃ���

	bool isHit(const Object3d* Capcule);
};