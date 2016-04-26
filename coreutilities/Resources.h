/**********************************************************************

Copyright (c) 2014-2016 Robert May

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

#ifndef Resources_H
#define Resources_H

#include "..\buildsettings.h"
#include "..\coreutilities\containers.h"
#include "..\coreutilities\memoryutilities.h"
#include "..\graphicsutilities\graphics.h"
#include "..\graphicsutilities\AnimationUtilities.h"


/************************************************************************************************/


namespace FlexKit
{
	typedef size_t GUID_t;
	static const size_t ID_LENGTH = 64;

	enum EResourceType : size_t
	{
		EResource_Font,
		EResource_GameDB,
		EResource_Skeleton,
		EResource_SkeletalAnimation,
		EResource_Shader,
		EResource_Scene,
		EResource_TriMesh,
		EResource_Texture,
	};

	struct Resource
	{
		size_t			ResourceSize;
		EResourceType	Type;

	#pragma warning(disable:4200)

		GUID_t	GUID;
		enum	ResourceState : uint32_t
		{
			EResourceState_UNLOADED,
			EResourceState_LOADING,
			EResourceState_LOADED,
			EResourceState_EVICTED,
		};
		ResourceState	State;
		uint32_t		RefCount;

		char	ID[ID_LENGTH];
		char	Memory[];

	private:
		Resource();
	};

	/************************************************************************************************/


	struct ResourceEntry
	{
		GUID_t					GUID;
		size_t					ResourcePosition;
		char*					ResouceLOC; // Not Used in File
		EResourceType			Type;
		char					ID[ID_LENGTH];
	};

	struct ResourceTable
	{
		size_t			MagicNumber;
		size_t			Version;
		size_t			ResourceCount;
		ResourceEntry	Entries[];
	};

	typedef size_t ResourceHandle;


	/************************************************************************************************/

	
	struct Resources
	{
		struct DIR
		{
			char str[256];
		};
		BlockAllocator*						ResourceMemory;
		static_vector<ResourceTable*, 16>	Tables;
		static_vector<DIR, 16>				ResourceFiles;
		static_vector<Resource*, 256>		ResourcesLoaded;
		static_vector<GUID_t, 256>			ResourceGUIDs;
	};


	/************************************************************************************************/


	FLEXKITAPI size_t		ReadResourceTableSize	(FILE* F);
	FLEXKITAPI size_t		ReadResourceSize		(FILE* F, ResourceTable* Table, size_t Index);

	FLEXKITAPI void			AddResourceFile		(char* FILELOC, Resources* RM);
	FLEXKITAPI Resource*	GetResource			(Resources* RM, ResourceHandle RHandle);

	FLEXKITAPI bool			ReadResourceTable	(FILE* F, ResourceTable* Out, size_t TableSize);
	FLEXKITAPI bool			ReadResource		(FILE* F, ResourceTable* Table, size_t Index, Resource* out);

	FLEXKITAPI ResourceHandle LoadGameResource	(Resources* RM, const char* ID);
	FLEXKITAPI ResourceHandle LoadGameResource	(Resources* RM, GUID_t GUID, EResourceType T);

	FLEXKITAPI void FreeResource			(Resources* RM, ResourceHandle RHandle);
	FLEXKITAPI void FreeAllResources		(Resources* RM);
	FLEXKITAPI void FreeAllResourceFiles	(Resources* RM);

	FLEXKITAPI bool isResourceAvailable	(Resources* RM, GUID_t ID, EResourceType Type);
	FLEXKITAPI bool isResourceAvailable	(Resources* RM, const char* ID);


	/************************************************************************************************/


	const size_t GUIDMASK		= 0x00000000FFFFFFFF;
	const GUID_t INVALIDHANDLE	= 0xFFFFFFFFFFFFFFFF;

	/************************************************************************************************/


	struct TriMeshResourceBlob
	{
		#pragma warning(disable:4200)

		size_t			ResourceSize;
		EResourceType	Type;
		GUID_t GUID;
		size_t Pad;

		char ID[FlexKit::ID_LENGTH];
		bool HasAnimation;
		bool HasIndexBuffer;
		size_t BufferCount;
		size_t IndexCount;
		
		struct RInfo
		{
			float minx; 
			float miny; 
			float minz;
			float maxx;
			float maxy;
			float maxz;
			float r;
			byte   _PAD[12];
		}Info;

		uint32_t	Pad_2;

		struct Buffer
		{
			uint16_t Format;
			uint16_t Type;
			size_t	 Begin;
			size_t	 size;
		}Buffers[16];
		char Memory[];
	};


	/************************************************************************************************/


	struct SkeletonResourceBlob
	{
		#pragma warning(disable:4200)

		size_t ResourceSize;
		EResourceType Type;
		size_t GUID;
		size_t Pad;
		char ID[FlexKit::ID_LENGTH];

		struct JointEntry
		{
			FlexKit::float4x4		IPose;
			FlexKit::JointPose		Pose;
			FlexKit::JointHandle	Parent;
			uint16_t				Pad;
			char					ID[64];
		};

		size_t JointCount;
		JointEntry Joints[];
	};


	/************************************************************************************************/


	struct AnimationResourceBlob
	{
		size_t			ResourceSize;
		EResourceType	Type;

	#pragma warning(disable:4200)

		GUID_t	GUID;
		enum	ResourceState : uint32_t
		{
			EResourceState_UNLOADED,
			EResourceState_LOADING,
			EResourceState_LOADED,
			EResourceState_EVICTED,
		};

		ResourceState	State;
		uint32_t		RefCount;

		char   ID[FlexKit::ID_LENGTH];

		GUID_t Skeleton;
		size_t FrameCount;
		size_t FPS;
		bool   IsLooping;

		struct FrameEntry
		{
			size_t JointCount;
			size_t PoseCount;
			size_t JointStarts;
			size_t PoseStarts;
		};

		char	Buffer[];
	};


	/************************************************************************************************/


	FLEXKITAPI AnimationClip	Resource2AnimationClip(Resource* R, BlockAllocator* Memory);
	FLEXKITAPI Skeleton*		Resource2Skeleton(Resources* RM, ResourceHandle RHandle, BlockAllocator* Memory);
	FLEXKITAPI TriMesh*			Resource2TriMesh(RenderSystem* RS, Resources* RM, ResourceHandle RHandle, BlockAllocator* Memory, ShaderSetHandle SH, ShaderTable* ST, bool ClearBuffers = true);


	/************************************************************************************************/
}

#endif