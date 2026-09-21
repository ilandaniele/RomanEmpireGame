// Copyright Roman Empire Game. All Rights Reserved.
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include "Camera/SeamlessZoomCamera.h"
#include "Core/RomanEmpireGameState.h"
#include "Core/RomanEmpirePlayerController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "RomanEmpireGame.h"
#include "Units/UnitBase.h"

namespace
{
	// Explicit local diagnostics only. No tick, persisted target, changed input
	// mappings, network service or test/editor dependency; absent from Shipping.
	// This invokes gameplay commands, not physical input routing.
	FAutoConsoleCommandWithWorldAndArgs SelectPresentationUnit(
		TEXT("roman.Debug.SelectPlayerUnit"),
		TEXT("Select/focus the nearest eligible player unit to the world origin for a presentation check."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ARomanEmpirePlayerController* Controller = World ? Cast<ARomanEmpirePlayerController>(World->GetFirstPlayerController()) : nullptr;
			const ARomanEmpireGameState* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
			if (!Args.IsEmpty() || !Controller || !State || !Controller->CanIssueTacticalCommands())
			{
				UE_LOG(LogRomanEmpire, Error, TEXT("Presentation selection requires initialized tactical gameplay and no arguments"));
				return;
			}
			AUnitBase* Candidate = nullptr;
			for (TActorIterator<AUnitBase> It(World); It; ++It)
			{
				if (It->CanAcceptCommands() && It->GetOwnerFaction() == State->GetPlayerFaction() &&
					(!Candidate || It->GetActorLocation().SizeSquared2D() < Candidate->GetActorLocation().SizeSquared2D()))
				{
					Candidate = *It;
				}
			}
			if (!Candidate)
			{
				UE_LOG(LogRomanEmpire, Error, TEXT("Presentation selection found no eligible player unit"));
				return;
			}
			Controller->SelectUnit(Candidate);
			// Selection observers can synchronously replace selection or destroy actors.
			if (!IsValid(Controller) || !IsValid(Candidate) || !Candidate->CanAcceptCommands() ||
				!Candidate->IsSelected() || !Controller->CanIssueTacticalCommands() ||
				Controller->GetSelectedUnits().Num() != 1 || Controller->GetSelectedUnits()[0] != Candidate)
			{
				UE_LOG(LogRomanEmpire, Error, TEXT("Presentation selection changed during notification"));
				return;
			}
			if (ASeamlessZoomCamera* Camera = Cast<ASeamlessZoomCamera>(Controller->GetPawn()))
			{
				Camera->FocusOnActor(Candidate);
			}
			UE_LOG(LogRomanEmpire, Display, TEXT("Presentation selected %s at %s (selected=%d)"),
				*Candidate->GetPathName(), *Candidate->GetActorLocation().ToString(), Candidate->IsSelected());
		}), ECVF_Cheat);

	FAutoConsoleCommandWithWorldAndArgs PossessPresentationUnit(
		TEXT("roman.Debug.PossessSelectedUnit"), TEXT("Request normal possession of the single selected unit."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateLambda([](const TArray<FString>& Args, UWorld* World)
		{
			ARomanEmpirePlayerController* Controller = World ? Cast<ARomanEmpirePlayerController>(World->GetFirstPlayerController()) : nullptr;
			if (!Args.IsEmpty() || !Controller || Controller->GetSelectedUnits().Num() != 1)
			{
				UE_LOG(LogRomanEmpire, Error, TEXT("Presentation possession requires one selected unit and no arguments"));
				return;
			}
			Controller->EnterFirstPersonMode(Controller->GetSelectedUnits()[0]);
		}), ECVF_Cheat);
}
#endif
