#pragma once

#include "..\Components\Page.h"
#include "..\Components\GameClosedEventHandler.h"
#include "..\Components\GameParameters.h"
#include "..\Components\GameView.xaml.h"
#include "GamePage.g.h"

namespace Roblox
{
    [Windows::Foundation::Metadata::WebHostHidden]
    public ref class GamePage sealed
    {
    public:
        GamePage();

        void StartGame(Controls::GameParameters^ params);

        event Controls::GameClosedEventHandler^ OnGameShutDown;

    internal:
        void LeaveGame();

    private:
        void OnGameViewClosed();

        Windows::UI::Xaml::UIElement^ m_previousContent;
    };
}
