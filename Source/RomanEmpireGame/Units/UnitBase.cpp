// Copyright Roman Empire Game. All Rights Reserved.

#include "UnitBase.h"
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
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

AUnitBase::AUnitBase()
{
	PrimaryActorTick.bCanEverTick = true;

	// Configure capsule (collision only, not visible in-game)
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetVisibility(false);
	GetCapsuleComponent()->SetHiddenInGame(true);
	GetCapsuleComponent()->SetCastShadow(false);

	// Hide default skeletal mesh (we use custom static meshes instead)
	if (GetMesh())
	{
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
	WalkAnimTime = 0.0f;
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
	return HasActorBegunPlay() && IsAlive() && !IsActorBeingDestroyed();
}

EUnitActivity AUnitBase::GetActivity() const
{
	if (GetLifeState() == EUnitLifeState::Ended) return EUnitActivity::Ended;
	if (GetLifeState() == EUnitLifeState::Dead) return EUnitActivity::Dead;
	if (!CanAcceptCommands()) return EUnitActivity::Unavailable;
	if (Combat.IsAttackInProgress()) return EUnitActivity::Attacking;
	return Orders.GetOrderType() == EUnitOrderType::None ? EUnitActivity::Idle : EUnitActivity::Commanded;
}

void AUnitBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Vitals.EndLife();
	SetLifeSpan(0.0f);
	StopAllActions();
	SetActorEnableCollision(false);
	if (GetCharacterMovement()) GetCharacterMovement()->DisableMovement();
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
	if (!CanAcceptCommands()) return;

	// The only temporal combat update, before either AI or direct-control actions.
	Combat.Advance(DeltaSeconds);
	if (!bIsPossessedByPlayer)
	{
		// Order state owns intent; this legacy executor is replaced by navigation in M3.3.
		if (Orders.GetOrderType() == EUnitOrderType::Attack)
		{
			AUnitBase* Target = Cast<AUnitBase>(Orders.GetTarget());
			if (Target && Target->CanAcceptCommands() && Target->GetOwnerFaction() != OwnerFaction)
			{
				float Dist2D = FVector::Dist2D(GetActorLocation(), Target->GetActorLocation());
				if (Dist2D <= UnitData.AttackRange)
				{
					// Commit recovery before damage observers can reenter this unit.
					Orders.StopMovement();
					if (Combat.TryBeginAttack(GetAttackRecoveryDuration()))
					{
						const float Dmg = static_cast<float>(UnitData.BaseStats.MeleeAttack);
						Target->TakeCombatDamage(Dmg, this, false);
						if (!CanAcceptCommands()) return;
						if (!IsValid(Target) || !Target->CanAcceptCommands())
						{
							// Do not overwrite a replacement order issued by a damage callback.
							if (Orders.GetTarget() == Target) Orders.Clear();
							else Orders.ClearInvalidTarget();
							return;
						}
						if (Orders.GetTarget() != Target) return;

						// Arm swing
						if (RightArmMesh) RightArmMesh->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));

						// BRIGHT GREEN — impossible to miss on screen
						if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
							FString::Printf(TEXT("HIT! %s->%s DMG:%.0f HP:%d/%d"),
								*UnitData.DisplayName.ToString(),
								*Target->UnitData.DisplayName.ToString(),
								Dmg, Target->GetCurrentHealth(),
								Target->UnitData.BaseStats.MaxHealth));

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
			// Auto-detect nearest enemy within 5000
			AUnitBase* ClosestEnemy = nullptr;
			float ClosestDist = 5000.0f;
			TArray<AActor*> AllActors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), AllActors);
			for (AActor* Actor : AllActors)
			{
				AUnitBase* Other = Cast<AUnitBase>(Actor);
				if (!Other || Other == this || !Other->CanAcceptCommands()) continue;
				if (Other->GetOwnerFaction() == OwnerFaction) continue;
				if (Other->GetOwnerFaction() == EFactionID::None) continue;
				float D = FVector::Dist2D(GetActorLocation(), Other->GetActorLocation());
				if (D < ClosestDist) { ClosestDist = D; ClosestEnemy = Other; }
			}
			if (ClosestEnemy) CommandAttack(ClosestEnemy);
		}

		// === MOVEMENT (inside bIsPossessedByPlayer check) ===
		if (Orders.HasMoveCommand())
		{
			FVector CurrentLoc = GetActorLocation();
			FVector Dir = Orders.GetDestination() - CurrentLoc;
			Dir.Z = 0.0f;
			float Dist = Dir.Size();

			if (Dist > 80.0f)
			{
				Dir.Normalize();
				float MoveSpeed = 300.0f;
				if (GetCharacterMovement())
				{
					MoveSpeed = GetCharacterMovement()->MaxWalkSpeed;
				}
				float StepSize = MoveSpeed * DeltaSeconds;
				if (StepSize > Dist) StepSize = Dist;

				FVector NewLoc = CurrentLoc + Dir * StepSize;
				NewLoc.Z = CurrentLoc.Z;
				SetActorLocation(NewLoc, false);

				FRotator TargetRot = Dir.Rotation();
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, 10.0f));
			}
			else
			{
				Orders.StopMovement();
			}
		}
	} // END if (!bIsPossessedByPlayer)

	// Procedural walking animation (limb swinging)
	bool bIsMoving = (GetVelocity().SizeSquared2D() > 100.0f);
	if (bIsMoving)
	{
		WalkAnimTime += DeltaSeconds * 8.0f; // Walk cycle speed
		float Swing = FMath::Sin(WalkAnimTime) * 15.0f; // degrees

		// Arms swing opposite to legs
		if (RightArmMesh)
			RightArmMesh->SetRelativeRotation(FRotator(Swing, 0.0f, 0.0f));
		if (LeftArmMesh)
			LeftArmMesh->SetRelativeRotation(FRotator(-Swing, 0.0f, 0.0f));

		// Legs alternate
		if (RightLegMesh)
			RightLegMesh->SetRelativeRotation(FRotator(-Swing, 0.0f, 0.0f));
		if (LeftLegMesh)
			LeftLegMesh->SetRelativeRotation(FRotator(Swing, 0.0f, 0.0f));
	}
	else
	{
		// Reset limbs to default when idle
		WalkAnimTime = 0.0f;
		if (RightArmMesh) RightArmMesh->SetRelativeRotation(FRotator::ZeroRotator);
		if (LeftArmMesh) LeftArmMesh->SetRelativeRotation(FRotator::ZeroRotator);
		if (RightLegMesh) RightLegMesh->SetRelativeRotation(FRotator::ZeroRotator);
		if (LeftLegMesh) LeftLegMesh->SetRelativeRotation(FRotator::ZeroRotator);
	}

	UpdateStamina(DeltaSeconds);
}

void AUnitBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	// Input is handled by PlayerController in FPS mode
}

void AUnitBase::SetOwnerFaction(EFactionID NewOwner)
{
	OwnerFaction = NewOwner;
	ApplyFactionColor();
}

void AUnitBase::ApplyFactionColor()
{
	if (!VisualMesh) return;

	const URomanContentRegistrySubsystem* Registry = URomanContentRegistrySubsystem::Get(this);
	const URomanFactionDefinition* Faction = Registry ? Registry->FindFaction(OwnerFaction) : nullptr;
	const FLinearColor FactionColor = Faction
		? Faction->Definition.FactionColor
		: FLinearColor(0.5f, 0.5f, 0.5f);

	UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
	if (BaseMat)
	{
		UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMat, this);
		if (MID)
		{
			MID->SetVectorParameterValue(TEXT("Color"), FactionColor);
			VisualMesh->SetMaterial(0, MID);
		}

		// Color head with a lighter skin tone variant
		if (HeadMesh)
		{
			UMaterialInstanceDynamic* HeadMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (HeadMID)
			{
				FLinearColor HeadColor = FLinearColor(0.85f, 0.72f, 0.55f); // Skin tone
				HeadMID->SetVectorParameterValue(TEXT("Color"), HeadColor);
				HeadMesh->SetMaterial(0, HeadMID);
			}
		}

		// Color sword blade (silver metallic)
		if (WeaponMesh)
		{
			UMaterialInstanceDynamic* SwordMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (SwordMID)
			{
				SwordMID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.75f, 0.75f, 0.8f));
				WeaponMesh->SetMaterial(0, SwordMID);
			}
		}

		// Color shield (darker faction color)
		if (ShieldMesh)
		{
			UMaterialInstanceDynamic* ShieldMID = UMaterialInstanceDynamic::Create(BaseMat, this);
			if (ShieldMID)
			{
				FLinearColor ShieldColor = FactionColor * 0.6f; // Darker faction
				ShieldColor.A = 1.0f;
				ShieldMID->SetVectorParameterValue(TEXT("Color"), ShieldColor);
				ShieldMesh->SetMaterial(0, ShieldMID);
			}
		}

		// Color limbs with skin tone
		FLinearColor SkinColor = FLinearColor(0.85f, 0.72f, 0.55f);
		TArray<UStaticMeshComponent*> SkinMeshes = {
			RightArmMesh, LeftArmMesh,
			RightLegMesh, LeftLegMesh,
			RightHandMesh, LeftHandMesh,
			RightFootMesh, LeftFootMesh
		};
		for (UStaticMeshComponent* LimbMesh : SkinMeshes)
		{
			if (LimbMesh)
			{
				UMaterialInstanceDynamic* LimbMID = UMaterialInstanceDynamic::Create(BaseMat, this);
				if (LimbMID)
				{
					LimbMID->SetVectorParameterValue(TEXT("Color"), SkinColor);
					LimbMesh->SetMaterial(0, LimbMID);
				}
			}
		}
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
	Orders.IssueMove(Destination);
}

void AUnitBase::CommandAttack(AActor* Target)
{
	if (!CanAcceptCommands() || !IsValid(Target) || Target == this || Target->GetWorld() != GetWorld()) return;
	const AUnitBase* TargetUnit = Cast<AUnitBase>(Target);
	if (!TargetUnit || !TargetUnit->CanAcceptCommands() || TargetUnit->GetOwnerFaction() == OwnerFaction) return;
	Orders.IssueAttack(Target, Target->GetActorLocation());
}

void AUnitBase::CommandStop()
{
	Orders.Clear();
	ConsumeMovementInputVector();
	if (GetCharacterMovement()) GetCharacterMovement()->StopMovementImmediately();
}

void AUnitBase::CommandHold()
{
	if (!CanAcceptCommands()) return;
	CommandStop();
	Orders.Hold();
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
	if (!CanAcceptCommands() || !Combat.CanAttack() || RecoveryDuration <= 0.0f)
	{
		return; // Still on cooldown
	}

	if (!TrySpendUnitStamina(10.0f))
	{
		return; // Not enough stamina
	}

	Combat.TryBeginAttack(RecoveryDuration);
	OnUnitVitalsChanged.Broadcast(this);
	if (!CanAcceptCommands()) return;

	// Perform melee attack trace
	FVector Start = GetActorLocation() + FVector(0, 0, 80);
	FVector End = Start + GetActorForwardVector() * UnitData.AttackRange;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Pawn, Params))
	{
		if (AUnitBase* HitUnit = Cast<AUnitBase>(HitResult.GetActor()))
		{
			// Check if enemy
			if (HitUnit->GetOwnerFaction() != OwnerFaction)
			{
				float Damage = UnitData.BaseStats.MeleeAttack;
				HitUnit->TakeCombatDamage(Damage, this, false);
			}
		}
	}

	// TODO: Play attack animation
	
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Unit %s attacking"), *GetName());
}

void AUnitBase::StartBlocking()
{
	if (!CanAcceptCommands() || !UnitData.bHasShield)
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
	if (!CanAcceptCommands() || !FMath::IsFinite(Damage) || Damage <= 0.0f)
	{
		return;
	}

	// Calculate damage reduction from armor and blocking
	const float ActualDamage = CalculateDamageReduction(Damage, bIsRanged);
	if (!FMath::IsFinite(ActualDamage)) return;
	// Clamp in double precision before rounding/conversion to avoid int32 overflow.
	const int32 HealthDamage = FMath::RoundToInt(FMath::Min(double(ActualDamage), double(GetCurrentHealth())));
	const int32 MoraleDamage = FMath::RoundToInt(FMath::Min(double(ActualDamage) * 0.1, double(GetCurrentMorale())));
	if (!Vitals.ApplyDamage(HealthDamage, MoraleDamage)) return;
	if (!IsAlive()) OnDeath();
	if (IsActorBeingDestroyed() || GetLifeState() == EUnitLifeState::Ended) return;
	OnUnitDamaged.Broadcast(this, ActualDamage);
	if (!IsActorBeingDestroyed() && GetLifeState() != EUnitLifeState::Ended) OnUnitVitalsChanged.Broadcast(this);
}

void AUnitBase::SetCurrentHealth(int32 NewHealth)
{
	if (!CanAcceptCommands()) return;
	const int32 PreviousHealth = GetCurrentHealth();
	if (Vitals.SetHealth(FMath::Max(0, NewHealth)) && GetCurrentHealth() != PreviousHealth)
	{
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

