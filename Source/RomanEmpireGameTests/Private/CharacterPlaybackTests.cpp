// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "VisualRegression.h"
#include "AssetCompilingManager.h"
#include "Animation/AnimMontage.h"
#include "Components/SceneCaptureComponent2D.h"
#include "CoreGlobals.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/GameViewportClient.h"
#include "Slate/SceneViewport.h"
#include "Engine/SkeletalMesh.h"
#include "Faction/FactionData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RenderingThread.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"
#include "Tests/AutomationEditorCommon.h"
#include "Units/RomanCharacterPresentation.h"
#include "Camera/ZoomConfiguration.h"
#include "Units/RomanSoldierAnimInstance.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanCharacterPlayback
{
struct FCall
{
	FAutomationTestBase& Test; UObject* Target; UFunction* Function; FStructOnScope Params;
	FCall(FAutomationTestBase& InTest, UObject* InTarget, FName Name)
		: Test(InTest), Target(InTarget), Function(Target ? Target->FindFunction(Name) : nullptr), Params(Function)
	{ Test.TestNotNull(Name.ToString(), Function); }
	template<typename P, typename T> void Set(FName Name, T Value)
	{
		P* Property = Function ? FindFProperty<P>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name.ToString(), Property)) Property->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Faction(EFactionID Value)
	{
		FEnumProperty* Property = Function ? FindFProperty<FEnumProperty>(Function, TEXT("NewOwner")) : nullptr;
		if (Test.TestNotNull(TEXT("Canonical faction parameter"), Property))
			Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Params.GetStructMemory()), static_cast<int64>(Value));
	}
	void Object(FName Name, UObject* Value)
	{
		FObjectPropertyBase* Property = Function ? FindFProperty<FObjectPropertyBase>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name.ToString(), Property)) Property->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Value);
	}
	void Run() { if (Function && IsValid(Target)) Target->ProcessEvent(Function, Params.GetStructMemory()); }
};
struct FReleased
{
	TWeakObjectPtr<ACharacter> Unit;
	TWeakObjectPtr<USkeletalMeshComponent> Mesh;
	TWeakObjectPtr<URomanSoldierAnimInstance> Animation;
};
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FReleased> Released;
	TWeakObjectPtr<URomanCharacterPresentation> Config;
	TWeakObjectPtr<UMaterialInstanceDynamic> OldMID;
	FQuat IdleSpine, IdleThigh, IdleArm, HeldGuard, DeadRoot;
	int32 Phase = 0; double Started = FPlatformTime::Seconds();
	TArray<TWeakObjectPtr<AActor>> RenderActors;
	TWeakObjectPtr<ASceneCapture2D> Camera;
	TStrongObjectPtr<UTextureRenderTarget2D> Target;
	double CaptureStarted = 0; uint64 CaptureFrame = 0;
	FLinearColor ExpectedColors[3];
	TWeakObjectPtr<APlayerController> CapturePlayer;
	TWeakObjectPtr<ACharacter> CaptureSoldier;
	TWeakObjectPtr<APawn> CaptureRTSPawn;
	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> StableView;
	FTransform SavedRTSTransform;
	float SavedRTSZoom = 0;
	void Command(FName Name) { FCall Call(Test, Released->Unit.Get(), Name); Call.Run(); }
	void Step(float Seconds)
	{
		Released->Mesh->TickAnimation(Seconds, false);
		Released->Mesh->RefreshBoneTransforms();
	}
	FQuat Bone(const TCHAR* Name) const { return Released->Mesh->GetBoneQuaternion(Name, EBoneSpaces::ComponentSpace); }
	bool BeginCapture(UWorld* World)
	{
		if (RomanVisualRegression::Requested() || RomanVisualRegression::CandidatesRequested())
		{
			StableView = RomanVisualRegression::Stabilize(World);
			if (UGameViewportClient* Client = World->GetGameViewport())
				if (FSceneViewport* Viewport = Client->GetGameViewport()) Viewport->SetViewportSize(1280, 720);
		}
		if (!Test.TestFalse(TEXT("Raster capture requires real RHI"), FParse::Param(FCommandLine::Get(), TEXT("NullRHI")))) return false;
		const FVector Origin(420000, 420000, 0);
		const URomanContentRegistrySubsystem* Content = URomanContentRegistrySubsystem::Get(World);
		const URomanFactionDefinition* Rome = Content ? Content->FindFaction(EFactionID::Rome) : nullptr;
		const URomanFactionDefinition* Carthage = Content ? Content->FindFaction(EFactionID::Carthage) : nullptr;
		const URomanFactionDefinition* Gaul = Content ? Content->FindFaction(EFactionID::Gaul) : nullptr;
		if (!Test.TestTrue(TEXT("Capture reads canonical faction colors"), Rome && Carthage && Gaul)) return false;
		ExpectedColors[0] = Rome->Definition.FactionColor; ExpectedColors[1] = Carthage->Definition.FactionColor; ExpectedColors[2] = Gaul->Definition.FactionColor;
		FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(Origin + FVector(350, 0, -10), FRotator::ZeroRotator, Spawn);
		if (!Test.TestNotNull(TEXT("Character capture floor"), Floor)) return false;
		RenderActors.Add(Floor); Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
		Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube")));
		Floor->SetActorScale3D(FVector(14, 16, .2f));
		UMaterialInstanceDynamic* Ground = UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/RomanVisual/MI_Surface")), Floor);
		if (!Test.TestNotNull(TEXT("Character capture ground material"), Ground)) return false;
		Ground->SetVectorParameterValue(TEXT("Color"), FLinearColor(.07f, .09f, .12f)); Floor->GetStaticMeshComponent()->SetMaterial(0, Ground);
		for (int32 Row = 0; Row < 3; ++Row) for (int32 Pose = 0; Pose < 5; ++Pose)
		{
			Spawn.bDeferConstruction = true;
			ACharacter* Unit = World->SpawnActor<ACharacter>(Released->Unit->GetClass(), Origin + FVector(Row * 350, (Pose - 2) * 230, 96), FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Actual rendered soldier"), Unit)) return false;
			RenderActors.Add(Unit);
			if (Row == 0 && Pose == 0) CaptureSoldier = Unit;
			FCall Init(Test, Unit, TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Faction(EFactionID::Rome); Init.Run();
			Unit->FinishSpawning(Unit->GetActorTransform()); Unit->SetActorTickEnabled(false);
			FCall Hold(Test, Unit, TEXT("CommandHold")); Hold.Run(); Unit->GetCharacterMovement()->DisableMovement();
			if (Row > 0)
			{
				FCall Owner(Test, Unit, TEXT("SetOwnerFaction")); Owner.Faction(Row == 1 ? EFactionID::Carthage : EFactionID::Gaul); Owner.Run();
				Unit->SetActorRotation(FRotator(0, 180, 0)); // Opposing soldiers face the player, not away.
			}
			USkeletalMeshComponent* Mesh = Unit->GetMesh(); Mesh->SetComponentTickEnabled(false);
			if (!Test.TestTrue(TEXT("Capture uses actual native AnimBP, not pose image"), Mesh->GetSkeletalMeshAsset() && Cast<URomanSoldierAnimInstance>(Mesh->GetAnimInstance()))) return false;
			if (Pose == 1) Unit->GetCharacterMovement()->Velocity = FVector(150, 0, 0);
			if (Pose == 2) { FCall Guard(Test, Unit, TEXT("StartBlocking")); Guard.Run(); }
			if (Pose == 3)
			{
				FCall Possess(Test, Unit, TEXT("SetPossessedByPlayer")); Possess.Set<FBoolProperty>(TEXT("bPossessed"), true); Possess.Run();
				FCall Attack(Test, Unit, TEXT("PerformAttack")); Attack.Run();
			}
			if (Pose == 4) { FCall Kill(Test, Unit, TEXT("SetCurrentHealth")); Kill.Set<FIntProperty>(TEXT("NewHealth"), 0); Kill.Run(); Unit->SetLifeSpan(60.f); }
			Mesh->TickAnimation(Pose == 4 ? 1.1f : .25f, false); Mesh->RefreshBoneTransforms();
		}
		Spawn.bDeferConstruction = false;
		const FVector View = Origin + FVector(1700, -1000, 1300);
		ASceneCapture2D* Actor = World->SpawnActor<ASceneCapture2D>(View, (Origin + FVector(350, 0, 90) - View).Rotation(), Spawn);
		if (!Test.TestNotNull(TEXT("Character actual camera"), Actor)) return false;
		RenderActors.Add(Actor); Camera = Actor;
		Target.Reset(NewObject<UTextureRenderTarget2D>(Actor)); Target->InitCustomFormat(1280, 720, PF_B8G8R8A8, false); Target->ClearColor = FLinearColor::Black;
		USceneCaptureComponent2D* Capture = Actor->GetCaptureComponent2D(); Capture->TextureTarget = Target.Get();
		Capture->ProjectionType = ECameraProjectionMode::Orthographic; Capture->OrthoWidth = 2000;
		Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR; Capture->bCaptureEveryFrame = true;
		if (FParse::Param(FCommandLine::Get(), TEXT("RomanCharacterPlayerView")) || FParse::Param(FCommandLine::Get(), TEXT("RomanCharacterRTSView")))
		{
			CapturePlayer = World->GetFirstPlayerController();
			if (!Test.TestTrue(TEXT("Actual local controller/eligible capture soldier"), CapturePlayer.IsValid() && CaptureSoldier.IsValid())) return false;
			if (FParse::Param(FCommandLine::Get(), TEXT("RomanCharacterRTSView")))
			{
				CaptureRTSPawn = CapturePlayer->GetPawn();
				FFloatProperty* Zoom = CaptureRTSPawn.IsValid() ? FindFProperty<FFloatProperty>(CaptureRTSPawn->GetClass(), TEXT("CurrentZoomLevel")) : nullptr;
				if (!Test.TestTrue(TEXT("Real owned RTS camera pawn"), Zoom && CaptureRTSPawn->FindFunction(TEXT("SetZoomLevel")))) return false;
				SavedRTSTransform = CaptureRTSPawn->GetActorTransform(); SavedRTSZoom = Zoom->GetPropertyValue_InContainer(CaptureRTSPawn.Get());
				CaptureRTSPawn->SetActorLocation(Origin + FVector(350, 0, 0));
				FCall SetZoom(Test, CaptureRTSPawn.Get(), TEXT("SetZoomLevel")); SetZoom.Set<FFloatProperty>(TEXT("NewZoomLevel"), RomanEmpireCamera::GetDefaultZoomConfiguration().DefaultTactical); SetZoom.Run();
			}
			else
			{
				FCall Enter(Test, CapturePlayer.Get(), TEXT("EnterFirstPersonMode")); Enter.Object(TEXT("UnitToPossess"), CaptureSoldier.Get()); Enter.Run();
				if (!Test.TestTrue(TEXT("Real player possession committed"), CapturePlayer->GetPawn() == CaptureSoldier.Get())) return false;
				Test.TestTrue(TEXT("Actual local possession hides only rigged head"), CaptureSoldier->GetMesh()->IsBoneHiddenByName(TEXT("head")));
				Test.TestFalse(TEXT("Player sword arm remains visible"), CaptureSoldier->GetMesh()->IsBoneHiddenByName(TEXT("hand_r")));
				CaptureSoldier->GetMesh()->TickAnimation(.01f, false); CaptureSoldier->GetMesh()->RefreshBoneTransforms();
				CapturePlayer->SetControlRotation(FRotator::ZeroRotator);
			}
		}
		FAssetCompilingManager::Get().FinishAllCompilation(); World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
		CaptureStarted = FPlatformTime::Seconds(); CaptureFrame = GFrameCounter; return true;
	}
	bool FinishCapture()
	{
		if (FPlatformTime::Seconds() - CaptureStarted > 20) { Test.AddError(TEXT("Character real render frames timed out")); return true; }
		if (FPlatformTime::Seconds() - CaptureStarted < 1 || GFrameCounter < CaptureFrame + 12) return false;
		if (!Test.TestTrue(TEXT("Owned camera/target survived real render frames"), Camera.IsValid() && Target.IsValid())) return true;
		// The target has already captured every real frame; a redundant manual call warns.
		FImage Image;
		const FString Folder = FPaths::Combine(FPaths::ProjectDir(), TEXT("Artifacts/C017G_Rendered")); IFileManager::Get().MakeDirectory(*Folder, true);
		if (CapturePlayer.IsValid())
		{
			UWorld* World = GEditor->PlayWorld; UGameViewportClient* Client = World ? World->GetGameViewport() : nullptr;
			FViewport* Viewport = Client ? Client->Viewport : nullptr;
			if (!Test.TestTrue(TEXT("Actual committed player/RTS viewport"), Viewport && CapturePlayer->GetPawn() == (CaptureRTSPawn.IsValid() ? CaptureRTSPawn.Get() : CaptureSoldier.Get()))) return true;
			TArray<FColor> Pixels;
			if (!Test.TestTrue(TEXT("Actual player pixels read"), Viewport->ReadPixels(Pixels))) return true;
			const FIntPoint Size = Viewport->GetSizeXY(); Image.Init(Size.X, Size.Y, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
			if (!Test.TestEqual(TEXT("Complete real player pixel buffer"), Pixels.Num(), Size.X * Size.Y)) return true;
			FMemory::Memcpy(Image.RawData.GetData(), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		}
		else if (!Test.TestTrue(TEXT("Actual character raster read"), FImageUtils::GetRenderTargetImage(Target.Get(), Image))) return true;
		if (RomanVisualRegression::Requested() || RomanVisualRegression::CandidatesRequested())
			RomanVisualRegression::SaveAndCompare(Test, Image, CaptureRTSPawn.IsValid() ? TEXT("CharacterRTS") : CapturePlayer.IsValid() ? TEXT("CharacterPossession") : TEXT("CharacterProfiles"));
		else Test.TestTrue(TEXT("Actual profile/player image saved"), FImageUtils::SaveImageByExtension(*FPaths::Combine(Folder, CaptureRTSPawn.IsValid() ? TEXT("CharacterRTS.png") : CapturePlayer.IsValid() ? TEXT("CharacterPossession.png") : TEXT("CharacterProfiles.png")), Image));
		if (Test.TestEqual(TEXT("Actual capture BGRA8"), Image.Format, ERawImageFormat::BGRA8))
		{
			int32 Counts[3] = {}; const float Hues[] = {ExpectedColors[0].LinearRGBToHSV().R, ExpectedColors[1].LinearRGBToHSV().R, ExpectedColors[2].LinearRGBToHSV().R};
			for (const FColor Pixel : Image.AsBGRA8())
			{
				const FLinearColor HSV = FLinearColor(Pixel).LinearRGBToHSV();
				if (HSV.G < .25f || HSV.B < .1f) continue;
				for (int32 Profile = 0; Profile < 3; ++Profile)
				{ const float Difference = FMath::Abs(HSV.R - Hues[Profile]); if (FMath::Min(Difference, 360.f - Difference) < 18.f) ++Counts[Profile]; }
			}
			Test.AddInfo(FString::Printf(TEXT("Actual character pixels Rome=%d hue=%.1f Carthage=%d hue=%.1f Gaul=%d hue=%.1f"), Counts[0], Hues[0], Counts[1], Hues[1], Counts[2], Hues[2]));
			if (CaptureRTSPawn.IsValid()) Test.TestTrue(TEXT("Three actual factions visible at production tactical zoom"), Counts[0] > 100 && Counts[1] > 100 && Counts[2] > 100);
			else Test.TestTrue(TEXT("Actual opposing soldier visible to possessed player, not own-head occlusion"), Counts[1] > 1000);
			if (!CapturePlayer.IsValid()) Test.TestTrue(TEXT("All three real faction profiles visible, not blank/background"), Counts[0] > 1000 && Counts[2] > 1000);
		}
		return true;
	}
public:
	FExercise(FAutomationTestBase* InTest, TSharedRef<FReleased> InReleased) : Test(*InTest), Released(InReleased) {}
	virtual ~FExercise() override
	{
		if (CaptureRTSPawn.IsValid())
		{
			CaptureRTSPawn->SetActorTransform(SavedRTSTransform);
			FCall SetZoom(Test, CaptureRTSPawn.Get(), TEXT("SetZoomLevel")); SetZoom.Set<FFloatProperty>(TEXT("NewZoomLevel"), SavedRTSZoom); SetZoom.Run();
		}
		else if (CapturePlayer.IsValid())
		{
			FCall Exit(Test, CapturePlayer.Get(), TEXT("ExitFirstPersonMode")); Exit.Run();
			if (CaptureSoldier.IsValid()) Test.TestFalse(TEXT("Actual exit restores rigged head for RTS"), CaptureSoldier->GetMesh()->IsBoneHiddenByName(TEXT("head")));
		}
		for (const auto& Actor : RenderActors) if (Actor.IsValid()) Actor->Destroy();
		Target.Reset(); if (Released->Unit.IsValid()) Released->Unit->Destroy();
	}
	virtual bool Update() override
	{
		if (Phase != 10 && FPlatformTime::Seconds() - Started > 30) { Test.AddError(TEXT("Character pose PIE timed out")); return true; }
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
		if (!State || !State->IsGameplayInitialized()) return false;
		if (Phase == 10) return FinishCapture();
		if (Phase == 0)
		{
			Config = LoadObject<URomanCharacterPresentation>(nullptr, TEXT("/Game/Characters/RomanSoldier/DA_RomanCharacterPresentation"));
			UClass* UnitClass = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
			if (!Test.TestTrue(TEXT("Actual presentation bundle and factory"), Config.IsValid() && UnitClass)) return true;
			FActorSpawnParameters Spawn; Spawn.bDeferConstruction = true; Spawn.ObjectFlags |= RF_Transient;
			Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ACharacter* Unit = World->SpawnActor<ACharacter>(UnitClass, FVector(410000, 410000, 96), FRotator::ZeroRotator, Spawn);
			if (!Test.TestNotNull(TEXT("Actual unit"), Unit)) return true;
			Released->Unit = Unit;
			FCall Init(Test, Unit, TEXT("InitializeUnit")); Init.Set<FNameProperty>(TEXT("DefinitionID"), FName(TEXT("Legionary"))); Init.Faction(EFactionID::Rome); Init.Run();
			Unit->FinishSpawning(Unit->GetActorTransform()); Unit->SetActorTickEnabled(false); Command(TEXT("CommandHold"));
			Unit->GetCharacterMovement()->DisableMovement();
			Released->Mesh = Unit->GetMesh();
			Released->Animation = Cast<URomanSoldierAnimInstance>(Unit->GetMesh()->GetAnimInstance());
			if (!Test.TestTrue(TEXT("Live functional AnimBP class"), Released->Animation.IsValid() && Released->Animation->GetClass() == Config->AnimationClass)) return true;
			USkeletalMeshComponent* Mesh = Released->Mesh.Get(); Mesh->SetComponentTickEnabled(false);
			Test.TestTrue(TEXT("Actual visible skeletal profile"), Mesh->IsVisible() && !Mesh->bHiddenInGame && Mesh->GetSkeletalMeshAsset() == Config->RomeMesh);
			Test.TestEqual(TEXT("Mesh never creates second collision"), Mesh->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			Test.TestFalse(TEXT("Mesh does not modify navmesh"), Mesh->CanEverAffectNavigation());
			Test.TestEqual(TEXT("Capsule width retained"), Unit->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 42.f);
			Test.TestEqual(TEXT("Capsule height retained"), Unit->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight(), 96.f);
			Test.TestEqual(TEXT("Actual mesh ground offset"), Mesh->GetRelativeLocation(), FVector(0, 0, -96));
			TArray<UStaticMeshComponent*> Parts; Unit->GetComponents(Parts);
			for (const UStaticMeshComponent* Part : Parts)
				if (Part->GetFName() != TEXT("SelectionRing")) Test.TestTrue(TEXT("No duplicate procedural figure"), !Part->IsVisible() && Part->bHiddenInGame);
			Step(.01f); IdleSpine = Bone(TEXT("spine")); IdleThigh = Bone(TEXT("thigh_l")); IdleArm = Bone(TEXT("forearm_l"));
			const FSkeletalMeshModel* Imported = Mesh->GetSkeletalMeshAsset()->GetImportedModel();
			if (Test.TestTrue(TEXT("Actual imported facing witness"), Imported && Imported->LODModels.Num() == 1))
			{
				const int32 Foot = Mesh->GetBoneIndex(TEXT("foot_l")); FVector Center = FVector::ZeroVector; int32 Count = 0;
				for (const FSkelMeshSection& Section : Imported->LODModels[0].Sections)
					for (const FSoftSkinVertex& Vertex : Section.SoftVertices)
						for (int32 Influence = 0; Influence < MAX_TOTAL_INFLUENCES; ++Influence)
							if (Vertex.InfluenceWeights[Influence] > 0 && Section.BoneMap.IsValidIndex(Vertex.InfluenceBones[Influence])
								&& Section.BoneMap[Vertex.InfluenceBones[Influence]] == Foot) { Center += FVector(Vertex.Position); ++Count; }
				if (Test.TestTrue(TEXT("Actual toe vertices exist"), Count > 0))
				{
					const FVector Toe = Mesh->GetComponentTransform().TransformPosition(Center / Count) - Mesh->GetBoneLocation(TEXT("foot_l"));
					const double Forward = FVector::DotProduct(Toe, Unit->GetActorForwardVector());
					Test.AddInfo(FString::Printf(TEXT("Actual toe facing actor-forward=%.3fcm worlddelta=%s"), Forward, *Toe.ToString()));
					Test.TestTrue(TEXT("Visual toe points toward canonical forward/hit trace"), Forward > 3);
				}
				Test.TestTrue(TEXT("Rig left/right aligns canonical actor sides"), FVector::DotProduct(Mesh->GetBoneLocation(TEXT("foot_l")) - Unit->GetActorLocation(), Unit->GetActorRightVector()) < -5
					&& FVector::DotProduct(Mesh->GetBoneLocation(TEXT("foot_r")) - Unit->GetActorLocation(), Unit->GetActorRightVector()) > 5);
			}
			OldMID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
			Test.TestTrue(TEXT("Instance-local actual skeletal material"), OldMID.IsValid() && OldMID->GetOuter() == Unit);
		}
		else
		{
			if (!Test.TestTrue(TEXT("Actor/component/animation retained"), Released->Unit.IsValid() && Released->Mesh.IsValid() && Released->Animation.IsValid())) return true;
			ACharacter* Unit = Released->Unit.Get(); URomanSoldierAnimInstance* Animation = Released->Animation.Get();
			switch (Phase)
			{
			case 1:
				Step(.35f); Test.TestFalse(TEXT("Actual idle graph deforms spine"), Bone(TEXT("spine")).Equals(IdleSpine, .0001f)); break;
			case 2:
				Unit->GetCharacterMovement()->Velocity = FVector(150, 0, 0); Step(.25f);
				Test.TestTrue(TEXT("Snapshot derives movement from canonical velocity"), Animation->bMoving);
				Test.TestFalse(TEXT("Actual moving graph deforms leg"), Bone(TEXT("thigh_l")).Equals(IdleThigh, .01f)); break;
			case 3:
				Unit->GetCharacterMovement()->Velocity = FVector::ZeroVector; Command(TEXT("StartBlocking")); Step(.2f);
				Test.TestTrue(TEXT("Canonical block drives actual snapshot"), Animation->bBlocking);
				HeldGuard = Bone(TEXT("forearm_l"));
				Test.TestFalse(TEXT("Actual guard pose raises shield arm"), HeldGuard.Equals(IdleArm, .01f)); break;
			case 4:
				Step(.6f); Test.TestTrue(TEXT("Held block does not fall back to idle at clip end"), Bone(TEXT("forearm_l")).Equals(HeldGuard, .005f));
				Command(TEXT("StopBlocking"));
				{ FCall Possess(Test, Unit, TEXT("SetPossessedByPlayer")); Possess.Set<FBoolProperty>(TEXT("bPossessed"), true); Possess.Run(); }
				Command(TEXT("PerformAttack"));
				Test.TestTrue(TEXT("Accepted canonical attack starts actual montage"), Animation->Montage_IsPlaying(Config->AttackMontage));
				IdleArm = Bone(TEXT("forearm_r")); break;
			case 5:
				Step(.2f); Test.TestFalse(TEXT("Real montage slot deforms sword arm"), Bone(TEXT("forearm_r")).Equals(IdleArm, .01f));
				Test.TestTrue(TEXT("Actual animated sword hand advances toward canonical hit trace"),
					FVector::DotProduct(Unit->GetMesh()->GetBoneLocation(TEXT("hand_r")) - Unit->GetActorLocation(), Unit->GetActorForwardVector()) > 10);
				Command(TEXT("CommandStop")); Step(.1f);
				Test.TestFalse(TEXT("Canonical cancellation stops montage"), Animation->Montage_IsPlaying(Config->AttackMontage));
				{ FCall Owner(Test, Unit, TEXT("SetOwnerFaction")); Owner.Faction(EFactionID::Carthage); Owner.Run(); }
				Test.TestTrue(TEXT("Ownership selects distinct actual silhouette"), Unit->GetMesh()->GetSkeletalMeshAsset() == Config->CarthageMesh);
				Released->Animation = Cast<URomanSoldierAnimInstance>(Unit->GetMesh()->GetAnimInstance());
				Test.TestTrue(TEXT("Ownership replaces old actor MID"), Unit->GetMesh()->GetMaterial(0) != OldMID.Get()); break;
			case 6:
				{ FCall Health(Test, Unit, TEXT("SetCurrentHealth")); Health.Set<FIntProperty>(TEXT("NewHealth"), 50); Health.Run(); }
				for (const int32 Index : {0, 4, 5})
				{
					UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Unit->GetMesh()->GetMaterial(Index));
					if (Test.TestNotNull(TEXT("Canonical actor-owned damaged skeletal slot"), MID)) Test.TestTrue(TEXT("Actual committed health controls damage"), MID->K2_GetScalarParameterValue(TEXT("Damage")) > 0);
				}
				CollectGarbage(RF_NoFlags);
				Test.TestFalse(TEXT("Replaced MID actually collected"), OldMID.IsValid());
				Test.TestTrue(TEXT("CDO/instance retain bundle through real GC"), Config.IsValid());
				Step(.1f); Test.TestTrue(TEXT("Live graph retained through GC"), Released->Animation.IsValid()); break;
			case 7:
				{ FCall Owner(Test, Unit, TEXT("SetOwnerFaction")); Owner.Faction(EFactionID::Gaul); Owner.Run(); }
				Test.TestTrue(TEXT("Gaul ownership selects third actual silhouette"), Unit->GetMesh()->GetSkeletalMeshAsset() == Config->GaulMesh);
				Released->Animation = Cast<URomanSoldierAnimInstance>(Unit->GetMesh()->GetAnimInstance());
				Test.TestTrue(TEXT("Gaul retains shared live animation graph"), Released->Animation.IsValid() && Released->Animation->GetClass() == Config->AnimationClass);
				{ FCall Kill(Test, Unit, TEXT("SetCurrentHealth")); Kill.Set<FIntProperty>(TEXT("NewHealth"), 0); Kill.Run(); }
				Unit->SetLifeSpan(20.f); // Fixture-only extension to observe full death pose; production remains3s.
				Step(.12f); DeadRoot = Bone(TEXT("root"));
				Test.TestTrue(TEXT("Canonical death drives graph"), Animation->bDead);
				Test.TestFalse(TEXT("Corpse collision remains disabled"), Unit->GetActorEnableCollision()); break;
			case 8:
				Step(1.f); Test.TestFalse(TEXT("Actual nonloop death deforms root"), Bone(TEXT("root")).Equals(DeadRoot, .01f));
				DeadRoot = Bone(TEXT("root")); break;
			case 9:
				Step(.5f); Test.TestTrue(TEXT("Death holds final pose instead of looping/resurrecting"), Bone(TEXT("root")).Equals(DeadRoot, .005f));
				if (FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureCharacters")))
				{ if (!BeginCapture(World)) return true; ++Phase; return false; }
				Unit->Destroy(); return true;
			}
		}
		++Phase; return false;
	}
};
class FTeardown : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FReleased> Released; double Started = FPlatformTime::Seconds();
public:
	FTeardown(FAutomationTestBase* InTest, TSharedRef<FReleased> InReleased) : Test(*InTest), Released(InReleased) {}
	virtual bool Update() override
	{
		if (GEditor->PlayWorld && FPlatformTime::Seconds() - Started < 20) return false;
		Test.TestNull(TEXT("Owned PIE actually ended"), GEditor->PlayWorld);
		CollectGarbage(RF_NoFlags);
		Test.TestFalse(TEXT("Actual unit collected"), Released->Unit.IsValid());
		Test.TestFalse(TEXT("Actual skeletal component collected"), Released->Mesh.IsValid());
		Test.TestFalse(TEXT("Actual AnimInstance collected, no retained actor/world"), Released->Animation.IsValid());
		return true;
	}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanCharacterPlayback, "RomanEmpireGame.Characters.CanonicalPosePIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FRomanCharacterPlayback::RunTest(const FString& Parameters)
{
	const bool Visual = RomanVisualRegression::Requested() || RomanVisualRegression::CandidatesRequested();
	if (Visual)
	{
		if (!TestTrue(TEXT("Visual mode requires actual capture, never silent logic-only success"), FParse::Param(FCommandLine::Get(), TEXT("RomanCaptureCharacters")))) return false;
		if (!TestTrue(TEXT("Exactly one visual mode"), RomanVisualRegression::Requested() != RomanVisualRegression::CandidatesRequested())) return false;
		if (!TestFalse(TEXT("Player and RTS views are mutually exclusive"), FParse::Param(FCommandLine::Get(), TEXT("RomanCharacterPlayerView")) && FParse::Param(FCommandLine::Get(), TEXT("RomanCharacterRTSView")))) return false;
	}
	if (!TestNotNull(TEXT("Editor"), GEditor) || !TestNull(TEXT("Own fresh PIE"), GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	TSharedRef<RomanCharacterPlayback::FReleased> Released = MakeShared<RomanCharacterPlayback::FReleased>();
	AddCommand(new FStartPIECommand(false)); AddCommand(new RomanCharacterPlayback::FExercise(this, Released));
	AddCommand(new FEndPlayMapCommand()); AddCommand(new RomanCharacterPlayback::FTeardown(this, Released)); return true;
}
#endif
