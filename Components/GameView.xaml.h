#pragma once

#include "Components\GameView.g.h"
#include "GameClosedEventHandler.h"
#include "GameParameters.h"
#include "InputManager.h"
#include "..\Roblox.Rendering\CubeRenderer.h"

namespace Roblox
{
    namespace Controls
    {
        [Windows::Foundation::Metadata::WebHostHidden]
        public ref class GameView sealed
        {
        public:
            GameView();

            void StartGame(GameParameters^ params);
            void LeaveGame();
            bool IsGameRunning();
            void PresentGameLeaveMenu();

            event GameClosedEventHandler^ OnGameShutDown;

        private:
            ~GameView();

            bool m_isGameRunning;
            Roblox::Rendering::CubeRenderer^ m_cubeRenderer;
            Roblox::InputManager^ m_inputManager;
        };
    }
}
