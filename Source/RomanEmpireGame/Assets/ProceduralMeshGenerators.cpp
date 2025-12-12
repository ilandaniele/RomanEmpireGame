// Copyright Roman Empire Game. All Rights Reserved.

#include "ProceduralMeshGenerators.h"
#include "ProceduralMeshComponent.h"

AProceduralMeshGenerators::AProceduralMeshGenerators()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AProceduralMeshGenerators::GenerateBarracksMesh(UProceduralMeshComponent* MeshComponent, FVector Size)
{
	if (!MeshComponent)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	float HalfX = Size.X * 0.5f;
	float HalfY = Size.Y * 0.5f;

	// Main building - box
	GenerateBox(MeshComponent, Size);

	// Add roof - simple pitched roof
	// Would add additional geometry for roof

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AProceduralMeshGenerators::GenerateWallMesh(UProceduralMeshComponent* MeshComponent, float Length, float Height, float Thickness)
{
	if (!MeshComponent)
	{
		return;
	}

	GenerateBox(MeshComponent, FVector(Length, Thickness, Height));
}

void AProceduralMeshGenerators::GenerateTowerMesh(UProceduralMeshComponent* MeshComponent, float Radius, float Height)
{
	if (!MeshComponent)
	{
		return;
	}

	GenerateCylinder(MeshComponent, Radius, Height, 12);
}

void AProceduralMeshGenerators::GenerateForumMesh(UProceduralMeshComponent* MeshComponent, FVector Size)
{
	if (!MeshComponent)
	{
		return;
	}

	// Main platform
	GenerateBox(MeshComponent, FVector(Size.X, Size.Y, Size.Z * 0.2f));

	// Columns would be added as separate sections
}

void AProceduralMeshGenerators::GenerateFlatTerrain(UProceduralMeshComponent* MeshComponent, FVector2D Size, int32 Resolution)
{
	if (!MeshComponent)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	float StepX = Size.X / Resolution;
	float StepY = Size.Y / Resolution;
	float HalfX = Size.X * 0.5f;
	float HalfY = Size.Y * 0.5f;

	// Generate grid of vertices
	for (int32 Y = 0; Y <= Resolution; ++Y)
	{
		for (int32 X = 0; X <= Resolution; ++X)
		{
			float PosX = -HalfX + X * StepX;
			float PosY = -HalfY + Y * StepY;
			
			Vertices.Add(FVector(PosX, PosY, 0.0f));
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D((float)X / Resolution, (float)Y / Resolution));
		}
	}

	// Generate triangles
	for (int32 Y = 0; Y < Resolution; ++Y)
	{
		for (int32 X = 0; X < Resolution; ++X)
		{
			int32 V0 = Y * (Resolution + 1) + X;
			int32 V1 = V0 + 1;
			int32 V2 = V0 + Resolution + 1;
			int32 V3 = V2 + 1;

			// First triangle
			Triangles.Add(V0);
			Triangles.Add(V2);
			Triangles.Add(V1);

			// Second triangle
			Triangles.Add(V1);
			Triangles.Add(V2);
			Triangles.Add(V3);
		}
	}

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AProceduralMeshGenerators::GenerateHillyTerrain(UProceduralMeshComponent* MeshComponent, FVector2D Size, int32 Resolution, float MaxHeight)
{
	if (!MeshComponent)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	float StepX = Size.X / Resolution;
	float StepY = Size.Y / Resolution;
	float HalfX = Size.X * 0.5f;
	float HalfY = Size.Y * 0.5f;

	// Generate grid with height variation
	for (int32 Y = 0; Y <= Resolution; ++Y)
	{
		for (int32 X = 0; X <= Resolution; ++X)
		{
			float PosX = -HalfX + X * StepX;
			float PosY = -HalfY + Y * StepY;
			
			// Simple perlin-like noise approximation
			float Height = FMath::Sin(PosX * 0.001f) * FMath::Cos(PosY * 0.001f) * MaxHeight * 0.5f;
			Height += FMath::Sin(PosX * 0.003f + 1.5f) * FMath::Sin(PosY * 0.002f) * MaxHeight * 0.25f;
			
			Vertices.Add(FVector(PosX, PosY, Height));
			UVs.Add(FVector2D((float)X / Resolution, (float)Y / Resolution));
		}
	}

	// Calculate normals
	for (int32 Y = 0; Y <= Resolution; ++Y)
	{
		for (int32 X = 0; X <= Resolution; ++X)
		{
			int32 Idx = Y * (Resolution + 1) + X;
			
			FVector Normal = FVector::UpVector;
			
			// Calculate normal from neighboring vertices
			if (X > 0 && X < Resolution && Y > 0 && Y < Resolution)
			{
				FVector Left = Vertices[Idx - 1];
				FVector Right = Vertices[Idx + 1];
				FVector Up = Vertices[Idx + Resolution + 1];
				FVector Down = Vertices[Idx - Resolution - 1];
				
				FVector Dx = Right - Left;
				FVector Dy = Up - Down;
				Normal = FVector::CrossProduct(Dy, Dx).GetSafeNormal();
			}
			
			Normals.Add(Normal);
		}
	}

	// Generate triangles (same as flat terrain)
	for (int32 Y = 0; Y < Resolution; ++Y)
	{
		for (int32 X = 0; X < Resolution; ++X)
		{
			int32 V0 = Y * (Resolution + 1) + X;
			int32 V1 = V0 + 1;
			int32 V2 = V0 + Resolution + 1;
			int32 V3 = V2 + 1;

			Triangles.Add(V0);
			Triangles.Add(V2);
			Triangles.Add(V1);

			Triangles.Add(V1);
			Triangles.Add(V2);
			Triangles.Add(V3);
		}
	}

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AProceduralMeshGenerators::GenerateBox(UProceduralMeshComponent* MeshComponent, FVector Size)
{
	if (!MeshComponent)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	float HalfX = Size.X * 0.5f;
	float HalfY = Size.Y * 0.5f;
	float HalfZ = Size.Z * 0.5f;

	// Front face
	AddQuad(Vertices, Triangles, Normals, UVs,
		FVector(-HalfX, -HalfY, -HalfZ),
		FVector(HalfX, -HalfY, -HalfZ),
		FVector(HalfX, -HalfY, HalfZ),
		FVector(-HalfX, -HalfY, HalfZ),
		FVector::ForwardVector * -1);

	// Back face
	AddQuad(Vertices, Triangles, Normals, UVs,
		FVector(HalfX, HalfY, -HalfZ),
		FVector(-HalfX, HalfY, -HalfZ),
		FVector(-HalfX, HalfY, HalfZ),
		FVector(HalfX, HalfY, HalfZ),
		FVector::ForwardVector);

	// Left face
	AddQuad(Vertices, Triangles, Normals, UVs,
		FVector(-HalfX, HalfY, -HalfZ),
		FVector(-HalfX, -HalfY, -HalfZ),
		FVector(-HalfX, -HalfY, HalfZ),
		FVector(-HalfX, HalfY, HalfZ),
		FVector::RightVector * -1);

	// Right face
	AddQuad(Vertices, Triangles, Normals, UVs,
		FVector(HalfX, -HalfY, -HalfZ),
		FVector(HalfX, HalfY, -HalfZ),
		FVector(HalfX, HalfY, HalfZ),
		FVector(HalfX, -HalfY, HalfZ),
		FVector::RightVector);

	// Top face
	AddQuad(Vertices, Triangles, Normals, UVs,
		FVector(-HalfX, -HalfY, HalfZ),
		FVector(HalfX, -HalfY, HalfZ),
		FVector(HalfX, HalfY, HalfZ),
		FVector(-HalfX, HalfY, HalfZ),
		FVector::UpVector);

	// Bottom face
	AddQuad(Vertices, Triangles, Normals, UVs,
		FVector(-HalfX, HalfY, -HalfZ),
		FVector(HalfX, HalfY, -HalfZ),
		FVector(HalfX, -HalfY, -HalfZ),
		FVector(-HalfX, -HalfY, -HalfZ),
		FVector::UpVector * -1);

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AProceduralMeshGenerators::GenerateCylinder(UProceduralMeshComponent* MeshComponent, float Radius, float Height, int32 Segments)
{
	if (!MeshComponent || Segments < 3)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	float HalfHeight = Height * 0.5f;
	float AngleStep = 2.0f * PI / Segments;

	// Generate side vertices
	for (int32 i = 0; i <= Segments; ++i)
	{
		float Angle = i * AngleStep;
		float X = FMath::Cos(Angle) * Radius;
		float Y = FMath::Sin(Angle) * Radius;
		
		// Bottom vertex
		Vertices.Add(FVector(X, Y, -HalfHeight));
		Normals.Add(FVector(X, Y, 0).GetSafeNormal());
		UVs.Add(FVector2D((float)i / Segments, 0));
		
		// Top vertex
		Vertices.Add(FVector(X, Y, HalfHeight));
		Normals.Add(FVector(X, Y, 0).GetSafeNormal());
		UVs.Add(FVector2D((float)i / Segments, 1));
	}

	// Generate side triangles
	for (int32 i = 0; i < Segments; ++i)
	{
		int32 V0 = i * 2;
		int32 V1 = V0 + 1;
		int32 V2 = V0 + 2;
		int32 V3 = V0 + 3;

		Triangles.Add(V0);
		Triangles.Add(V2);
		Triangles.Add(V1);

		Triangles.Add(V1);
		Triangles.Add(V2);
		Triangles.Add(V3);
	}

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AProceduralMeshGenerators::GenerateCone(UProceduralMeshComponent* MeshComponent, float Radius, float Height, int32 Segments)
{
	if (!MeshComponent || Segments < 3)
	{
		return;
	}

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;

	float AngleStep = 2.0f * PI / Segments;

	// Apex
	int32 ApexIndex = Vertices.Num();
	Vertices.Add(FVector(0, 0, Height));
	Normals.Add(FVector::UpVector);
	UVs.Add(FVector2D(0.5f, 0));

	// Base vertices
	for (int32 i = 0; i <= Segments; ++i)
	{
		float Angle = i * AngleStep;
		float X = FMath::Cos(Angle) * Radius;
		float Y = FMath::Sin(Angle) * Radius;
		
		Vertices.Add(FVector(X, Y, 0));
		Normals.Add(FVector(X, Y, Radius / Height).GetSafeNormal());
		UVs.Add(FVector2D((float)i / Segments, 1));
	}

	// Generate triangles from apex to base
	for (int32 i = 0; i < Segments; ++i)
	{
		Triangles.Add(ApexIndex);
		Triangles.Add(ApexIndex + 1 + i + 1);
		Triangles.Add(ApexIndex + 1 + i);
	}

	MeshComponent->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, TArray<FLinearColor>(), TArray<FProcMeshTangent>(), true);
}

void AProceduralMeshGenerators::AddQuad(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs,
	FVector V0, FVector V1, FVector V2, FVector V3, FVector Normal)
{
	int32 StartIndex = Vertices.Num();

	Vertices.Add(V0);
	Vertices.Add(V1);
	Vertices.Add(V2);
	Vertices.Add(V3);

	Normals.Add(Normal);
	Normals.Add(Normal);
	Normals.Add(Normal);
	Normals.Add(Normal);

	UVs.Add(FVector2D(0, 0));
	UVs.Add(FVector2D(1, 0));
	UVs.Add(FVector2D(1, 1));
	UVs.Add(FVector2D(0, 1));

	// First triangle
	Triangles.Add(StartIndex);
	Triangles.Add(StartIndex + 1);
	Triangles.Add(StartIndex + 2);

	// Second triangle
	Triangles.Add(StartIndex);
	Triangles.Add(StartIndex + 2);
	Triangles.Add(StartIndex + 3);
}
