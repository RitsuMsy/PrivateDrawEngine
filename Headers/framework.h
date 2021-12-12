#pragma once

#include <d3d11.h>
#include <memory>
#include <wrl.h>
#include <windows.h>
#include <tchar.h>
#include <sstream>
#include "misc.h"
#include "high_resolution_timer.h"

#include "Sampler.h"

#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern ImWchar glyphRangesJapanese[];
#endif

CONST LONG SCREEN_WIDTH{ 1280 };
CONST LONG SCREEN_HEIGHT{ 720 };
CONST BOOL FULLSCREEN{ FALSE };
CONST LPCWSTR APPLICATION_NAME{ L"MyDrawEngine" };	// ウィンドウ上に表示されるテキスト ウィンドウ名

class framework
{
	// 変数
private:
	high_resolution_timer tictoc;
	uint32_t frames{ 0 };
	float elapsed_time{ 0.0f };
	void calculate_frame_stats();

	Microsoft::WRL::ComPtr<ID3D11Device> device;

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> immediate_context;	// 描画コマンドの追加や送信などの処理を扱っている。CPU側で追加された描画コマンドをGPU側に送信する。
																	// Immediateは生成したコマンドを即時実行することを表す。反対にDeferredというものが存在する。

	Microsoft::WRL::ComPtr<IDXGISwapChain> swap_chain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		render_target_view;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		depth_stencil_view;

	static const int DEPTH_STENCIL_TYPE = 9;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState>		depth_stencil_state[DEPTH_STENCIL_TYPE];

	static const int BLEND_TYPE = 9;
	Microsoft::WRL::ComPtr<ID3D11BlendState> bd_states[BLEND_TYPE];

	Microsoft::WRL::ComPtr<ID3D11SamplerState>	sampler_states[3];
	std::shared_ptr<Sampler> sample;

	static framework* instance;
	//-------------------------------------------------------------------
	// fps算出系
	int frameRate = 60;
	float MIN_FREAM_TIME;
	float frameTime;
	LARGE_INTEGER timeStart;
	LARGE_INTEGER timeEnd;
	LARGE_INTEGER timeFreq;
	// fpsを取得するなら0で初期化しないとゴミが混ざってマイナスから始まったりする(かも知れない)
	float fps;
	//-------------------------------------------------------------------

public:
	CONST HWND hwnd;

	//DepthStencilState
	enum { DS_FALSE, DS_TRUE, DS_FALSE_WRITE, DS_TRUE_WRITE, DS_END };
	// DlendState
	enum { BS_NONE, BS_ALPHA, BS_ADD, BS_SUBTRACT, BS_REPLACE, BS_MULTIPLY, BS_LIGHTEN, BS_DARKEN, BS_SCREEN, BS_END };

	// 関数
private:
	bool initialize();
	bool uninitialize();

	bool CreateDeviceAndSwapCain();		// DeviceとSwapChain作成
	bool CreateRenderTargetView();		// RenderTargetViewの作成
	bool CreateDepthStencileView();		// DepthStencilViewの作成
	bool CreateDepthStencileState();	// DepthStencilStateの作成
	bool CreateBlendState();			// BlendStateの作成

public:
	framework(HWND hwnd);
	~framework();

	framework(const framework&) = delete;
	framework& operator=(const framework&) = delete;
	framework(framework&&) noexcept = delete;
	framework& operator=(framework&&) noexcept = delete;

	void Clear(FLOAT color[4]);
	void Flip(int n = 0);

	int run();	// ゲームループ
	LRESULT CALLBACK handle_message(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	static framework* getInstance() { return instance; }

	bool CreateViewPort();				// ViewPortの作成

	// ゲッター
	ID3D11Device* GetDevice() { return device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() { return immediate_context.Get(); }
	ID3D11BlendState* GetBlendState(int state) { return bd_states[state].Get(); }
	ID3D11DepthStencilState* GetDepthStencileState(int state) { return depth_stencil_state[state].Get(); }

	const float GetElapsedTime() { return tictoc.time_interval(); }
};

#define FRAMEWORK framework::getInstance()