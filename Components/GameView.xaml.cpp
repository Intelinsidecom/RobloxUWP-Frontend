#include "pch.h"
#include "GameView.xaml.h"

using namespace Roblox::Controls;
using namespace Roblox::Rendering;
using namespace Windows::UI::Xaml;

GameView::GameView()
    : m_isGameRunning(false)
    , m_cubeRenderer(nullptr)
    , m_inputManager(nullptr)
{
    InitializeComponent();
}

GameView::~GameView()
{
    if (m_cubeRenderer != nullptr)
    {
        m_cubeRenderer->SetInputManager(nullptr);
        m_cubeRenderer->SetLeaveCallback(nullptr);
    }

    if (m_inputManager != nullptr)
    {
        delete m_inputManager;
        m_inputManager = nullptr;
    }

    if (m_cubeRenderer != nullptr)
    {
        m_cubeRenderer->Stop();
        m_cubeRenderer = nullptr;
    }
}

void GameView::StartGame(GameParameters^ params)
{
    m_isGameRunning = true;
    rbxSwapChain->Visibility = Windows::UI::Xaml::Visibility::Visible;
    ChatTextBox->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    if (m_cubeRenderer == nullptr)
    {
        m_cubeRenderer = ref new CubeRenderer();
    }
    m_cubeRenderer->Initialize(rbxSwapChain);
    m_cubeRenderer->SetGameParameters(params);
    m_cubeRenderer->Start();

    if (m_inputManager == nullptr)
    {
        m_inputManager = ref new Roblox::InputManager(Windows::UI::Core::CoreWindow::GetForCurrentThread());
        m_inputManager->registerListeners();
    }

    m_cubeRenderer->SetInputManager(m_inputManager);
    m_cubeRenderer->SetLeaveCallback([this]() { LeaveGame(); });
}

void GameView::LeaveGame()
{
    if (!m_isGameRunning)
    {
        return;
    }

    m_isGameRunning = false;

    if (m_cubeRenderer != nullptr)
    {
        m_cubeRenderer->SetInputManager(nullptr);
        m_cubeRenderer->SetLeaveCallback(nullptr);
    }

    if (m_inputManager != nullptr)
    {
        delete m_inputManager;
        m_inputManager = nullptr;
    }

    if (m_cubeRenderer != nullptr)
    {
        m_cubeRenderer->Stop();
        m_cubeRenderer = nullptr;
    }

    OnGameShutDown();
}

bool GameView::IsGameRunning()
{
    return m_isGameRunning;
}

void GameView::PresentGameLeaveMenu()
{
    ChatTextBox->Visibility = Windows::UI::Xaml::Visibility::Visible;
}
