// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshGenerators.generated.h"

class UProceduralMeshComponent;

/**
 * Generates procedural meshes for buildings and terrain
 * Used for prototype/placeholder visuals
 */
UCLASS()
class ROMANEMPIREGAME_API AProceduralMeshGenerators : public AActor
{
	GENERATED_BODY()

public:
	AProceduralMeshGenerators();

	// Building mesh generators
	UFUNCTION(BlueprintCallable, Category = "Procedural|Building")
	static void GenerateBarracksMesh(UProceduralMeshComponent* MeshComponent, FVector Size = FVector(400, 300, 250));

	UFUNCTION(BlueprintCallable, Category = "Procedural|Building")
	static void GenerateWallMesh(UProceduralMeshComponent* MeshComponent, float Length = 500, float Height = 300, float Thickness = 100);

	UFUNCTION(BlueprintCallable, Category = "Procedural|Building")
	static void GenerateTowerMesh(UProceduralMeshComponent* MeshComponent, float Radius = 150, float Height = 500);

	UFUNCTION(BlueprintCallable, Category = "Procedural|Building")
	static void GenerateForumMesh(UProceduralMeshComponent* MeshComponent, FVector Size = FVector(600, 600, 200));

	// Terrain mesh generators
	UFUNCTION(BlueprintCallable, Category = "Procedural|Terrain")
	static void GenerateFlatTerrain(UProceduralMeshComponent* MeshComponent, FVector2D Size = FVector2D(10000, 10000), int32 Resolution = 32);

	UFUNCTION(BlueprintCallable, Category = "Procedural|Terrain")
	static void GenerateHillyTerrain(UProceduralMeshComponent* MeshComponent, FVector2D Size = FVector2D(10000, 10000), int32 Resolution = 32, float MaxHeight = 500);

	// Primitive generators
	UFUNCTION(BlueprintCallable, Category = "Procedural|Primitives")
	static void GenerateBox(UProceduralMeshComponent* MeshComponent, FVector Size);

	UFUNCTION(BlueprintCallable, Category = "Procedural|Primitives")
	static void GenerateCylinder(UProceduralMeshComponent* MeshComponent, float Radius, float Height, int32 Segments = 16);

	UFUNCTION(BlueprintCallable, Category = "Procedural|Primitives")
	static void GenerateCone(UProceduralMeshComponent* MeshComponent, float Radius, float Height, int32 Segments = 16);

private:
	static void AddQuad(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, 
		FVector V0, FVector V1, FVector V2, FVector V3, FVector Normal);
};
