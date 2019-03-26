/*=============================================================================
	UnPrim.cpp: Unreal primitive functions.

	Copyright 1997 Epic MegaGames, Inc. This software is a trade secret.
	Compiled with Visual C++ 4.0. Best viewed with Tabs=4.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "Unreal.h"

/*----------------------------------------------------------------------------
	UPrimitive object implementation.
----------------------------------------------------------------------------*/

IMPLEMENT_CLASS(UPrimitive);

/*----------------------------------------------------------------------------
	UPrimitive collision checking.
----------------------------------------------------------------------------*/

//
// GetCollisionBoundingBox.
// Treats the primitive as a cyllinder.
//
FBoundingBox UPrimitive::GetCollisionBoundingBox( const AActor *Owner ) const
{
	guard(UPrimitive::GetCollisionBoundingBox);
	if( Owner )
	{
		FVector Extent( Owner->CollisionRadius+4, Owner->CollisionRadius+4, Owner->CollisionHeight+4 );
		return FBoundingBox( Owner->Location - Extent, Owner->Location + Extent );
	}
	else
	{
		return LocalBound;
	}
	unguard;
}

//
// GetRenderBoundingBox.
// Treats the primitive as a cyllinder.
//
FBoundingBox UPrimitive::GetRenderBoundingBox( const AActor *Owner ) const
{
	guard(UPrimitive::GetRenderBoundingBox);
	if( Owner )
	{
		FVector Extent( Owner->CollisionRadius+4, Owner->CollisionRadius+4, Owner->CollisionHeight+4 );
		return FBoundingBox( Owner->Location - Extent, Owner->Location + Extent );
	}
	else
	{
		return LocalBound;
	}
	unguard;
}

//
// PointCheck.
// Treats the primitive as a cyllinder.
//
INT UPrimitive::PointCheck
(
	FCheckResult	&Result,
	AActor			*Owner,
	FVector			Location,
	FVector			Extent,
	DWORD           ExtraNodeFlags
)
{
	guard(UPrimitive::PointCheck);
	if
	(	Owner
	&&	Square(Owner->Location.Z-Location.Z)                                      < Square(Owner->CollisionHeight+Extent.Z)
	&&	Square(Owner->Location.X-Location.X)+Square(Owner->Location.Y-Location.Y) < Square(Owner->CollisionRadius+Extent.X) )
	{
		// Hit.
		Result.Actor    = Owner;
		Result.Normal   = (Location - Owner->Location).Normal();
		if     ( Result.Normal.Z < -0.5 ) Result.Location = FVector( Location.X, Location.Y, Owner->Location.Z - Extent.Z);
		else if( Result.Normal.Z > +0.5 ) Result.Location = FVector( Location.X, Location.Y, Owner->Location.Z - Extent.Z);
		else                              Result.Location = (Owner->Location - Extent.X * (Result.Normal*FVector(1,1,0)).Normal()) + FVector(0,0,Location.Z);
		return 0;
	}
	else return 1;
	unguard;
}

//
// LineCheck.
// Treats the primitive as a cyllinder.
//
INT UPrimitive::LineCheck
(
	FCheckResult	&Result,
	AActor			*Owner,
	FVector			Start,
	FVector			End,
	FVector			Extent,
	DWORD           ExtraNodeFlags
)
{
	guard(UPrimitive::LineCheck);

	if( !Owner )
		return 1;

	// Treat this actor as a cyllinder.
	FVector NetExtent = Extent + Owner->GetCollisionExtent();

	// Quick X reject.
	FLOAT MaxX = Owner->Location.X + NetExtent.X;
	if( Start.X>MaxX && End.X>MaxX )
		return 1;
	FLOAT MinX = Owner->Location.X - NetExtent.X;
	if( Start.X<MinX && End.X<MinX )
		return 1;

	// Quick Y reject.
	FLOAT MaxY = Owner->Location.Y + NetExtent.Y;
	if( Start.Y>MaxY && End.Y>MaxY )
		return 1;
	FLOAT MinY = Owner->Location.Y - NetExtent.Y;
	if( Start.Y<MinY && End.Y<MinY )
		return 1;

	// Quick Z reject.
	FLOAT TopZ = Owner->Location.Z + NetExtent.Z;
	if( Start.Z>TopZ && End.Z>TopZ )
		return 1;
	FLOAT BotZ = Owner->Location.Z - NetExtent.Z;
	if( Start.Z<BotZ && End.Z<BotZ )
		return 1;

	// Clip to top of cyllinder.
	FLOAT T0=0, T1=1.0;
	if( Start.Z>TopZ && End.Z<TopZ )
	{
		FLOAT T = (TopZ - Start.Z)/(End.Z - Start.Z);
		if( T > T0 )
		{
			T0 = T;
			Result.Normal = FVector(0,0,1);
		}
	}
	else if( Start.Z<TopZ && End.Z>TopZ )
		T1 = ::Min( T1, (TopZ - Start.Z)/(End.Z - Start.Z) );

	// Clip to bottom of cyllinder.
	if( Start.Z<BotZ && End.Z>BotZ )
	{
		FLOAT T = (BotZ - Start.Z)/(End.Z - Start.Z);
		if( T > T0 )
		{
			T0 = T;
			Result.Normal = FVector(0,0,-1);
		}
	}
	else if( Start.Z>BotZ && End.Z<BotZ )
		T1 = ::Min( T0, (BotZ - Start.Z)/(End.Z - Start.Z) );

	// Reject.
	if( T0 >= T1 )
		return 1;

	// Test setup.
	FLOAT   Kx        = Start.X - Owner->Location.X;
	FLOAT   Ky        = Start.Y - Owner->Location.Y;

	// 2D circle clip about origin.
	FLOAT   Vx        = End.X - Start.X;
	FLOAT   Vy        = End.Y - Start.Y;
	FLOAT   A         = Vx*Vx + Vy*Vy;
	FLOAT   B         = 2.0 * (Kx*Vx + Ky*Vy);
	FLOAT   C         = Kx*Kx + Ky*Ky - Square(NetExtent.X);
	FLOAT   Discrim   = B*B - 4.0*A*C;

	// If already inside sphere, oppose further movement inward.
	if( C < Square(1.0) )
	{
		FLOAT Dir = ((End-Start)*FVector(1,1,0)) | (Start-Owner->Location);
		if( Dir < -0.1 )
		{
			Result.Time      = 0.0;
			Result.Location  = Start;
			Result.Normal    = ((Start-Owner->Location)*FVector(1,1,0)).Normal();
			Result.Actor     = Owner;
			Result.Primitive = NULL;
			return 0;
		}
		else return 1;
	}

	// No intersection if discriminant is negative.
	if( Discrim < 0 )
		return 1;

	// Unstable intersection if velocity is tiny.
	if( A < Square(0.0001) )
	{
		// Outside.
		if( C > 0 )
			return 1;
	}
	else
	{
		// Compute intersection times.
		Discrim   = sqrt(Discrim);
		FLOAT R2A = 0.5/A;
		T1        = ::Min( T1, +(Discrim-B) * R2A );
		FLOAT T   = -(Discrim+B) * R2A;
		if( T > T0 )
		{
			T0 = T;
			Result.Normal   = (Start + (End-Start)*T0 - Owner->Location);
			Result.Normal.Z = 0;
			Result.Normal.Normalize();
		}
		if( T0 >= T1 )
			return 1;
	}
	Result.Time      = Clamp(T0-0.001,0.0,1.0);
	Result.Location  = Start + (End-Start) * Result.Time;
	Result.Actor     = Owner;
	Result.Primitive = NULL;
	return 0;

	unguard;
}

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/