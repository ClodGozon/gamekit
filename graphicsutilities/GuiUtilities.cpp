/**********************************************************************

Copyright (c) 2015 - 2016 Robert May

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

#include "GuiUtilities.h"

namespace FlexKit
{
	/************************************************************************************************/


	void InitiateSimpleWindow(iAllocator* Memory, SimpleWindow* Out, SimpleWindow_Desc& Desc)
	{
		Out->Elements.Allocator			= Memory;
		Out->TexturedButtons.Allocator	= Memory;
		Out->Lists.Allocator			= Memory;
		Out->Root.Allocator				= Memory;
		Out->Dividers.Allocator			= Memory;
		Out->CellState.Allocator		= Memory;
		Out->TextButtons.Allocator		= Memory;
		Out->Memory						= Memory;

		Out->ColumnCount	= Desc.ColumnCount;
		Out->RowCount		= Desc.RowCount;
		Out->AspectRatio	= Desc.AspectRatio;

		Out->CellBorder = float2(0.005f * 1/Desc.AspectRatio, 0.005f);

#if 0
		Out->RHeights	= Desc.RowHeights;
		Out->CWidths	= Desc.ColumnWidths;
#endif

		Out->Position	= { 0.0f, 0.0f };
		Out->WH			= { Desc.Width, Desc.Height };


		size_t CellCount = Desc.ColumnCount * Desc.RowCount;
		Out->CellState.reserve(CellCount);

		for (size_t I = 0; I < CellCount; ++I)
			Out->CellState.push_back(false);
	}


	/************************************************************************************************/


	void CleanUpSimpleWindow(SimpleWindow* W)
	{
		for (auto TB : W->TextButtons)
			CleanUpTextArea(&TB.Text, W->Memory);

		W->Elements.Release();
		W->TexturedButtons.Release();
		W->Lists.Release();
		W->Root.Release();
		W->Dividers.Release();
		W->CellState.Release();
		W->TextButtons.Release();
	}


	/************************************************************************************************/


	float2 GetCellPosition(SimpleWindow* Window, uint2 Cell)
	{
		uint32_t ColumnCount = Window->ColumnCount;
		uint32_t RowCount	 = Window->RowCount;
		
		const float WindowWidth  = Window->WH[0];
		const float WindowHeight = Window->WH[1];
		
		const float CellGapWidth  = Window->CellBorder[0];
		const float CellGapHeight = Window->CellBorder[1];
		
		const float CellWidth	= (WindowWidth  / ColumnCount)	- CellGapWidth  - CellGapWidth  / ColumnCount;
		const float CellHeight	= (WindowHeight / RowCount)		- CellGapHeight - CellGapHeight / RowCount;//	

		return Window->Position + float2{ (CellWidth + CellGapWidth) * Cell[0] + CellGapWidth, (CellHeight + CellGapHeight) * Cell[1] + CellGapHeight };
	}


	float2 GetCellDimension(SimpleWindow* Window, uint2 WH = {1, 1})
	{
		uint32_t ColumnCount = Window->ColumnCount;
		uint32_t RowCount	 = Window->RowCount;
		
		const float WindowWidth  = Window->WH[0];
		const float WindowHeight = Window->WH[1];
		
		const float CellGapWidth  = Window->CellBorder[0];
		const float CellGapHeight = Window->CellBorder[1];
		
		const float CellWidth	= (WindowWidth  / ColumnCount)	- CellGapWidth  - CellGapWidth  / ColumnCount;
		const float CellHeight	= (WindowHeight / RowCount)		- CellGapHeight - CellGapHeight / RowCount;	

		return float2(CellWidth, CellHeight) * WH + float2(CellGapWidth *(WH[0] - 1), CellGapHeight);
	}


	/************************************************************************************************/


	bool CursorInside(float2 CursorPos, float2 POS, float2 WH)
	{
		auto ElementCord = CursorPos - POS;
		auto T = WH - ElementCord;

		return (ElementCord.x > 0.0f && ElementCord.y > 0.0f) && (T.x > 0.0f && T.y > 0.0f);
	}


	/************************************************************************************************/
	

	struct FormattingState
	{
		float2	CurrentPosition;
		float2	End;

		float RowEnd;
		float ColumnBegin;
	};


	FormattingState InitiateFormattingState(FormattingOptions& Formatting, ParentInfo& P)
	{
		FormattingState	Out;
		Out.CurrentPosition = P.ClipArea_BL + float2{0.0f, P.ClipArea_WH[1] } - Formatting.CellDividerSize * float2(0.0f, 1.0f);
		Out.RowEnd			= P.ClipArea_TR[0];
		Out.ColumnBegin		= P.ClipArea_TR[1];
		Out.End				= { Out.RowEnd, P.ClipArea_BL[1] };

		return Out;
	}

	bool WillFit(float2 ElementBLeft, float2 ElementTRight, float2 AreaBLeft, float2 AreaTRight)
	{
		return (
			ElementBLeft.x >=  AreaBLeft.x  &&
			ElementBLeft.y >=  AreaBLeft.y  &&
			ElementTRight.x <  AreaTRight.x &&
			ElementTRight.y <  AreaTRight.y);
	}

	bool IncrementFormatingState(FormattingState& State, FormattingOptions& Options, float2 WH)
	{
		switch (Options.DrawDirection)
		{
		case DD_Rows:
			State.CurrentPosition += (WH * float2(1.0f, 0.0f));
			break;
		case DD_Columns:
			State.CurrentPosition += (WH * float2(0.0f,-1.0f));
			break;
		case DD_Grid: {
			auto temp = State.CurrentPosition += (WH * float2(0.0f, 1.0f));

			if (temp.x > State.End.x)
			{
				State.CurrentPosition.x = State.ColumnBegin;
				State.CurrentPosition.y += WH[1];
			}
			if (temp.y > State.End.y)
				return true;

		}	break;
		default:
			break;
		}
		return false;
	}

	float2 GetFormattedPosition(FormattingState& State, FormattingOptions& Options, float2 WH)
	{
		float2 Position = State.CurrentPosition;
		Position[1] -= WH[1];

#if 0
		switch (Options.DrawDirection)
		{
		case DD_Rows:
			break;
		case DD_Columns:
			Position[1] -= WH[1];
			break;
		case DD_Grid:
			Position[0] += WH[0];
			Position[1] += WH[1];
			break;
		default:
			break;
		}
#endif

		return Position;
	}

	void HandleWindowInput(SimpleWindowInput* Input, GUIChildList& Elements, SimpleWindow* Window, ParentInfo& P, FormattingOptions& Options = FormattingOptions())
	{
		FormattingState CurrentFormatting = InitiateFormattingState(Options, P);

		for (auto& I : Elements)
		{
			const auto& E = Window->Elements[I];

			switch (E.Type) {
			case EGE_HLIST:
			case EGE_VLIST: {
				float2 Position = GetCellPosition(Window, E.Position);
				float2 WH = GetCellDimension(Window, Window->Lists[E.Index].WH);

				ParentInfo This;
				This.ParentPosition = Position;
				This.ClipArea_BL	= Position;
				This.ClipArea_TR	= This.ClipArea_BL + WH;
				This.ClipArea_WH	= WH;

				if (CursorInside(Input->MousePosition, Position, WH)) {
					FormattingOptions Formatting;
					
					if (E.Type == EGE_HLIST)
						Formatting.DrawDirection = EDrawDirection::DD_Rows;
					else if (E.Type == EGE_VLIST)
						Formatting.DrawDirection = EDrawDirection::DD_Columns;

					HandleWindowInput(Input, Window->Lists[E.Index].Children, Window, This, Formatting);
				}

				if (IncrementFormatingState(CurrentFormatting, Options, WH))
					return;// Ran out of Space in Row/Column
			}	break;
			case EGE_BUTTON_TEXTURED: {
				auto		CurrentButton	= &Window->TexturedButtons[E.Index];
				float2		Position		= GetFormattedPosition(CurrentFormatting, Options, CurrentButton->WH);
				float2		WH				= CurrentButton->WH;
				Texture2D*	Texture			= CurrentButton->Active ? CurrentButton->Texture_Active : CurrentButton->Texture_InActive;

				if (CursorInside(Input->MousePosition, Position, WH)) {

					if (CurrentButton->OnClicked_CB && Input->LeftMouseButtonPressed)
						CurrentButton->OnClicked_CB(CurrentButton->_ptr, I);
					if (CurrentButton->OnEntered_CB && !CurrentButton->Entered)
						CurrentButton->OnEntered_CB(CurrentButton->_ptr, I);

					CurrentButton->Active  = true;
					CurrentButton->Entered = true;
				}
				else {
					if (CurrentButton->OnExit_CB && CurrentButton->Entered)
						CurrentButton->OnExit_CB(CurrentButton->_ptr, I);
					CurrentButton->Entered = false;
				}

				if (!WillFit(Position, Position + WH, P.ClipArea_BL, P.ClipArea_TR))
					return;

				if (IncrementFormatingState(CurrentFormatting, Options, WH))
					return;// Ran out of Space in Row/Column
			}	break;
			case EGE_BUTTON_TEXT: {
				auto		CurrentButton	= &Window->TextButtons[E.Index];
				float2		Position		= GetFormattedPosition(CurrentFormatting, Options, CurrentButton->WH);
				float2		WH				= CurrentButton->WH;

				if (CursorInside(Input->MousePosition, Position, WH)) {

					if (CurrentButton->OnClicked_CB && Input->LeftMouseButtonPressed)
						CurrentButton->OnClicked_CB(CurrentButton->CB_Args, I);
					if (CurrentButton->OnEntered_CB && !CurrentButton->Entered)
						CurrentButton->OnEntered_CB(CurrentButton->CB_Args, I);

					CurrentButton->Active  = true;
					CurrentButton->Entered = true;
				}
				else {
					if (CurrentButton->OnExit_CB && CurrentButton->Entered)
						CurrentButton->OnExit_CB(CurrentButton->CB_Args, I);
					CurrentButton->Entered = false;
				}

				if (!WillFit(Position, Position + WH, P.ClipArea_BL, P.ClipArea_TR))
					return;

				if (IncrementFormatingState(CurrentFormatting, Options, WH))
					return;// Ran out of Space in Row/Column
			}	break;
			case EGE_DIVIDER: {
				auto DivSize = Window->Dividers[E.Index].Size;
				if (IncrementFormatingState(CurrentFormatting, Options, DivSize))
					return;// Ran out of Space in Row/Column
			}	break;
			default:
				break;
			}
		}
	}

	void DrawChildren(GUIChildList& Elements, SimpleWindow* Window, GUIRender* Out, ParentInfo& P, FormattingOptions& Options)
	{
		FormattingState CurrentFormatting = InitiateFormattingState(Options, P);

		for (auto& I : Elements)
		{
			const auto& E = Window->Elements[I];

			switch (E.Type) {
			case EGE_HLIST:
			case EGE_VLIST: {
				float2 Position = GetCellPosition(Window, E.Position);
				float2 WH       = GetCellDimension(Window, Window->Lists[E.Index].WH);

				Draw_RECT_CLIPPED	Panel;
				Panel.BLeft           = Position;
				Panel.TRight          = Panel.BLeft + WH;
				Panel.Color           = Window->Lists[E.Index].Color;
				Panel.CLIPAREA_BLEFT  = P.ClipArea_BL;
				Panel.CLIPAREA_TRIGHT = P.ClipArea_TR;

				PushRect(Out, Panel);

				ParentInfo This;
				This.ParentPosition = Position;
				This.ClipArea_BL	= Position;
				This.ClipArea_TR	= This.ClipArea_BL + WH;// + Window->CellBorder;
				This.ClipArea_WH	= WH;

				FormattingOptions Formatting;
				Formatting.CellDividerSize = Window->CellBorder;

				if (E.Type == EGE_HLIST) 
					Formatting.DrawDirection = EDrawDirection::DD_Rows;
				else if (E.Type == EGE_VLIST)
					Formatting.DrawDirection = EDrawDirection::DD_Columns;

				DrawChildren(Window->Lists[E.Index].Children, Window, Out, This, Formatting);

				if (IncrementFormatingState(CurrentFormatting, Options, WH))
					return;// Ran out of Space in Row/Column
			}	break;
			case EGE_BUTTON_TEXTURED: {
				auto		CurrentButton	= &Window->TexturedButtons[E.Index];
				float2		Position		= GetFormattedPosition(CurrentFormatting, Options, CurrentButton->WH);
				float2		WH				= CurrentButton->WH;
				Texture2D*	Texture			= CurrentButton->Active ?  CurrentButton->Texture_Active : CurrentButton->Texture_InActive;

				CurrentButton->Active		= false;

				Draw_TEXTURED_RECT NewButton;
				NewButton.TextureHandle = Texture;
				NewButton.BLeft         = Position;//Position;
				NewButton.Color         = float4(WHITE, 1);
				NewButton.TRight        = Position + WH;

				if (!WillFit(NewButton.BLeft, NewButton.TRight, P.ClipArea_BL, P.ClipArea_TR))
					return;

				PushRect(Out, NewButton);

				if(IncrementFormatingState(CurrentFormatting, Options, WH))
					return;// Ran out of Space in Row/Column
			}	break;
			case EGE_BUTTON_TEXT: {
				auto		CurrentButton	= &Window->TextButtons[E.Index];
				float2		Position		= GetFormattedPosition(CurrentFormatting, Options, CurrentButton->WH);
				float2		WH				= CurrentButton->WH;

				//CurrentButton->Text.Position	= Position;
				CurrentButton->Active				= false;
				CurrentButton->Text.ScreenPosition	= Position + WH * float2{0, 1.0f};

				Draw_RECT NewButton;
				NewButton.BLeft     = Position;//Position;
				NewButton.Color     = float4(RED, 1);
				NewButton.TRight    = Position + WH;

				Draw_TEXT Text;
				Text.Font			 =  CurrentButton->Font;
				Text.Text			 = &CurrentButton->Text;
				Text.CLIPAREA_BLEFT  =  Position;
				Text.CLIPAREA_TRIGHT =  Position + WH;
				Text.Color			 =  float4(GREEN, 1);

				if (!WillFit(Text.CLIPAREA_BLEFT, Text.CLIPAREA_TRIGHT, P.ClipArea_BL, P.ClipArea_TR))
					return;

				PushRect(Out, NewButton);
				PushText(Out, Text);

				if (IncrementFormatingState(CurrentFormatting, Options, WH))
					return;// Ran out of Space in Row/Column
			}	break;
			case EGE_DIVIDER: {
				auto DivSize = Window->Dividers[E.Index].Size;
				if (IncrementFormatingState(CurrentFormatting, Options, DivSize))
					return;// Ran out of Space in Row/Column
			}	break;
			default:
				break;
			}
		}
	}


	/************************************************************************************************/


	bool CursorInsideCell(SimpleWindow* Window, float2 CursorPos, uint2 Cell)
	{
		auto CellPOS = GetCellPosition	(Window, Cell);
		auto CellWH  = GetCellDimension	(Window);

		return CursorInside(CursorPos, CellPOS, CellWH);
	}


	/************************************************************************************************/


	void UpdateSimpleWindow(SimpleWindowInput* Input, SimpleWindow* Window)
	{
		ParentInfo P;
		P.ClipArea_BL = Window->Position;
		P.ClipArea_TR = Window->Position + Window->WH;
		P.ParentPosition = Window->Position;

		HandleWindowInput(Input, Window->Root, Window, P);
	}

	void DrawSimpleWindow(SimpleWindowInput Input, SimpleWindow* Window, GUIRender* Out)
	{
		//Window->Position = Input.MousePosition;

		{
			Draw_RECT	BackPanel;
			BackPanel.BLeft		= Window->Position;
			BackPanel.TRight	= Window->Position + Window->WH;
			BackPanel.Color		= float4(Gray(0.35f), 1);

			PushRect(Out, BackPanel);
		}

		uint32_t ColumnCount = Window->ColumnCount;
		uint32_t RowCount	 = Window->RowCount;

		const float WindowWidth  = Window->WH[0];
		const float WindowHeight = Window->WH[1];

		const float CellGapWidth  = Window->CellBorder[0];
		const float CellGapHeight = Window->CellBorder[1];

		const float CellWidth	= (WindowWidth  / ColumnCount)	- CellGapWidth  - CellGapWidth  / ColumnCount;
		const float CellHeight	= (WindowHeight / RowCount)		- CellGapHeight - CellGapHeight / RowCount;

		for (uint32_t I = 0; I < Window->ColumnCount; ++I){
			for (uint32_t J = 0; J < Window->RowCount; ++J)	{
				if (!GetCellState({I, J}, Window))
				{
					Draw_RECT BackPanel;
					//BackPanel.BLeft		= Window->Position + float2{ CellGapWidth + (CellGapWidth + CellWidth) * I, CellGapHeight + (CellGapHeight + CellHeight) * J };
					BackPanel.BLeft  = Window->Position + float2{ (CellWidth + CellGapWidth) * I + CellGapWidth, (CellHeight + CellGapHeight) * J + CellGapHeight };
					BackPanel.TRight = BackPanel.BLeft  + float2{ CellWidth	, CellHeight };
					
					if (Input.LeftMouseButtonPressed && CursorInside(Input.MousePosition, BackPanel.BLeft, float2{ CellWidth, CellHeight }))
						BackPanel.Color = float4(Gray(1.0f), 1);
					else if (CursorInside(Input.MousePosition, BackPanel.BLeft, float2{ CellWidth	, CellHeight }))
						BackPanel.Color = float4(Gray(0.75f), 1);
					else
						BackPanel.Color = float4(Gray(0.45f), 1);

					PushRect(Out, BackPanel);
				}
			}
		}
		
		ParentInfo P;
		P.ClipArea_BL		= Window->Position;
		P.ClipArea_TR		= Window->Position + Window->WH;
		P.ParentPosition	= Window->Position;
		DrawChildren(Window->Root, Window, Out, P);

		float AspectRatio = Window->WH[0] / Window->WH[1];

		FlexKit::Draw_RECT	TestCursor;
		TestCursor.BLeft	= Input.MousePosition + float2{ 0.0f, -0.005f };
		TestCursor.TRight	= Input.MousePosition + float2{ 0.005f / AspectRatio, 0.005f };
		TestCursor.Color	= float4(GREEN, 1);
		PushRect(Out, TestCursor);
	}


	/************************************************************************************************/


	bool GameWindow_PushElement(SimpleWindow* Window, size_t Target, GUIElement E)
	{
		switch (Window->Elements[Target].Type)
		{
		case EGE_HLIST:
		case EGE_VLIST:
			Window->Lists[Window->Elements[Target].Index].Children.push_back(Window->Elements.size());// > Window->Elements[Parent].Index Window -
			return true;
		default:
			break;
		}
		return false;
	}


	/************************************************************************************************/


	int32_t GetCellState(uint2 Cell, SimpleWindow* W)
	{
		return W->CellState[Cell[0] * W->RowCount + Cell[1]];
	}

	void SetCellState(uint2 Cell, int32_t S, SimpleWindow* W)
	{
		W->CellState[Cell[0] * W->RowCount + Cell[1]] = S;
	}

	void SetCellState(uint2 Cell, uint2 WH, int32_t S, SimpleWindow* W)
	{
		for (size_t I = 0; I < WH[0]; ++I)
			for (size_t J = 0; J < WH[1]; ++J) 
				SetCellState(Cell + uint2{I, J}, S, W);
	}


	/************************************************************************************************/


	bool RoomAvailable(SimpleWindow* W, uint2 POS, uint2 WH, int32_t ID) {
		for (size_t I = 0; I < WH[0]; ++I) {
			for (size_t J = 0; J < WH[0]; ++J) {
				if (!GetCellState(POS + uint2{ I, J }, W))
					return false;
			}
		}

		return true;
	}

	size_t SimpleWindowAddVerticalList(SimpleWindow* Window, GUIList& Desc, size_t Parent)
	{
		GUIElement		 E;
		GUIElement_List  P;

		E.Type               = EGE_VLIST;
		E.Position           = Desc.Position;
		E.Index              = Window->Lists.size();

		P.WH                 = Desc.WH;
		P.Color              = Desc.Color;
		P.Children.Allocator = Window->Memory;

		bool Push = false;
		if (Parent == -1) 
		{
			FK_ASSERT(!RoomAvailable(Window, Desc.Position, Desc.WH, Parent));

			Window->Root.push_back(Window->Elements.size());
			Push = true;
		}
		else
			Push = GameWindow_PushElement(Window, Parent, E);

		if (Push) {
			Window->Elements.push_back(E);
			Window->Lists.push_back(P);
			SetCellState(E.Position, P.WH, Window->Elements.size(), Window);
		}

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	size_t SimpleWindowAddHorizonalList(SimpleWindow* Window, GUIList& Desc, size_t Parent)
	{
		GUIElement		E;
		GUIElement_List P;

		E.Type               = EGE_HLIST;
		E.Position           = Desc.Position;
		E.Index              = Window->Lists.size();

		P.WH                 = Desc.WH;
		P.Color              = Desc.Color;
		P.Children.Allocator = Window->Memory;

		bool Push = false;
		if (Parent == -1) 
		{
			FK_ASSERT(!RoomAvailable(Window, Desc.Position, Desc.WH, Parent));

			Window->Root.push_back(Window->Elements.size());
			Push = true;
		}
		else
			Push = GameWindow_PushElement(Window, Parent, E);

		if (Push) {
			Window->Elements.push_back(E);
			Window->Lists.push_back(P);
			SetCellState(E.Position, P.WH, true, Window);
		}

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	size_t SimpleWindowAddTexturedButton(SimpleWindow* Window, GUITexturedButton_Desc&	Desc, size_t Parent)
	{
		GUIElement					E;
		GUIElement_TexuredButton	TB;

		E.Position	= 0;// Unused here
		E.Type		= EGE_BUTTON_TEXTURED;
		E.Index		= Window->TexturedButtons.size();

		TB.Texture_Active	= Desc.Texture_Active;
		TB.Texture_InActive = Desc.Texture_InActive;
		TB.WH				= Desc.WH;
		TB.OnClicked_CB		= Desc.OnClicked_CB;
		TB.OnEntered_CB		= Desc.OnEntered_CB;
		TB.OnExit_CB		= Desc.OnExit_CB;

		FK_ASSERT(Parent != -1); // Must be a child of a panel or other Element

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->TexturedButtons.push_back(TB);

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	size_t SimpleWindowAddTextButton(SimpleWindow*	Window, GUITextButton_Desc& Desc, RenderSystem* RS, size_t Parent)
	{
		TextArea_Desc TA_Desc = { 
			{ 0, 0 },	// Position Will be set later
			{ Desc.WindowSize * float2{float(Desc.WH[0]), float(Desc.WH[1]) }},
			{ 16, 16 } 
		};

		auto Text = CreateTextArea(RS, Window->Memory, &TA_Desc);
		PrintText(&Text, Desc.Text);

		GUIElement			  E;
		GUIElement_TextButton TB;
		E.Type			= EGE_BUTTON_TEXT;
		E.Index			= Window->TextButtons.size();;
		E.Position		= 0;// Unused here

		TB.OnClicked_CB = Desc.OnClicked_CB;
		TB.OnEntered_CB = Desc.OnEntered_CB;
		TB.OnExit_CB    = Desc.OnExit_CB;
		TB.CB_Args		= Desc.CB_Args;
		TB.Text			= Text;
		TB.WH			= Desc.WH;
		TB.Font			= Desc.Font;

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->TextButtons.push_back(TB);

		return Window->Elements.size() - 1;
	}


	/************************************************************************************************/


	void SimpleWindowAddDivider(SimpleWindow* Window, float2 Size, size_t Parent)
	{
		GUIElement	   E;
		GUIElement_Div D;

		E.Position	= 0;// Unused here
		E.Type		= EGE_DIVIDER;
		E.Index		= Window->Dividers.size();
		D.Size		= Size;

		bool Push = false;
		FK_ASSERT(Parent != -1); // Must be a child of a panel or other Element

		GameWindow_PushElement(Window, Parent, E);

		Window->Elements.push_back(E);
		Window->Dividers.push_back(D);
	}


	/************************************************************************************************/


}// namespace FlexKit