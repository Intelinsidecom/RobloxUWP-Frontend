#pragma once

#include <wrl.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <DirectXMath.h>
#include <functional>

namespace Roblox
{
    ref class InputManager;

    namespace Controls
    {
        ref class GameParameters;
    }

    namespace Rendering
    {
        private ref class CubeRenderer sealed
        {
        public:
            CubeRenderer();
            void Initialize(Windows::UI::Xaml::Controls::SwapChainPanel^ panel);
            void Start();
            void Stop();
            void SetInputManager(Roblox::InputManager^ inputManager);
            void SetGameParameters(Roblox::Controls::GameParameters^ parameters);

        internal:
            void SetLeaveCallback(std::function<void()> callback);

        private:
            ~CubeRenderer();

            bool CreateDeviceResources();
            void CreateSwapChain();
            bool CreateCubeResources();
            bool CreateTextResources();
            void Resize(unsigned int width, unsigned int height);
            void Render();
            void RenderText();
            void UpdateExitButton();

            void OnTick(Platform::Object^ sender, Platform::Object^ e);

            Windows::UI::Xaml::Controls::SwapChainPanel^ m_panel;
            Windows::UI::Xaml::DispatcherTimer^ m_timer;
            CubeRenderer^ m_selfReference;

            Microsoft::WRL::ComPtr<ID3D11Device> m_device;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context;
            Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
            Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_depthStencilView;
            Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizerState;
            Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
            Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
            Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
            Microsoft::WRL::ComPtr<ID3D11Buffer> m_indexBuffer;
            Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBuffer;

            Microsoft::WRL::ComPtr<ID3D11VertexShader> m_textVertexShader;
            Microsoft::WRL::ComPtr<ID3D11PixelShader> m_textPixelShader;
            Microsoft::WRL::ComPtr<ID3D11InputLayout> m_textInputLayout;
            Microsoft::WRL::ComPtr<ID3D11Buffer> m_textVertexBuffer;
            Microsoft::WRL::ComPtr<ID3D11Buffer> m_textConstantBuffer;
            Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_fontTextureView;
            Microsoft::WRL::ComPtr<ID3D11SamplerState> m_fontSampler;
            Microsoft::WRL::ComPtr<ID3D11BlendState> m_textBlendState;
            Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_textDepthState;

            Roblox::InputManager^ m_inputManager;

            Roblox::Controls::GameParameters^ m_gameParameters;

            std::function<void()> m_leaveCallback;

            float m_angle;
            unsigned int m_width;
            unsigned int m_height;

            float m_exitX;
            float m_exitY;
            float m_exitVX;
            float m_exitVY;
            bool m_exitInitialized;
            bool m_exitTriggered;
        };
    }
}
