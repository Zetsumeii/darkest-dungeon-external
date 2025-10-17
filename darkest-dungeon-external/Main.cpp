#include <d3d11.h>
#include <dwmapi.h>
#include <Windows.h>

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "Darkest.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dwmapi.lib")

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

HWND g_hWnd = nullptr;
HWND g_targetWindow = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
	Instance<Context::Cheat>::Get(); // Initialize the cheat instance

    g_targetWindow = FindWindowA(nullptr, "Darkest Dungeon");
    if (!g_targetWindow)
    {
        MessageBoxA(nullptr, "Target window not found!", "Error", MB_OK);
        return 1;
    }

    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hInstance, nullptr, nullptr, nullptr, nullptr, L"DDG Overlay Class", nullptr };
    RegisterClassExW(&wc);
    RECT targetRect;
    GetWindowRect(g_targetWindow, &targetRect);

    g_hWnd = CreateWindowExW(
        WS_EX_LAYERED,
        wc.lpszClassName,
        L"DDG Overlay",
        WS_POPUP,
        targetRect.left - 48,
        targetRect.top - 22,
        targetRect.right - targetRect.left,
        targetRect.bottom - targetRect.top,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    SetLayeredWindowAttributes(g_hWnd, RGB(0, 0, 0), BYTE(255), LWA_ALPHA);
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(g_hWnd, &margins);

    if (!CreateDeviceD3D(g_hWnd))
    {
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(g_hWnd, SW_SHOWDEFAULT);
    UpdateWindow(g_hWnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.3f);

    ImGui_ImplWin32_Init(g_hWnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	bool overlayEnabled = true;
    bool done = false;

    while (!done)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        if (GetAsyncKeyState(VK_INSERT) & 1 || GetAsyncKeyState(VK_DELETE) & 1) {
            overlayEnabled = !overlayEnabled;
            if (overlayEnabled) {
                SetWindowLong(g_hWnd, GWL_EXSTYLE, GetWindowLong(g_hWnd, GWL_EXSTYLE) & ~WS_EX_TRANSPARENT);
            }
            else {
                SetWindowLong(g_hWnd, GWL_EXSTYLE, GetWindowLong(g_hWnd, GWL_EXSTYLE) | WS_EX_TRANSPARENT);
			}
        }

        if (g_targetWindow)
        {
            RECT rect;
            if (GetWindowRect(g_targetWindow, &rect))
            {
                SetWindowPos(g_hWnd, HWND_TOPMOST,
                    rect.left - 48, rect.top - 22,
                    rect.right - rect.left, rect.bottom - rect.top,
                    SWP_NOACTIVATE);
            }
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

		ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove);
		ImGui::SetWindowSize(ImVec2(300, 200), ImGuiCond_Always);
		ImGui::Text("FPS: %.1f", io.Framerate);

		{
			static const char* ressources[] = { "Gold", "Bustes", "Portraits", "Contracts", "Blasons", "Shards", "Dust", "Blueprints" };
			static int current_ressource = 0;
			static int prev_ressource = 0;
			static int amount = Instance<Context::Cheat>::Get()->getRessourceAmount();

			ImGui::Combo("RESSOURCES", &current_ressource, ressources, IM_ARRAYSIZE(ressources));
			if (current_ressource != prev_ressource) {
				prev_ressource = current_ressource;
				Instance<Context::Cheat>::Get()->setCurrent(static_cast<Context::Ressources>(current_ressource));
				Instance<Context::Cheat>::Get()->refreshRessourceAmount();
				amount = Instance<Context::Cheat>::Get()->getRessourceAmount();
			}
			ImGui::InputInt("Amount", &amount);
            ImGui::NewLine();
            if (ImGui::Button("Set amount"))
            {
				Instance<Context::Cheat>::Get()->setRessourceAmount(static_cast<std::size_t>(amount));
				Instance<Context::Cheat>::Get()->refreshRessourceAmount();
				amount = Instance<Context::Cheat>::Get()->getRessourceAmount();
            }
		}
		
		ImGui::NewLine();
		if (ImGui::Button("Exit"))
			done = true;
		
		ImGui::End();
		ImGui::Render();
		const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, 0);
    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(g_hWnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    );

    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
