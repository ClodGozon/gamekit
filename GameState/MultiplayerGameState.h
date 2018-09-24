/**********************************************************************

Copyright (c) 2018 Robert May

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**********************************************************************/


#ifndef MULTIPLAYERGAMESTATE_H
#define MULTIPLAYERGAMESTATE_H

#include "..\coreutilities\GameFramework.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\ResourceHandles.h"

#include "BaseState.h"
#include "Gameplay.h"
#include "MultiplayerState.h"


/************************************************************************************************/


class NetworkState;
class PacketHandler;


/************************************************************************************************/


class MultiplayerGame : public FlexKit::FrameworkState
{
public:
	MultiplayerGame		(FlexKit::GameFramework* IN_framework, NetworkState* network, size_t playerCount, BaseState* base);
	~MultiplayerGame	();

	bool Update			(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) final;
	bool PreDrawUpdate	(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT) final;
	bool Draw			(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& Graph) final;
	bool PostDrawUpdate	(FlexKit::EngineCore* Engine, FlexKit::UpdateDispatcher& Dispatcher, double dT, FlexKit::FrameGraph& Graph) final;


private:
	FlexKit::TriMeshHandle							characterModel;

	NetworkState*									network;
	Game											localGame;
	BaseState*										base;

	size_t											frameID;
	EventList										frameEvents;
	FlexKit::CircularBuffer<FrameSnapshot, 120>		frameCache;

	LocalPlayerHandler								localHandler;

	FlexKit::Vector<PlayerPuppet>					puppets;
	FlexKit::Vector<PacketHandler*>					handler;
};


/************************************************************************************************/
#endif