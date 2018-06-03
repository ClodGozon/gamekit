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

#include "Gameplay.h"
#include "..\coreutilities\containers.h"


/************************************************************************************************/


Player_Handle GameGrid::CreatePlayer(GridID_t CellID)
{
	Players.push_back(GridPlayer());
	Players.back().XY = CellID;

	MarkCell(CellID, EState::Player);

	return Players.size() - 1;
}


/************************************************************************************************/


GridObject_Handle GameGrid::CreateGridObject(GridID_t CellID)
{
	Objects.push_back(GridObject());

	Objects.back().XY = CellID;
	MarkCell(CellID, EState::Object);

	return Objects.size() - 1;
}


/************************************************************************************************/


void GameGrid::CreateBomb(EBombType Type, GridID_t CellID, BombID_t ID, Player_Handle PlayerID)
{
	if (!Players[PlayerID].DecrementBombSlot(Type))
		return;

	Bombs.push_back({ 
		CellID, 
		EBombType::Regular, 
		0, 
		ID,
		{    0,   0		},
		{ 0.0f, 0.0f	}});

	auto* Task = &Memory->allocate_aligned<RegularBombTask>(
		ID, 
		this);

	Tasks.push_back(Task);
}


/************************************************************************************************/


bool GameGrid::MovePlayer(Player_Handle Player, GridID_t GridID)
{
	if (Players[Player].State != GridPlayer::PS_Idle)
		return false;

	if (!IsCellClear(GridID))
		return false;

	auto OldPOS = Players[Player].XY;

	auto* Task = &Memory->allocate_aligned<MovePlayerTask>(
		OldPOS,
		GridID,
		Player,
		0.25f,
		this);

	Tasks.push_back(Task);

	return true;
}


/************************************************************************************************/


bool GameGrid::MoveBomb(GridID_t GridID, int2 Direction)
{
	if (GetCellState(GridID) != EState::Bomb)
		return false;

	auto res = FlexKit::find(Bombs, [GridID](auto& entry) {
		return entry.XY == GridID; });

	if (!res)
		return false;

	res->Direction = Direction;

	return true;
}


/************************************************************************************************/


bool GameGrid::IsCellClear(GridID_t GridID)
{
	if (IDInGrid(GridID))
		return false;

	size_t Idx = GridID2Index(GridID);

	return (Grid[Idx] == EState::Empty);
}


/************************************************************************************************/


bool GameGrid::IsCellDestroyed(GridID_t GridID)
{
	if (IDInGrid(GridID))
		return false;

	size_t Idx = GridID2Index(GridID);

	return (Grid[Idx] == EState::Object);
}


/************************************************************************************************/


void GameGrid::Update(const double dt, iAllocator* TempMemory)
{
	auto RemoveList = FlexKit::Vector<iGridTask**>(TempMemory);

	for (auto& Task : Tasks)
	{
		Task->Update(dt);

		if (Task->Complete())
			RemoveList.push_back(&Task);
	}

	for (auto* Task : RemoveList)
	{	// Ugh!
		(*Task)->~iGridTask();
		Memory->free(*Task);
		Tasks.remove_stable(Task);
	}
}


/************************************************************************************************/


bool GameGrid::MarkCell(GridID_t CellID, EState State)
{
	size_t Idx = GridID2Index(CellID);

	//if (!IsCellClear(CellID))
	//	return false;

	Grid[Idx] = State;

	return true;
}


/************************************************************************************************/

// Certain State will be lost
void GameGrid::Resize(uint2 wh)
{
	WH = wh;
	Grid.resize(WH.Product());

	for (auto& Cell : Grid)
		Cell = EState::Empty;

	for (auto Obj : Objects)
		MarkCell(Obj.XY, EState::Object);

	for (auto Player : Players)
		MarkCell(Player.XY, EState::Player);
}


/************************************************************************************************/


bool GameGrid::GetBomb(BombID_t ID, GridBomb& Out)
{
	auto Res = FlexKit::find(Bombs, 
		[this, ID](auto res) 
		{ 
			return (res.ID == ID);
		});

	Out = *(GridBomb*)Res;

	return (bool)Res;
}


/************************************************************************************************/


bool GameGrid::SetBomb(BombID_t ID, const GridBomb& In)
{
	auto Res = FlexKit::find(Bombs,
		[this, ID](auto res)
	{
		return (res.ID == ID);
	});

	*(GridBomb*)Res = In;

	return (bool)Res;
}


/************************************************************************************************/


EState GameGrid::GetCellState(GridID_t CellID)
{
	size_t Idx = WH[0] * CellID[1] + CellID[0];

	return Grid[Idx];
}


/************************************************************************************************/


bool GameGrid::RemoveBomb(BombID_t ID) 
{
	auto Res = FlexKit::find(Bombs,
		[this, ID](auto res)
	{
		return (res.ID == ID);
	});

	if (Res)
		Bombs.remove_unstable(Res);

	return Res;
}



/************************************************************************************************/


void MovePlayerTask::Update(const double dt)
{
	T += dt / D;

	if (T < (1.0f - 1.0f/60.f))
	{
		int2 temp	= B - A;
		float2 C	= { 
			(float)temp[0], 
			(float)temp[1] };

		Grid->Players[Player].Offset = { C * T };
	}
	else
	{
		Grid->MarkCell(A, EState::Empty);
		Grid->MarkCell(B, EState::Player);

		Grid->Players[Player].Offset = { 0.f, 0.f };
		Grid->Players[Player].XY	 = B;
		Grid->Players[Player].State	 = GridPlayer::PS_Idle;
		complete = true;
	}
}


/************************************************************************************************/


void RegularBombTask::Update(const double dt)
{
	T += dt;

	GridBomb BombEntry;
	if (!Grid->GetBomb(Bomb, BombEntry))
		return;

	auto Direction = BombEntry.Direction;

 	if ((Direction * Direction).Sum() > 0)
	{
		T2 += dt * 4;
		auto A = BombEntry.XY;
		auto B = BombEntry.XY + BombEntry.Direction;

		if (Grid->GetCellState(B) != EState::Empty)
		{
			T = 2.0f;
		}
		else if (T2 < (1.0f - 1.0f / 60.f))
		{

			int2 temp = B - A;
			float2 C = {
				(float)temp[0],
				(float)temp[1] };

			BombEntry.Offset = { C * T2 };
		}
		else
		{
			Grid->MarkCell(A, EState::Empty);
			Grid->MarkCell(B, EState::Bomb);

			T2					= 0.0f;
			BombEntry.Offset	= { 0.f, 0.f };
			BombEntry.XY		= B;
		}
	}

	if (T >= 2.0f)
	{
		Completed = true;


		Grid->RemoveBomb(Bomb);

		Grid->MarkCell(BombEntry.XY + int2{ -1, -1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  0, -1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  1, -1 }, EState::Destroyed);

		Grid->MarkCell(BombEntry.XY + int2{ -1,  0 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  0,  0 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  1,  0 }, EState::Destroyed);

		Grid->MarkCell(BombEntry.XY + int2{ -1,  1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  0,  1 }, EState::Destroyed);
		Grid->MarkCell(BombEntry.XY + int2{  1,  1 }, EState::Destroyed);

		Grid->CreateGridObject(BombEntry.XY + int2{ -1, -1 });
		Grid->CreateGridObject(BombEntry.XY + int2{ -0, -1 });
		Grid->CreateGridObject(BombEntry.XY + int2{  1, -1 });

		Grid->CreateGridObject(BombEntry.XY + int2{ -1, -0 });
		Grid->CreateGridObject(BombEntry.XY + int2{ -0, -0 });
		Grid->CreateGridObject(BombEntry.XY + int2{  1, -0 });

		Grid->CreateGridObject(BombEntry.XY + int2{ -1,  1 });
		Grid->CreateGridObject(BombEntry.XY + int2{  0,  1 });
		Grid->CreateGridObject(BombEntry.XY + int2{  1,  1 });
	}

	Grid->SetBomb(Bomb, BombEntry);
}


/************************************************************************************************/


void DrawGameGrid_Debug(
	double					dt,
	float					AspectRatio,
	GameGrid&				Grid,
	FrameGraph&				FrameGraph,
	ConstantBufferHandle	ConstantBuffer,
	VertexBufferHandle		VertexBuffer,
	TextureHandle			RenderTarget,
	iAllocator*				TempMem
	)
{
	const size_t ColumnCount	= Grid.WH[1];
	const size_t RowCount		= Grid.WH[0];

	LineSegments Lines(TempMem);
	Lines.reserve(ColumnCount + RowCount);

	const auto RStep = 1.0f / RowCount;

	for (size_t I = 1; I < RowCount; ++I)
		Lines.push_back({ {0, RStep  * I,1}, {1.0f, 1.0f, 1.0f}, { 1, RStep  * I, 1, 1 }, {1, 1, 1, 1} });


	const auto CStep = 1.0f / ColumnCount;
	for (size_t I = 1; I < ColumnCount; ++I)
		Lines.push_back({ { CStep  * I, 0, 0 },{ 1.0f, 1.0f, 1.0f },{ CStep  * I, 1, 0 },{ 1, 1, 1, 1 } });


	DrawShapes(EPIPELINESTATES::DRAW_LINE_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
		LineShape(Lines));


	for (auto Player : Grid.Players)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
			CircleShape(
				float2{	
					CStep / 2 + Player.XY[0] * CStep + Player.Offset.x * CStep,
					RStep / 2 + Player.XY[1] * RStep + Player.Offset.y * RStep },
				min(
					(CStep / 2.0f) / AspectRatio,
					(RStep / 2.0f)),
				float4{1.0f, 1.0f, 1.0f, 1.0f}, AspectRatio));


	for (auto Bombs : Grid.Bombs)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
			CircleShape(
				float2{
					CStep / 2 + Bombs.XY[0] * CStep + Bombs.Offset.x * CStep,
					RStep / 2 + Bombs.XY[1] * RStep + Bombs.Offset.y * RStep },
					min(
						(CStep / 2.0f) / AspectRatio,
						(RStep / 2.0f)),
				float4{ 0.0f, 1.0f, 0.0f, 0.0f }, AspectRatio, 4));


	for (auto Object : Grid.Objects)
		DrawShapes(EPIPELINESTATES::DRAW_PSO, FrameGraph, VertexBuffer, ConstantBuffer, RenderTarget, TempMem,
			RectangleShape(float2{ 
				Object.XY[0] * CStep, 
				Object.XY[1] * RStep }, 
				{ CStep , RStep },
				{0.5f, 0.5f, 0.5f, 1.0f}));
}


/************************************************************************************************/