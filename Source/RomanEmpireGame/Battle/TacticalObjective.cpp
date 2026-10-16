// Copyright Roman Empire Game. All Rights Reserved.
#include "Battle/TacticalObjective.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Templates/UnrealTemplate.h"
#include "Units/RomanUnitRegistry.h"
#include "Units/UnitBase.h"

ATacticalObjective::ATacticalObjective()
{
	PrimaryActorTick.bCanEverTick = false;
	OccupationZone = CreateDefaultSubobject<UBoxComponent>(TEXT("OccupationZone"));
	SetRootComponent(OccupationZone);
	OccupationZone->SetBoxExtent(FVector(650, 650, 250));
	OccupationZone->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	OccupationZone->SetCollisionResponseToAllChannels(ECR_Ignore);
	OccupationZone->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	OccupationZone->SetGenerateOverlapEvents(true);
	OccupationZone->SetCanEverAffectNavigation(false);
	ObjectiveMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectiveMesh"));
	ObjectiveMesh->SetupAttachment(OccupationZone);
	ObjectiveMesh->SetRelativeLocation(FVector(0, 0, 150));
	ObjectiveMesh->SetRelativeScale3D(FVector(3.5f, 5.0f, 3.0f));
	ObjectiveMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
}

FVector ATacticalObjective::GetOccupationApproachLocation() const
{
	return OccupationZone->GetComponentLocation() + FVector(0, OccupationZone->GetScaledBoxExtent().Y * 0.85, 0);
}

void ATacticalObjective::BeginPlay()
{
	Super::BeginPlay();
	UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	if (Battle && Battle->RegisterObjective(this))
	{
		// One objective at 4Hz: engine-local overlaps + live registry, never per-unit world scans.
		GetWorldTimerManager().SetTimer(CaptureTimer, this, &ATacticalObjective::CheckCapture, 0.25f, true);
	}
}

void ATacticalObjective::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(CaptureTimer);
	Super::EndPlay(Reason);
}

void ATacticalObjective::CheckCapture()
{
	TryCapture();
	const UTacticalBattleSubsystem* Battle = GetWorld()->GetSubsystem<UTacticalBattleSubsystem>();
	if (!Battle || Battle->GetResult().IsResolved()) GetWorldTimerManager().ClearTimer(CaptureTimer);
}

bool ATacticalObjective::TryCapture()
{
	if (bEvaluatingCapture || !HasActorBegunPlay() || IsActorBeingDestroyed()) return false;
	TGuardValue<bool> Guard(bEvaluatingCapture, true);
	UWorld* World = GetWorld();
	UTacticalBattleSubsystem* Battle = World ? World->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	URomanUnitRegistrySubsystem* Units = World ? World->GetSubsystem<URomanUnitRegistrySubsystem>() : nullptr;
	if (!Battle || !Battle->HasEncounter() || !Units || Battle->GetObjectiveActor() != this || Battle->GetResult().IsResolved() ||
		!Units->GetUnitsForFaction(Battle->GetDefendingFaction()).IsEmpty()) return false;
	TArray<AActor*> Occupants;
	OccupationZone->GetOverlappingActors(Occupants, AUnitBase::StaticClass());
	for (AActor* Actor : Occupants)
	{
		AUnitBase* Unit = Cast<AUnitBase>(Actor);
		if (IsValid(Unit) && Unit->GetWorld() == World && Unit->CanParticipateInWorld() &&
			Unit->GetOwnerFaction() == Battle->GetAttackingFaction() && Battle->CaptureObjective(this, Unit)) return true;
	}
	return false;
}
