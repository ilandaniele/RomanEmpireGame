// Private Editor-only source readback and real-player viewport proof.
#include "Misc/AutomationTest.h"
#include "Animation/AnimInstance.h"
#include "AssetCompilingManager.h"
#include "Camera/ZoomConfiguration.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/RomanEmpireGameState.h"
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
#include "PhysicsEngine/BodySetup.h"
#include "RHI.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "Slate/SceneViewport.h"
#include "Tests/AutomationEditorCommon.h"
#include "Units/RomanCharacterPresentation.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"
#include "UnrealClient.h"
#include "VisualRegression.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
namespace RomanCampPreview
{
constexpr const TCHAR* Names[] = { TEXT("SM_CampGround"), TEXT("SM_CampWall"), TEXT("SM_CampBarracks") };
constexpr int32 Triangles[] = { 1152, 3704, 46602 };
constexpr int32 Sections[] = { 1, 2, 5 };

bool IsKnownCamp(const FString& Name)
{
	return Name == TEXT("Camp01") || Name == TEXT("Camp02") || Name == TEXT("Camp03");
}
FString CampName()
{
	FString Name(TEXT("Camp01"));
	FParse::Value(FCommandLine::Get(), TEXT("RomanCampSource="), Name);
	return Name;
}
int32 ExpectedTriangles(const FString& Name, int32 Index)
{
	if (!IsKnownCamp(Name) || Index < 0 || Index >= static_cast<int32>(UE_ARRAY_COUNT(Triangles))) return INDEX_NONE;
	if (Index == 2 && Name == TEXT("Camp02")) return 28130;
	if (Index == 2 && Name == TEXT("Camp03")) return 30554;
	return Triangles[Index];
}
bool PrivateProject(FAutomationTestBase& Test)
{
	FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()); FPaths::NormalizeDirectoryName(Project);
	return Test.TestTrue(TEXT("Exact private project"), Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview")))
		&& Test.TestTrue(TEXT("Explicit supported camp namespace"), IsKnownCamp(CampName()));
}
UStaticMesh* LoadMesh(int32 Index)
{
	return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/ArtCandidates/%s/%s"), *CampName(), Names[Index]));
}
struct FCall
{
	FAutomationTestBase& Test; UObject* Target; UFunction* Function; FStructOnScope Params;
	FCall(FAutomationTestBase& T, UObject* O, FName Name) : Test(T), Target(O), Function(O ? O->FindFunction(Name) : nullptr), Params(Function)
	{ Test.TestNotNull(Name.ToString(), Function); }
	template<class P, class V> void Set(FName Name, V Value)
	{
		P* Property = Function ? FindFProperty<P>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name.ToString(), Property)) Property->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Object(FName Name, UObject* Value)
	{
		FObjectPropertyBase* Property = Function ? FindFProperty<FObjectPropertyBase>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name.ToString(), Property)) Property->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Faction()
	{
		FEnumProperty* Property = Function ? FindFProperty<FEnumProperty>(Function, TEXT("NewOwner")) : nullptr;
		if (Test.TestNotNull(TEXT("Faction parameter"), Property))
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(EFactionID::Rome));
	}
	void Run() { if (Function && IsValid(Target)) Target->ProcessEvent(Function, Params.GetStructMemory()); }
};
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; FString Output, Receipt;
	TWeakObjectPtr<ACharacter> Soldier;
	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<APawn> RTS;
	TArray<TWeakObjectPtr<AStaticMeshActor>> Camp;
	TArray<FColor> RTSAll, FPSAll;
	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> StableView;
	FTransform SavedRTS; float SavedZoom = 0.f;
	FVector HeldSoldierPosition, RTSCamera, FPSCamera;
	FRotator RTSCameraRotation, FPSCameraRotation;
	int32 Phase = 0; bool Complete = false;
	double Started = FPlatformTime::Seconds(), RenderStarted = 0;
	uint64 RenderFrame = 0;
	void Zoom(float Value)
	{ FCall Call(Test, RTS.Get(), TEXT("SetZoomLevel")); Call.Set<FFloatProperty>(TEXT("NewZoomLevel"), Value); Call.Run(); }
	void HoldSourcePose()
	{
		// Possession/exit legitimately re-enable movement. Re-park ONLY this owned
		// source-review actor; the visual ground must NOT gain gameplay collision.
		if (FParse::Param(FCommandLine::Get(), TEXT("RomanCampSkipPoseHold"))) return;
		Soldier->GetCharacterMovement()->DisableMovement();
		Soldier->GetCharacterMovement()->SetComponentTickEnabled(false);
		Soldier->SetActorTickEnabled(false); Soldier->GetMesh()->SetComponentTickEnabled(false);
	}
	void Arm(UWorld* World)
	{
		World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
		RenderStarted = FPlatformTime::Seconds(); RenderFrame = GFrameCounter;
	}
	bool Setup(UWorld* World)
	{
		Player = World->GetFirstPlayerController(); RTS = Player.IsValid() ? Player->GetPawn() : nullptr;
		FFloatProperty* CurrentZoom = RTS.IsValid() ? FindFProperty<FFloatProperty>(RTS->GetClass(), TEXT("CurrentZoomLevel")) : nullptr;
		if (!Test.TestTrue(TEXT("Actual controller/RTS pawn"), Player.IsValid() && CurrentZoom)) return false;
		SavedRTS = RTS->GetActorTransform(); SavedZoom = CurrentZoom->GetPropertyValue_InContainer(RTS.Get());
		StableView = RomanVisualRegression::Stabilize(World);
		if (UGameViewportClient* Client = World->GetGameViewport())
			if (FSceneViewport* Viewport = Client->GetGameViewport()) Viewport->SetViewportSize(1280, 720);
		const FVector Origin(440000, 440000, 0);
		FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		for (int32 Index = 0; Index < 3; ++Index)
		{
			UStaticMesh* Asset = LoadMesh(Index);
			if (!Test.TestNotNull(TEXT("Real versioned camp asset"), Asset)) return false;
			AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(Origin, FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Owned transient camp component"), Actor)) return false;
			Camp.Add(Actor);
			UStaticMeshComponent* Component = Actor->GetStaticMeshComponent();
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetCollisionEnabled(ECollisionEnabled::NoCollision); Component->SetCanEverAffectNavigation(false);
			Component->SetStaticMesh(Asset);
			Test.TestTrue(TEXT("Common origin/scale preserved"), Actor->GetActorLocation().Equals(Origin) && Actor->GetActorScale3D().Equals(FVector::OneVector));
			Test.TestTrue(TEXT("Art cannot affect collision/nav"), Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Component->CanEverAffectNavigation());
			for (int32 Slot = 0; Slot < Asset->GetStaticMaterials().Num(); ++Slot)
				Test.TestTrue(TEXT("Original imported PBR, no legacy tint/MID"), Component->GetMaterial(Slot) == Asset->GetStaticMaterials()[Slot].MaterialInterface);
		}
		TStrongObjectPtr<URomanCharacterPresentation> Config(LoadObject<URomanCharacterPresentation>(nullptr, TEXT("/Game/ArtCandidates/Attempt10_1/LOD01/DA_LODPreview")));
		UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
		FObjectPropertyBase* Art = Class ? FindFProperty<FObjectPropertyBase>(Class, TEXT("CharacterPresentation")) : nullptr;
		if (!Test.TestTrue(TEXT("Canonical soldier class/candidate"), Art && Config.IsValid())) return false;
		Spawn.bDeferConstruction = true;
		Soldier = World->SpawnActor<ACharacter>(Class, Origin + FVector(-175, 320, 96), FRotator(0, -55, 0), Spawn);
		if (!Test.TestNotNull(TEXT("Real soldier, not body proxy"), Soldier.Get())) return false;
		HeldSoldierPosition = Soldier->GetActorLocation();
		Art->SetObjectPropertyValue_InContainer(Soldier.Get(), Config.Get());
		FCall Init(Test, Soldier.Get(), TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Faction(); Init.Run();
		Soldier->FinishSpawning(Soldier->GetActorTransform()); Soldier->SetActorTickEnabled(false);
		FCall Hold(Test, Soldier.Get(), TEXT("CommandHold")); Hold.Run(); Soldier->GetCharacterMovement()->DisableMovement();
		USkeletalMeshComponent* Mesh = Soldier->GetMesh();
		if (!Test.TestTrue(TEXT("Canonical art and animation graph"), Mesh->GetSkeletalMeshAsset() == Config->RomeMesh && Mesh->GetAnimInstance())) return false;
		Mesh->SetComponentTickEnabled(false); Mesh->TickAnimation(.1f, false); Mesh->RefreshBoneTransforms();
		RTS->SetActorLocation(Origin); Zoom(RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical);
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		Receipt = FString::Printf(TEXT("camp=%s\nrenderer=D3D12/SM6\nviewport=1280x720\ncommon_scale=1\nart_collision=none\nart_nav=false\npose=held source review, NOT realtime battle/performance\n"), *CampName());
		Arm(World); return true;
	}
	bool Capture(UWorld* World, const TCHAR* Name, TArray<FColor>* Keep = nullptr, const TArray<FColor>* Against = nullptr)
	{
		Test.TestTrue(TEXT("Production pawn remains actual view target"), Player->GetViewTarget() == Player->GetPawn());
		if (!Test.TestTrue(TEXT("Owned source actor never falls/moves between art witnesses"), Soldier->GetActorLocation().Equals(HeldSoldierPosition,.1))) return false;
		FVector Camera; FRotator Rotation; Player->GetPlayerViewPoint(Camera, Rotation);
		if (Keep == &RTSAll) { RTSCamera = Camera; RTSCameraRotation = Rotation; }
		if (Keep == &FPSAll) { FPSCamera = Camera; FPSCameraRotation = Rotation; }
		if (Against)
		{
			const FVector& ExpectedCamera = Against == &RTSAll ? RTSCamera : FPSCamera;
			const FRotator& ExpectedRotation = Against == &RTSAll ? RTSCameraRotation : FPSCameraRotation;
			if (!Test.TestTrue(TEXT("Hidden witness uses IDENTICAL camera, not motion-induced pixels"), Camera.Equals(ExpectedCamera,.1) && Rotation.Equals(ExpectedRotation,.001))) return false;
		}
		UGameViewportClient* Client = World->GetGameViewport(); FViewport* Viewport = Client ? Client->Viewport : nullptr;
		if (!Test.TestNotNull(TEXT("Actual game viewport"), Viewport)) return false;
		TArray<FColor> Pixels;
		if (!Test.TestTrue(TEXT("Actual render buffer"), Viewport->ReadPixels(Pixels))) return false;
		if (!Test.TestTrue(TEXT("Complete1280x720 buffer"), Viewport->GetSizeXY() == FIntPoint(1280,720) && Pixels.Num() == 1280*720)) return false;
		if (Against)
		{
			if (!Test.TestEqual(TEXT("Witness buffer size"), Pixels.Num(), Against->Num())) return false;
			int32 Changed = 0;
			for (int32 Index = 0; Index < Pixels.Num(); ++Index)
				if (FMath::Abs(int32(Pixels[Index].R)-(*Against)[Index].R) > 8 || FMath::Abs(int32(Pixels[Index].G)-(*Against)[Index].G) > 8 || FMath::Abs(int32(Pixels[Index].B)-(*Against)[Index].B) > 8) ++Changed;
			Test.TestTrue(TEXT("Actual environment contributes visible pixels"), Changed > 100);
			Receipt += FString::Printf(TEXT("%s witness_pixels=%d\n"), Name, Changed);
		}
		Receipt += FString::Printf(TEXT("%s camera=%s rotation=%s pawn=%s\n"), Name, *Camera.ToString(), *Rotation.ToString(), *Player->GetPawn()->GetName());
		FImage Image; Image.Init(1280,720,ERawImageFormat::BGRA8,EGammaSpace::sRGB);
		FMemory::Memcpy(Image.RawData.GetData(),Pixels.GetData(),Pixels.Num()*sizeof(FColor));
		if (!Test.TestTrue(TEXT("Fresh native PNG"), FImageUtils::SaveImageByExtension(*FPaths::Combine(Output,Name),Image))) return false;
		if (Keep) *Keep = MoveTemp(Pixels);
		return true;
	}
public:
	FExercise(FAutomationTestBase& T, FString O) : Test(T), Output(MoveTemp(O)) {}
	virtual ~FExercise() override
	{
		if (Player.IsValid() && Soldier.IsValid() && Player->GetPawn() == Soldier.Get()) { FCall Exit(Test,Player.Get(),TEXT("ExitFirstPersonMode")); Exit.Run(); }
		if (RTS.IsValid()) { RTS->SetActorTransform(SavedRTS); Zoom(SavedZoom); }
		if (Soldier.IsValid()) Soldier->Destroy();
		for (const auto& Actor : Camp) if (Actor.IsValid()) Actor->Destroy();
		FAutomationTestExecutionInfo Execution; Test.GetExecutionInfo(Execution);
		Receipt += FString::Printf(TEXT("completed=%d\ntest_errors=%d\nowner_approved=false\nperformance_approved=false\n"), int32(Complete), Execution.GetErrorTotal());
		Test.TestTrue(TEXT("Create-only camp receipt"),FFileHelper::SaveStringToFile(Receipt,*FPaths::Combine(Output,TEXT("camp-gpu-receipt.txt"))));
	}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds()-Started > 180) { Test.AddError(TEXT("Camp viewport timeout")); return true; }
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
		if (!State || !State->IsGameplayInitialized()) return false;
		if (Phase == 0) { if (!Setup(World)) return true; Phase = 1; return false; }
		if (!Test.TestTrue(TEXT("Owned actors survive"), Player.IsValid() && Soldier.IsValid() && Camp.Num() == 3)) return true;
		if (FPlatformTime::Seconds()-RenderStarted < 1 || GFrameCounter < RenderFrame+12) return false;
		bool OK = true;
		switch (Phase)
		{
		case 1: OK = Capture(World,TEXT("RTSAll.png"),&RTSAll); Camp[0]->GetStaticMeshComponent()->SetVisibility(false); break;
		case 2: OK = Capture(World,TEXT("RTSHiddenGround.png"),nullptr,&RTSAll); Camp[0]->GetStaticMeshComponent()->SetVisibility(true); Camp[1]->GetStaticMeshComponent()->SetVisibility(false); break;
		case 3: OK = Capture(World,TEXT("RTSHiddenWall.png"),nullptr,&RTSAll); Camp[1]->GetStaticMeshComponent()->SetVisibility(true); Camp[2]->GetStaticMeshComponent()->SetVisibility(false); break;
		case 4:
			OK = Capture(World,TEXT("RTSHiddenBarracks.png"),nullptr,&RTSAll); Camp[2]->GetStaticMeshComponent()->SetVisibility(true);
			{ FCall Enter(Test,Player.Get(),TEXT("EnterFirstPersonMode")); Enter.Object(TEXT("UnitToPossess"),Soldier.Get()); Enter.Run(); }
			if (!Test.TestTrue(TEXT("Real possession pawn"), Player->GetPawn() == Soldier.Get())) return true;
			HoldSourcePose(); Player->SetControlRotation(FRotator(-3,-55,0)); Soldier->GetMesh()->TickAnimation(.2f,false); Soldier->GetMesh()->RefreshBoneTransforms(); break;
		case 5:
			Test.TestTrue(TEXT("FPS head hiding retained"), Soldier->GetMesh()->IsBoneHiddenByName(TEXT("head")));
			OK = Capture(World,TEXT("FPSAll.png"),&FPSAll); for (const auto& Actor : Camp) Actor->GetStaticMeshComponent()->SetVisibility(false); break;
		case 6:
			OK = Capture(World,TEXT("FPSHiddenCamp.png"),nullptr,&FPSAll); for (const auto& Actor : Camp) Actor->GetStaticMeshComponent()->SetVisibility(true);
			{ FCall Exit(Test,Player.Get(),TEXT("ExitFirstPersonMode")); Exit.Run(); }
			HoldSourcePose();
			Test.TestTrue(TEXT("Same RTS pawn after exit"), Player->GetPawn() == RTS.Get());
			Test.TestFalse(TEXT("Head restored on exit"), Soldier->GetMesh()->IsBoneHiddenByName(TEXT("head"))); break;
		case 7: OK = Capture(World,TEXT("RTSReturn.png")); Complete = OK; return true;
		}
		Arm(World); ++Phase; return !OK;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanCampReadback,"RomanPreview.Camp.Readback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanCampReadback::RunTest(const FString& Parameters)
{
	if (!RomanCampPreview::PrivateProject(*this)) return false;
	TestTrue(TEXT("Legacy camp accepted"), RomanCampPreview::IsKnownCamp(TEXT("Camp01")));
	TestTrue(TEXT("Revised camp accepted"), RomanCampPreview::IsKnownCamp(TEXT("Camp02")));
	TestTrue(TEXT("Third revision accepted"), RomanCampPreview::IsKnownCamp(TEXT("Camp03")));
	TestFalse(TEXT("Cross-namespace path rejected"), RomanCampPreview::IsKnownCamp(TEXT("../Camp01")));
	TestFalse(TEXT("Unknown profile rejected"), RomanCampPreview::IsKnownCamp(TEXT("Camp04")));
	TestEqual(TEXT("Legacy barrack count preserved"), RomanCampPreview::ExpectedTriangles(TEXT("Camp01"),2),46602);
	TestEqual(TEXT("Revised barrack count explicit"), RomanCampPreview::ExpectedTriangles(TEXT("Camp02"),2),28130);
	TestEqual(TEXT("Third barrack count explicit"), RomanCampPreview::ExpectedTriangles(TEXT("Camp03"),2),30554);
	TestEqual(TEXT("Invalid source cannot inherit default geometry"), RomanCampPreview::ExpectedTriangles(TEXT("Other"),2),INDEX_NONE);
	TestEqual(TEXT("Invalid mesh index rejected"), RomanCampPreview::ExpectedTriangles(TEXT("Camp02"),3),INDEX_NONE);
	FAssetCompilingManager::Get().FinishAllCompilation();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TStrongObjectPtr<UStaticMesh> Mesh(RomanCampPreview::LoadMesh(Index));
		if (!TestNotNull(TEXT("Actual saved StaticMesh"),Mesh.Get())) return false;
		FAssetCompilingManager::Get().FinishAllCompilation();
		TestEqual(TEXT("Native full-resolution triangles"),Mesh->GetNumTriangles(0),RomanCampPreview::ExpectedTriangles(RomanCampPreview::CampName(),Index));
		TestEqual(TEXT("Actual sections"),Mesh->GetNumSections(0),RomanCampPreview::Sections[Index]);
		TestEqual(TEXT("Single original LOD"),Mesh->GetNumLODs(),1); TestEqual(TEXT("Single source UV"),Mesh->GetNumTexCoords(0),1);
		const UBodySetup* Body = Mesh->GetBodySetup();
		TestTrue(TEXT("No generated simple collision"),!Body || Body->AggGeom.GetElementCount() == 0);
		AddInfo(FString::Printf(TEXT("%s triangles=%d sections=%d simpleCollision=%d bounds=%s"),RomanCampPreview::Names[Index],Mesh->GetNumTriangles(0),Mesh->GetNumSections(0),Body ? Body->AggGeom.GetElementCount() : 0,*Mesh->GetBoundingBox().GetSize().ToString()));
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanCampGPU,"RomanPreview.Camp.GPU",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanCampGPU::RunTest(const FString& Parameters)
{
	if (!RomanCampPreview::PrivateProject(*this)) return false;
	if (!TestTrue(TEXT("D3D12/SM6 actual renderer"),GDynamicRHI && FString(GDynamicRHI->GetName()) == TEXT("D3D12") && GMaxRHIShaderPlatform == SP_PCD3D_SM6)) return false;
	FString Name;
	if (!TestTrue(TEXT("Explicit fresh output namespace"),FParse::Value(FCommandLine::Get(),TEXT("RomanCampCapture="),Name) && !Name.IsEmpty())) return false;
	for (TCHAR C : Name) if (!TestTrue(TEXT("Safe output name"),FChar::IsAlnum(C) || C == TEXT('_'))) return false;
	const FString Output = FPaths::Combine(FPaths::ProjectDir(),TEXT("Artifacts"),Name);
	if (!TestFalse(TEXT("Never overwrite"),IFileManager::Get().DirectoryExists(*Output))) return false;
	if (!TestTrue(TEXT("Fresh evidence directory"),IFileManager::Get().MakeDirectory(*Output,true))) return false;
	if (!TestNotNull(TEXT("Editor"),GEditor) || !TestNull(TEXT("Fresh owned PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanCampPreview::FExercise(*this,Output)); AddCommand(new FEndPlayMapCommand()); return true;
}
#endif
