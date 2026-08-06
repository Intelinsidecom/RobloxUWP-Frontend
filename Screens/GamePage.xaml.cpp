#include "pch.h"
#include "GamePage.xaml.h"
#include "AppShell.xaml.h"

using namespace Roblox;
using namespace Roblox::Controls;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Core;

GamePage::GamePage()
    : m_previousContent(nullptr)
{
    InitializeComponent();

    gameViewControl->OnGameShutDown +=
        ref new GameClosedEventHandler(this, &GamePage::OnGameViewClosed);
}

void GamePage::StartGame(GameParameters^ params)
{
    m_previousContent = Window::Current->Content;
    Window::Current->Content = this;

    auto nav = SystemNavigationManager::GetForCurrentView();
    if (nav != nullptr)
    {
        nav->AppViewBackButtonVisibility = AppViewBackButtonVisibility::Collapsed;
    }

    gameViewControl->StartGame(params);
}

void GamePage::LeaveGame()
{
    gameViewControl->LeaveGame();
}

void GamePage::OnGameViewClosed()
{
    if (m_previousContent != nullptr)
    {
        Window::Current->Content = m_previousContent;
        m_previousContent = nullptr;
    }

    AppShell::RefreshBackButtonState();

    OnGameShutDown();
}
