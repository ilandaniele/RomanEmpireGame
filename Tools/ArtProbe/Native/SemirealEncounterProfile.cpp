// Private, process-only observer for EXISTING Encounter50/100 tests.
// No alternate battle, paused actors, quality overrides or asset saves.
#include "Misc/AutomationTest.h"
#include "Animation/AnimInstance.h"
#include "AssetCompilingManager.h"
#include "Components/SkeletalMeshComponent.h"
#include "Core/RomanEmpireGameState.h"
#include "CoreGlobals.h"
#include "DynamicRHI.h"
#include "DynamicResolutionState.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "RHI.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "Units/RomanCharacterPresentation.h"
#include "Units/RomanUnitRegistry.h"
#include "UnrealClient.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
CSV_DEFINE_CATEGORY(RomanSemireal, true);
namespace RomanSemirealEncounter
{
struct FViewObservation
{
	int32 Families = 0;
	float PrimaryMin = TNumericLimits<float>::Max(), PrimaryMax = 0.f;
	float SecondaryMin = TNumericLimits<float>::Max(), SecondaryMax = 0.f;
	int32 OutputWidth = 0, OutputHeight = 0;
};
// Observe only this PIE world's actual game view family. No show flag,
// resolution driver, view, camera, post-process or renderer setting is modified.
class FReadOnlyView : public FWorldSceneViewExtension
{
	TSharedRef<FViewObservation, ESPMode::ThreadSafe> Data;
public:
	FReadOnlyView(const FAutoRegister& Register, UWorld* World, TSharedRef<FViewObservation, ESPMode::ThreadSafe> InData)
		: FWorldSceneViewExtension(Register, World), Data(InData) {}
	virtual void BeginRenderViewFamily(FSceneViewFamily& Family) override
	{
		const ISceneViewFamilyScreenPercentage* Driver = Family.GetScreenPercentageInterface();
		if (!Driver || Family.Views.IsEmpty() || !Family.Views[0]->bIsGameView) return;
		const float Primary = Driver->GetResolutionFractionsUpperBound()[GDynamicPrimaryResolutionFraction];
		Data->PrimaryMin = FMath::Min(Data->PrimaryMin, Primary); Data->PrimaryMax = FMath::Max(Data->PrimaryMax, Primary);
		Data->SecondaryMin = FMath::Min(Data->SecondaryMin, Family.SecondaryViewFraction); Data->SecondaryMax = FMath::Max(Data->SecondaryMax, Family.SecondaryViewFraction);
		Data->OutputWidth = Family.Views[0]->UnscaledViewRect.Width(); Data->OutputHeight = Family.Views[0]->UnscaledViewRect.Height();
		++Data->Families;
	}
};
class FProbe
{
	FDelegateHandle PreHandle, WorldInitHandle, SpawnHandle, TickHandle, EndHandle, RegistryHandle;
	FAutomationTestBase* Test = nullptr;
	FString Name, Output, Receipt, Rows;
	int32 Desired = 0, Frames = 0, FullCSVFrames = 0, FullRTSCSVFrames = 0, Peak = 0, BoundBeforeInit = 0;
	bool Active = false, Dirty = true, Screenshot = false, ReportedInvalid = false, RuntimeSnapshot = false;
	double PreviousWall = 0;
	UClass* UnitClass = nullptr;
	TWeakObjectPtr<UObject> CDO;
	FObjectPropertyBase* Art = nullptr;
	TStrongObjectPtr<UObject> Original;
	TStrongObjectPtr<URomanCharacterPresentation> Candidate;
	TWeakObjectPtr<URomanUnitRegistrySubsystem> Registry;
	TWeakObjectPtr<UWorld> SpawnWorld;
	TArray<TWeakObjectPtr<ACharacter>> Units;
	TSharedRef<FViewObservation, ESPMode::ThreadSafe> ViewData = MakeShared<FViewObservation, ESPMode::ThreadSafe>();
	TSharedPtr<ISceneViewExtension, ESPMode::ThreadSafe> ViewExtension;
	bool Require(bool Value, const TCHAR* Message)
	{
		if (!Value && Test) Test->AddError(Message);
		return Value;
	}
	void Invalidate() { Dirty = true; }
	void Restore()
	{
		if (SpawnWorld.IsValid() && SpawnHandle.IsValid()) SpawnWorld->RemoveOnActorPreSpawnInitialization(SpawnHandle);
		SpawnHandle.Reset(); SpawnWorld.Reset();
		ViewExtension.Reset();
		if (Registry.IsValid() && RegistryHandle.IsValid()) Registry->OnUnitRegistryChanged.Remove(RegistryHandle);
		RegistryHandle.Reset(); Registry.Reset(); Candidate.Reset(); Original.Reset(); Active = false;
	}
	void Begin(bool Simulating)
	{
		Test = FAutomationTestFramework::Get().GetCurrentTest();
		FString Project = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()); FPaths::NormalizeDirectoryName(Project);
		if (!Require(Test && !Simulating && !Active && Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview")), TEXT("Semireal profiler requires exact private project and owned test PIE"))) return;
		bool Safe = !Name.IsEmpty() && Name.Len() <= 48;
		for (TCHAR Character : Name) Safe &= FChar::IsAlnum(Character) || Character == TEXT('_');
		if (!Require(Safe && (Desired == 50 || Desired == 100), TEXT("Explicit safe profiler name and total50/100 required"))) return;
		const FString ExpectedTest = FString::Printf(TEXT("RomanEmpireGame.Performance.Encounter%dPIE"), Desired);
		if (!Require(Test->GetTestFullName() == ExpectedTest, TEXT("Profiler total must match the EXISTING canonical encounter test"))) return;
		if (!Require(GDynamicRHI && FString(GDynamicRHI->GetName()) == TEXT("D3D12") && GMaxRHIShaderPlatform == SP_PCD3D_SM6,
			TEXT("Semireal performance requires actual D3D12/SM6, no NullRHI substitution"))) return;
		Output = FPaths::Combine(Project, TEXT("Artifacts/SemirealPerformance"), Name);
		if (!Require(!IFileManager::Get().DirectoryExists(*Output), TEXT("Refuse overwrite of profiler evidence")) ||
			!Require(IFileManager::Get().MakeDirectory(*Output, true), TEXT("Cannot create profiler evidence folder"))) return;
		UnitClass = LoadClass<ACharacter>(nullptr, TEXT("/Script/RomanEmpireGame.Legionary"));
		CDO = UnitClass ? UnitClass->GetDefaultObject() : nullptr;
		Art = UnitClass ? FindFProperty<FObjectPropertyBase>(UnitClass, TEXT("CharacterPresentation")) : nullptr;
		Candidate.Reset(LoadObject<URomanCharacterPresentation>(nullptr, TEXT("/Game/ArtCandidates/Attempt10_1/LOD01/DA_LODPreview")));
		if (!Require(CDO.IsValid() && Art && Candidate.IsValid() && Candidate->RomeMesh && Candidate->RomeMesh->GetLODNum() == 3,
			TEXT("Verified candidate and canonical class default required"))) return;
		Original.Reset(Art->GetObjectPropertyValue_InContainer(CDO.Get()));
		if (!Require(Original.IsValid(), TEXT("Keep original default for process-only restoration"))) return;
		FAssetCompilingManager::Get().FinishAllCompilation();
		// Native constructors assign their own bootstrap on EVERY instance; changing
		// a CDO does not reliably replace that. Install per-instance before any init.
		Active = true; Dirty = true; PreviousWall = 0;
		Receipt = FString::Printf(TEXT("candidate=%s\ncanonical_test=%s\ninitial_total=%d\nrhi=%s\nadapter=%s\ncpu=%s\nphysical_ram_bytes=%llu\n"),
			*Candidate->GetPathName(), *ExpectedTest, Desired, GDynamicRHI->GetName(), *GRHIAdapterName, *FPlatformMisc::GetCPUBrand(), FPlatformMemory::GetStats().TotalPhysical);
		for (const TCHAR* Key : {TEXT("r.ScreenPercentage"), TEXT("r.DynamicRes.OperationMode"), TEXT("r.VSync"), TEXT("t.MaxFPS"),
			TEXT("sg.ViewDistanceQuality"), TEXT("sg.AntiAliasingQuality"), TEXT("sg.ShadowQuality"), TEXT("sg.GlobalIlluminationQuality"),
			TEXT("sg.ReflectionQuality"), TEXT("sg.PostProcessQuality"), TEXT("sg.TextureQuality"), TEXT("sg.EffectsQuality"), TEXT("sg.FoliageQuality"), TEXT("sg.ShadingQuality")})
			if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Key)) Receipt += FString(Key) + TEXT("=") + Variable->GetString() + TEXT("\n");
		Rows = TEXT("EngineFrame,WorldSeconds,WallFrameMs,SimDeltaSeconds,CSVActive,Alive,Candidate,ActorTicks,AnimTicks,Moving,RecentlyOnScreen,ProjectedCenters,FPS,PredictedLOD0,PredictedLOD1,PredictedLOD2,TimeDilation\n");
		Require(FFileHelper::SaveStringToFile(Receipt, *FPaths::Combine(Output, TEXT("started.txt"))), TEXT("Profiler startup receipt saved"));
	}
	void WorldInitialized(UWorld* World, const UWorld::InitializationValues Values)
	{
		if (!Active || World->WorldType != EWorldType::PIE || SpawnWorld.IsValid()) return;
		SpawnWorld = World;
		ViewExtension = FSceneViewExtensions::NewExtension<FReadOnlyView>(World, ViewData);
		// Installed LevelActor.cpp broadcasts this BEFORE Actor->PostSpawnInitialize.
		SpawnHandle = World->AddOnActorPreSpawnInitialization(FOnActorSpawned::FDelegate::CreateRaw(this, &FProbe::BeforeSpawnInitialization));
	}
	void BeforeSpawnInitialization(AActor* Actor)
	{
		if (!Active || !SpawnWorld.IsValid() || Actor->GetWorld() != SpawnWorld.Get() || !Actor->IsA(UnitClass)) return;
		ACharacter* Unit = Cast<ACharacter>(Actor);
		if (!Require(Unit && !Unit->HasActorBegunPlay() && Unit->GetMesh() && !Unit->GetMesh()->GetSkeletalMeshAsset(),
			TEXT("Candidate must bind BEFORE unit/presentation initialization, never repair a live unit"))) return;
		Art->SetObjectPropertyValue_InContainer(Unit, Candidate.Get()); ++BoundBeforeInit;
	}
	void Rebuild(UWorld* World)
	{
		Units.Reset();
		bool Valid = true;
		for (TActorIterator<ACharacter> It(World); It; ++It)
		{
			ACharacter* Unit = *It;
			if (!Unit->IsA(UnitClass) || !Registry->ContainsUnit(Unit)) continue;
			Units.Add(Unit);
			USkeletalMeshComponent* Mesh = Unit->GetMesh();
			Valid &= Mesh && Mesh->GetSkeletalMeshAsset() == Candidate->RomeMesh && Mesh->GetAnimInstance()
				&& Mesh->GetAnimInstance()->GetClass()->GetPathName().Contains(TEXT("/IntegrationV3/"));
			if (Mesh && Mesh->GetSkeletalMeshAsset() == Candidate->RomeMesh)
			{
				const auto& Materials = Candidate->RomeMesh->GetMaterials();
				for (int32 Index = 0; Index < Materials.Num(); ++Index)
				{
					UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(Index));
					Valid &= MID && MID->Parent == Materials[Index].MaterialInterface && MID->GetOuter() == Unit;
				}
			}
		}
		Valid &= Units.Num() == Registry->GetRegisteredUnitCount();
		if (!Valid && !ReportedInvalid) { Require(false, TEXT("Every registered soldier/recruit must use LOD01/IntegrationV3 and owned original PBR parents")); ReportedInvalid = true; }
		Dirty = false;
	}
	void Tick(UWorld* World, ELevelTick TickType, float Delta)
	{
		if (!Active || World->WorldType != EWorldType::PIE) return;
		CSV_SCOPED_TIMING_STAT(RomanSemireal, ObserverTick);
		const ARomanEmpireGameState* State = World->GetGameState<ARomanEmpireGameState>();
		if (!State || !State->IsGameplayInitialized()) return;
		URomanUnitRegistrySubsystem* Current = World->GetSubsystem<URomanUnitRegistrySubsystem>();
		if (!Current) return;
		if (!Registry.IsValid()) { Registry = Current; RegistryHandle = Current->OnUnitRegistryChanged.AddRaw(this, &FProbe::Invalidate); }
		if (Registry.Get() != Current) return;
		if (Dirty) Rebuild(World);
		const int32 Alive = Current->GetRegisteredUnitCount();
		int32 Correct = 0, ActorTicks = 0, AnimTicks = 0, Moving = 0, Recent = 0, Projected = 0, LODs[3] = {0, 0, 0};
		APlayerController* Player = World->GetFirstPlayerController();
		const bool FPS = Player && Player->GetPawn() && Player->GetPawn()->IsA(UnitClass);
		for (const auto& Weak : Units)
		{
			ACharacter* Unit = Weak.Get(); if (!Unit || !Current->ContainsUnit(Unit)) continue;
			USkeletalMeshComponent* Mesh = Unit->GetMesh();
			Correct += Mesh && Mesh->GetSkeletalMeshAsset() == Candidate->RomeMesh;
			ActorTicks += Unit->IsActorTickEnabled(); AnimTicks += Mesh && Mesh->IsComponentTickEnabled();
			Moving += Unit->GetVelocity().SizeSquared() > 1.;
			if (Mesh)
			{
				Recent += Mesh->GetLastRenderTimeOnScreen() > 0.f && World->GetTimeSeconds() - Mesh->GetLastRenderTimeOnScreen() < .5f;
				const int32 LOD = Mesh->GetPredictedLODLevel(); if (LOD >= 0 && LOD < 3) ++LODs[LOD];
			}
			FVector2D Position;
			Projected += Player && Player->ProjectWorldLocationToScreen(Unit->GetActorLocation(), Position) && Position.X >= 0 && Position.X < 1920 && Position.Y >= 0 && Position.Y < 1080;
		}
		const bool Capturing = FCsvProfiler::IsCapturing();
		if (Capturing && !RuntimeSnapshot)
		{
			RuntimeSnapshot = true;
			Receipt += FString::Printf(TEXT("runtime.engine_battery_classification=%d\nruntime.smooth_frame_rate=%d\nruntime.smooth_upper=%.6f\n"),
				int32(FPlatformMisc::IsRunningOnBattery()), GEngine ? int32(GEngine->bSmoothFrameRate) : -1,
				GEngine && GEngine->SmoothedFrameRateRange.HasUpperBound() ? GEngine->SmoothedFrameRateRange.GetUpperBoundValue() : -1.f);
			for (const TCHAR* Key : {TEXT("r.DontLimitOnBattery"), TEXT("t.MaxFPS"), TEXT("r.ScreenPercentage"), TEXT("r.DynamicRes.OperationMode"), TEXT("r.Test.OverrideScreenPercentageInterface"), TEXT("r.VSync")})
				if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(Key)) Receipt += TEXT("runtime.") + FString(Key) + TEXT("=") + Variable->GetString() + TEXT("\n");
		}
		const double Now = FPlatformTime::Seconds(), WallMs = PreviousWall > 0 ? (Now - PreviousWall) * 1000. : 0.; PreviousWall = Now;
		const float Dilation = World->GetWorldSettings()->GetEffectiveTimeDilation();
		Rows += FString::Printf(TEXT("%llu,%.6f,%.6f,%.6f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.6f\n"),
			GFrameCounter, World->GetTimeSeconds(), WallMs, Delta, int32(Capturing), Alive, Correct, ActorTicks, AnimTicks, Moving, Recent, Projected, int32(FPS), LODs[0], LODs[1], LODs[2], Dilation);
		++Frames; Peak = FMath::Max(Peak, Alive);
		CSV_CUSTOM_STAT(RomanSemireal, EngineFrame, int32(GFrameCounter), ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(RomanSemireal, CandidateUnits, Correct, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(RomanSemireal, ActorTicks, ActorTicks, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(RomanSemireal, AnimTicks, AnimTicks, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(RomanSemireal, Moving, Moving, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(RomanSemireal, OnScreenRecent, Recent, ECsvCustomStatOp::Set);
		CSV_CUSTOM_STAT(RomanSemireal, FPS, int32(FPS), ECsvCustomStatOp::Set);
		if (Capturing && Alive >= Desired && Correct == Alive)
		{
			++FullCSVFrames; if (!FPS) ++FullRTSCSVFrames;
			if ((ActorTicks != Alive || AnimTicks != Alive || Dilation != 1.f || World->IsPaused()) && !ReportedInvalid)
			{ Require(false, TEXT("Full-load measurement must retain canonical actor/animation ticks and real time")); ReportedInvalid = true; }
		}
		// Pixel readback only BEFORE original encounter starts CSV, never within timings.
		if (!Screenshot && !Capturing && Frames > 10 && Alive == Desired && Correct == Alive && Projected == Alive && Recent == Alive)
		{
			UGameViewportClient* Client = World->GetGameViewport(); FViewport* Viewport = Client ? Client->Viewport : nullptr;
			TArray<FColor> Pixels;
			if (Viewport && Viewport->GetSizeXY() == FIntPoint(1920, 1080) && Viewport->ReadPixels(Pixels) && Pixels.Num() == 1920 * 1080)
			{
				FImage Image; Image.Init(1920, 1080, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
				FMemory::Memcpy(Image.RawData.GetData(), Pixels.GetData(), Pixels.Num() * sizeof(FColor));
				Screenshot = FImageUtils::SaveImageByExtension(*FPaths::Combine(Output, TEXT("PreCapture.png")), Image);
			}
		}
	}
	void End(bool Simulating)
	{
		if (!Active) return;
		Require(FullCSVFrames >= 60 && FullRTSCSVFrames >= 60, TEXT("Need at least60 measured frames with full candidate population, including60 RTS frames"));
		Require(Screenshot, TEXT("Need real1080p full-population on-screen screenshot BEFORE measurement"));
		Require(RuntimeSnapshot && ViewData->Families > 0 && ViewData->PrimaryMin > 0.f && ViewData->PrimaryMax <= 4.f,
			TEXT("Runtime cap and owned view-family resolution observations required"));
		Receipt += FString::Printf(TEXT("view_families_observed=%d\nprimary_resolution_upper_min=%.6f\nprimary_resolution_upper_max=%.6f\nsecondary_fraction_min=%.6f\nsecondary_fraction_max=%.6f\nview_output_width=%d\nview_output_height=%d\n"),
			ViewData->Families, ViewData->PrimaryMin, ViewData->PrimaryMax, ViewData->SecondaryMin, ViewData->SecondaryMax, ViewData->OutputWidth, ViewData->OutputHeight);
		Receipt += FString::Printf(TEXT("world_frames=%d\nfull_population_csv_frames=%d\nfull_population_rts_csv_frames=%d\npeak_registered=%d\nbound_before_spawn_initialization=%d\npre_capture_png=%d\nobserver_scope=Editor only; no packaged60FPS gate\n"), Frames, FullCSVFrames, FullRTSCSVFrames, Peak, BoundBeforeInit, int32(Screenshot));
		Require(FFileHelper::SaveStringToFile(Rows, *FPaths::Combine(Output, TEXT("population.csv"))), TEXT("Profiler frame populations saved"));
		UObject* SavedOriginal = Original.Get();
		Restore();
		Require(CDO.IsValid() && Art && Art->GetObjectPropertyValue_InContainer(CDO.Get()) == SavedOriginal, TEXT("Exact original class presentation restored after owned PIE"));
		FAutomationTestExecutionInfo Execution; if (Test) Test->GetExecutionInfo(Execution);
		Receipt += FString::Printf(TEXT("test_errors_at_observer_end=%d\nperformance_target_accepted=false\nowner_approved=false\n"), Execution.GetErrorTotal());
		Require(FFileHelper::SaveStringToFile(Receipt, *FPaths::Combine(Output, TEXT("profile-receipt.txt"))), TEXT("Profiler final receipt saved"));
	}
public:
	FProbe()
	{
		if (!FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealProfile="), Name)) return;
		FParse::Value(FCommandLine::Get(), TEXT("RomanSemirealProfileUnits="), Desired);
		PreHandle = FEditorDelegates::PreBeginPIE.AddRaw(this, &FProbe::Begin);
		WorldInitHandle = FWorldDelegates::OnPostWorldInitialization.AddRaw(this, &FProbe::WorldInitialized);
		TickHandle = FWorldDelegates::OnWorldPostActorTick.AddRaw(this, &FProbe::Tick);
		EndHandle = FEditorDelegates::EndPIE.AddRaw(this, &FProbe::End);
	}
	~FProbe()
	{
		Restore();
		FEditorDelegates::PreBeginPIE.Remove(PreHandle); FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitHandle);
		FWorldDelegates::OnWorldPostActorTick.Remove(TickHandle); FEditorDelegates::EndPIE.Remove(EndHandle);
	}
};
FProbe Probe;
}
#endif
