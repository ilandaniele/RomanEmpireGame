// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Audio/RomanFeedbackSubsystem.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Battle/TacticalObjective.h"
#include "Components/AudioComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "Misc/ScopeExit.h"
#include "Sound/SoundBase.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/GarbageCollection.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include <limits>
#include <type_traits>

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
namespace RomanAudioTests
{
struct FCall
{
	FAutomationTestBase& Test; UObject* Object; UFunction* Function; FStructOnScope Params;
	FCall(FAutomationTestBase& InTest, UObject* InObject, const TCHAR* Name)
		: Test(InTest), Object(InObject), Function(InObject ? InObject->FindFunction(Name) : nullptr), Params(Function)
	{ Test.TestNotNull(FString::Printf(TEXT("Actual public function %s"), Name), Function); }
	template<class P, class V> void Set(const TCHAR* Name, V Value)
	{
		P* Property = Function ? FindFProperty<P>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name, Property))
		{
			if constexpr (std::is_same_v<P,FObjectPropertyBase>) Property->SetObjectPropertyValue_InContainer(Params.GetStructMemory(), Value);
			else Property->SetPropertyValue_InContainer(Params.GetStructMemory(), Value);
		}
	}
	template<class V> void Struct(const TCHAR* Name, V Value)
	{
		FStructProperty* Property = Function ? FindFProperty<FStructProperty>(Function, Name) : nullptr;
		if (Test.TestNotNull(Name, Property)) *Property->ContainerPtrToValuePtr<V>(Params.GetStructMemory()) = Value;
	}
	void Execute() { if (Function && IsValid(Object)) Object->ProcessEvent(Function, Params.GetStructMemory()); }
	int32 IntResult()
	{
		FIntProperty* Property = Function ? FindFProperty<FIntProperty>(Function,TEXT("ReturnValue")) : nullptr;
		return Test.TestNotNull(TEXT("Actual integer result"),Property) ? Property->GetPropertyValue_InContainer(Params.GetStructMemory()) : -1;
	}
	TArray<AActor*> Actors()
	{
		TArray<AActor*> Result;
		FArrayProperty* Array = Function ? FindFProperty<FArrayProperty>(Function, TEXT("ReturnValue")) : nullptr;
		if (!Test.TestNotNull(TEXT("Actual public actor array"), Array)) return Result;
		FObjectPropertyBase* Inner = CastField<FObjectPropertyBase>(Array->Inner);
		if (!Test.TestNotNull(TEXT("Actual actor references"), Inner)) return Result;
		FScriptArrayHelper Helper(Array, Array->ContainerPtrToValuePtr<void>(Params.GetStructMemory()));
		for (int32 Index = 0; Index < Helper.Num(); ++Index) if (auto* Actor = Cast<AActor>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index)))) Result.Add(Actor);
		return Result;
	}
};

TArray<UAudioComponent*> Voices(FAutomationTestBase& Test, URomanFeedbackSubsystem* Audio)
{
	TArray<UAudioComponent*> Result;
	FArrayProperty* Array = Audio ? FindFProperty<FArrayProperty>(Audio->GetClass(), TEXT("Voices")) : nullptr;
	if (!Test.TestNotNull(TEXT("GC-owned actual voice pool"), Array)) return Result;
	FObjectPropertyBase* Inner = CastField<FObjectPropertyBase>(Array->Inner);
	FScriptArrayHelper Helper(Array, Array->ContainerPtrToValuePtr<void>(Audio));
	for (int32 Index = 0; Index < Helper.Num(); ++Index) Result.Add(Cast<UAudioComponent>(Inner->GetObjectPropertyValue(Helper.GetRawPtr(Index))));
	return Result;
}
bool HasCue(FAutomationTestBase& Test, URomanFeedbackSubsystem* Audio, const TCHAR* Name)
{
	for (UAudioComponent* Voice : Voices(Test, Audio)) if (Voice && Voice->IsPlaying() && Voice->Sound && Voice->Sound->GetName() == Name) return true;
	return false;
}
struct FTeardownState
{
	TWeakObjectPtr<URomanFeedbackSubsystem> Audio;
	TArray<TWeakObjectPtr<UAudioComponent>> Voices;
};
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FTeardownState> State; bool bGameplay, bDefeat;
	double Started = FPlatformTime::Seconds(), PhaseStarted = 0; int32 Phase = 0;
public:
	FExercise(FAutomationTestBase* InTest, TSharedRef<FTeardownState> InState, bool InGameplay, bool InDefeat)
		: Test(*InTest), State(InState), bGameplay(InGameplay), bDefeat(InDefeat) {}
	virtual bool Update() override
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		const auto* Content = World ? URomanContentRegistrySubsystem::Get(World) : nullptr;
		APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
		auto* Audio = World ? World->GetSubsystem<URomanFeedbackSubsystem>() : nullptr;
		if (!World || !World->HasBegunPlay() || !Player || !Content || !Content->IsReady() || !Audio)
		{
			if (FPlatformTime::Seconds() - Started < 20) return false;
			Test.AddError(TEXT("Ready actual audio world timed out")); return true;
		}
		if (Phase == 0)
		{
			State->Audio = Audio;
			const auto Pool = Voices(Test, Audio);
			Test.TestEqual(TEXT("Bounded pool is exactly eight actual registered voices"), Pool.Num(), 8);
			for (UAudioComponent* Voice : Pool)
			{
				if (!Test.TestNotNull(TEXT("Actual pooled component"), Voice)) return true;
				Test.TestTrue(TEXT("Voice owns this world, registered and non-auto-destroy"), Voice->GetWorld() == World && Voice->IsRegistered() && !Voice->bAutoDestroy);
				State->Voices.Add(Voice);
			}
			Test.TestFalse(TEXT("Editor world unsupported"), Audio->DoesSupportWorldType(EWorldType::Editor));
			Test.TestFalse(TEXT("Preview world unsupported"), Audio->DoesSupportWorldType(EWorldType::EditorPreview));
			Test.TestFalse(TEXT("Unknown cue rejected"), Audio->PlayCue(static_cast<ERomanFeedbackCue>(255)));
			Test.TestFalse(TEXT("Nonfinite source rejected"), Audio->PlayCue(ERomanFeedbackCue::Impact, FVector(std::numeric_limits<double>::infinity(),0,0)));
			Test.TestTrue(TEXT("Valid levels projected"), Audio->SetLevels(.8f,.5f));
			Test.TestFalse(TEXT("NaN level rejected"), Audio->SetLevels(std::numeric_limits<float>::quiet_NaN(), 1));
			Test.TestFalse(TEXT("Out-of-range level rejected"), Audio->SetLevels(1, 2));
			Test.TestEqual(TEXT("Rejected settings preserve master"), Audio->GetMasterLevel(), .8f);
			Test.TestEqual(TEXT("Rejected settings preserve effects"), Audio->GetEffectsLevel(), .5f);
			if (bGameplay) return ExerciseGameplay(World, Player, Audio, Content);
			for (ERomanFeedbackCue Cue : {ERomanFeedbackCue::Select, ERomanFeedbackCue::Order, ERomanFeedbackCue::Complete, ERomanFeedbackCue::Victory, ERomanFeedbackCue::Defeat})
				Test.TestTrue(TEXT("Actual audio device starts imported feedback"), Audio->PlayCue(Cue));
			Test.TestFalse(TEXT("Immediate mass duplicate is coalesced"), Audio->PlayCue(ERomanFeedbackCue::Select));
			Test.TestTrue(TEXT("Actual active playback exists"), Audio->GetPlayingVoiceCount() >= 5);
			Phase = 1; PhaseStarted = FPlatformTime::Seconds(); return false;
		}
		if (FPlatformTime::Seconds() - PhaseStarted < .12) return false;
		if (bGameplay)
		{
			auto* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
			if (!Test.TestNotNull(TEXT("Same canonical battle after coalescing interval"),Battle)) return true;
			const int32 Count = Audio->GetPlayingVoiceCount();
			Battle->OnTacticalBattleChanged.Broadcast(Battle->GetResult());
			Test.TestEqual(TEXT("Repeated committed result after coalescing interval does not replay"),Audio->GetPlayingVoiceCount(),Count);
			return true;
		}
		for (ERomanFeedbackCue Cue : {ERomanFeedbackCue::Select, ERomanFeedbackCue::Order, ERomanFeedbackCue::Complete, ERomanFeedbackCue::Victory, ERomanFeedbackCue::Defeat}) Audio->PlayCue(Cue);
		Test.TestTrue(TEXT("Active voices never exceed eight"), Audio->GetPlayingVoiceCount() <= 8);
		Test.TestTrue(TEXT("Mute updates already-playing voices"), Audio->SetLevels(0,1));
		for (UAudioComponent* Voice : Voices(Test, Audio)) Test.TestEqual(TEXT("Actual current voice muted"), Voice->VolumeMultiplier, 0.f);
		Test.TestTrue(TEXT("Unmute updates reused pool"), Audio->SetLevels(.8f,.5f));
		CollectGarbage(RF_NoFlags);
		for (const auto& Voice : State->Voices) Test.TestTrue(TEXT("Real GC preserves owned voice"), Voice.IsValid() && FMath::IsNearlyEqual(Voice->VolumeMultiplier,.4f));
		return true;
	}
	bool ExerciseGameplay(UWorld* World, APlayerController* Player, URomanFeedbackSubsystem* Audio, const URomanContentRegistrySubsystem* Content)
	{
		auto* Battle = World->GetSubsystem<UTacticalBattleSubsystem>();
		UClass* RegistryClass = FindObject<UClass>(nullptr,TEXT("/Script/RomanEmpireGame.RomanUnitRegistrySubsystem"));
		auto* Registry = RegistryClass ? World->GetSubsystemBase(RegistryClass) : nullptr;
		const auto* PublicState = World->GetGameState<ARomanEmpireGameState>();
		if (!Test.TestTrue(TEXT("Actual sandbox encounter and registries"), Battle && Battle->HasEncounter() && !Battle->IsResolved() && Registry && PublicState)) return true;
		TArray<AActor*> Friendly, Enemies;
		for (const auto& Squad : Battle->GetResult().Squads)
		{
			FCall Members(Test,Registry,TEXT("GetSquadMembers")); Members.Struct(TEXT("SquadID"),Squad.SquadID); Members.Execute();
			for (AActor* Member : Members.Actors())
			{
				Member->SetActorTickEnabled(false);
				FCall Hold(Test,Member,TEXT("CommandHold")); Hold.Execute();
				(Squad.Faction == PublicState->GetPlayerFaction() ? Friendly : Enemies).Add(Member);
			}
		}
		if (!Test.TestTrue(TEXT("Real player/enemy squad members"), !Friendly.IsEmpty() && !Enemies.IsEmpty())) return true;
		FVector Listener, Front, Right; Player->GetAudioListenerPosition(Listener,Front,Right);
		Test.TestTrue(TEXT("RTS listener is actual ground focus, not elevated view camera"), Player->GetPawn() && Listener.Equals(Player->GetPawn()->GetActorLocation(),.1));
		FCall Select(Test,Player,TEXT("SelectUnit")); Select.Set<FObjectPropertyBase>(TEXT("Unit"),Friendly[0]); Select.Execute();
		Test.TestTrue(TEXT("Canonical selection starts actual cue"), HasCue(Test,Audio,TEXT("S_RomanSelect")));
		FCall Move(Test,Player,TEXT("CommandSelectedUnitsMove")); Move.Struct(TEXT("Destination"),Friendly[0]->GetActorLocation()+FVector(200,0,0)); Move.Execute();
		Test.TestTrue(TEXT("Canonical accepted order starts actual cue"), HasCue(Test,Audio,TEXT("S_RomanOrder")));
		Enemies[0]->SetActorLocation(Listener + FVector(400,0,100));
		FMulticastDelegateProperty* DeathEvent = FindFProperty<FMulticastDelegateProperty>(Enemies[0]->GetClass(),TEXT("OnUnitDied"));
		if (Test.TestNotNull(TEXT("Actual committed death event"),DeathEvent))
			Test.TestTrue(TEXT("Actual battle is a unit-death observer"),DeathEvent->GetMulticastDelegate(DeathEvent->ContainerPtrToValuePtr<void>(Enemies[0]))->Contains(Battle,FName(TEXT("HandleUnitDied"))));
		Test.AddInfo(FString::Printf(TEXT("Actual audio encounter playerFaction=%d friends=%d enemies=%d squads=%d"),static_cast<int32>(PublicState->GetPlayerFaction()),Friendly.Num(),Enemies.Num(),Battle->GetResult().Squads.Num()));
		FCall Hit(Test,Enemies[0],TEXT("TakeCombatDamage")); Hit.Set<FFloatProperty>(TEXT("Damage"),1000.f); Hit.Set<FObjectPropertyBase>(TEXT("DamageSource"),Friendly[0]); Hit.Set<FBoolProperty>(TEXT("bIsRanged"),false); Hit.Execute();
		Test.TestTrue(TEXT("Committed combat damage starts real spatial cue"), HasCue(Test,Audio,TEXT("S_RomanImpact")));
		for (UAudioComponent* Voice : Voices(Test,Audio)) if (Voice->Sound && Voice->Sound->GetName() == TEXT("S_RomanImpact"))
			Test.TestTrue(TEXT("Impact has actual finite spatial attenuation"), !Voice->bIsUISound && Voice->bAllowSpatialization && Voice->bOverrideAttenuation && Voice->AttenuationOverrides.bAttenuate && Voice->AttenuationOverrides.FalloffDistance == 1600.f);
		const auto* Definition = Content->FindBuilding(TEXT("Barracks"));
		UClass* Class = Definition ? Cast<UClass>(Definition->BuildingClass.ToSoftObjectPath().ResolveObject()) : nullptr;
		if (!Test.TestNotNull(TEXT("Actual building factory"),Class)) return true;
		FActorSpawnParameters Spawn; Spawn.ObjectFlags |= RF_Transient; Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Building = World->SpawnActor<AActor>(Class,FVector(400000,400000,100),FRotator::ZeroRotator,Spawn);
		if (!Test.TestNotNull(TEXT("Actual transient construction actor"),Building)) return true;
		ON_SCOPE_EXIT { if (IsValid(Building)) Building->Destroy(); };
		FCall Owner(Test,Building,TEXT("SetOwnerFaction"));
		if (FEnumProperty* Property = FindFProperty<FEnumProperty>(Owner.Function,TEXT("NewOwner"))) Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Owner.Params.GetStructMemory()),static_cast<int64>(PublicState->GetPlayerFaction()));
		else Test.AddError(TEXT("Building ownership API parameter missing"));
		Owner.Execute();
		FCall Start(Test,Building,TEXT("StartConstruction")); Start.Execute(); Building->Tick(Definition->Definition.ConstructionTime+.1f); Building->SetActorTickEnabled(false);
		Test.TestTrue(TEXT("Real completed construction starts cue"), HasCue(Test,Audio,TEXT("S_RomanComplete")));
		if (bDefeat)
		{
			for (AActor* Unit : Friendly)
			{ FCall Kill(Test,Unit,TEXT("SetCurrentHealth")); Kill.Set<FIntProperty>(TEXT("NewHealth"),0); Kill.Execute(); }
			Test.TestEqual(TEXT("Actual attacker elimination commits human defeat"),Battle->GetResult().Outcome,ETacticalBattleOutcome::PlayerDefeat);
			Test.TestTrue(TEXT("Canonical terminal defeat starts actual cue"),HasCue(Test,Audio,TEXT("S_RomanDefeat")));
			Test.TestFalse(TEXT("Defeat never dispatches victory"),HasCue(Test,Audio,TEXT("S_RomanVictory")));
			Phase = 1; PhaseStarted = FPlatformTime::Seconds(); return false;
		}
		for (AActor* Enemy : Enemies)
		{
			FCall Before(Test,Enemy,TEXT("GetCurrentHealth")); Before.Execute();
			const int32 InitialHealth = Before.IntResult();
			FCall Kill(Test,Enemy,TEXT("SetCurrentHealth")); Kill.Set<FIntProperty>(TEXT("NewHealth"),0); Kill.Execute();
			FCall After(Test,Enemy,TEXT("GetCurrentHealth")); After.Execute();
			Test.AddInfo(FString::Printf(TEXT("Actual enemy %s begun=%d valid=%d health=%d->%d"),*Enemy->GetName(),Enemy->HasActorBegunPlay(),IsValid(Enemy),InitialHealth,After.IntResult()));
		}
		int32 Casualties = 0, Survivors = 0;
		for (const auto& Squad : Battle->GetResult().Squads) if (Squad.Faction != PublicState->GetPlayerFaction())
		{ Casualties += Squad.Casualties; Survivors += Squad.Survivors; }
		Test.TestEqual(TEXT("All actual defender casualties recorded"),Casualties,Enemies.Num());
		Test.TestEqual(TEXT("No surviving defenders remain"),Survivors,0);
		Test.TestEqual(TEXT("Kills alone preserve occupation contract"),Battle->GetResult().Outcome,ETacticalBattleOutcome::InProgress);
		Test.TestFalse(TEXT("No victory feedback before occupation"),HasCue(Test,Audio,TEXT("S_RomanVictory")));
		ATacticalObjective* Objective = Battle->GetObjectiveActor();
		if (!Test.TestNotNull(TEXT("Registered actual physical objective"),Objective)) return true;
		Friendly[0]->SetActorLocation(Objective->GetOccupationApproachLocation()+FVector(0,0,96),false,nullptr,ETeleportType::TeleportPhysics);
		Test.TestTrue(TEXT("Engine reports real occupation overlap"),Objective->IsOverlappingActor(Friendly[0]));
		Test.TestTrue(TEXT("Physical objective commits canonical capture"),Objective->TryCapture());
		Test.TestEqual(TEXT("Real occupation result committed"),Battle->GetResult().Outcome,ETacticalBattleOutcome::PlayerVictory);
		Test.TestEqual(TEXT("Captured stable objective ID"),Battle->GetResult().CapturedObjectiveID,Objective->GetObjectiveID());
		Test.TestTrue(TEXT("Canonical terminal event starts actual victory cue"),HasCue(Test,Audio,TEXT("S_RomanVictory")));
		Phase = 1; PhaseStarted = FPlatformTime::Seconds(); return false;
	}
};
class FVerifyTeardown : public IAutomationLatentCommand
{
	FAutomationTestBase& Test; TSharedRef<FTeardownState> State;
	double Started = FPlatformTime::Seconds();
public:
	FVerifyTeardown(FAutomationTestBase* InTest,TSharedRef<FTeardownState> InState) : Test(*InTest),State(InState) {}
	virtual bool Update() override
	{
		// FEndPlayMapCommand requests shutdown; it completes before the Editor tick
		// actually ends PIE. Observe the real lifecycle, not the queued request.
		if (GEditor->PlayWorld && FPlatformTime::Seconds() - Started < 20) return false;
		Test.TestNull(TEXT("Owned PIE ended"),GEditor->PlayWorld);
		if (State->Audio.IsValid())
		{
			Test.TestEqual(TEXT("Actual world teardown releases voice pool"),State->Audio->GetPlayingVoiceCount(),0);
			Test.TestFalse(TEXT("Stale service cannot play into new/editor world"),State->Audio->PlayCue(ERomanFeedbackCue::Order));
		}
		for (const auto& Voice : State->Voices) if (Voice.IsValid()) Test.TestFalse(TEXT("Old voice unregistered/stopped"),Voice->IsRegistered() || Voice->IsPlaying());
		CollectGarbage(RF_NoFlags);
		Test.TestFalse(TEXT("World-local service actually collected, no retained PIE world"),State->Audio.IsValid());
		for (const auto& Voice : State->Voices) Test.TestFalse(TEXT("Released voice collected for real"),Voice.IsValid());
		State->Audio.Reset(); return true;
	}
};
bool Run(FAutomationTestBase* Test,bool bGameplay,bool bDefeat = false)
{
	if (!Test->TestNotNull(TEXT("Editor"),GEditor) || !Test->TestNull(TEXT("Owns fresh PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	TSharedRef<FTeardownState> State = MakeShared<FTeardownState>();
	Test->AddCommand(new FStartPIECommand(false));
	Test->AddCommand(new FExercise(Test,State,bGameplay,bDefeat));
	Test->AddCommand(new FEndPlayMapCommand());
	Test->AddCommand(new FVerifyTeardown(Test,State)); return true;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanAudioPlayback,"RomanEmpireGame.Audio.WorldPlaybackPIE",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanAudioPlayback::RunTest(const FString& Parameters) { return RomanAudioTests::Run(this,false); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanAudioGameplay,"RomanEmpireGame.Audio.CanonicalGameplayPIE",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanAudioGameplay::RunTest(const FString& Parameters) { return RomanAudioTests::Run(this,true); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanAudioDefeat,"RomanEmpireGame.Audio.CanonicalDefeatPIE",EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FRomanAudioDefeat::RunTest(const FString& Parameters) { return RomanAudioTests::Run(this,true,true); }
#endif
