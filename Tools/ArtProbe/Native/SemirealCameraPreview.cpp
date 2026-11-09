// Private Editor fixture only. Actual game viewport, not a SceneCapture proxy.
#include "Misc/AutomationTest.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "AssetCompilingManager.h"
#include "Camera/ZoomConfiguration.h"
#include "Components/CapsuleComponent.h"
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
namespace RomanSemirealCamera
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
		if (Test.TestNotNull(TEXT("Native faction parameter"), Property))
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(Value));
	}
	void Run() { if (Function && IsValid(Target)) Target->ProcessEvent(Function, Params.GetStructMemory()); }
};
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test;
	FString View, Output, Candidate;
	int32 Phase = 0; double Started = FPlatformTime::Seconds(), RenderStarted = 0; uint64 RenderFrame = 0;
	TArray<TWeakObjectPtr<AActor>> Actors;
	TWeakObjectPtr<ACharacter> Soldier;
	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<APawn> RTSPawn;
	FTransform SavedRTS; float SavedZoom = 0.f;
	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> StableView;
	TArray<FColor> VisiblePixels;
	TArray<FColor> AttackPixels;
	TWeakObjectPtr<UAnimMontage> AttackMontage;
	FString Receipt;
	bool Setup(UWorld* World)
	{
		TStrongObjectPtr<URomanCharacterPresentation> Config(LoadObject<URomanCharacterPresentation>(nullptr, *Candidate));
		if (!Test.TestTrue(TEXT("Actual opt-in candidate bundle"), Config.IsValid() && Config->bUseMeshMaterialParents && Config->RomeMesh)) return false;
		AttackMontage = Config->AttackMontage;
		Test.TestFalse(TEXT("This candidate keeps brass fittings untinted"), Config->FactionTintSlots.Contains(TEXT("Crest")));
		Player = World->GetFirstPlayerController(); RTSPawn = Player.IsValid() ? Player->GetPawn() : nullptr;
		FFloatProperty* Zoom = RTSPawn.IsValid() ? FindFProperty<FFloatProperty>(RTSPawn->GetClass(), TEXT("CurrentZoomLevel")) : nullptr;
		if (!Test.TestTrue(TEXT("Actual production camera pawn/controller"), Player.IsValid() && Zoom)) return false;
		SavedRTS = RTSPawn->GetActorTransform(); SavedZoom = Zoom->GetPropertyValue_InContainer(RTSPawn.Get());
		StableView = RomanVisualRegression::Stabilize(World); // Fixed exposure/time, not altered camera/FOV.
		if (UGameViewportClient* Client = World->GetGameViewport())
			if (FSceneViewport* Viewport = Client->GetGameViewport()) Viewport->SetViewportSize(1280, 720);
		const FVector Origin(440000, 440000, 0);
		FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(Origin + FVector(200, 0, -10), FRotator::ZeroRotator, Spawn);
		if (!Test.TestNotNull(TEXT("Private preview floor"), Floor)) return false;
		Actors.Add(Floor); Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube")));
		Floor->SetActorScale3D(FVector(18, 18, .2)); Floor->GetStaticMeshComponent()->SetCanEverAffectNavigation(false);
		UMaterialInstanceDynamic* Ground = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/RomanVisual/MI_Surface")), Floor);
		if (!Test.TestNotNull(TEXT("Owned floor material"), Ground)) return false;
		Ground->SetVectorParameterValue(TEXT("Color"), FLinearColor(.07f, .09f, .12f)); Floor->GetStaticMeshComponent()->SetMaterial(0, Ground);
		UClass* UnitClass = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
		FObjectPropertyBase* Art = UnitClass ? FindFProperty<FObjectPropertyBase>(UnitClass, TEXT("CharacterPresentation")) : nullptr;
		if (!Test.TestNotNull(TEXT("Native presentation bootstrap"), Art)) return false;
		for (int32 Index = 0; Index < 2; ++Index)
		{
			Spawn.bDeferConstruction = true;
			ACharacter* Unit = World->SpawnActor<ACharacter>(UnitClass, Origin + FVector(Index * 400, 0, 96), FRotator(0, Index ? 180 : 0, 0), Spawn);
			if (!Test.TestNotNull(TEXT("Real game soldier, not proxy mesh actor"), Unit)) return false;
			Actors.Add(Unit); if (Index == 0) Soldier = Unit;
			Art->SetObjectPropertyValue_InContainer(Unit, Config.Get());
			FCall Init(Test, Unit, TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Faction(EFactionID::Rome); Init.Run();
			Unit->FinishSpawning(Unit->GetActorTransform()); Unit->SetActorTickEnabled(false);
			FCall Hold(Test, Unit, TEXT("CommandHold")); Hold.Run(); Unit->GetCharacterMovement()->DisableMovement();
			if (Index) { FCall Owner(Test, Unit, TEXT("SetOwnerFaction")); Owner.Faction(EFactionID::Carthage); Owner.Run(); }
			USkeletalMeshComponent* Mesh = Unit->GetMesh();
			if (!Test.TestTrue(TEXT("Actual candidate mesh/animation bound by canonical actor"), Mesh->GetSkeletalMeshAsset() == Config->RomeMesh && Mesh->GetAnimInstance())) return false;
			const auto& Slots = Mesh->GetSkeletalMeshAsset()->GetMaterials();
			for (int32 Slot = 0; Slot < Slots.Num(); ++Slot)
			{
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Slot));
				if (!Test.TestTrue(TEXT("Rendered PBR parent retained by real faction update"), MID && MID->Parent == Slots[Slot].MaterialInterface)) return false;
			}
			Mesh->SetComponentTickEnabled(false); Mesh->TickAnimation(.1f, false); Mesh->RefreshBoneTransforms();
			const double Right = FVector::DotProduct(Mesh->GetBoneLocation(TEXT("hand_r")) - Unit->GetActorLocation(), Unit->GetActorRightVector());
			const double Left = FVector::DotProduct(Mesh->GetBoneLocation(TEXT("hand_l")) - Unit->GetActorLocation(), Unit->GetActorRightVector());
			Test.TestTrue(TEXT("Actual natural handedness in actor space"), Right > 0 && Left < 0);
			Receipt += FString::Printf(TEXT("unit%d_mesh=%s\nunit%d_right_cm=%.3f\nunit%d_left_cm=%.3f\n"), Index, *Mesh->GetSkeletalMeshAsset()->GetPathName(), Index, Right, Index, Left);
		}
		FCall Select(Test, Soldier.Get(), TEXT("SetSelected")); Select.Set<FBoolProperty>(TEXT("bNewSelected"), true); Select.Run();
		if (View == TEXT("RTS"))
		{
			RTSPawn->SetActorLocation(Origin + FVector(200, 0, 0));
			FCall SetZoom(Test, RTSPawn.Get(), TEXT("SetZoomLevel")); SetZoom.Set<FFloatProperty>(TEXT("NewZoomLevel"), RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical); SetZoom.Run();
			Test.TestEqual(TEXT("Production default tactical zoom"), Zoom->GetPropertyValue_InContainer(RTSPawn.Get()), RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical);
		}
		else
		{
			FCall Enter(Test, Player.Get(), TEXT("EnterFirstPersonMode")); Enter.Object(TEXT("UnitToPossess"), Soldier.Get()); Enter.Run();
			if (!Test.TestTrue(TEXT("Real local controller possession"), Player->GetPawn() == Soldier.Get())) return false;
			Test.TestTrue(TEXT("Local player head hidden"), Soldier->GetMesh()->IsBoneHiddenByName(TEXT("head")));
			Test.TestFalse(TEXT("Sword arm not hidden with head"), Soldier->GetMesh()->IsBoneHiddenByName(TEXT("hand_r")));
			Player->SetControlRotation(FRotator::ZeroRotator);
			Soldier->GetMesh()->TickAnimation(.2f, false); Soldier->GetMesh()->RefreshBoneTransforms();
		}
		FAssetCompilingManager::Get().FinishAllCompilation();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
		FVector Camera; FRotator Rotation; Player->GetPlayerViewPoint(Camera, Rotation);
		Receipt += TEXT("view=") + View + TEXT("\nsetup_cached_camera=") + Camera.ToString() + TEXT("\nsetup_cached_rotation=") + Rotation.ToString() + TEXT("\n");
		RenderStarted = FPlatformTime::Seconds(); RenderFrame = GFrameCounter; return true;
	}
	bool Capture(UWorld* World, const TCHAR* Name, TArray<FColor>& Pixels)
	{
		UGameViewportClient* Client = World->GetGameViewport(); FViewport* Viewport = Client ? Client->Viewport : nullptr;
		if (!Test.TestTrue(TEXT("Actual owned game viewport/controller"), Viewport && Player.IsValid() && Player->GetPawn() == (View == TEXT("FPS") ? static_cast<APawn*>(Soldier.Get()) : RTSPawn.Get()))) return false;
		Test.TestTrue(TEXT("Production pawn is the actual camera view target"), Player->GetViewTarget() == Player->GetPawn());
		// Sample after real render frames: setup can still see last frame's cache.
		FVector Camera; FRotator Rotation; Player->GetPlayerViewPoint(Camera, Rotation);
		Receipt += FString(Name) + TEXT("_camera=") + Camera.ToString() + TEXT("\n") + FString(Name) + TEXT("_rotation=") + Rotation.ToString() + TEXT("\n");
		Receipt += FString(Name) + TEXT("_soldier_location=") + Soldier->GetActorLocation().ToString() + TEXT("\n");
		FlushRenderingCommands();
		if (!Test.TestTrue(TEXT("Actual GPU viewport pixels"), Viewport->ReadPixels(Pixels))) return false;
		const FIntPoint Size = Viewport->GetSizeXY();
		if (!Test.TestEqual(TEXT("Complete BGRA buffer"), Pixels.Num(), Size.X * Size.Y)) return false;
		FImage Image; Image.Init(Size.X, Size.Y, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		FMemory::Memcpy(Image.RawData.GetData(), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		return Test.TestTrue(TEXT("Create-only native viewport PNG saved"), FImageUtils::SaveImageByExtension(*FPaths::Combine(Output, Name), Image));
	}
public:
	FExercise(FAutomationTestBase* T, FString V, FString O, FString C) : Test(*T), View(MoveTemp(V)), Output(MoveTemp(O)), Candidate(MoveTemp(C)) {}
	virtual ~FExercise() override
	{
		if (Player.IsValid() && Soldier.IsValid() && Player->GetPawn() == Soldier.Get())
		{
			FCall Exit(Test, Player.Get(), TEXT("ExitFirstPersonMode")); Exit.Run();
			Test.TestTrue(TEXT("Exit restores original RTS pawn identity"), Player->GetPawn() == RTSPawn.Get());
			Test.TestFalse(TEXT("Exit restores head for RTS"), Soldier->GetMesh()->IsBoneHiddenByName(TEXT("head")));
			Soldier->GetMesh()->TickAnimation(.2f, false); Soldier->GetMesh()->RefreshBoneTransforms();
			URomanSoldierAnimInstance* Animation = Cast<URomanSoldierAnimInstance>(Soldier->GetMesh()->GetAnimInstance());
			if (Test.TestNotNull(TEXT("Same body retains canonical animation instance after exit"), Animation))
			{
				Test.TestFalse(TEXT("Exit disables local first-person reach"), Animation->bLocalFirstPerson);
				Test.TestTrue(TEXT("Exit clears first-person targets"), Animation->FirstPersonHandTarget.IsNearlyZero() && Animation->FirstPersonElbowTarget.IsNearlyZero());
			}
		}
		if (RTSPawn.IsValid())
		{
			RTSPawn->SetActorTransform(SavedRTS);
			FCall Zoom(Test, RTSPawn.Get(), TEXT("SetZoomLevel")); Zoom.Set<FFloatProperty>(TEXT("NewZoomLevel"), SavedZoom); Zoom.Run();
		}
		for (const auto& Actor : Actors) if (Actor.IsValid()) Actor->Destroy();
	}
	virtual bool Update() override
	{
		if (FPlatformTime::Seconds() - Started > 180) { Test.AddError(TEXT("Private camera preview timed out")); return true; }
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
		if (!State || !State->IsGameplayInitialized()) return false;
		if (Phase == 0) { if (!Setup(World)) return true; ++Phase; return false; }
		if (FPlatformTime::Seconds() - RenderStarted < 1 || GFrameCounter < RenderFrame + 12) return false;
		if (Phase == 1)
		{
			if (!Capture(World, TEXT("Visible.png"), VisiblePixels)) return true;
			for (const auto& Actor : Actors) if (ACharacter* Unit = Cast<ACharacter>(Actor.Get())) Unit->GetMesh()->SetVisibility(false);
			World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
			RenderStarted = FPlatformTime::Seconds(); RenderFrame = GFrameCounter; ++Phase; return false;
		}
		if (Phase == 3)
		{
			UAnimInstance* Animation = Soldier->GetMesh()->GetAnimInstance();
			if (!Test.TestTrue(TEXT("Live canonical attack for FPS apex"), Animation && AttackMontage.IsValid() && Animation->Montage_IsPlaying(AttackMontage.Get()))) return true;
			const float Rate = Animation->Montage_GetPlayRate(AttackMontage.Get());
			if (!Test.TestTrue(TEXT("Finite positive canonical montage rate"), FMath::IsFinite(Rate) && Rate > 0.f)) return true;
			Soldier->GetMesh()->TickAnimation(AttackMontage->GetPlayLength() * .5f / Rate, false); Soldier->GetMesh()->RefreshBoneTransforms();
			FVector2D Hand;
			const bool Projected = Player->ProjectWorldLocationToScreen(Soldier->GetMesh()->GetBoneLocation(TEXT("hand_r")), Hand);
			Receipt += TEXT("attack_hand_screen=") + Hand.ToString() + TEXT("\n");
			Test.TestTrue(TEXT("Own attack hand projects inside real FPS view"), Projected && Hand.X >= 0 && Hand.X < 1280 && Hand.Y >= 0 && Hand.Y < 720);
			World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
			RenderStarted = FPlatformTime::Seconds(); RenderFrame = GFrameCounter; ++Phase; return false;
		}
		if (Phase == 4)
		{
			if (!Capture(World, TEXT("Attack.png"), AttackPixels)) return true;
			Soldier->GetMesh()->SetVisibility(false); World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
			RenderStarted = FPlatformTime::Seconds(); RenderFrame = GFrameCounter; ++Phase; return false;
		}
		const bool AttackWitness = Phase == 5;
		TArray<FColor> Hidden; if (!Capture(World, AttackWitness ? TEXT("AttackOwnHidden.png") : TEXT("HiddenWitness.png"), Hidden)) return true;
		const TArray<FColor>& Reference = AttackWitness ? AttackPixels : VisiblePixels;
		if (!Test.TestEqual(TEXT("Paired witness buffer size"), Hidden.Num(), Reference.Num())) return true;
		int32 Changed = 0;
		for (int32 Index = 0; Index < Hidden.Num(); ++Index)
			if (FMath::Abs(int32(Hidden[Index].R) - Reference[Index].R) > 8 || FMath::Abs(int32(Hidden[Index].G) - Reference[Index].G) > 8 || FMath::Abs(int32(Hidden[Index].B) - Reference[Index].B) > 8) ++Changed;
		Test.AddInfo(FString::Printf(TEXT("Actual mesh visibility witness changed_pixels=%d"), Changed));
		Test.TestTrue(TEXT("Real candidate affects GPU viewport, not only UI/background"), Changed > 200);
		Receipt += FString::Printf(TEXT("%s=%d\n"), AttackWitness ? TEXT("own_attack_changed_pixels") : TEXT("mesh_visibility_changed_pixels"), Changed);
		if (View == TEXT("FPS") && !AttackWitness)
		{
			for (const auto& Actor : Actors) if (ACharacter* Unit = Cast<ACharacter>(Actor.Get())) Unit->GetMesh()->SetVisibility(true);
			FCall Attack(Test, Soldier.Get(), TEXT("PerformAttack")); Attack.Run();
			RenderStarted = FPlatformTime::Seconds(); RenderFrame = GFrameCounter; Phase = 3; return false;
		}
		Receipt += TEXT("owner_approved=false\n");
		Test.TestTrue(TEXT("Native camera receipt saved"), FFileHelper::SaveStringToFile(Receipt, *FPaths::Combine(Output, TEXT("camera-receipt.txt"))));
		return true;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanSemirealCameraPreview, "RomanPreview.Cameras.Candidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanSemirealCameraPreview::RunTest(const FString& Parameters)
{
	FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()); FPaths::NormalizeDirectoryName(Project);
	if (!TestTrue(TEXT("Exact private project only"), Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview")))) return false;
	if (!TestTrue(TEXT("Actual D3D12/SM6, no NullRHI substitution"), GDynamicRHI && FString(GDynamicRHI->GetName()) == TEXT("D3D12") && GMaxRHIShaderPlatform == SP_PCD3D_SM6)) return false;
	FString View, Name, Candidate;
	if (!TestTrue(TEXT("Explicit RTS or FPS camera"), FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealCamera="), View) && (View == TEXT("RTS") || View == TEXT("FPS")))) return false;
	const bool HasName = FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealCapture="), Name);
	bool SafeName = HasName && !Name.IsEmpty();
	for (TCHAR Character : Name) SafeName &= FChar::IsAlnum(Character) || Character == TEXT('_');
	if (!TestTrue(TEXT("Explicit safe create-only output name"), SafeName)) return false;
	if (!TestTrue(TEXT("Explicit candidate namespace"), FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealPresentation="), Candidate) && Candidate.StartsWith(TEXT("/Game/ArtCandidates/Attempt10_1/Runtime")) && !Candidate.Contains(TEXT("..")))) return false;
	const FString Output = FPaths::Combine(Project, TEXT("Artifacts"), Name);
	if (!TestFalse(TEXT("Refuse overwrite of existing camera evidence"), IFileManager::Get().DirectoryExists(*Output))) return false;
	if (!TestTrue(TEXT("New evidence folder created"), IFileManager::Get().MakeDirectory(*Output, true))) return false;
	if (!TestNotNull(TEXT("Editor"), GEditor) || !TestNull(TEXT("Own fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanSemirealCamera::FExercise(this, View, Output, Candidate)); AddCommand(new FEndPlayMapCommand()); return true;
}
#endif
