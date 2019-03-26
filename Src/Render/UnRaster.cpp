/*=============================================================================
	UnRaster.cpp: Unreal polygon rasterizer

	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.
	Compiled with Visual C++ 4.0. Best viewed with Tabs=4.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "Unreal.h"
#include "UnRender.h"
#include "UnRaster.h"

/*-----------------------------------------------------------------------------
	TRasterSetup template implementation
-----------------------------------------------------------------------------*/

//
// Compute screen bounding box of a polygon.
//
template <class TRasterSideSetupT,class TRasterLineT,class TRasterPolyT,class TThisT,class TTransformT>
inline void TRasterSetup<TRasterSideSetupT,TRasterLineT,TRasterPolyT,TThisT,TTransformT>::CalcBound
(const TTransformT *Pts, int NumPts, FScreenBounds &BoxBounds)
{
	guardSlow(TRasterSetup::CalcBound);
	const TTransformT *P;
	FLOAT MinX,MaxX,MinY,MaxY;

	MinY = MaxY = Pts[0].ScreenY;
	MinX = MaxX = Pts[0].ScreenX;

	P = &Pts [1];
	for (int i=1; i<NumPts; i++)
	{
		if	    (P->ScreenX < MinX) MinX = P->ScreenX;
		else if (P->ScreenX > MaxX) MaxX = P->ScreenX;

		if		(P->ScreenY < MinY) MinY = P->ScreenY;
		else if (P->ScreenY > MaxY) MaxY = P->ScreenY;
		P++;
	}
	BoxBounds.MinX = MinX/65536.0;
	BoxBounds.MaxX = MaxX/65536.0;
	BoxBounds.MinY = MinY;
	BoxBounds.MaxY = MaxY;
	unguardSlow;
}

//
// Setup rasterization for a set of points.
//
template <class TRasterSideSetupT,class TRasterLineT,class TRasterPolyT,class TThisT,class TTransformT>
inline void TRasterSetup<TRasterSideSetupT,TRasterLineT,TRasterPolyT,TThisT,TTransformT>::Setup
(const UCamera *Camera, const TTransformT *Pts, int NumPts, FMemStack *MemStack)
{
	guard(TRasterSetup::Setup);
	STAT(clockSlow(GStat.RasterSetup));

	const TTransformT	*P1,*P2;
	int					iTop,iBottom,Y,i,j;
	FLOAT				TopY,BottomY;
	TRasterSideSetupT	*Side,**SidePtr;

	Mem = MemStack;
	Mark.Push(*Mem);

	// Find top/bottom.
	TopY    = Pts[0].ScreenY; iTop    = 0;
	BottomY = Pts[0].ScreenY; iBottom = 0;

	P1 = &Pts [1];
	for( i=1; i<NumPts; i++ )
	{
		if		(P1->ScreenY < TopY   ) {TopY    = P1->ScreenY; iTop    = i;}
		else if (P1->ScreenY > BottomY) {BottomY = P1->ScreenY; iBottom = i;};
		P1++;
	}
	StartY = ftoi(TopY-0.5);
	EndY   = ftoi(BottomY-0.5);

	// Rasterize left side.
	i       = iTop;
	P1      = &Pts[i]; 
	Y       = StartY;
	SidePtr = &LeftSide;
	while( i != iBottom )
	{
		j			= (i>=1) ? (i-1) : (NumPts-1);
		P2			= &Pts[j];
		Side		= new(*Mem)TRasterSideSetupT;
		*SidePtr	= Side;
		SidePtr		= &Side->Next;
		Side->DY	= ftoi(P2->ScreenY-0.5) - Y;
		Side->SetupSide( P1, P2, Y );
		Y		   += Side->DY;
		P1			= P2;
		i			= j;
	}
	*SidePtr = NULL;

	// Rasterize right side.
	i		= iTop;
	P1		= &Pts[i]; 
	Y       = StartY;
	SidePtr = &RightSide;
	while( i != iBottom )
	{
		j        = ((i+1)<NumPts) ? (i+1) : 0;
		P2       = &Pts[j];
		Side     = new(*Mem)TRasterSideSetupT;
		*SidePtr = Side;
		SidePtr  = &Side->Next;
		Side->DY = ftoi(P2->ScreenY-0.5) - Y;
		Side->SetupSide( P1, P2, Y );
		P1		 = P2;
		Y       += Side->DY;
		i		 = j;
	}
	*SidePtr = NULL;

	STAT(GStat.NumSides += NumPts;)
	STAT(GStat.NumRasterPolys ++;)

	STAT(unclockSlow(GStat.RasterSetup));
	unguard;
}

//
// Setup rasterization for a set of points, utilizing a side setup cache.
// Note that we don't cache Side->DY values, since cache entries 0-3
// are reserved for automatic setup of sides clipped by the view frustrum
// planes.
//
template <class TRasterSideSetupT,class TRasterLineT,class TRasterPolyT,class TThisT,class TTransformT>
inline void TRasterSetup<TRasterSideSetupT,TRasterLineT,TRasterPolyT,TThisT,TTransformT>::SetupCached
(const UCamera *Camera, const TTransformT *Pts, int NumPts, FMemStack *TempPool, FMemStack *CachePool, TRasterSideSetupT **Cache)
{
	guard(TRasterSetup::SetupCached);
	STAT(clockSlow(GStat.RasterSetupCached));

	const TTransformT	*P1,*P2;
	int					iTop,iBottom,Y,i,j;
	FLOAT               TopY,BottomY;
	TRasterSideSetupT	*Side,**SidePtr;

	Mem = TempPool;
	Mark.Push(*Mem);

	// Find top/bottom.
	TopY = BottomY = Pts[0].ScreenY;
	iTop = iBottom = 0;

	P1       = &Pts [1];
	for( i=1; i<NumPts; i++ )
	{
		if (P1->ScreenY < TopY   ) {TopY    = P1->ScreenY; iTop    = i;};
		if (P1->ScreenY > BottomY) {BottomY = P1->ScreenY; iBottom = i;};
		P1++;
	}
	StartY = ftoi(TopY-0.5);
	EndY   = ftoi(BottomY-0.5);

	// Rasterize left side.
	Y       = TopY;
	i       = iTop;
	P1      = &Pts[i]; 
	SidePtr = &LeftSide;
	while( i != iBottom )
	{
		j			= (i>=1) ? (i-1) : (NumPts-1);
		P2			= &Pts[j];

		TRasterSideSetupT **CachePtr = &Cache[P1->iSide];
		if( P1->iSide==INDEX_NONE || *CachePtr==NULL )
		{
			if( P1->iSide==INDEX_NONE )
			{
				Side = new(*TempPool)TRasterSideSetupT;
			}
			else
			{
				Side = new(*CachePool)TRasterSideSetupT;
				*CachePtr=Side;
			}
			Side->DY = ftoi(P2->ScreenY-0.5) - Y;
			Side->SetupSide (P1,P2,Y);
		}
		else
		{
			Side		= *CachePtr;
			Side->DY	= ftoi(P2->ScreenY-0.5) - Y;
			STAT(GStat.NumSidesCached++);
		}
		*SidePtr	= Side;
		SidePtr		= &Side->Next;
		Y		   += Side->DY;
		P1			= P2;
		i			= j;
	}
	*SidePtr = NULL;

	// Rasterize right side.
	Y       = TopY;
	i		= iTop;
	P1		= &Pts[i]; 
	SidePtr = &RightSide;
	while( i != iBottom )
	{
		j        = ((i+1)<NumPts) ? (i+1) : 0;
		P2       = &Pts[j];

		TRasterSideSetupT **CachePtr = &Cache[P2->iSide];
		if( P2->iSide==INDEX_NONE || *CachePtr==NULL )
		{
			if ( P2->iSide==INDEX_NONE )
			{
				Side = new(*TempPool)TRasterSideSetupT;
			}
			else
			{
				Side = new(*CachePool)TRasterSideSetupT;
				*CachePtr=Side;
			}
			Side->DY = ftoi(P2->ScreenY-0.5) - Y;
			Side->SetupSide( P1, P2, Y );
		}
		else
		{
			Side		= *CachePtr;
			Side->DY	= ftoi(P2->ScreenY-0.5) - Y;
			STAT(GStat.NumSidesCached++);
		}
		*SidePtr = Side;
		SidePtr  = &Side->Next;
		P1		 = P2;
		Y       += Side->DY;
		i		 = j;
	}
	*SidePtr = NULL;

	STAT(GStat.NumSides += NumPts;)
	STAT(GStat.NumRasterPolys ++;)

	STAT(unclockSlow(GStat.RasterSetupCached));
	unguard;
}

//
// Generate a rasterization that has been set up already.
//
template <class TRasterSideSetupT,class TRasterLineT,class TRasterPolyT,class TThisT,class TTransformT>
inline void TRasterSetup<TRasterSideSetupT,TRasterLineT,TRasterPolyT,TThisT,TTransformT>::Generate
(TRasterPolyT *Raster) const
{
	guard(TRasterSetup::Generate);
	STAT(clockSlow(GStat.RasterGenerate));

	Raster->StartY = StartY;
	Raster->EndY   = EndY;

	TRasterSideSetupT *Side = LeftSide;
	for ( int c=0; c<2; c++ )
	{
		TRasterLineT *Line = &Raster->Lines[0];
		while(Side)
		{
			Side->GenerateSide( Line, c, Side->DY );
			Line += Side->DY;
			Side  = Side->Next;
		}
		Side = RightSide;
	}
	STAT(unclockSlow(GStat.Generate));
	unguardf(("(Start=%i, End=%i)",StartY,EndY));
}

/*-----------------------------------------------------------------------------
	TRasterPoly template implementation
-----------------------------------------------------------------------------*/

//
// Force a rasterized polygon to be forward-faced.  If you don't call this, a
// rasterized backfaced polygon will have its end before its start and won't be drawn.
//
template <class TRasterLineT,class TRasterPointT>
inline void TRasterPoly<TRasterLineT,TRasterPointT>::ForceForwardFace 
()
{
	guardSlow(TRasterPoly::ForceForwardFace);
	TRasterLineT *Line = &Lines[0];
	for( int i=StartY; i<EndY; i++ )
	{
		if( Line->IsBackwards() )
		{
			TRasterPointT Temp	= Line->Start;
			Line->Start			= Line->End;
			Line->End 			= Temp;
		}
		Line++;
	}
	unguardSlow;
}

//
// Draw this flat-shaded.
//
template <class TRasterLineT,class TRasterPointT>
inline void TRasterPoly<TRasterLineT,TRasterPointT>::DrawFlat
(
	const UCamera*	Camera,
	BYTE			Color,
	FSpanBuffer*	SpanBuffer
)
{
	guard(TRasterPoly::DrawFlat);

	// Draw all spans.
	if( SpanBuffer )
	{
		// Draw with span clipping if span buffer is non-empty.
		if( SpanBuffer->ValidLines )
		{
			// Figure out the region we want to draw.
			int NewStartY = Max(StartY,SpanBuffer->StartY);
			int NewEndY   = Min(EndY,  SpanBuffer->EndY);

			// Set up.
			const TRasterLineT	*Line	= &Lines [NewStartY-StartY];
			BYTE				*Dest	= Camera->Screen + NewStartY * Camera->ByteStride;
			FSpan				**Index	= &SpanBuffer->Index[NewStartY-SpanBuffer->StartY];

			// Draw all spans.
			for( int i=NewStartY; i<NewEndY; i++ )
			{
				FSpan *ThisSpan = *Index++;
				if( ThisSpan )
				{
					Line->DrawFlatSpanClipped( Camera,Dest,Color,ThisSpan );
				}
				Dest += Camera->ByteStride;
				Line++;
			}
		}
	}
	else
	{
		// Not span clipped.
		const TRasterLineT	*Line = &Lines [0];
		BYTE				*Dest = Camera->Screen + StartY * Camera->ByteStride;

		for( int i=StartY; i<EndY; i++ )
		{
			Line->DrawFlatSpan( Camera,Dest,Color );
			Dest += Camera->ByteStride;
			Line++;
		}
	}
	unguard;
}

//
// Draw this using the specified polygon setup info.
//
template <class TRasterLineT,class TRasterPointT>
inline void TRasterPoly<TRasterLineT,TRasterPointT>::Draw
(
	const UCamera*				Camera,
	FPolySpanTextureInfoBase&	Info,
	FSpanBuffer*				SpanBuffer
)
{
	guard(TRasterPoly::Draw);
	if( SpanBuffer )
	{
		// Draw with span clipping if span buffer is non-empty.
		if( SpanBuffer->ValidLines )
		{
			// Figure out the region we want to draw.
			int NewStartY = Max(StartY,SpanBuffer->StartY);
			int NewEndY   = Min(EndY,  SpanBuffer->EndY);

			// Set up.
			BYTE			*Dest				= Camera->Screen + NewStartY * Camera->ByteStride;
			TRasterLineT	*Line,*Line1		= &Lines [NewStartY-StartY];
			FSpan			**Index,**Index1	= &SpanBuffer->Index[NewStartY-SpanBuffer->StartY];

			// Set up all spans.
			Index = Index1;
			Line  = Line1;
			for( int i=NewStartY; i<NewEndY; i++ )
			{
				if( *Index++ )
				{
					Line->SetupSpan(Info);
				}
				Line++;
			}

			// Draw all spans.
			Index = Index1;
			Line  = Line1;
			for( i=NewStartY; i<NewEndY; i++ )
			{
				FSpan *ThisSpan = *Index++;
				if( ThisSpan )
				{
					Line->DrawSpanClipped(Camera,Dest,Info,i,ThisSpan);
				}
				Dest += Camera->ByteStride;
				Line++;
			}
		}
	}
	else
	{
		// Draw without span clipping.
		BYTE			*Dest        = Camera->Screen + StartY * Camera->ByteStride;
		TRasterLineT	*Line,*Line1 = &Lines [0];

		// Set up all spans.
		Line = Line1;
		for( int i=StartY; i<EndY; i++ )
		{
			Line->SetupSpan(Info);
			Line++;
		}

		// Draw all spans.
		Line = Line1;
		for( i=StartY; i<EndY; i++ )
		{
			Line->DrawSpan(Camera,Dest,Info,i);
			Dest += Camera->ByteStride;
			Line++;
		}
	}
	unguard;
}

/*-----------------------------------------------------------------------------
	Forced template instantiation
-----------------------------------------------------------------------------*/

template void TRasterPoly
	<
	class FRasterLine,
	class FRasterPoint
	>::ForceForwardFace();
template void TRasterPoly
	<
	class FRasterLine,
	class FRasterPoint
	>::DrawFlat(const UCamera *Camera,BYTE Color,FSpanBuffer *SpanBuffer);
template void TRasterPoly
	<
	class FRasterLine,
	class FRasterPoint
	>::Draw(const UCamera *Camera,FPolySpanTextureInfoBase &Info,FSpanBuffer *SpanBuffer);
template void TRasterSetup
	<
	class FRasterSideSetup,
	class FRasterLine,
	class FRasterPoly,
	class FRasterSetup,
	class FTransform
	>::Setup(const UCamera *Camera,const FTransform *Pts, int NumPts, FMemStack *Mem);
template void TRasterSetup
	<
	class FRasterSideSetup,
	class FRasterLine,
	class FRasterPoly,
	class FRasterSetup,
	class FTransform
	>::SetupCached(const UCamera *Camera,const FTransform *Pts, int NumPts, FMemStack *TempPool, FMemStack *CachePool, FRasterSideSetup **Cache);
template void TRasterSetup
	<
	class FRasterSideSetup,
	class FRasterLine,
	class FRasterPoly,
	class FRasterSetup,
	class FTransform
	>::CalcBound(const FTransform *Pts, int NumPts, FScreenBounds &Bounds);
template void TRasterSetup
	<
	class FRasterSideSetup,
	class FRasterLine,
	class FRasterPoly,
	class FRasterSetup,
	class FTransform
	>::Generate(FRasterPoly *Raster) const;

template void TRasterPoly
	<
	class FRasterTexLine,
	class FRasterTexPoint
	>::ForceForwardFace();
template void TRasterPoly
	<
	class FRasterTexLine,
	class FRasterTexPoint
	>::DrawFlat(const UCamera *Camera,BYTE Color,FSpanBuffer *SpanBuffer);
template void TRasterPoly
	<
	class FRasterTexLine,
	class FRasterTexPoint
	>::Draw(const UCamera *Camera,FPolySpanTextureInfoBase &Info,FSpanBuffer *SpanBuffer);
template void TRasterSetup
	<
	class FRasterTexSideSetup,
	class FRasterTexLine,
	class FRasterTexPoly,
	class FRasterTexSetup,
	class FTransTexture
	>::Setup(const UCamera *Camera,const FTransTexture *Pts, int NumPts, FMemStack *Mem);
template void TRasterSetup
	<
	class FRasterTexSideSetup,
	class FRasterTexLine,
	class FRasterTexPoly,
	class FRasterTexSetup,
	class FTransTexture
	>::CalcBound(const FTransTexture *Pts, int NumPts, FScreenBounds &Bounds);
template void TRasterSetup
	<
	class FRasterTexSideSetup,
	class FRasterTexLine,
	class FRasterTexPoly,
	class FRasterTexSetup,
	class FTransTexture
	>::Generate(FRasterTexPoly *Raster) const;

/*-----------------------------------------------------------------------------
	Rasterizer globals
-----------------------------------------------------------------------------*/

//
// Init.
//
void FGlobalRaster::Init( void )
{
	guard(FGlobalRaster::Init);

	Raster = (FRasterPoly *)appMalloc(sizeof(FRasterPoly)+FGlobalRaster::MAX_RASTER_LINES*sizeof (FRasterLine),"GRaster");
	debug(LOG_Init,"Rasterizer initialized");

	unguard;
}

//
// Exit.
//
void FGlobalRaster::Exit()
{
	guard(FGlobalRaster::Exit);

	appFree(Raster);
	debug(LOG_Exit,"Rasterizer closed");

	unguard;
}

/*-----------------------------------------------------------------------------
	The End
-----------------------------------------------------------------------------*/
