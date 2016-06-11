/**********************************************************************

Copyright (c) 2015-2016 Robert May

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

struct Plane
{
	float4 Normal;
	float4 Orgin;
};

cbuffer CameraConstants : register( b0 )
{
	float4x4 View;
	float4x4 Proj;
	float4x4 CameraWT;			// World Transform
	float4x4 PV;				// Projection x View
	float4x4 CameraInverse;
	float4   CameraPOS;
	uint  	 MinZ;
	uint  	 MaxZ;
	int 	 PointLightCount;
	int 	 SpotLightCount;
};


cbuffer LocalConstants : register( b1 )
{
	float4	 Albedo; // + roughness
	float4	 Specular;
	float4x4 WT;
}

struct VIN
{
	float4 POS : POSITION;
};

struct VOUT
{
	float4 POS 	: SV_POSITION;
	float4 WPOS : POSITION;
};

VOUT VMain( VIN In )
{
	VOUT Out;
	Out.POS 	= mul(PV, mul(WT, In.POS)); 
	//Out.POS 	= mul(Proj, mul(View, mul(WT, In.POS)));
	Out.WPOS 	= mul(WT, In.POS);

	return Out;
}

struct VIN2
{
	float4 POS 		: POSITION;
	float4 N	 	: NORMAL;
};

struct PS_IN
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float4 POS 	 	: SV_POSITION;
};

PS_IN V2Main( VIN2 In )
{
	float3x3 rot = WT;
	PS_IN Out;
	In.POS.w = 1;

	Out.WPOS 	= mul(WT, In.POS);
	Out.POS 	= In.POS;
	Out.POS 	= mul(PV, mul(WT, In.POS));
	//Out.POS 	= mul(Proj, mul(View, mul(WT, In.POS)));
	Out.N   	= float4(normalize(mul(rot, In.N)), Out.POS.w);
	
	return Out;
}

struct VIN3
{
	float4 POS 		: POSITION;
	float4 N	 	: NORMAL;
	float3 T 		: TANGENT;
};

struct PS_IN2
{
	float3 WPOS 	: TEXCOORD0;
	float4 N 		: TEXCOORD1;
	float3 T 		: TEXCOORD2;
	float3 B 		: TEXCOORD3;
	float4 POS 	 	: SV_POSITION;
};

PS_IN2 V3Main( VIN3 In )
{
	float3x3 rot = WT;
	PS_IN2 Out;
	Out.WPOS 	= mul(WT, float4(In.POS.xyz, 1.0f));
	Out.POS 	= In.POS;
	Out.POS 	= mul(PV, mul(WT, float4(In.POS.xyz, 1.0f)));
	Out.N   	= float4(mul(rot, In.N), 0);
	Out.T   	= mul(rot, In.T);
	Out.B   	= mul(rot, cross(In.N, In.T));
	
	return Out;
}

struct Joint
{
	float4x4 I;
	float4x4 T;
};

StructuredBuffer<Joint> Bones : register(t0);

struct VIN4
{
	float4 POS 		: POSITION;
	float4 N	 	: NORMAL;
	float3 W		: WEIGHTS;
	uint4  I		: WEIGHTINDICES;
};

PS_IN VMainVertexPallet(VIN4 In)
{
	PS_IN Out;
	float3 N = float3(0.0f, 0.0f, 0.0f);
	float4 V = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 W = float4(In.W.xyz, 1 - In.W.x - In.W.y - In.W.z);
	float4 P = In.POS;

	[unroll(4)]
	for (int I = 0; I < 4; ++I)
	{
		float4x4 BoneT  = Bones[In.I[I]].T;
		float4x4 BoneI  = Bones[In.I[I]].I;
		float3x3 BoneR  = Bones[In.I[I]].T;
		float3x3 BoneIR = Bones[In.I[I]].I;

		float4 TP = mul(BoneI,  In.POS);
		float3 TN = mul(BoneIR, In.N);

		V += mul(BoneT, TP) * W[I];
		N += mul(BoneR, TN) * W[I];
	}

	float3x3 rot = WT;
	Out.WPOS	 = mul(WT, V);
	Out.POS		 = mul(PV, mul(WT, V));
	Out.N		 = float4(normalize(mul(rot, N)), Out.POS.w);

	return Out;
}
