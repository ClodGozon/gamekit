/**********************************************************************
Copyright (c) 2015 - 2017 Robert May

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

#pragma once

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\MathUtils.h"
#include "..\coreutilities\memoryutilities.h"
#include <algorithm>

namespace FlexKit
{
	// Enumerations
	enum Token : char
	{
		ERRORTOKEN,
		BEGINOBJECT,
		COMMENT,
		POSITION_COORD,
		UV_COORD,
		INDEX,
		Normal_COORD,
		WEIGHT,
		WEIGHTINDEX,
		LOADMATERIAL,
		SMOOTHING,
		PATCHBEGIN,
		PATCHEND,
		END
	};
	
	const size_t MAXSIZE = 24000;

	// Structs
	struct s_TokenVertexLayout
	{
		float f[3];
		char  padding[4];
	};

	struct s_TokenWIndexLayout
	{
		uint32_t i[4];
	};

	struct s_TokenValue
	{
		byte	buffer[31];
		Token	token;

		static s_TokenValue Empty()
		{
			s_TokenValue BLANKTOKEN;
			BLANKTOKEN.token = COMMENT;
			return BLANKTOKEN;
		}
	};

	struct CombinedVertex
	{
		CombinedVertex() : NORMAL(0), POS(0), TEXCOORD(0), WEIGHTS(0) {}
		float3	 NORMAL;
		float3	 POS;
		float3	 TEXCOORD;
		float3	 WEIGHTS;
		uint4_32 WIndices;

		struct IndexBitlayout
		{
			/*
			IndexBitlayout()
			{
				p_Index = 0;
				n_Index = 0;
				t_Index = 0;
			}

			IndexBitlayout(size_t p, size_t n, size_t t) : 
				p_Index(p),
				n_Index(n),
				t_Index(t){}

			IndexBitlayout(const IndexBitlayout& in)
			{
				n_Index = in.n_Index;
				p_Index = in.p_Index;
				t_Index = in.t_Index;
			}
			*/

			unsigned int p_Index : 24;
			unsigned int n_Index : 24;
			unsigned int t_Index : 24;
			//unsigned int padding : 32;

			size_t Hash() const 
			{ 
				size_t H = size_t(p_Index) << 40 | size_t(n_Index) << 24 | t_Index;
				return H; 
			};

			bool operator == (const IndexBitlayout in) const { return (in.Hash() == Hash()); }
			//bool operator <	 (const IndexBitlayout in) const { return  (in.Hash() < Hash()); }
			//bool operator >	 (const IndexBitlayout in) const { return !(in.Hash() < Hash()); }
		} index;

		bool operator == ( const CombinedVertex& rhs )
		{
			return eqlcompare( this, &rhs );
		}

		bool eqlcompare( const CombinedVertex * __restrict lhs, const CombinedVertex * __restrict rhs )
		{
			IndexBitlayout rhsindex = rhs->index;
			if( rhsindex.p_Index == index.p_Index )
				if( rhsindex.n_Index == index.n_Index )
					if( rhsindex.t_Index == index.t_Index )
						return true;
			return false;
		}
	};

	FLEXKITAPI bool operator < ( const CombinedVertex::IndexBitlayout lhs, const CombinedVertex::IndexBitlayout rhs );

	namespace MeshUtilityFunctions
	{
		// RETURNS FALSE ON FAILURE
		typedef fixed_vector<s_TokenValue>		TokenList;
		typedef fixed_vector<uint32_t>			IndexList;
		typedef fixed_vector<CombinedVertex>	CombinedVertexBuffer;

		struct MeshBuildInfo
		{
			size_t IndexCount;
			size_t VertexCount;
		};

		// This Function is AWFUL!!! ITS SLOW AND SUCKS AND IS ASHAMED OF ITSELF!
		FLEXKITAPI FlexKit::Pair<bool, MeshBuildInfo>	BuildVertexBuffer(const TokenList&, CombinedVertexBuffer ROUT out_buffer, IndexList ROUT out_indexes, StackAllocator& LevelSpace, StackAllocator& ScratchSpace, bool weights = false );
		FLEXKITAPI char*								ScrubLine( char* inLine, size_t RINOUT lineLength );

		namespace OBJ_Tools
		{
			struct LoaderState
			{
				LoaderState()
				{
					Color_1	= false;
					Normals	= false;
					UV_1	= false;
				}
				bool Color_1;
				bool Normals;
				bool UV_1;
			};

			typedef Pair<float3, uint4_32> WeightIndexPair;
			FLEXKITAPI void			AddVertexToken		(float3 in, TokenList& out);
			FLEXKITAPI void			AddWeightToken		(WeightIndexPair in, TokenList& out );
			FLEXKITAPI void			AddNormalToken		(float3 in, TokenList& out);
			FLEXKITAPI void			AddTexCordToken		(float3 in, TokenList& out);
			FLEXKITAPI void			AddIndexToken		(size_t V, size_t N, size_t T, TokenList& out);
			FLEXKITAPI void			AddPatchBeginToken	(TokenList& out);
			FLEXKITAPI void			AddPatchEndToken	(TokenList& out);
			FLEXKITAPI void			CStrToToken			(const char* in_Line, size_t lineLength, TokenList& out, LoaderState& State_in_out );
			FLEXKITAPI std::string	GetNextLine			(std::string::iterator& File_Position, std::string& File_Buffer );
			FLEXKITAPI void			SkipToFloat			(std::string::const_iterator& in, const std::string& in_str );
		}
	}
}