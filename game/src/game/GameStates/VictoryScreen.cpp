//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/GameStates/VictoryScreen.cpp
/// @details After beating a module, this screen is display with some end-game text
/// @author Johan Jansen

#include "game/GameStates/VictoryScreen.hpp"
#include "game/GameStates/PlayingState.hpp"
#include "game/GameStates/SelectPlayersState.hpp"
#include "game/Core/GameEngine.hpp"
#include "game/GUI/Button.hpp"
#include "game/GUI/Label.hpp"
#include "game/Module/Module.hpp"
#include "game/game.h"
#include "egolib/Graphics/GraphicsSystem.hpp"

VictoryScreen::VictoryScreen(PlayingState *playingState, const bool forceExit) :
	_playingState(playingState)
{
	//Add the buttons
	int yOffset = Ego::GraphicsSystem::gfx_height-80;
	std::shared_ptr<Button> exitButton = std::make_shared<Button>(_currentModule->isExportValid() ? "Save and Exit" : "Exit Game", SDLK_SPACE);
	exitButton->setSize(200, 30);
	exitButton->setPosition(20, yOffset);
	exitButton->setOnClickFunction(
	[]{
		_gameEngine->setGameState(std::make_shared<SelectPlayersState>());
	});
	addComponent(exitButton);

	//Add a button to allow players continue playing if they want
	if(!forceExit) {
		std::shared_ptr<Button> abortButton = std::make_shared<Button>("Continue Playing", SDLK_ESCAPE);
		abortButton->setSize(200, 30);
		abortButton->setPosition(exitButton->getX() + exitButton->getWidth() + 20, exitButton->getY());
		abortButton->setOnClickFunction(
		[this]{
			endState();
		});
		addComponent(abortButton);		
	}

	std::shared_ptr<Label> victoryText = std::make_shared<Label>(endtext);
	victoryText->setPosition(50, 50);
	addComponent(victoryText);
}

void VictoryScreen::update()
{
}

void VictoryScreen::drawContainer()
{
	//Render the playing state beackground first
	if(_playingState != nullptr) {
		_playingState->drawAll();
	}
}

void VictoryScreen::beginState()
{
    // menu settings
    SDL_SetWindowGrab(sdl_scr.window, SDL_FALSE);
    _gameEngine->enableMouseCursor();
}
