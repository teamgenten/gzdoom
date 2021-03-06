/*
** warptexture.cpp
** Texture class for warped textures
**
**---------------------------------------------------------------------------
** Copyright 2004-2006 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include "doomtype.h"
#include "files.h"
#include "templates.h"
#include "r_utility.h"
#include "textures/textures.h"
#include "warpbuffer.h"
#include "v_palette.h"


FWarpTexture::FWarpTexture (FTexture *source, int warptype)
: GenTime (0), GenTimeBgra(0), Speed (1.f), SourcePic (source), Pixels (0), Spans (0)
{
	CopyInfo(source);
	if (warptype == 2) SetupMultipliers(256, 128); 
	SetupMultipliers(128, 128); // [mxd]
	bWarped = warptype;
}

FWarpTexture::~FWarpTexture ()
{
	Unload ();
	if (Spans != NULL)
	{
		FreeSpans (Spans);
		Spans = NULL;
	}
	delete SourcePic;
}

void FWarpTexture::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
	if (Spans != NULL)
	{
		FreeSpans (Spans);
		Spans = NULL;
	}
	SourcePic->Unload ();
	FTexture::Unload();
}

bool FWarpTexture::CheckModified ()
{
	return r_viewpoint.FrameTime != GenTime;
}

const uint8_t *FWarpTexture::GetPixels ()
{
	uint32_t time = r_viewpoint.FrameTime;

	if (Pixels == NULL || time != GenTime)
	{
		MakeTexture (time);
	}
	return Pixels;
}

const uint32_t *FWarpTexture::GetPixelsBgra()
{
	uint32_t time = r_viewpoint.FrameTime;
	if (Pixels == NULL || time != GenTime)
		MakeTexture(time);

	if (PixelsBgra.empty() || time != GenTimeBgra)
	{
		CreatePixelsBgraWithMipmaps();
		for (int i = 0; i < Width * Height; i++)
		{
			if (Pixels[i] != 0)
				PixelsBgra[i] = 0xff000000 | GPalette.BaseColors[Pixels[i]].d;
			else
				PixelsBgra[i] = 0;
		}
		GenerateBgraMipmapsFast();
		GenTimeBgra = time;
	}
	return PixelsBgra.data();
}

const uint8_t *FWarpTexture::GetColumn (unsigned int column, const Span **spans_out)
{
	uint32_t time = r_viewpoint.FrameTime;

	if (Pixels == NULL || time != GenTime)
	{
		MakeTexture (time);
	}
	if ((unsigned)column >= (unsigned)Width)
	{
		if (WidthMask + 1 == Width)
		{
			column &= WidthMask;
		}
		else
		{
			column %= Width;
		}
	}
	if (spans_out != NULL)
	{
		if (Spans == NULL)
		{
			Spans = CreateSpans (Pixels);
		}
		*spans_out = Spans[column];
	}
	return Pixels + column*Height;
}


void FWarpTexture::MakeTexture(uint32_t time)
{
	const uint8_t *otherpix = SourcePic->GetPixels();

	if (Pixels == NULL)
	{
		Pixels = new uint8_t[Width * Height];
	}
	if (Spans != NULL)
	{
		FreeSpans(Spans);
		Spans = NULL;
	}

	GenTime = time;
	WarpBuffer(Pixels, otherpix, Width, Height, WidthOffsetMultiplier, HeightOffsetMultiplier, time, Speed, bWarped);
}

// [mxd] Non power of 2 textures need different offset multipliers, otherwise warp animation won't sync across texture
void FWarpTexture::SetupMultipliers (int width, int height)
{
	WidthOffsetMultiplier = width;
	HeightOffsetMultiplier = height;
	int widthpo2 = NextPo2(Width);
	int heightpo2 = NextPo2(Height);
	if(widthpo2 != Width) WidthOffsetMultiplier = (int)(WidthOffsetMultiplier * ((float)widthpo2 / Width));
	if(heightpo2 != Height) HeightOffsetMultiplier = (int)(HeightOffsetMultiplier * ((float)heightpo2 / Height));
}

int FWarpTexture::NextPo2 (int v)
{
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	return ++v;
}

//==========================================================================
//
// FMultiPatchTexture :: TexPart :: TexPart
//
//==========================================================================

FTexture *FWarpTexture::GetRedirect(bool wantwarped)
{
	if (!wantwarped) return SourcePic->GetRedirect(false);
	else return this;
}

//==========================================================================
//
// FMultiPatchTexture :: CopyTrueColorPixels
//
// This doesn't warp. It's just here in case someone tries to use a warp
// texture for compositing a multipatch texture
//
//==========================================================================

int FWarpTexture::CopyTrueColorPixels(FBitmap *bmp, int x, int y, int rotate, FCopyInfo *inf)
{
	return SourcePic->CopyTrueColorPixels(bmp, x, y, rotate, inf);
}

