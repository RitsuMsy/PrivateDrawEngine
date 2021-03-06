#pragma once

#include <d3d11.h>

class Score
{
private:
	int KillCount;	// 倒した数
	float GameTime;	// 制限時間

private:
	void Initialize();
	void Update();
	void Render();

	void Kill() { KillCount++; }	// 敵を倒したカウント
};
