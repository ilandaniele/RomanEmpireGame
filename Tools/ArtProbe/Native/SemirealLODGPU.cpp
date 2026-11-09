// Private Editor viewport proof. No SceneCapture, asset save, camera/FOV shortcut.
#include "Misc/AutomationTest.h"
#include "Animation/AnimInstance.h"
#include "AssetCompilingManager.h"
#include "Camera/ZoomConfiguration.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/RomanEmpireGameState.h"
#include "CoreGlobals.h"
#include "DynamicRHI.h"
#include "Editor.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Faction/FactionData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "RHI.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "SkeletalRenderPublic.h"
#include "Slate/SceneViewport.h"
#include "Tests/AutomationEditorCommon.h"
#include "Units/RomanCharacterPresentation.h"
#include "Units/RomanSoldierAnimInstance.h"
#include "UnrealClient.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "VisualRegression.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanLODGPU
{
struct FCall
{
	FAutomationTestBase& Test; UObject* Target; UFunction* Function; FStructOnScope Params;
	FCall(FAutomationTestBase& T, UObject* O, FName Name)
		: Test(T), Target(O), Function(O ? O->FindFunction(Name) : nullptr), Params(Function) { Test.TestNotNull(Name.ToString(), Function); }
	template<typename P, typename V> void Set(FName Name, V Value)
	{
		P* Property = Function ? FindFProperty<P>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name.ToString(), Property)) Property->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Object(FName Name, UObject* Value)
	{
		FObjectPropertyBase* Property = Function ? FindFProperty<FObjectPropertyBase>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name.ToString(), Property)) Property->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Faction(EFactionID Value)
	{
		FEnumProperty* Property = Function ? FindFProperty<FEnumProperty>(Function, TEXT("NewOwner")) : nullptr;
		if (Test.TestNotNull(TEXT("Faction parameter"), Property))
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(Value));
	}
	void Run() { if (Function && IsValid(Target)) Target->ProcessEvent(Function, Params.GetStructMemory()); }
};
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; FString Output, Receipt;
	TWeakObjectPtr<ACharacter> Soldier;
	TWeakObjectPtr<AStaticMeshActor> Floor;
	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<APawn> RTSPawn;
	FTransform SavedRTS; float SavedZoom = 0.f;
	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> StableView;
	TArray<FColor> FPSPixels[4];
	int32 Phase = 0; bool Complete = false;
	double Started = FPlatformTime::Seconds(), RenderStarted = 0;
	uint64 RenderFrame = 0;
	void Zoom(float Value)
	{
		FCall Set(Test, RTSPawn.Get(), TEXT("SetZoomLevel")); Set.Set<FFloatProperty>(TEXT("NewZoomLevel"), Value); Set.Run();
	}
	void Arm(UWorld* World)
	{
		if (Soldier.IsValid()) { Soldier->GetMesh()->UpdateLODStatus(); Soldier->GetMesh()->RefreshBoneTransforms(); }
		World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
		RenderStarted = FPlatformTime::Seconds(); RenderFrame = GFrameCounter;
	}
	bool Setup(UWorld* World)
	{
		TStrongObjectPtr<URomanCharacterPresentation> Config(LoadObject<URomanCharacterPresentation>(nullptr,
			TEXT("/Game/ArtCandidates/Attempt10_1/LOD01/DA_LODPreview")));
		if (!Test.TestTrue(TEXT("Exact real LOD01 candidate"), Config.IsValid() && Config->bUseMeshMaterialParents && Config->RomeMesh && Config->RomeMesh->GetLODNum() == 3)) return false;
		Player = World->GetFirstPlayerController(); RTSPawn = Player.IsValid() ? Player->GetPawn() : nullptr;
		FFloatProperty* CurrentZoom = RTSPawn.IsValid() ? FindFProperty<FFloatProperty>(RTSPawn->GetClass(), TEXT("CurrentZoomLevel")) : nullptr;
		if (!Test.TestTrue(TEXT("Actual player and production RTS pawn"), Player.IsValid() && CurrentZoom)) return false;
		SavedRTS = RTSPawn->GetActorTransform(); SavedZoom = CurrentZoom->GetPropertyValue_InContainer(RTSPawn.Get());
		StableView = RomanVisualRegression::Stabilize(World);
		if (UGameViewportClient* Client = World->GetGameViewport())
			if (FSceneViewport* Viewport = Client->GetGameViewport()) Viewport->SetViewportSize(1280, 720);
		const FVector Origin(440000, 440000, 0);
		FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Floor = World->SpawnActor<AStaticMeshActor>(Origin + FVector(0, 0, -10), FRotator::ZeroRotator, Spawn);
		if (!Test.TestNotNull(TEXT("Owned transient ground"), Floor.Get())) return false;
		Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube")));
		Floor->SetActorScale3D(FVector(18, 18, .2)); Floor->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
		UMaterialInstanceDynamic* Ground = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/RomanVisual/MI_Surface")), Floor.Get());
		if (!Test.TestNotNull(TEXT("Owned ground material"), Ground)) return false;
		Ground->SetVectorParameterValue(TEXT("Color"), FLinearColor(.07f, .09f, .12f)); Floor->GetStaticMeshComponent()->SetMaterial(0, Ground);
		UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
		FObjectPropertyBase* Art = Class ? FindFProperty<FObjectPropertyBase>(Class, TEXT("CharacterPresentation")) : nullptr;
		if (!Test.TestNotNull(TEXT("Canonical actor bootstrap"), Art)) return false;
		Spawn.bDeferConstruction = true;
		Soldier = World->SpawnActor<ACharacter>(Class, Origin + FVector(0, 0, 96), FRotator::ZeroRotator, Spawn);
		if (!Test.TestNotNull(TEXT("Canonical soldier, not proxy mesh"), Soldier.Get())) return false;
		Art->SetObjectPropertyValue_InContainer(Soldier.Get(), Config.Get());
		FCall Init(Test, Soldier.Get(), TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Faction(EFactionID::Rome); Init.Run();
		Soldier->FinishSpawning(Soldier->GetActorTransform()); Soldier->SetActorTickEnabled(false);
		FCall Hold(Test, Soldier.Get(), TEXT("CommandHold")); Hold.Run(); Soldier->GetCharacterMovement()->DisableMovement();
		USkeletalMeshComponent* Mesh = Soldier->GetMesh();
		if (!Test.TestTrue(TEXT("Canonical actor bound candidate and graph"), Mesh->GetSkeletalMeshAsset() == Config->RomeMesh && Mesh->GetAnimInstance())) return false;
		const auto& Slots = Config->RomeMesh->GetMaterials();
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Index));
			Test.TestTrue(TEXT("Actor retains actual PBR parent"), MID && MID->Parent == Slots[Index].MaterialInterface);
		}
		Mesh->SetComponentTickEnabled(false); Mesh->TickAnimation(.1f, false); Mesh->RefreshBoneTransforms(); Mesh->SetForcedLOD(0);
		RTSPawn->SetActorLocation(Origin); Zoom(RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical);
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		Receipt = TEXT("candidate=/Game/ArtCandidates/Attempt10_1/LOD01/DA_LODPreview\nrenderer=D3D12/SM6\nviewport=1280x720\n");
		Arm(World); return true;
	}
	bool Capture(UWorld* World, const TCHAR* Name, int32 Expected, TArray<FColor>* Keep = nullptr)
	{
		USkeletalMeshComponent* Mesh = Soldier->GetMesh();
		const FSkeletalMeshObject* Object = Mesh->GetMeshObject();
		int32 RenderLOD = INDEX_NONE;
		float ScreenFactor = 0.f;
		if (Mesh->IsVisible())
		{
			if (!Test.TestNotNull(TEXT("Actual visible GPU mesh object"), Object)) return false;
			ENQUEUE_RENDER_COMMAND(RomanReadActualLOD)([Object, &RenderLOD, &ScreenFactor](FRHICommandListImmediate&) { RenderLOD = Object->GetLOD(); ScreenFactor = Object->MaxDistanceFactor; });
			FlushRenderingCommands();
		}
		else Test.TestEqual(TEXT("Hidden witness does not claim a rendered mesh LOD"), Expected, INDEX_NONE);
		FVector Camera; FRotator Rotation; Player->GetPlayerViewPoint(Camera, Rotation);
		Receipt += FString::Printf(TEXT("%s forced=%d predicted=%d render=%d screen_factor=%.6f camera=%s rotation=%s\n"),
			Name, Mesh->GetForcedLOD(), Mesh->GetPredictedLODLevel(), RenderLOD, ScreenFactor, *Camera.ToString(), *Rotation.ToString());
		Test.TestEqual(TEXT("Actual rendered LOD matches requested production stage"), RenderLOD, Expected);
		Test.TestTrue(TEXT("Actual production pawn/view target"), Player->GetViewTarget() == Player->GetPawn());
		UGameViewportClient* Client = World->GetGameViewport(); FViewport* Viewport = Client ? Client->Viewport : nullptr;
		if (!Test.TestNotNull(TEXT("Real game viewport"), Viewport)) return false;
		TArray<FColor> Pixels;
		if (!Test.TestTrue(TEXT("GPU viewport pixels read"), Viewport->ReadPixels(Pixels))) return false;
		const FIntPoint Size = Viewport->GetSizeXY();
		if (!Test.TestTrue(TEXT("Complete real1280x720 buffer"), Size == FIntPoint(1280, 720) && Pixels.Num() == 1280 * 720)) return false;
		FImage Image; Image.Init(Size.X, Size.Y, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		FMemory::Memcpy(Image.RawData.GetData(), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		if (!Test.TestTrue(TEXT("Fresh native PNG saved"), FImageUtils::SaveImageByExtension(*FPaths::Combine(Output, Name), Image))) return false;
		if (Keep) *Keep = MoveTemp(Pixels);
		return true;
	}
public:
	FExercise(FAutomationTestBase& T, FString O) : Test(T), Output(MoveTemp(O)) {}
	virtual ~FExercise() override
	{
		if (Soldier.IsValid())
		{
			Soldier->GetMesh()->SetVisibility(true); Soldier->GetMesh()->SetForcedLOD(0);
			if (Player.IsValid() && Player->GetPawn() == Soldier.Get()) { FCall Exit(Test, Player.Get(), TEXT("ExitFirstPersonMode")); Exit.Run(); }
			Test.TestFalse(TEXT("Exit restores actual head visibility"), Soldier->GetMesh()->IsBoneHiddenByName(TEXT("head")));
		}
		if (RTSPawn.IsValid()) { RTSPawn->SetActorTransform(SavedRTS); Zoom(SavedZoom); }
		if (Soldier.IsValid()) Soldier->Destroy(); if (Floor.IsValid()) Floor->Destroy();
		FAutomationTestExecutionInfo Execution; Test.GetExecutionInfo(Execution);
		Receipt += FString::Printf(TEXT("completed=%d\ntest_errors=%d\nperformance_verified=false\nowner_approved=false\n"), int32(Complete), Execution.GetErrorTotal());
		Test.TestTrue(TEXT("Create-only GPU receipt saved"), FFileHelper::SaveStringToFile(Receipt, *FPaths::Combine(Output, TEXT("gpu-lod-receipt.txt"))));
	}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Started > 180) { Test.AddError(TEXT("GPU LOD probe timeout")); return true; }
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
		if (!State || !State->IsGameplayInitialized()) return false;
		if (Phase == 0) { if (!Setup(World)) return true; Phase = 1; return false; }
		if (!Test.TestTrue(TEXT("Real soldier survives stages"), Soldier.IsValid() && Player.IsValid())) return true;
		USkeletalMeshComponent* Mesh = Soldier->GetMesh();
		// Frozen art pose, but genuine render-driven LOD feedback across frames.
		if (Mesh->UpdateLODStatus()) Mesh->RefreshBoneTransforms();
		if (FPlatformTime::Seconds() - RenderStarted < 1 || GFrameCounter < RenderFrame + 12) return false;
		bool OK = true;
		switch (Phase)
		{
		case 1:
			Test.TestEqual(TEXT("Automatic LOD enabled in default RTS"), Mesh->GetForcedLOD(), 0);
			OK = Capture(World, TEXT("AutoFarRTS.png"), 2);
			Zoom(FMath::Lerp(RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical, RomanEmpireCamera::GetDefaultZoomConfiguration().UserControlledMaximum, .75f)); break;
		case 2:
			OK = Capture(World, TEXT("AutoMidRTS.png"), 1);
			Zoom(RomanEmpireCamera::GetDefaultZoomConfiguration().UserControlledMaximum); break;
		case 3:
			OK = Capture(World, TEXT("AutoNearRTS.png"), 0);
			{ FCall Enter(Test, Player.Get(), TEXT("EnterFirstPersonMode")); Enter.Object(TEXT("UnitToPossess"), Soldier.Get()); Enter.Run(); }
			if (!Test.TestTrue(TEXT("Actual possession pawn"), Player->GetPawn() == Soldier.Get())) return true;
			Player->SetControlRotation(FRotator::ZeroRotator);
			Mesh->TickAnimation(.2f, false); Mesh->RefreshBoneTransforms(); break;
		case 4:
			Test.TestTrue(TEXT("Own head hidden, sword hand retained"), Mesh->IsBoneHiddenByName(TEXT("head")) && !Mesh->IsBoneHiddenByName(TEXT("hand_r")));
			Test.TestEqual(TEXT("Automatic LOD still enabled in FPS"), Mesh->GetForcedLOD(), 0);
			OK = Capture(World, TEXT("AutoFPS0.png"), 0, &FPSPixels[0]); Mesh->SetForcedLOD(2); break;
		case 5: OK = Capture(World, TEXT("ForcedFPS1.png"), 1, &FPSPixels[1]); Mesh->SetForcedLOD(3); break;
		case 6: OK = Capture(World, TEXT("ForcedFPS2.png"), 2, &FPSPixels[2]); Mesh->SetForcedLOD(1); break;
		case 7: OK = Capture(World, TEXT("ForcedFPS0.png"), 0, &FPSPixels[3]); Mesh->SetVisibility(false); break;
		case 8:
		{
			TArray<FColor> Hidden;
			OK = Capture(World, TEXT("HiddenFPS.png"), INDEX_NONE, &Hidden);
			for (int32 LOD = 0; OK && LOD < 4; ++LOD)
			{
				if (!Test.TestEqual(TEXT("Comparable own-mesh witness size"), FPSPixels[LOD].Num(), Hidden.Num())) return true;
				int32 Changed = 0;
				for (int32 Index = 0; Index < Hidden.Num(); ++Index)
					if (FMath::Abs(int32(Hidden[Index].R) - FPSPixels[LOD][Index].R) > 8 || FMath::Abs(int32(Hidden[Index].G) - FPSPixels[LOD][Index].G) > 8 || FMath::Abs(int32(Hidden[Index].B) - FPSPixels[LOD][Index].B) > 8) ++Changed;
				Test.TestTrue(TEXT("Every actual GPU LOD contributes visible own-body pixels"), Changed > 200);
				Receipt += FString::Printf(TEXT("fps_capture%d_mesh_witness_pixels=%d\n"), LOD, Changed);
			}
			Mesh->SetVisibility(true); Mesh->SetForcedLOD(0);
			{ FCall Exit(Test, Player.Get(), TEXT("ExitFirstPersonMode")); Exit.Run(); }
			Test.TestTrue(TEXT("Exit restores same RTS pawn"), Player->GetPawn() == RTSPawn.Get());
			Test.TestFalse(TEXT("Exit restores head"), Mesh->IsBoneHiddenByName(TEXT("head")));
			Mesh->TickAnimation(.2f, false); Mesh->RefreshBoneTransforms();
			{ URomanSoldierAnimInstance* Anim = Cast<URomanSoldierAnimInstance>(Mesh->GetAnimInstance());
			  Test.TestTrue(TEXT("Exit clears local FPS reach on same body"), Anim && !Anim->bLocalFirstPerson && Anim->FirstPersonHandTarget.IsNearlyZero() && Anim->FirstPersonElbowTarget.IsNearlyZero()); }
			Zoom(RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical); break;
		}
		case 9:
			Test.TestEqual(TEXT("Return restores automatic mesh LOD"), Mesh->GetForcedLOD(), 0);
			OK = Capture(World, TEXT("AutoReturnRTS.png"), 2); Complete = OK; return true;
		}
		Arm(World); ++Phase; return !OK;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanSemirealLODGPU, "RomanPreview.LOD.GPU",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanSemirealLODGPU::RunTest(const FString& Parameters)
{
	FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()); FPaths::NormalizeDirectoryName(Project);
	if (!TestTrue(TEXT("Exact private project only"), Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview")))) return false;
	if (!TestTrue(TEXT("Actual D3D12/SM6 required"), GDynamicRHI && FString(GDynamicRHI->GetName()) == TEXT("D3D12") && GMaxRHIShaderPlatform == SP_PCD3D_SM6)) return false;
	FString Name;
	if (!TestTrue(TEXT("Explicit new GPU evidence directory"), FParse::Value(FCommandLine::Get(), TEXT("RomanLODCapture="), Name) && !Name.IsEmpty())) return false;
	for (TCHAR Character : Name) if (!TestTrue(TEXT("Safe output namespace"), FChar::IsAlnum(Character) || Character == TEXT('_'))) return false;
	const FString Output = FPaths::Combine(Project, TEXT("Artifacts"), Name);
	if (!TestFalse(TEXT("Never overwrite GPU evidence"), IFileManager::Get().DirectoryExists(*Output))) return false;
	if (!TestTrue(TEXT("Fresh GPU evidence directory"), IFileManager::Get().MakeDirectory(*Output, true))) return false;
	if (!TestNotNull(TEXT("Editor"), GEditor) || !TestNull(TEXT("Own fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanLODGPU::FExercise(*this, Output)); AddCommand(new FEndPlayMapCommand()); return true;
}
#endif
