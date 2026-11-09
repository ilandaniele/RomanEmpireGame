// Copyright Roman Empire Game. All Rights Reserved.

#include "UnitBase.h"
#include "Audio/RomanFeedbackSubsystem.h"
#include "Battle/TacticalBattleSubsystem.h"
#include "Units/RomanUnitRegistry.h"
#include "Units/RomanUnitAIController.h"
#include "Units/RomanCharacterPresentation.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/SkeletalMesh.h"
#include "Content/RomanContentRegistry.h"
#include "RomanEmpireGame.h"
#include "CollisionQueryParams.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = true;
	AIControllerClass = ARomanUnitAIController::StaticClass();
	// Initialize content/lifecycle before creating the tactical controller in BeginPlay.
	AutoPossessAI = EAutoPossessAI::Disabled;
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Surface(TEXT("/Game/Materials/RomanVisual/MI_Surface"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Rome(TEXT("/Game/Materials/RomanVisual/MI_FactionRome"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Carthage(TEXT("/Game/Materials/RomanVisual/MI_FactionCarthage"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Gaul(TEXT("/Game/Materials/RomanVisual/MI_FactionGaul"));
	SurfaceMaterial = Surface.Object;
	FactionMaterials.Add(EFactionID::Rome, Rome.Object);
	FactionMaterials.Add(EFactionID::Carthage, Carthage.Object);
	FactionMaterials.Add(EFactionID::Gaul, Gaul.Object);
	// Small mandatory bootstrap bundle, retained by reflected CDO/instances.
	// No loads/scans on orders, damage, animation updates or ownership changes.
	static ConstructorHelpers::FObjectFinder<URomanCharacterPresentation> Characters(TEXT("/Game/Characters/RomanSoldier/DA_RomanCharacterPresentation"));
	CharacterPresentation = Characters.Object;

	// Configure capsule (collision only, not visible in-game)
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetVisibility(false);
	GetCapsuleComponent()->SetHiddenInGame(true);
	GetCapsuleComponent()->SetCastShadow(false);
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);

	// Capsule remains sole collision/nav authority. Verified imported forward+Y;
	// reflect transverse X then yaw-90 to preserve +X-forward/+Y-right anatomy.
	if (GetMesh())
	{
		GetMesh()->SetRelativeLocation(FVector(0, 0, -96));
		GetMesh()->SetRelativeRotation(FRotator(0, -90, 0));
		GetMesh()->SetRelativeScale3D(FVector(-1, 1, 1));
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->SetCanEverAffectNavigation(false);
		GetMesh()->SetVisibility(false);
		GetMesh()->SetHiddenInGame(true);
		GetMesh()->SetCastShadow(false);
	}

	// Create visible placeholder mesh (capsule body — no flat base)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	if (VisualMesh)
	{
		VisualMesh->SetupAttachment(GetRootComponent());
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
		VisualMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 1.8f));
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Use Sphere for body — no flat base visible
		static ConstructorHelpers::FObjectFinder<UStaticMesh> BodyMesh(TEXT("/Engine/BasicShapes/Sphere"));
		if (BodyMesh.Succeeded())
		{
			VisualMesh->SetStaticMesh(BodyMesh.Object);
		}
	}

	// Create selection decal (ring under unit when selected)
	SelectionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecal"));
	if (SelectionDecal)
	{
		SelectionDecal->SetupAttachment(GetRootComponent());
		SelectionDecal->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		SelectionDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		SelectionDecal->DecalSize = FVector(64.0f, 64.0f, 64.0f);
		SelectionDecal->SetVisibility(false);
	}

	// Create selection ring (flat disc - hidden until selected)
	SelectionRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
	if (SelectionRingMesh)
	{
		SelectionRingMesh->SetupAttachment(GetRootComponent());
		SelectionRingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -95.0f));
		SelectionRingMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 0.02f));
		SelectionRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SelectionRingMesh->SetVisibility(false); // Hidden until selected
		SelectionRingMesh->SetHiddenInGame(true); // Truly hidden
		static ConstructorHelpers::FObjectFinder<UMaterialInterface> Selection(TEXT("/Game/Materials/RomanVisual/MI_Selection"));
		SelectionRingMesh->SetMaterial(0, Selection.Object);

		static ConstructorHelpers::FObjectFinder<UStaticMesh> RingMesh(TEXT("/Engine/BasicShapes/Cylinder"));
		if (RingMesh.Succeeded())
		{
			SelectionRingMesh->SetStaticMesh(RingMesh.Object);
		}
	}

	// Create head sphere on top of body
	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	if (HeadMesh)
	{
		HeadMesh->SetupAttachment(GetRootComponent());
		HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
		HeadMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 0.7f));
		HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
		if (SphereMesh.Succeeded())
		{
			HeadMesh->SetStaticMesh(SphereMesh.Object);
		}
	}

	// Create sword (thin stretched cube = blade shape)
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	if (WeaponMesh)
	{
		WeaponMesh->SetupAttachment(GetRootComponent());
		WeaponMesh->SetRelativeLocation(FVector(20.0f, 50.0f, 10.0f)); // Right side
		WeaponMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -30.0f)); // Angled
		WeaponMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 1.2f)); // Long thin blade
		WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
		if (CubeMesh.Succeeded())
		{
			WeaponMesh->SetStaticMesh(CubeMesh.Object);
		}
	}

	// Create shield (flat cylinder on left side)
	ShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldMesh"));
	if (ShieldMesh)
	{
		ShieldMesh->SetupAttachment(GetRootComponent());
		ShieldMesh->SetRelativeLocation(FVector(10.0f, -55.0f, -10.0f)); // Left side
		ShieldMesh->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
		ShieldMesh->SetRelativeScale3D(FVector(0.7f, 0.05f, 0.7f)); // Flat disc
		ShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		static ConstructorHelpers::FObjectFinder<UStaticMesh> ShieldCylinder(TEXT("/Engine/BasicShapes/Cylinder"));
		if (ShieldCylinder.Succeeded())
		{
			ShieldMesh->SetStaticMesh(ShieldCylinder.Object);
		}
	}

	// --- FPS Camera (for first-person mode) ---
	FPSCameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("FPSCameraArm"));
	if (FPSCameraArm)
	{
		FPSCameraArm->SetupAttachment(GetRootComponent());
		FPSCameraArm->SetRelativeLocation(FVector(0.0f, 0.0f, 70.0f)); // Eye height
		FPSCameraArm->TargetArmLength = 0.0f; // First person = no arm
		FPSCameraArm->bUsePawnControlRotation = true;
		FPSCameraArm->bDoCollisionTest = false;
	}

	FPSCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FPSCamera"));
	if (FPSCamera && FPSCameraArm)
	{
		FPSCamera->SetupAttachment(FPSCameraArm);
		FPSCamera->bAutoActivate = false; // Only active in FPS mode
	}

	// --- Limbs ---
	// Right arm
	RightArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightArm"));
	if (RightArmMesh)
	{
		RightArmMesh->SetupAttachment(GetRootComponent());
		RightArmMesh->SetRelativeLocation(FVector(0.0f, 35.0f, 20.0f));
		RightArmMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.7f));
		RightArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Left arm
	LeftArmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftArm"));
	if (LeftArmMesh)
	{
		LeftArmMesh->SetupAttachment(GetRootComponent());
		LeftArmMesh->SetRelativeLocation(FVector(0.0f, -35.0f, 20.0f));
		LeftArmMesh->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.7f));
		LeftArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Right leg
	RightLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLeg"));
	if (RightLegMesh)
	{
		RightLegMesh->SetupAttachment(GetRootComponent());
		RightLegMesh->SetRelativeLocation(FVector(0.0f, 15.0f, -55.0f));
		RightLegMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.8f));
		RightLegMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Left leg
	LeftLegMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLeg"));
	if (LeftLegMesh)
	{
		LeftLegMesh->SetupAttachment(GetRootComponent());
		LeftLegMesh->SetRelativeLocation(FVector(0.0f, -15.0f, -55.0f));
		LeftLegMesh->SetRelativeScale3D(FVector(0.2f, 0.2f, 0.8f));
		LeftLegMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Right hand (small sphere at arm tip)
	RightHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightHand"));
	if (RightHandMesh)
	{
		RightHandMesh->SetupAttachment(GetRootComponent());
		RightHandMesh->SetRelativeLocation(FVector(0.0f, 40.0f, -18.0f));
		RightHandMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.22f));
		RightHandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Left hand
	LeftHandMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftHand"));
	if (LeftHandMesh)
	{
		LeftHandMesh->SetupAttachment(GetRootComponent());
		LeftHandMesh->SetRelativeLocation(FVector(0.0f, -40.0f, -18.0f));
		LeftHandMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.22f));
		LeftHandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Right foot (small cube)
	RightFootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightFoot"));
	if (RightFootMesh)
	{
		RightFootMesh->SetupAttachment(GetRootComponent());
		RightFootMesh->SetRelativeLocation(FVector(5.0f, 15.0f, -100.0f));
		RightFootMesh->SetRelativeScale3D(FVector(0.3f, 0.18f, 0.1f));
		RightFootMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Left foot
	LeftFootMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftFoot"));
	if (LeftFootMesh)
	{
		LeftFootMesh->SetupAttachment(GetRootComponent());
		LeftFootMesh->SetRelativeLocation(FVector(5.0f, -15.0f, -100.0f));
		LeftFootMesh->SetRelativeScale3D(FVector(0.3f, 0.18f, 0.1f));
		LeftFootMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Assign meshes to limbs (using same static meshes)
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> LimbCylinder(TEXT("/Engine/BasicShapes/Cylinder"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> LimbSphere(TEXT("/Engine/BasicShapes/Sphere"));
		static ConstructorHelpers::FObjectFinder<UStaticMesh> LimbCube(TEXT("/Engine/BasicShapes/Cube"));

		if (LimbCylinder.Succeeded())
		{
			if (RightArmMesh) RightArmMesh->SetStaticMesh(LimbCylinder.Object);
			if (LeftArmMesh) LeftArmMesh->SetStaticMesh(LimbCylinder.Object);
			if (RightLegMesh) RightLegMesh->SetStaticMesh(LimbCylinder.Object);
			if (LeftLegMesh) LeftLegMesh->SetStaticMesh(LimbCylinder.Object);
		}
		if (LimbSphere.Succeeded())
		{
			if (RightHandMesh) RightHandMesh->SetStaticMesh(LimbSphere.Object);
			if (LeftHandMesh) LeftHandMesh->SetStaticMesh(LimbSphere.Object);
		}
		if (LimbCube.Succeeded())
		{
			if (RightFootMesh) RightFootMesh->SetStaticMesh(LimbCube.Object);
			if (LeftFootMesh) LeftFootMesh->SetStaticMesh(LimbCube.Object);
		}
	}

	// Configure movement
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = 0.0f;
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

	// Don't rotate when controller rotates (for RTS mode)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Initialize state
	OwnerFaction = EFactionID::None;
	bIsPossessedByPlayer = false;
}

void AUnitBase::BeginPlay()
{
	Super::BeginPlay();

	if (GetLifeState() == EUnitLifeState::Uninitialized && !InitializeUnit(UnitDefinitionID, OwnerFaction))
	{
		UE_LOG(LogRomanEmpire, Error, TEXT("Unit %s has no valid content definition"), *GetName());
		Vitals.MarkInitializationFailed();
		StopAllActions();
		SetActorEnableCollision(false);
		GetCharacterMovement()->DisableMovement();
		SetActorTickEnabled(false);
		SetLifeSpan(1.0f);
		return;
	}
	if (!IsAlive()) return;
	
	// Set movement speed from unit data
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed;
	}
	
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Unit spawned: %s"), *UnitData.DisplayName.ToString());
	ApplyFactionColor();  // Apply faction color on spawn

	// Enable unit-to-unit avoidance so they don't overlap
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bUseRVOAvoidance = true;
		GetCharacterMovement()->AvoidanceWeight = 0.5f;
	}
	if (URomanUnitRegistrySubsystem* Registry = GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>())
	{
		Registry->SynchronizeUnit(this);
	}
	RestoreTacticalController();
	if (IsRouted()) BeginRout(nullptr);
}

void AUnitBase::RestoreTacticalController()
{
	if (!CanParticipateInWorld() || bIsPossessedByPlayer || GetController()) return;
	if (IsValid(TacticalController) && !TacticalController->GetPawn())
	{
		TacticalController->Possess(this);
	}
	else
	{
		SpawnDefaultController();
		TacticalController = Cast<ARomanUnitAIController>(GetController());
	}
}

void AUnitBase::RefreshPossessionHeadVisibility()
{
	if (!GetMesh() || !GetMesh()->GetSkeletalMeshAsset() || IsActorBeingDestroyed()) return;
	// Offline local possession only. Hide the rigged head/helmet, not arms,
	// weapon or collision. Restore on AI/RTS/death/ownership teardown.
	// This affects this local world's mesh, not a per-view multiplayer solution.
	const bool Hide = bIsPossessedByPlayer && IsPlayerControlled() && IsLocallyControlled();
	if (Hide) GetMesh()->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_None);
	else GetMesh()->UnHideBoneByName(TEXT("head"));
}

void AUnitBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// SetPossessedByPlayer(true) commits before the actual controller transfer.
	RefreshPossessionHeadVisibility();
}

void AUnitBase::UnPossessed()
{
	Super::UnPossessed();
	RefreshPossessionHeadVisibility();
}

void AUnitBase::StopNavigation()
{
	if (IsValid(TacticalController)) TacticalController->CancelOrderNavigation();
}

void AUnitBase::CompleteNavigation(uint64 Revision, bool bSuccess)
{
	if (!CanParticipateInWorld() || bIsPossessedByPlayer || Orders.GetRevision() != Revision) return;
	if (bSuccess)
	{
		Orders.StopMovement();
		NavigationResult = EUnitNavigationResult::Arrived;
	}
	else
	{
		// Fail closed rather than walk directly through geometry or retry the same path each frame.
		Orders.Hold();
		NavigationResult = EUnitNavigationResult::Unreachable;
	}
}

bool AUnitBase::InitializeUnit(FName DefinitionID, EFactionID NewOwner)
{
	if (GetLifeState() != EUnitLifeState::Uninitialized || IsActorBeingDestroyed() ||
		!RomanEmpireContent::IsStableID(DefinitionID))
	{
		return false;
	}

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanUnitDefinition* Asset = Registry ? Registry->FindUnit(DefinitionID) : nullptr;
	const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(NewOwner) : nullptr;
	if (!Registry || !Registry->IsReady() || !Asset || !Faction ||
		!Asset->AvailableToFactions.Contains(Faction->Definition.FactionID))
	{
		return false;
	}

	FUnitVitalState InitialVitals;
	if (!InitialVitals.Initialize(Asset->Definition.BaseStats.MaxHealth,
		Asset->Definition.BaseStats.Stamina, Asset->Definition.BaseStats.Morale)) return false;

	UnitDefinitionID = DefinitionID;
	UnitData = Asset->Definition;
	OwnerFaction = NewOwner;
	Vitals = InitialVitals;
	return true;
}

bool AUnitBase::CanAcceptCommands() const
{
	return CanParticipateInWorld() && !IsRouted() && !IsTacticalBattleResolved();
}

bool AUnitBase::IsTacticalBattleResolved() const
{
	const UTacticalBattleSubsystem* Battle = GetWorld() ? GetWorld()->GetSubsystem<UTacticalBattleSubsystem>() : nullptr;
	return Battle && Battle->IsResolved();
}

void AUnitBase::StopForBattleResolution()
{
	StopAllActions();
	SetActorTickEnabled(false);
}

bool AUnitBase::CanParticipateInWorld() const
{
	return HasActorBegunPlay() && IsAlive() && !IsActorBeingDestroyed();
}

EUnitActivity AUnitBase::GetActivity() const
{
	if (GetLifeState() == EUnitLifeState::Ended) return EUnitActivity::Ended;
	if (GetLifeState() == EUnitLifeState::Dead) return EUnitActivity::Dead;
	if (CanParticipateInWorld() && IsRouted()) return EUnitActivity::Routed;
	if (!CanAcceptCommands()) return EUnitActivity::Unavailable;
	if (Combat.IsAttackInProgress()) return EUnitActivity::Attacking;
	return Orders.GetOrderType() == EUnitOrderType::None ? EUnitActivity::Idle : EUnitActivity::Commanded;
}

void AUnitBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Vitals.EndLife();
	if (URomanUnitRegistrySubsystem* Registry = GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>()) Registry->RemoveUnit(this);
	SetLifeSpan(0.0f);
	StopAllActions();
	SetActorEnableCollision(false);
	if (GetCharacterMovement()) GetCharacterMovement()->DisableMovement();
	ARomanUnitAIController* RetiredTacticalController = TacticalController;
	TacticalController = nullptr;
	if (IsValid(RetiredTacticalController)) RetiredTacticalController->Destroy();
	Super::EndPlay(EndPlayReason);
}

void AUnitBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f) return;
	// Presentation only. Actor lifespan owns cleanup independently of world altitude.
	if (GetLifeState() == EUnitLifeState::Dead)
	{
		FVector Loc = GetActorLocation();
		Loc.Z -= 30.0f * DeltaSeconds;
		SetActorLocation(Loc, false);
		return; // Dead units skip all other logic
	}
	if (IsTacticalBattleResolved())
	{
		StopForBattleResolution();
		return;
	}
	if (CanParticipateInWorld() && IsRouted())
	{
		if (ARomanUnitAIController* AI = Cast<ARomanUnitAIController>(GetController())) AI->SynchronizeOrder(this);
		return; // Withdrawal has no attack, acquisition, stamina action or rally loop.
	}
	if (!CanAcceptCommands()) return;

	// The only temporal combat update, before either AI or direct-control actions.
	if (Combat.Advance(DeltaSeconds))
	{
		ResolveMeleeImpact();
		return; // A damage observer may replace orders; evaluate them on the next tick.
	}
	if (!bIsPossessedByPlayer)
	{
		// Order state owns intent; the AI controller executes it through path following.
		if (Orders.GetOrderType() == EUnitOrderType::Attack)
		{
			AUnitBase* Target = Cast<AUnitBase>(Orders.GetTarget());
			if (Target && Target->CanParticipateInWorld() && Target->GetOwnerFaction() != OwnerFaction)
			{
				float Dist2D = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
				if (Dist2D <= UnitData.AttackRange)
				{
					// Commit recovery before damage observers can reenter this unit.
					Orders.StopMovement();
					NavigationResult = EUnitNavigationResult::Arrived;
					const uint64 Revision = Orders.GetRevision();
					StopNavigation();
					if (!CanAcceptCommands() || Orders.GetRevision() != Revision) return;
					const float RecoveryDuration = GetAttackRecoveryDuration();
					const float WindupDuration = RecoveryDuration * 0.25f;
					if (WindupDuration > 0.0f && Combat.TryBeginAttack(RecoveryDuration, WindupDuration))
					{
						PendingMeleeKind = EPendingMeleeKind::Ordered;
						PendingMeleeTarget = Target;
						PendingMeleeOrderRevision = Revision;

						PlayAttackPresentation(WindupDuration);
					}
					// Face target while fighting
					FVector FaceDir = Target->GetActorLocation() - GetActorLocation();
					FaceDir.Z = 0.0f;
					if (!FaceDir.IsNearlyZero())
						SetActorRotation(FMath::RInterpTo(GetActorRotation(), FaceDir.Rotation(), DeltaSeconds, 10.0f));
				}
				else
				{
					// Out of range: walk toward target
					Orders.UpdatePursuitDestination(Target->GetActorLocation());
				}
			}
			else Orders.Clear(); // Dead/destroyed/friendly targets retire pursuit as well.
		}
		else if (Orders.GetOrderType() == EUnitOrderType::None)
		{
			// Query the live faction index; never enumerate the world from a unit tick.
			if (const URomanUnitRegistrySubsystem* Registry = GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>())
			{
				if (AUnitBase* Enemy = Registry->FindNearestEnemy(GetActorLocation(), OwnerFaction, 5000.0)) CommandAttack(Enemy);
			}
		}

		if (ARomanUnitAIController* AI = Cast<ARomanUnitAIController>(GetController())) AI->SynchronizeOrder(this);
	} // END if (!bIsPossessedByPlayer)

	UpdateStamina(DeltaSeconds);
}

void AUnitBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Input is handled by PlayerController in FPS mode
}

void AUnitBase::SetOwnerFaction(EFactionID NewOwner)
{
	if (OwnerFaction == NewOwner || IsActorBeingDestroyed() || GetLifeState() == EUnitLifeState::Ended) return;
	OwnerFaction = NewOwner;
	// Reindex before observers can query or issue nested ownership/lifecycle commands.
	if (UWorld* World = GetWorld())
	{
		if (URomanUnitRegistrySubsystem* Registry = World->GetSubsystem<URomanUnitRegistrySubsystem>()) Registry->SynchronizeUnit(this);
	}
	ApplyFactionColor();
	OnOwnerFactionChanged.Broadcast(this);
}

void AUnitBase::ConfigureCharacterPresentation()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (!CharacterMesh || !CharacterPresentation || !IsInitialized()) return;
	USkeletalMesh* Profile = OwnerFaction == EFactionID::Carthage ? CharacterPresentation->CarthageMesh
		: OwnerFaction == EFactionID::Gaul ? CharacterPresentation->GaulMesh : CharacterPresentation->RomeMesh;
	if (!Profile || !CharacterPresentation->AnimationClass || !CharacterPresentation->AttackMontage) return;
	if (CharacterMesh->GetSkeletalMeshAsset() != Profile)
	{
		SkeletalVisualMaterials.Empty(); CharacterMesh->EmptyOverrideMaterials();
		CharacterMesh->SetSkeletalMeshAsset(Profile);
	}
	if (!CharacterMesh->GetAnimInstance() || CharacterMesh->GetAnimInstance()->GetClass() != CharacterPresentation->AnimationClass)
		CharacterMesh->SetAnimInstanceClass(CharacterPresentation->AnimationClass);
	CharacterMesh->SetVisibility(true); CharacterMesh->SetHiddenInGame(false); CharacterMesh->SetCastShadow(true);
	RefreshPossessionHeadVisibility();
	// Keep legacy subobject identities for existing BP serialization/diagnostic
	// compatibility, but never display both figures or animate procedural limbs.
	for (UStaticMeshComponent* Part : {VisualMesh.Get(), HeadMesh.Get(), WeaponMesh.Get(), ShieldMesh.Get(),
		RightArmMesh.Get(), LeftArmMesh.Get(), RightLegMesh.Get(), LeftLegMesh.Get(),
		RightHandMesh.Get(), LeftHandMesh.Get(), RightFootMesh.Get(), LeftFootMesh.Get()})
		if (Part) { Part->SetVisibility(false); Part->SetHiddenInGame(true); }
}

void AUnitBase::PlayAttackPresentation(float WindupSeconds)
{
	UAnimInstance* Animation = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;
	if (!Animation || !CharacterPresentation || !CharacterPresentation->AttackMontage || !CanAcceptCommands() || !IsAttacking()
		|| !FMath::IsFinite(WindupSeconds) || WindupSeconds <= 0.f) return;
	// Original clip apex is halfway through. Match the existing canonical windup,
	// without animation callbacks, root motion, notifies or damage ownership.
	const float Rate = CharacterPresentation->AttackMontage->GetPlayLength() / (2.f * WindupSeconds);
	if (FMath::IsFinite(Rate) && Rate > 0.f) Animation->Montage_Play(CharacterPresentation->AttackMontage, Rate);
}

void AUnitBase::ApplyFactionColor()
{
	if (!VisualMesh) return;
	ConfigureCharacterPresentation();

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(OwnerFaction) : nullptr;
	const FLinearColor FactionColor = Faction
		? Faction->Definition.FactionColor
		: FLinearColor(0.5f, 0.5f, 0.5f);

	const auto Apply = [this](UStaticMeshComponent* Part, UMaterialInterface* Parent, FLinearColor Color, float Metallic = 0.f)
	{
		if (!Part || !Parent) return;
		UMaterialInstanceDynamic* MID = VisualMaterials.FindRef(Part);
		if (!MID || MID->Parent != Parent)
		{
			MID = UMaterialInstanceDynamic::Create(Parent, this);
			if (!MID) return;
			VisualMaterials.Add(Part, MID);
			Part->SetMaterial(0, MID);
		}
		MID->SetVectorParameterValue(TEXT("Color"), Color);
		MID->SetScalarParameterValue(TEXT("Metallic"), Metallic);
	};
	UMaterialInterface* FactionParent = FactionMaterials.FindRef(OwnerFaction);
	if (!FactionParent) FactionParent = SurfaceMaterial;
	if (GetMesh() && GetMesh()->GetSkeletalMeshAsset() && CharacterPresentation && CharacterPresentation->bUseMeshMaterialParents)
	{
		// Preserve each authored PBR parent; faction/health are actor-local parameters,
		// never replacements by a flat faction material or changes to shared assets.
		const TArray<FSkeletalMaterial>& Slots = GetMesh()->GetSkeletalMeshAsset()->GetMaterials();
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			UMaterialInterface* Parent = Slots[Index].MaterialInterface;
			if (!Parent) continue;
			UMaterialInstanceDynamic* MID = SkeletalVisualMaterials.FindRef(Index);
			if (!MID || MID->Parent != Parent)
			{
				MID = UMaterialInstanceDynamic::Create(Parent, this);
				if (!MID) continue;
				SkeletalVisualMaterials.Add(Index, MID);
			}
			GetMesh()->SetMaterial(Index, MID);
			const bool Tint = CharacterPresentation->FactionTintSlots.Contains(Slots[Index].MaterialSlotName);
			FLinearColor Color = Tint ? FactionColor : FLinearColor::White;
			if (Tint && Slots[Index].MaterialSlotName == TEXT("Shield")) Color *= .6f;
			Color.A = 1.f;
			MID->SetVectorParameterValue(TEXT("Color"), Color);
			MID->SetScalarParameterValue(TEXT("FactionTintStrength"), Tint ? 1.f : 0.f);
		}
	}
	else if (GetMesh() && GetMesh()->GetSkeletalMeshAsset() && FactionParent)
	{
		for (const int32 Index : {0, 4, 5})
		{
			UMaterialInstanceDynamic* MID = SkeletalVisualMaterials.FindRef(Index);
			if (!MID || MID->Parent != FactionParent)
			{
				MID = UMaterialInstanceDynamic::Create(FactionParent, this);
				if (!MID) continue;
				SkeletalVisualMaterials.Add(Index, MID); GetMesh()->SetMaterial(Index, MID);
			}
			FLinearColor Color = Index == 4 ? FactionColor * .6f : FactionColor; Color.A = 1.f;
			MID->SetVectorParameterValue(TEXT("Color"), Color);
		}
	}
	Apply(VisualMesh, FactionParent, FactionColor);
	FLinearColor ShieldColor = FactionColor * 0.6f; ShieldColor.A = 1.f;
	Apply(ShieldMesh, FactionParent, ShieldColor);
	Apply(HeadMesh, SurfaceMaterial, FLinearColor(0.85f, 0.72f, 0.55f));
	Apply(WeaponMesh, SurfaceMaterial, FLinearColor(0.75f, 0.75f, 0.8f), 0.8f);
	for (UStaticMeshComponent* Part : {RightArmMesh.Get(), LeftArmMesh.Get(), RightLegMesh.Get(), LeftLegMesh.Get(),
		RightHandMesh.Get(), LeftHandMesh.Get(), RightFootMesh.Get(), LeftFootMesh.Get()})
		Apply(Part, SurfaceMaterial, FLinearColor(0.85f, 0.72f, 0.55f));
	RefreshDamageVisuals();
}

void AUnitBase::RefreshDamageVisuals()
{
	for (const auto& Entry : SkeletalVisualMaterials)
		if (Entry.Value) Entry.Value->SetScalarParameterValue(TEXT("Damage"), 1.f - GetHealthPercent());
	for (UStaticMeshComponent* Part : {VisualMesh.Get(), ShieldMesh.Get()})
	{
		if (UMaterialInstanceDynamic* MID = VisualMaterials.FindRef(Part))
			MID->SetScalarParameterValue(TEXT("Damage"), 1.f - GetHealthPercent());
	}
}

void AUnitBase::SetSelected(bool bNewSelected)
{
	if (bNewSelected && !CanAcceptCommands()) return;
	SelectionPresentation.SetSelected(bNewSelected, SelectionDecal, SelectionRingMesh, this);
}

void AUnitBase::CommandMoveTo(const FVector& Destination)
{
	if (!CanAcceptCommands()) return;
	if (Orders.IssueMove(Destination))
	{
		CancelPendingMelee();
		NavigationResult = EUnitNavigationResult::None;
		StopNavigation();
	}
}

void AUnitBase::CommandAttack(AActor* Target)
{
	if (!CanAcceptCommands() || !IsValid(Target) || Target == this || Target->GetWorld() != GetWorld()) return;
	const AUnitBase* TargetUnit = Cast<AUnitBase>(Target);
	if (!TargetUnit || !TargetUnit->CanParticipateInWorld() || TargetUnit->GetOwnerFaction() == OwnerFaction) return;
	if (Orders.IssueAttack(Target, Target->GetActorLocation()))
	{
		CancelPendingMelee();
		NavigationResult = EUnitNavigationResult::None;
		StopNavigation();
	}
}

void AUnitBase::CommandStop()
{
	CancelPendingMelee();
	Orders.Clear();
	NavigationResult = EUnitNavigationResult::None;
	ConsumeMovementInputVector();
	if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
	// Completion observers may issue a new order. Publish the entire stop first.
	StopNavigation();
}

void AUnitBase::CommandHold()
{
	if (!CanAcceptCommands()) return;
	CancelPendingMelee();
	Orders.Hold();
	NavigationResult = EUnitNavigationResult::None;
	ConsumeMovementInputVector();
	if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
	StopNavigation();
}

void AUnitBase::MoveInput(const FVector2D& InputVector)
{
	if (!CanAcceptCommands() || !bIsPossessedByPlayer || InputVector.ContainsNaN())
	{
		return;
	}

	// Get controller rotation
	const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// Get forward and right vectors
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	// Apply movement
	AddMovementInput(ForwardDirection, InputVector.Y);
	AddMovementInput(RightDirection, InputVector.X);
}

void AUnitBase::LookInput(const FVector2D& InputVector)
{
	if (!CanAcceptCommands() || !bIsPossessedByPlayer || InputVector.ContainsNaN())
	{
		return;
	}

	// Add rotation from mouse input
	AddControllerYawInput(InputVector.X);
	AddControllerPitchInput(InputVector.Y);
}

void AUnitBase::PerformAttack()
{
	const float RecoveryDuration = GetAttackRecoveryDuration();
	const float WindupDuration = RecoveryDuration * 0.25f;
	if (!CanAcceptCommands() || !bIsPossessedByPlayer || !Combat.CanAttack() ||
		WindupDuration <= 0.0f || WindupDuration >= RecoveryDuration)
	{
		return; // Still on cooldown
	}

	if (!TrySpendUnitStamina(10.0f))
	{
		return; // Not enough stamina
	}

	if (!Combat.TryBeginAttack(RecoveryDuration, WindupDuration)) return;
	PendingMeleeKind = EPendingMeleeKind::Direct;
	PendingMeleeTarget.Reset();
	PendingMeleeOrderRevision = 0;
	PlayAttackPresentation(WindupDuration);
	OnUnitVitalsChanged.Broadcast(this);
	if (!CanAcceptCommands()) return;
}

void AUnitBase::CancelPendingMelee()
{
	if (CharacterPresentation && CharacterPresentation->AttackMontage && GetMesh() && GetMesh()->GetAnimInstance())
		GetMesh()->GetAnimInstance()->Montage_Stop(.05f, CharacterPresentation->AttackMontage);
	Combat.CancelPendingImpact();
	PendingMeleeKind = EPendingMeleeKind::None;
	PendingMeleeTarget.Reset();
	PendingMeleeOrderRevision = 0;
}

bool AUnitBase::TryApplyMeleeHit(AUnitBase* Target)
{
	if (!CanAcceptCommands() || !IsValid(Target) || Target == this || Target->GetWorld() != GetWorld() ||
		!Target->CanParticipateInWorld() || Target->GetOwnerFaction() == OwnerFaction ||
		FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) > UnitData.AttackRange ||
		FMath::Abs(GetActorLocation().Z - Target->GetActorLocation().Z) > 200.0) return false;
	Target->TakeCombatDamage(float(UnitData.BaseStats.MeleeAttack), this, false);
	return true;
}

void AUnitBase::ResolveMeleeImpact()
{
	const EPendingMeleeKind Kind = PendingMeleeKind;
	AUnitBase* Target = PendingMeleeTarget.Get();
	const uint64 Revision = PendingMeleeOrderRevision;
	PendingMeleeKind = EPendingMeleeKind::None;
	PendingMeleeTarget.Reset();
	PendingMeleeOrderRevision = 0;
	if (!CanAcceptCommands()) return;
	if (Kind == EPendingMeleeKind::Ordered)
	{
		if (bIsPossessedByPlayer || Orders.GetRevision() != Revision || Orders.GetTarget() != Target) return;
		TryApplyMeleeHit(Target);
		if (CanAcceptCommands() && Orders.GetRevision() == Revision &&
			(!IsValid(Target) || !Target->CanParticipateInWorld())) Orders.Clear();
	}
	else if (Kind == EPendingMeleeKind::Direct && bIsPossessedByPlayer)
	{
		const FVector Start = GetActorLocation() + FVector(0, 0, 80);
		const FVector End = Start + GetActorForwardVector() * UnitData.AttackRange;
		FHitResult HitResult;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, Params))
			TryApplyMeleeHit(Cast<AUnitBase>(HitResult.GetActor()));
	}
}

void AUnitBase::StartBlocking()
{
	if (!CanAcceptCommands() || !UnitData.bHasShield || Combat.IsBlocking())
	{
		return;
	}

	Combat.SetBlocking(true);
	
	// Reduce movement speed while blocking
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed * 0.5f;
	}
	OnUnitVitalsChanged.Broadcast(this);
}

void AUnitBase::StopBlocking()
{
	if (!Combat.IsBlocking()) return;
	Combat.SetBlocking(false);
	
	// Restore movement speed
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = bIsPossessedByPlayer ? 500.0f : UnitData.BaseStats.Speed;
	}
	OnUnitVitalsChanged.Broadcast(this);
}

void AUnitBase::PerformDodge(const FVector2D& Direction)
{
	if (!CanAcceptCommands() || Direction.ContainsNaN() || !TrySpendUnitStamina(20.0f))
	{
		return; // Not enough stamina
	}

	OnUnitVitalsChanged.Broadcast(this);
	if (!CanAcceptCommands()) return;

	// Launch in direction
	FVector DodgeVector = GetActorRightVector() * Direction.X + GetActorForwardVector() * Direction.Y;
	DodgeVector.Normalize();
	DodgeVector *= 500.0f * UnitData.BaseStats.DodgeSpeed;
	DodgeVector.Z = 100.0f; // Slight upward push

	LaunchCharacter(DodgeVector, true, true);
}

void AUnitBase::TakeCombatDamage(float Damage, AActor* DamageSource, bool bIsRanged)
{
	if (!CanParticipateInWorld() || IsTacticalBattleResolved() || !FMath::IsFinite(Damage) || Damage <= 0.0f)
	{
		return;
	}
	if (DamageSource && (!IsValid(DamageSource) || DamageSource->GetWorld() != GetWorld())) return;
	if (const AUnitBase* SourceUnit = Cast<AUnitBase>(DamageSource);
		SourceUnit && (SourceUnit == this || SourceUnit->GetOwnerFaction() == OwnerFaction)) return;

	// Calculate damage reduction from armor and blocking
	const float ActualDamage = CalculateDamageReduction(Damage, bIsRanged);
	if (!FMath::IsFinite(ActualDamage)) return;
	// Clamp in double precision before rounding/conversion to avoid int32 overflow.
	const int32 HealthDamage = FMath::RoundToInt(FMath::Min(double(ActualDamage), double(GetCurrentHealth())));
	const int32 MoraleDamage = FMath::RoundToInt(FMath::Min(double(ActualDamage) * 0.1, double(GetCurrentMorale())));
	const bool bWasRouted = IsRouted();
	if (!Vitals.ApplyDamage(HealthDamage, MoraleDamage)) return;
	RefreshDamageVisuals();
	if (HealthDamage > 0)
		if (auto* Audio = GetWorld()->GetSubsystem<URomanFeedbackSubsystem>()) Audio->PlayCue(ERomanFeedbackCue::Impact, GetActorLocation());
	if (!IsAlive()) OnDeath();
	else if (!bWasRouted && IsRouted()) BeginRout(DamageSource);
	if (IsActorBeingDestroyed() || GetLifeState() == EUnitLifeState::Ended) return;
	OnUnitDamaged.Broadcast(this, ActualDamage);
	if (!IsActorBeingDestroyed() && GetLifeState() != EUnitLifeState::Ended) OnUnitVitalsChanged.Broadcast(this);
}

void AUnitBase::SetCurrentHealth(int32 NewHealth)
{
	if (!CanParticipateInWorld() || IsTacticalBattleResolved()) return;
	const int32 PreviousHealth = GetCurrentHealth();
	if (Vitals.SetHealth(FMath::Max(0, NewHealth)) && GetCurrentHealth() != PreviousHealth)
	{
		RefreshDamageVisuals();
		if (!IsAlive()) OnDeath();
		if (!IsActorBeingDestroyed() && GetLifeState() != EUnitLifeState::Ended) OnUnitVitalsChanged.Broadcast(this);
	}
}

float AUnitBase::CalculateDamageReduction(float RawDamage, bool bIsRanged) const
{
	float Defense = bIsRanged ? UnitData.BaseStats.RangedDefense : UnitData.BaseStats.MeleeDefense;
	float ArmorReduction = UnitData.BaseStats.Armor;

	// Blocking reduces damage significantly
	if (Combat.IsBlocking() && !bIsRanged)
	{
		Defense += 20 * UnitData.BaseStats.BlockStrength;
	}

	// Apply defense and armor
	float DamageMultiplier = 100.0f / (100.0f + Defense);
	float FinalDamage = (RawDamage * DamageMultiplier) - ArmorReduction;

	return FMath::Max(1.0f, FinalDamage); // Minimum 1 damage
}

void AUnitBase::SetPossessedByPlayer(bool bPossessed)
{
	if (bPossessed && !CanAcceptCommands()) return;
	if (bPossessed != bIsPossessedByPlayer) CancelPendingMelee();
	bIsPossessedByPlayer = bPossessed;
	if (!bPossessed)
	{
		StopBlocking();
	}

	// Enable/disable FPS mode settings
	bUseControllerRotationYaw = bPossessed;
	bUseControllerRotationPitch = bPossessed;

	if (bPossessed)
	{
		// Stop AI movement when possessed
		CommandStop();

		// Increase walk speed in FPS
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = 500.0f;
		}
	}
	else
	{
		// Restore default walk speed
		if (GetCharacterMovement())
		{
			GetCharacterMovement()->MaxWalkSpeed = UnitData.BaseStats.Speed;
		}
	}
	// SetActive broadcasts synchronously. Finish all effects before observers can
	// exit possession or destroy this actor; never apply stale effects afterward.
	RefreshPossessionHeadVisibility();
	if (FPSCamera) FPSCamera->SetActive(bPossessed);
}

float AUnitBase::GetAttackRecoveryDuration() const
{
	const float Rate = UnitData.BaseStats.AttackSpeed;
	if (!FMath::IsFinite(Rate) || Rate <= 0.0f) return 0.0f;
	const double Duration = 1.0 / double(Rate);
	return Duration <= double(MAX_flt) ? static_cast<float>(Duration) : 0.0f;
}

void AUnitBase::UpdateStamina(float DeltaSeconds)
{
	// Regenerate stamina when not blocking or attacking
	if (CanAcceptCommands() && !Combat.IsBlocking() && !Combat.IsAttackInProgress())
	{
		float RegenRate = 10.0f; // Stamina per second
		const int32 PreviousPercent = FMath::FloorToInt(GetStaminaPercent() * 100.0f);
		Vitals.RegenerateStamina(RegenRate * DeltaSeconds);
		if (PreviousPercent != FMath::FloorToInt(GetStaminaPercent() * 100.0f))
		{
			OnUnitVitalsChanged.Broadcast(this);
		}
	}
}

void AUnitBase::OnDeath()
{
	if (GetLifeState() != EUnitLifeState::Dead || bDeathNotified) return;
	bDeathNotified = true;
	if (URomanUnitRegistrySubsystem* Registry = GetWorld()->GetSubsystem<URomanUnitRegistrySubsystem>()) Registry->RemoveUnit(this);
	StopAllActions();
	if (GetLifeState() != EUnitLifeState::Dead || IsActorBeingDestroyed()) return;
	SetActorEnableCollision(false);
	if (GetCharacterMovement()) GetCharacterMovement()->DisableMovement();
	if (GetLifeState() != EUnitLifeState::Dead || IsActorBeingDestroyed()) return;
	const float Lifetime = FMath::IsFinite(CorpseLifetimeSeconds) && CorpseLifetimeSeconds > 0.0f
		? CorpseLifetimeSeconds : 3.0f;
	SetLifeSpan(Lifetime);
	UE_LOG(LogRomanEmpire, Log, TEXT("Unit died: %s"), *GetName());
	// Every observer sees a fully quiescent corpse. Reentrant commands are rejected.
	OnUnitDied.Broadcast(this);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red,
			FString::Printf(TEXT("%s died!"), *GetName()));
	}
	
	// Sink animation is presentation only; the engine lifespan owns destruction.
}

bool AUnitBase::TrySpendUnitStamina(float Amount)
{
	return CanAcceptCommands() && Vitals.TrySpendStamina(Amount);
}

void AUnitBase::BeginRout(AActor* DamageSource)
{
	// Morale is already committed. Synchronous observers must see command rejection before cleanup.
	StopAllActions();
	if (!CanParticipateInWorld() || !IsRouted()) return;
	RestoreTacticalController();
	FVector Away = IsValid(DamageSource) && DamageSource->GetWorld() == GetWorld()
		? GetActorLocation() - DamageSource->GetActorLocation() : -GetActorForwardVector();
	Away.Z = 0.0;
	Away = Away.GetSafeNormal();
	if (Away.IsNearlyZero()) Away = FVector(-1.0, 0.0, 0.0);
	// One bounded slice withdrawal, not perpetual flight or an implicit recovery policy.
	Orders.IssueMove(GetActorLocation() + Away * 1000.0);
	NavigationResult = EUnitNavigationResult::None;
}

void AUnitBase::StopAllActions()
{
	StopSpecialActions();
	StopBlocking();
	SetPossessedByPlayer(false);
	SetSelected(false);
	CommandStop();
	if (GetCharacterMovement()) GetCharacterMovement()->ClearAccumulatedForces();
	Combat.Reset();
}

