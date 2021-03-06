#ifndef GameFramework_H
#define GameFramework_H

/**********************************************************************

Copyright (c) 2019 Robert May

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

#ifdef COMPILE_GAMESTATE
#define GAMESTATEAPI __declspec(dllexport)
#else
#define GAMESTATEAPI __declspec(dllimport)
#endif

#include "..\coreutilities\EngineCore.h"
#include "..\coreutilities\Components.h"
#include "..\coreutilities\GraphicScene.h"
#include "..\coreutilities\GraphicsComponents.h"
#include "..\coreutilities\Logging.h"
#include "..\coreutilities\Assets.h"

#include "..\graphicsutilities\FrameGraph.h"
#include "..\graphicsutilities\Graphics.h"
#include "..\graphicsutilities\TerrainRendering.h"
#include "..\graphicsutilities\SSReflections.h"

#include "Console.h"

#include <iostream>

namespace FlexKit
{
	/************************************************************************************************/


	struct MouseHandler
	{
	};

	class FrameworkState;


	/************************************************************************************************/


	void PushMessageToConsole(void* User, const char* Str, size_t StrLen);


	class GameFramework
	{
	public:
		GameFramework(EngineCore& core);

		GameFramework				(const GameFramework&) = delete;
		GameFramework& operator =	(const GameFramework&) = delete;

		void Initiate();

		void Update				(UpdateDispatcher& dispatcher, double dT);
		void UpdateFixed		(UpdateDispatcher& dispatcher, double dT); // called at fixed time rate
		void UpdatePreDraw		(UpdateDispatcher& dispatcher, iAllocator* TempMemory, double dT);
		void Draw				(UpdateDispatcher& dispatcher, iAllocator* TempMemory, double dT);
		void PostDraw			(UpdateDispatcher& dispatcher, iAllocator* TempMemory, double dT);
		void Release			();

		void DrawFrame(double dT);

		bool DispatchEvent(const Event& evt);

		void DrawDebugHUD(double dT, VertexBufferHandle TextBuffer, FrameGraph& Graph);

		void PostPhysicsUpdate	();
		void PrePhysicsUpdate	();

		void PopState();

		RenderSystem&	GetRenderSystem()	{ return core.RenderSystem; }

		template<typename TY_INITIALSTATE, typename ... TY_ARGS>
		TY_INITIALSTATE& PushState(TY_ARGS&& ... args)
		{
			FK_LOG_9("Pushing New State");

			auto& State = core.GetBlockMemory().allocate_aligned<TY_INITIALSTATE>(*this, std::forward<TY_ARGS>(args)...);

			subStates.push_back(&State);

			return State;
		}

		MouseInputState			MouseState;

		GraphicScene*			ActiveScene;
		RenderWindow*			ActiveWindow;

		float4					clearColor;

		LogCallback				logMessagePipe = {
			"INFO",
			this,
			PushMessageToConsole
		};

		EngineCore&				core;
		NodeHandle				rootNode;

		static_vector<MouseHandler>		mouseHandlers;
		static_vector<FrameworkState*>	subStates;

		double	physicsUpdateTimer;
		bool	consoleActive;
		bool	drawDebug;
		bool	drawDebugStats;
		bool	drawPhysicsDebug;
		bool	quit;


		double runningTime			= 0.0;
		double fixStepAccumulator	= 0.0;
		double fixedTimeStep		= 1.0 / 60.0;

		struct FrameStats
		{
			double t;
			double fpsT;
			size_t fps;
			size_t fpsCounter;
			size_t objectsDrawnLastFrame;
		}stats;

		struct {
			SpriteFontAsset*	Font;
			Texture2D			Terrain;
		}DefaultAssets =
		{
			LoadFontAsset(
				"assets\\fonts\\",
				"fontTest.fnt",
				core.RenderSystem,
				core.GetTempMemory(),
				core.GetBlockMemory())
		};

		Console					console;
	};


    /************************************************************************************************/


    inline uint2 GetWindowWH(GameFramework& framework)
    {
        return framework.GetRenderSystem().GetTextureWH(framework.ActiveWindow->backBuffer);
    }


	/************************************************************************************************/


	class FrameworkState
	{
	public:
		FrameworkState(const FrameworkState& in)				= delete;

		FrameworkState& operator = (const FrameworkState& in)	= delete;

		virtual ~FrameworkState() {}

		virtual void Update			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) {}
		virtual void DebugDraw		(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) {}
		virtual void PreDrawUpdate	(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT) {}
		virtual void Draw			(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) {}
		virtual void PostDrawUpdate	(EngineCore& Engine, UpdateDispatcher& Dispatcher, double dT, FrameGraph& Graph) {}

		virtual bool EventHandler	(Event evt) { return false;}

		GameFramework&	framework;
	protected:

		FrameworkState(GameFramework& in_framework) : framework(in_framework) {}

		RenderSystem*	GetRenderSystem() { return framework.GetRenderSystem(); }
	};


	/************************************************************************************************/


	bool LoadScene		 (EngineCore& Engine, GraphicScene& Scene, const char* SceneName);
	bool LoadScene		 (EngineCore& Engine, GraphicScene& Scene, GUID_t SceneID);
 
	void PushSubState	 (GameFramework& _ptr, FrameworkState& SS);
	void PopSubState	 (GameFramework& _ptr);
 
	void UpdateGameFramework  (EngineCore& Engine, GameFramework& _ptr, double dT);
	void PreDrawGameFramework (EngineCore& Engine, GameFramework& _ptr, double dT);
	void ReleaseGameFramework (EngineCore& Engine, GameFramework& _ptr);

	void InitiateFramework	(EngineCore& Engine, GameFramework& framework);


	void DrawMouseCursor(
		float2					CursorPos,
		float2					CursorSize,
		VertexBufferHandle		vertexBuffer,
		ConstantBufferHandle	constantBuffer,
		ResourceHandle			renderTarget,
		iAllocator*				tempMemory,
		FrameGraph*				frameGraph);


}	/************************************************************************************************/
#endif
