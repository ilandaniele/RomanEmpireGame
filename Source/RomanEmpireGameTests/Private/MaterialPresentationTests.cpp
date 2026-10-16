// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "AssetCompilingManager.h"
#include "Building/BuildingTypes.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "CoreGlobals.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Kismet/GameplayStatics.h"
#include "SceneView.h"
#include "ShaderCompiler.h"
#include "RenderingThread.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialInterface.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanMaterialTests
{
struct FCall
{
	FAutomationTestBase& Test;
	UObject* Target;
	UFunction* Function;
	FStructOnScope Params;
	FCall(FAutomationTestBase& InTest, UObject* InTarget, FName Name)
		: Test(InTest), Target(InTarget), Function(Target ? Target->FindFunction(Name) : nullptr), Params(Function)
	{
		Test.TestNotNull(*Name.ToString(), Function);
	}
	template<typename P, typename T> void Set(FName Name, T Value)
	{
		P* Prop = Function ? FindFProperty<P>(Function, Name) : nullptr;
		if (Test.TestNotNull(*Name.ToString(), Prop)) Prop->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Owner(EFactionID Value)
	{
		FEnumProperty* Prop = Function ? FindFProperty<FEnumProperty>(Function, TEXT("NewOwner")) : nullptr;
		if (Test.TestTrue(TEXT("Faction setter enum type"), Prop && Prop->GetEnum() == StaticEnum<EFactionID>()))
			Prop->GetUnderlyingProperty()->SetIntPropertyValue(Prop->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(Value));
	}
	void Execute() { if (Function) Target->ProcessEvent(Function, Params.GetStructMemory()); }
	template<typename P, typename T> T Read()
	{
		P* Prop = Function ? FindFProperty<P>(Function, TEXT("ReturnValue")) : nullptr;
		return Test.TestNotNull(TEXT("Return value"), Prop) ? Prop->GetPropertyValue_InContainer(Params.GetStructMemory()) : T();
	}
};
UStaticMeshComponent* Mesh(FAutomationTestBase& Test, UObject* Target, FName Name)
{
	FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(Target->GetClass(), Name);
	UStaticMeshComponent* Result = Prop ? Cast<UStaticMeshComponent>(Prop->GetObjectPropertyValue_InContainer(Target)) : nullptr;
	Test.TestNotNull(*Name.ToString(), Result);
	return Result;
}
UMaterialInstanceDynamic* Material(FAutomationTestBase& Test, UStaticMeshComponent* Component, const TCHAR* Parent)
{
	UMaterialInstanceDynamic* Result = Component ? Cast<UMaterialInstanceDynamic>(Component->GetMaterial(0)) : nullptr;
	if (Test.TestNotNull(TEXT("Real actor material instance"), Result))
	{
		Test.TestEqual(TEXT("Persistent parent"), Result->Parent->GetPathName(), FString::Printf(TEXT("/Game/Materials/RomanVisual/%s.%s"), Parent, Parent));
		UMaterial* Base = Result->GetMaterial();
		FMaterialInputDescription Description;
		Base->GetExpressionInputDescription(MP_EmissiveColor, Description);
		Test.AddInfo(FString::Printf(TEXT("MaterialDiagnostic %s unlit=%d inlineConstant=%d hidden=%d"), Parent,
			Base->GetShadingModels().HasShadingModel(MSM_Unlit), Description.bUseConstant, Description.bHidden));
		if (Base->GetShadingModels().HasShadingModel(MSM_Unlit))
			Test.TestTrue(TEXT("Effective shader cache includes connected emissive output"), Base->IsPropertyConnected(MP_EmissiveColor));
	}
	return Result;
}
void Command(FAutomationTestBase& Test, UObject* Actor, FName Name) { FCall Call(Test, Actor, Name); Call.Execute(); }
void Select(FAutomationTestBase& Test, UObject* Actor, bool Value)
{
	FCall Call(Test, Actor, TEXT("SetSelected")); Call.Set<FBoolProperty>(TEXT("bNewSelected"), Value); Call.Execute();
}
void Owner(FAutomationTestBase& Test, UObject* Actor, EFactionID Value)
{
	FCall Call(Test, Actor, TEXT("SetOwnerFaction")); Call.Owner(Value); Call.Execute();
}
float Health(FAutomationTestBase& Test, UObject* Actor)
{
	FCall Call(Test, Actor, TEXT("GetHealthPercent")); Call.Execute(); return Call.Read<FFloatProperty, float>();
}

class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test;
	double Started = FPlatformTime::Seconds();
	TArray<TWeakObjectPtr<AActor>> Scene;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> BuildingViews;
	TArray<TWeakObjectPtr<UStaticMeshComponent>> SelectionViews;
	TWeakObjectPtr<ASceneCapture2D> CaptureCamera;
	TStrongObjectPtr<UTextureRenderTarget2D> CaptureTarget;
	TWeakObjectPtr<APlayerController> CapturePlayer;
	TWeakObjectPtr<AActor> PreviousViewTarget;
	TWeakObjectPtr<ACameraActor> PlayerViewCamera;
	bool bCapturePending = false;
	double CaptureStarted = 0;
	uint64 CaptureStartFrame = 0;
	void ClearScene()
	{
		if (CapturePlayer.IsValid() && PreviousViewTarget.IsValid()) CapturePlayer->SetViewTarget(PreviousViewTarget.Get());
		for (const TWeakObjectPtr<AActor>& Actor : Scene) if (Actor.IsValid()) Actor->Destroy();
		Scene.Empty(); BuildingViews.Empty(); SelectionViews.Empty(); CaptureTarget.Reset(); bCapturePending = false;
	}
	void CheckPixels(TArrayView64<const FColor> Pixels, FIntPoint Size)
	{
		if (!Test.TestTrue(TEXT("Complete actual pixel buffer and live projection camera"), Size.X > 0 && Size.Y > 0 && Pixels.Num() == static_cast<int64>(Size.X) * Size.Y && CaptureCamera.IsValid())) return;
		int32 WhitePixels = 0;
		for (const FColor& Pixel : Pixels)
		{
			const int32 Maximum = FMath::Max3(Pixel.R, Pixel.G, Pixel.B);
			const int32 Minimum = FMath::Min3(Pixel.R, Pixel.G, Pixel.B);
			if (Minimum > 180 && Maximum - Minimum < 10) ++WhitePixels;
		}
		Test.AddInfo(FString::Printf(TEXT("Material pixels white=%d"), WhitePixels));
		Test.TestTrue(TEXT("Rendered selection has substantial bright non-color geometry"), WhitePixels > 500);
		if (!Test.TestEqual(TEXT("All six actual building views tracked"), BuildingViews.Num(), 6)) return;
		FMinimalViewInfo Info;
		CaptureCamera->GetCaptureComponent2D()->GetCameraView(0, Info);
		Info.AspectRatio = static_cast<float>(Size.X) / Size.Y;
		FMatrix ViewMatrix, Projection, ViewProjection;
		UGameplayStatics::CalculateViewProjectionMatricesFromMinimalView(Info, TOptional<FMatrix>(), ViewMatrix, Projection, ViewProjection);
		const auto Project = [&](const FVector& WorldPoint, FVector2D& Pixel)
		{
			return PlayerViewCamera.IsValid()
				? UGameplayStatics::ProjectWorldToScreen(CapturePlayer.Get(), WorldPoint, Pixel, true)
				: FSceneView::ProjectWorldToScreen(WorldPoint, FIntRect(FIntPoint::ZeroValue, Size), ViewProjection, Pixel);
		};
		struct FSample { FLinearColor Mean = FLinearColor::Black; int32 Green = 0, Red = 0, Transitions = 0, Count = 0; };
		TArray<FSample> Samples;
		for (int32 Index = 0; Index < BuildingViews.Num(); ++Index)
		{
			UStaticMeshComponent* Part = BuildingViews[Index].Get();
			if (!Test.TestNotNull(TEXT("Actual building mesh survives capture frames"), Part)) return;
			const FVector Roof = Part->Bounds.Origin + FVector(0,0,Part->Bounds.BoxExtent.Z);
			FVector2D Center, Offset;
			if (!Test.TestTrue(TEXT("Actual roof projects inside view"), Project(Roof, Center) && Project(Roof + FVector(Part->Bounds.BoxExtent.X * 0.3,0,0), Offset))) return;
			const int32 Radius = FMath::Max(3, FMath::RoundToInt(FVector2D::Distance(Center, Offset)));
			if (!Test.TestTrue(TEXT("Projected roof region is fully inside actual pixels"), Center.X > Radius && Center.Y > Radius && Center.X + Radius < Size.X && Center.Y + Radius < Size.Y)) return;
			FSample Sample;
			for (int32 Y = FMath::RoundToInt(Center.Y) - Radius / 3; Y <= FMath::RoundToInt(Center.Y) + Radius / 3; ++Y)
			{
				int32 PreviousRed = -1;
				for (int32 X = FMath::RoundToInt(Center.X) - Radius; X <= FMath::RoundToInt(Center.X) + Radius; ++X)
				{
					const FColor Pixel = Pixels[Y * Size.X + X];
					Sample.Mean += FLinearColor(Pixel.R, Pixel.G, Pixel.B, 0); ++Sample.Count;
					if (Pixel.G > Pixel.R + 15 && Pixel.G > Pixel.B + 10) ++Sample.Green;
					if (Pixel.R > Pixel.G + 15 && Pixel.R > Pixel.B + 10) ++Sample.Red;
					if (PreviousRed >= 0 && FMath::Abs(Pixel.R - PreviousRed) > 8) ++Sample.Transitions;
					PreviousRed = Pixel.R;
				}
			}
			Sample.Mean /= Sample.Count;
			Test.AddInfo(FString::Printf(TEXT("Material roof%d RGB=(%.1f,%.1f,%.1f) green=%d red=%d transitions=%d samples=%d"), Index, Sample.Mean.R, Sample.Mean.G, Sample.Mean.B, Sample.Green, Sample.Red, Sample.Transitions, Sample.Count));
			Samples.Add(Sample);
		}
		Test.TestTrue(TEXT("Actual valid-preview roof is green, not background or small green unit"), Samples[0].Green > Samples[0].Count * 0.8);
		Test.TestTrue(TEXT("Actual invalid-preview roof is red"), Samples[1].Red > Samples[1].Count * 0.8);
		Test.TestTrue(TEXT("Actual invalid preview has repeated non-color stripe transitions"), Samples[1].Transitions > Samples[1].Count / 10 && Samples[1].Transitions > Samples[0].Transitions + 10);
		Test.TestTrue(TEXT("Actual construction roof is distinct from complete"), (Samples[2].Mean - Samples[3].Mean).GetMax() > 20);
		Test.TestTrue(TEXT("Actual damaged roof is darker than complete"), Samples[4].Mean.GetLuminance() < Samples[3].Mean.GetLuminance() - 10);
		int32 BuildingWhite = 0, UnitWhite = 0;
		for (const TWeakObjectPtr<UStaticMeshComponent>& WeakPart : SelectionViews)
		{
			UStaticMeshComponent* Part = WeakPart.Get();
			if (!Test.TestNotNull(TEXT("Actual selection geometry survives capture"), Part)) return;
			FVector2D Minimum(Size.X, Size.Y), Maximum(0,0);
			for (int32 Corner = 0; Corner < 8; ++Corner)
			{
				const FVector Sign(Corner & 1 ? 1 : -1, Corner & 2 ? 1 : -1, Corner & 4 ? 1 : -1);
				FVector2D Point;
				if (!Project(Part->Bounds.Origin + Part->Bounds.BoxExtent * Sign, Point)) continue;
				Minimum.X = FMath::Min(Minimum.X, Point.X); Minimum.Y = FMath::Min(Minimum.Y, Point.Y);
				Maximum.X = FMath::Max(Maximum.X, Point.X); Maximum.Y = FMath::Max(Maximum.Y, Point.Y);
			}
			for (int32 Y = FMath::Clamp(FMath::FloorToInt(Minimum.Y), 0, Size.Y-1); Y <= FMath::Clamp(FMath::CeilToInt(Maximum.Y), 0, Size.Y-1); ++Y)
				for (int32 X = FMath::Clamp(FMath::FloorToInt(Minimum.X), 0, Size.X-1); X <= FMath::Clamp(FMath::CeilToInt(Maximum.X), 0, Size.X-1); ++X)
				{
					const FColor Pixel = Pixels[Y * Size.X + X];
					if (FMath::Min3(Pixel.R,Pixel.G,Pixel.B) > 180 && FMath::Max3(Pixel.R,Pixel.G,Pixel.B) - FMath::Min3(Pixel.R,Pixel.G,Pixel.B) < 10)
						(Part->GetName().StartsWith(TEXT("SelectionOutline")) ? BuildingWhite : UnitWhite)++;
				}
		}
		Test.AddInfo(FString::Printf(TEXT("Actual selection-region white building=%d unit=%d"), BuildingWhite, UnitWhite));
		Test.TestTrue(TEXT("Actual building outline is bright, not unrelated UI/background"), BuildingWhite > 50);
		Test.TestTrue(TEXT("Actual unit selection region is bright"), UnitWhite > 80);
	}
public:
	explicit FExercise(FAutomationTestBase* InTest) : Test(*InTest) {}
	virtual ~FExercise() override { ClearScene(); }
	void Capture(UWorld* World, UClass* UnitClass, UClass* BuildingClass, const URomanBuildingDefinition* BuildingDef)
	{
		if (!FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureMaterials"))) return;
		const FVector Origin(420000, 420000, 0);
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(Origin + FVector(0,150,-25), FRotator::ZeroRotator, Spawn);
		if (!Test.TestNotNull(TEXT("Capture floor"), Floor)) return;
		Scene.Add(Floor);
		Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube")));
		Floor->SetActorScale3D(FVector(24,20,0.5));
		UMaterialInterface* FloorParent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/RomanVisual/MI_Surface"));
		UMaterialInstanceDynamic* FloorMID = FloorParent ? UMaterialInstanceDynamic::Create(FloorParent, Floor) : nullptr;
		if (!Test.TestNotNull(TEXT("Capture floor material"), FloorMID)) return;
		FloorMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.12f,0.15f,0.18f));
		Floor->GetStaticMeshComponent()->SetMaterial(0, FloorMID);
		if (FParse::Param(FCommandLine::Get(), TEXT("RomanMaterialReference")))
		{
			UMaterial* Reference = NewObject<UMaterial>(GetTransientPackage(), NAME_None, RF_Transient);
			Reference->SetShadingModel(MSM_Unlit);
			UMaterialExpressionConstant3Vector* White = NewObject<UMaterialExpressionConstant3Vector>(Reference);
			White->Material = Reference; White->Constant = FLinearColor::White;
			Reference->GetEditorOnlyData()->ExpressionCollection.AddExpression(White);
			Reference->GetEditorOnlyData()->EmissiveColor.Connect(0, White);
			Reference->PreEditChange(nullptr); Reference->PostEditChange();
			AStaticMeshActor* Cube = World->SpawnActor<AStaticMeshActor>(Origin + FVector(1000,0,75), FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Transient reference cube"), Cube)) return;
			Scene.Add(Cube);
			Cube->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
			Cube->GetStaticMeshComponent()->SetStaticMesh(Floor->GetStaticMeshComponent()->GetStaticMesh());
			Cube->GetStaticMeshComponent()->SetMaterial(0, Reference);
			Cube->SetActorScale3D(FVector(1.5));
		}
		for (int32 Index = 0; Index < 6; ++Index)
		{
			const FVector Location = Origin + FVector((Index % 3 - 1) * 600, Index < 3 ? 0 : 650, BuildingDef->Definition.PrototypeScale.Z * 50);
			AActor* Actor = World->SpawnActor<AActor>(BuildingClass, Location, FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Render building"), Actor)) return;
			Scene.Add(Actor); Actor->SetActorTickEnabled(false); Owner(Test, Actor, EFactionID::Rome);
			BuildingViews.Add(Mesh(Test, Actor, TEXT("BuildingMesh")));
			if (Index < 2)
			{
				FCall PreviewCall(Test, Actor, TEXT("SetPlacementMode"));
				PreviewCall.Set<FBoolProperty>(TEXT("bPlacing"), true); PreviewCall.Set<FBoolProperty>(TEXT("bValidPlacement"), Index == 0); PreviewCall.Execute();
			}
			else if (Index == 2)
			{
				Command(Test, Actor, TEXT("StartConstruction")); Actor->Tick(BuildingDef->Definition.ConstructionTime * 0.9f);
			}
			else
			{
				Command(Test, Actor, TEXT("StartConstruction"));
				Actor->Tick(BuildingDef->Definition.ConstructionTime + 0.1f);
				if (Index == 4)
				{
					FCall Damage(Test, Actor, TEXT("ApplyBuildingDamage"));
					Damage.Set<FFloatProperty>(TEXT("Damage"), BuildingDef->Definition.MaxHealth * 0.65f + BuildingDef->Definition.Armor); Damage.Execute();
				}
				Select(Test, Actor, Index == 5);
				if (Index == 5)
				{
					TArray<UStaticMeshComponent*> Parts; Actor->GetComponents(Parts);
					for (UStaticMeshComponent* Part : Parts) if (Part->GetName().StartsWith(TEXT("SelectionOutline"))) SelectionViews.Add(Part);
				}
			}
		}
		for (int32 Index = 0; Index < 3; ++Index)
		{
			Spawn.bDeferConstruction = true;
			AActor* Actor = World->SpawnActor<AActor>(UnitClass, Origin + FVector((Index - 1) * 400, -450, 96), FRotator(0,-90,0), Spawn);
			if (!Test.TestNotNull(TEXT("Render unit"), Actor)) return;
			Scene.Add(Actor);
			FCall Init(Test, Actor, TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary")));
			// Initialize an available factory, then exercise the existing ownership-transfer API.
			Init.Owner(EFactionID::Rome); Init.Execute();
			if (!Test.TestTrue(TEXT("Render unit initialized"), Init.Read<FBoolProperty, bool>())) return;
			Actor->FinishSpawning(Actor->GetActorTransform()); Actor->SetActorTickEnabled(false);
			Owner(Test, Actor, Index == 0 ? EFactionID::Rome : Index == 1 ? EFactionID::Carthage : EFactionID::Gaul);
			if (ACharacter* Character = Cast<ACharacter>(Actor)) Character->GetCharacterMovement()->DisableMovement();
			Select(Test, Actor, Index == 0);
			if (Index == 0) SelectionViews.Add(Mesh(Test, Actor, TEXT("SelectionRingMesh")));
		}
		Spawn.bDeferConstruction = false;
		const FVector CameraLocation = Origin + FVector(0,-1800,2400);
		ASceneCapture2D* Camera = World->SpawnActor<ASceneCapture2D>(CameraLocation, (Origin + FVector(0,160,0) - CameraLocation).Rotation(), Spawn);
		if (!Test.TestNotNull(TEXT("Real scene capture"), Camera)) return;
		Scene.Add(Camera);
		CaptureCamera = Camera;
		CaptureTarget.Reset(NewObject<UTextureRenderTarget2D>());
		CaptureTarget->RenderTargetFormat = RTF_RGBA8; CaptureTarget->InitAutoFormat(1536,1024);
		USceneCaptureComponent2D* CaptureComponent = Camera->GetCaptureComponent2D();
		const bool bPerspective = FParse::Param(FCommandLine::Get(), TEXT("RomanCapturePerspective"));
		CaptureComponent->ProjectionType = bPerspective ? ECameraProjectionMode::Perspective : ECameraProjectionMode::Orthographic;
		CaptureComponent->OrthoWidth = 2300; CaptureComponent->FOVAngle = 55;
		CaptureComponent->CaptureSource = SCS_FinalColorLDR;
		CaptureComponent->bCaptureEveryFrame = false; CaptureComponent->bCaptureOnMovement = false;
		CaptureComponent->bAlwaysPersistRenderingState = true;
		CaptureComponent->ShowFlags.SetTranslucency(true);
		CaptureComponent->ShowFlags.SetSeparateTranslucency(true);
		CaptureComponent->TextureTarget = CaptureTarget.Get();
		if (FParse::Param(FCommandLine::Get(), TEXT("RomanMaterialPlayerView")))
		{
			CapturePlayer = World->GetFirstPlayerController();
			if (!Test.TestTrue(TEXT("Real player owns comparison view"), CapturePlayer.IsValid())) return;
			PreviousViewTarget = CapturePlayer->GetViewTarget();
			ACameraActor* PlayerCamera = World->SpawnActor<ACameraActor>(CameraLocation, Camera->GetActorRotation(), Spawn);
			if (!Test.TestNotNull(TEXT("Transient player comparison camera"), PlayerCamera)) return;
			Scene.Add(PlayerCamera); PlayerViewCamera = PlayerCamera;
			PlayerCamera->GetCameraComponent()->SetFieldOfView(55);
			PlayerCamera->GetCameraComponent()->bConstrainAspectRatio = false;
			CapturePlayer->SetViewTarget(PlayerCamera);
		}
		FAssetCompilingManager::Get().FinishAllCompilation();
		// Proxy recreation can request additional material usage permutations. Push
		// it BEFORE waiting for shaders, not inside the first CaptureScene afterward.
		World->SendAllEndOfFrameUpdates();
		FlushRenderingCommands();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		World->SendAllEndOfFrameUpdates();
		FlushRenderingCommands();
		bCapturePending = true;
		CaptureStarted = FPlatformTime::Seconds(); CaptureStartFrame = GFrameCounter;
	}
	bool FinishCapture(UWorld* World)
	{
		if (FPlatformTime::Seconds() - CaptureStarted > 20) { Test.AddError(TEXT("Actual material capture frames timed out")); ClearScene(); return true; }
		if (FPlatformTime::Seconds() - CaptureStarted < 1.0 || GFrameCounter < CaptureStartFrame + 12) return false;
		ON_SCOPE_EXIT { ClearScene(); };
		FString Folder = TEXT("Artifacts/C017D_Rendered");
		FParse::Value(FCommandLine::Get(), TEXT("RomanMaterialCaptureDir="), Folder);
		const FString Directory = FPaths::Combine(FPaths::ProjectDir(), Folder);
		IFileManager::Get().MakeDirectory(*Directory, true);
		if (PlayerViewCamera.IsValid())
		{
			if (!Test.TestTrue(TEXT("Comparison still uses actual player camera"), CapturePlayer.IsValid() && CapturePlayer->GetViewTarget() == PlayerViewCamera.Get())) return true;
			UGameViewportClient* Client = World->GetGameViewport();
			FViewport* Viewport = Client ? Client->Viewport : nullptr;
			if (!Test.TestTrue(TEXT("Real player viewport available"), Viewport && Viewport->GetSizeXY().X > 0 && Viewport->GetSizeXY().Y > 0)) return true;
			TArray<FColor> Pixels;
			if (!Test.TestTrue(TEXT("Actual player scene pixels read"), Viewport->ReadPixels(Pixels))) return true;
			const FIntPoint Size = Viewport->GetSizeXY();
			Test.TestEqual(TEXT("Complete actual player view read"), Pixels.Num(), Size.X * Size.Y);
			TArray64<uint8> PNG;
			const TArrayView64<const FColor> View(Pixels.GetData(), Pixels.Num());
			FImageUtils::PNGCompressImageArray(Size.X, Size.Y, View, PNG);
			Test.TestTrue(TEXT("Player material PNG saved"), FFileHelper::SaveArrayToFile(PNG, *FPaths::Combine(Directory,TEXT("PlayerMaterialStates.png"))));
			CheckPixels(View, Size);
		}
		else
		{
			if (!Test.TestTrue(TEXT("Scene capture survived real frame latency"), CaptureCamera.IsValid() && CaptureTarget.IsValid())) return true;
			CaptureCamera->GetCaptureComponent2D()->CaptureScene();
			FImage Image;
			if (!Test.TestTrue(TEXT("Latent material scene renders"), FImageUtils::GetRenderTargetImage(CaptureTarget.Get(), Image))) return true;
			Test.TestTrue(TEXT("Material scene PNG saved"), FImageUtils::SaveImageByExtension(*FPaths::Combine(Directory,TEXT("MaterialStates.png")), Image));
			if (!Test.TestEqual(TEXT("RGBA8 capture exposes BGRA8 pixels"), Image.Format, ERawImageFormat::BGRA8)) return true;
			CheckPixels(Image.AsBGRA8(), FIntPoint(Image.SizeX, Image.SizeY));
		}
		return true;
	}
	virtual bool Update() override
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		if (bCapturePending)
		{
			if (!Test.TestNotNull(TEXT("Owned capture world remains live"), World)) { ClearScene(); return true; }
			return FinishCapture(World);
		}
		const URomanContentRegistrySubsystem* Registry = World ? URomanContentRegistrySubsystem::Get(World) : nullptr;
		if (!World || !World->HasBegunPlay() || !Registry || !Registry->IsReady())
		{
			if (FPlatformTime::Seconds() - Started < 20) return false;
			Test.AddError(TEXT("Fresh ready material PIE timed out")); return true;
		}
		const URomanUnitDefinition* UnitDef = Registry->FindUnit(TEXT("Legionary"));
		const URomanBuildingDefinition* BuildingDef = Registry->FindBuilding(TEXT("Barracks"));
		UClass* UnitClass = UnitDef ? Cast<UClass>(UnitDef->UnitClass.ToSoftObjectPath().ResolveObject()) : nullptr;
		UClass* BuildingClass = BuildingDef ? Cast<UClass>(BuildingDef->BuildingClass.ToSoftObjectPath().ResolveObject()) : nullptr;
		if (!Test.TestTrue(TEXT("Canonical factories available"), UnitClass && BuildingClass)) return true;
		FActorSpawnParameters Spawn;
		Spawn.bDeferConstruction = true;
		Spawn.ObjectFlags |= RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Unit = World->SpawnActor<AActor>(UnitClass, FTransform(FVector(400000,400000,100)), Spawn);
		AActor* Building = World->SpawnActor<AActor>(BuildingClass, FTransform(FVector(402000,400000,100)), Spawn);
		ON_SCOPE_EXIT { if (IsValid(Unit)) Unit->Destroy(); if (IsValid(Building)) Building->Destroy(); };
		if (!Test.TestTrue(TEXT("Transient real actors spawned"), Unit && Building)) return true;
		FCall Init(Test, Unit, TEXT("InitializeUnit"));
		Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Owner(EFactionID::Rome); Init.Execute();
		if (!Test.TestTrue(TEXT("Unit initialized by canonical API"), Init.Read<FBoolProperty, bool>())) return true;
		FEnumProperty* InitialOwner = FindFProperty<FEnumProperty>(Building->GetClass(), TEXT("OwnerFaction"));
		if (!Test.TestNotNull(TEXT("Deferred building owner"), InitialOwner)) return true;
		InitialOwner->GetUnderlyingProperty()->SetIntPropertyValue(InitialOwner->ContainerPtrToValuePtr<void>(Building), static_cast<int64>(EFactionID::Rome));
		Unit->FinishSpawning(Unit->GetActorTransform()); Building->FinishSpawning(Building->GetActorTransform());
		Unit->SetActorTickEnabled(false); Building->SetActorTickEnabled(false);
		UStaticMeshComponent* Body = Mesh(Test, Unit, TEXT("VisualMesh"));
		UStaticMeshComponent* Shield = Mesh(Test, Unit, TEXT("ShieldMesh"));
		UStaticMeshComponent* Ring = Mesh(Test, Unit, TEXT("SelectionRingMesh"));
		UStaticMeshComponent* Structure = Mesh(Test, Building, TEXT("BuildingMesh"));
		UMaterialInstanceDynamic* BodyMID = Material(Test, Body, TEXT("MI_FactionRome"));
		UMaterialInstanceDynamic* ShieldMID = Material(Test, Shield, TEXT("MI_FactionRome"));
		if (!BodyMID || !ShieldMID || !Structure || !Ring) return true;
		Test.TestTrue(TEXT("Canonical unit color"), BodyMID->K2_GetVectorParameterValue(TEXT("Color")).Equals(Registry->FindFaction(EFactionID::Rome)->Definition.FactionColor));
		Test.TestTrue(TEXT("Shield uses a separate actor-local instance"), BodyMID != ShieldMID);
		FCall SetHealth(Test, Unit, TEXT("SetCurrentHealth"));
		SetHealth.Set<FIntProperty>(TEXT("NewHealth"), UnitDef->Definition.BaseStats.MaxHealth / 2); SetHealth.Execute();
		Test.TestTrue(TEXT("Damage reuses unit MID"), Body->GetMaterial(0) == BodyMID);
		Test.TestEqual(TEXT("Body damage follows committed health"), BodyMID->K2_GetScalarParameterValue(TEXT("Damage")), 1.f - Health(Test, Unit));
		Test.TestEqual(TEXT("Shield damage follows committed health"), ShieldMID->K2_GetScalarParameterValue(TEXT("Damage")), 1.f - Health(Test, Unit));
		SetHealth.Set<FIntProperty>(TEXT("NewHealth"), UnitDef->Definition.BaseStats.MaxHealth); SetHealth.Execute();
		Test.TestEqual(TEXT("Unit restoration clears damage"), BodyMID->K2_GetScalarParameterValue(TEXT("Damage")), 0.f);
		Select(Test, Unit, true);
		UMaterialInstanceDynamic* RingMID = Material(Test, Ring, TEXT("MI_Selection"));
		if (!RingMID) return true;
		Test.TestTrue(TEXT("White geometric selection"), RingMID->K2_GetVectorParameterValue(TEXT("Color")).Equals(FLinearColor::White) && Ring->IsVisible() && Ring->GetStaticMesh());
		Select(Test, Unit, false); Select(Test, Unit, true);
		Test.TestTrue(TEXT("Selection reuses instance"), Ring->GetMaterial(0) == RingMID);
		for (EFactionID Faction : {EFactionID::Carthage, EFactionID::Gaul, EFactionID::Rome})
		{
			Owner(Test, Unit, Faction);
			const TCHAR* Name = Faction == EFactionID::Rome ? TEXT("MI_FactionRome") : Faction == EFactionID::Gaul ? TEXT("MI_FactionGaul") : TEXT("MI_FactionCarthage");
			UMaterialInstanceDynamic* MID = Material(Test, Body, Name);
			if (MID) Test.TestTrue(TEXT("Owner change uses current canonical color"), MID->K2_GetVectorParameterValue(TEXT("Color")).Equals(Registry->FindFaction(Faction)->Definition.FactionColor));
		}
		const auto Preview = [&](bool Valid)
		{
			FCall Call(Test, Building, TEXT("SetPlacementMode"));
			Call.Set<FBoolProperty>(TEXT("bPlacing"), true); Call.Set<FBoolProperty>(TEXT("bValidPlacement"), Valid); Call.Execute();
			UMaterialInstanceDynamic* MID = Material(Test, Structure, Valid ? TEXT("MI_PreviewValid") : TEXT("MI_PreviewInvalid"));
			if (MID)
			{
				Test.TestEqual(TEXT("Preview is genuinely translucent"), MID->GetBlendMode(), BLEND_Translucent);
				Test.TestEqual(TEXT("Preview opacity"), MID->K2_GetScalarParameterValue(TEXT("Opacity")), 0.5f);
				Test.TestEqual(TEXT("Invalid preview has a non-color stripe cue"), MID->K2_GetScalarParameterValue(TEXT("PreviewPattern")), Valid ? 0.f : 1.f);
			}
			Test.TestFalse(TEXT("Preview has no actor collision"), Building->GetActorEnableCollision());
			return MID;
		};
		UMaterialInstanceDynamic* ValidMID = Preview(true);
		UMaterialInstanceDynamic* InvalidMID = Preview(false);
		Owner(Test, Building, EFactionID::Carthage);
		Test.TestTrue(TEXT("Ownership does not reset invalid preview"), Structure->GetMaterial(0) == InvalidMID);
		Test.TestTrue(TEXT("Valid preview is cached"), Preview(true) == ValidMID);
		Command(Test, Building, TEXT("StartConstruction"));
		Material(Test, Structure, TEXT("MI_Construction"));
		// Advance the real construction path rather than writing its state directly.
		Building->Tick(BuildingDef->Definition.ConstructionTime + 0.1f);
		UMaterialInstanceDynamic* CompleteMID = Material(Test, Structure, TEXT("MI_Surface"));
		if (!CompleteMID) return true;
		Test.TestTrue(TEXT("Complete building uses canonical definition and owner tint"), CompleteMID->K2_GetVectorParameterValue(TEXT("Color")).Equals(FMath::Lerp(BuildingDef->Definition.PrototypeColor, Registry->FindFaction(EFactionID::Carthage)->Definition.FactionColor, 0.25f)));
		Select(Test, Building, true);
		TArray<UStaticMeshComponent*> Parts; Building->GetComponents(Parts);
		int32 Edges = 0;
		for (UStaticMeshComponent* Part : Parts)
		{
			if (!Part->GetName().StartsWith(TEXT("SelectionOutline"))) continue;
			++Edges;
			Test.TestTrue(TEXT("Selection outline has real white geometry without collision/navigation"), Part->IsVisible() && !Part->bHiddenInGame && Part->GetStaticMesh() && Part->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Part->CanEverAffectNavigation());
			Test.TestEqual(TEXT("Persistent selection material"), Part->GetMaterial(0)->GetName(), FString(TEXT("MI_Selection")));
		}
		Test.TestEqual(TEXT("Four outline edges"), Edges, 4);
		FCall Damage(Test, Building, TEXT("ApplyBuildingDamage"));
		Damage.Set<FFloatProperty>(TEXT("Damage"), BuildingDef->Definition.MaxHealth * 0.65f + BuildingDef->Definition.Armor); Damage.Execute();
		UMaterialInstanceDynamic* DamagedMID = Material(Test, Structure, TEXT("MI_Damaged"));
		if (!DamagedMID) return true;
		Test.TestEqual(TEXT("Building damage follows committed health"), DamagedMID->K2_GetScalarParameterValue(TEXT("Damage")), 1.f - Health(Test, Building));
		FCall Repair(Test, Building, TEXT("Repair")); Repair.Set<FFloatProperty>(TEXT("Amount"), BuildingDef->Definition.MaxHealth); Repair.Execute();
		Test.TestTrue(TEXT("Repair restores cached complete material"), Structure->GetMaterial(0) == CompleteMID);
		Test.TestEqual(TEXT("Repair clears visual damage"), CompleteMID->K2_GetScalarParameterValue(TEXT("Damage")), 0.f);
		const TWeakObjectPtr<UMaterialInstanceDynamic> WeakComplete = CompleteMID;
		CollectGarbage(RF_NoFlags);
		Test.TestTrue(TEXT("Actor-local material cache survives real GC"), WeakComplete.IsValid() && Structure->GetMaterial(0) == WeakComplete.Get());
		Select(Test, Building, false);
		for (UStaticMeshComponent* Part : Parts) if (Part->GetName().StartsWith(TEXT("SelectionOutline"))) Test.TestFalse(TEXT("Deselection hides geometry"), Part->IsVisible());
		Select(Test, Building, true);
		Capture(World, UnitClass, BuildingClass, BuildingDef);
		Building->Destroy();
		for (UStaticMeshComponent* Part : Parts) if (Part->GetName().StartsWith(TEXT("SelectionOutline"))) Test.TestFalse(TEXT("EndPlay hides geometry"), Part->IsVisible());
		Unit->Destroy();
		Test.TestFalse(TEXT("Unit EndPlay hides geometry"), Ring->IsVisible());
		return !bCapturePending;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanMaterialPresentation, "RomanEmpireGame.Materials.ActorStatesPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanMaterialPresentation::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor required"), GEditor) || !TestNull(TEXT("Test owns fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanMaterialTests::FExercise(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
