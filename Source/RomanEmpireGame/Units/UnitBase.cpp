// Copyright Roman Empire Game. All Rights Reserved.

#include "UnitBase.h"
#include "../RomanEmpireGame.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
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

	// Create visible placeholder mesh (cylinder body)
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	if (VisualMesh)
	{
		VisualMesh->SetupAttachment(GetRootComponent());
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -90.0f));
		VisualMesh->SetRelativeScale3D(FVector(1.5f, 1.5f, 2.0f));  // Bigger so visible from RTS camera
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Use engine built-in cylinder mesh
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
		if (CylinderMesh.Succeeded())
		{
			VisualMesh->SetStaticMesh(CylinderMesh.Object);
		}
	}

	// Create selection decal (ring under unit when selected) - use a flat cylinder as selection ring
	SelectionDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("SelectionDecal"));
	if (SelectionDecal)
	{
		SelectionDecal->SetupAttachment(GetRootComponent());
		SelectionDecal->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		SelectionDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		SelectionDecal->DecalSize = FVector(64.0f, 64.0f, 64.0f);
		SelectionDecal->SetVisibility(false); // Hidden until selected
	}

	// Create visible selection ring (flat green cylinder)
	SelectionRingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SelectionRing"));
	if (SelectionRingMesh)
	{
		SelectionRingMesh->SetupAttachment(GetRootComponent());
		SelectionRingMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -94.0f));
		SelectionRingMesh->SetRelativeScale3D(FVector(2.5f, 2.5f, 0.05f)); // Flat disc
		SelectionRingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SelectionRingMesh->SetVisibility(false); // Hidden until selected

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
		Movement->MaxWalkSpeed = 300.0f;
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	}

	// Don't rotate when controller rotates (for RTS mode)
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Initialize state
	OwnerFaction = EFactionID::None;
	CurrentHealth = 100;
	CurrentStamina = 100.0f;
	CurrentMorale = 50;
	bIsSelected = false;
	bIsPossessedByPlayer = false;
	bIsBlocking = false;
	bIsAttacking = false;
	CurrentStance = EUnitStance::Defensive;
	bHasMoveCommand = false;
	AttackTarget = nullptr;
	AttackCooldown = 1.0f;
	AttackCooldownRemaining = 0.0f;
	WalkAnimTime = 0.0f;
	SelectionRingMesh = nullptr;
	HeadMesh = nullptr;
	WeaponMesh = nullptr;
	ShieldMesh = nullptr;
	FPSCameraArm = nullptr; // Will be set by CreateDefaultSubobject above
	FPSCamera = nullptr;
	RightArmMesh = nullptr;
	LeftArmMesh = nullptr;
	RightLegMesh = nullptr;
	LeftLegMesh = nullptr;
	RightHandMesh = nullptr;
	LeftHandMesh = nullptr;
	RightFootMesh = nullptr;
	LeftFootMesh = nullptr;
}

void AUnitBase::BeginPlay()
{
	Super::BeginPlay();
	
	// Initialize from unit data
	CurrentHealth = UnitData.BaseStats.MaxHealth;
	CurrentStamina = UnitData.BaseStats.Stamina;
	CurrentMorale = UnitData.BaseStats.Morale;
	
	// Set movement speed from unit data
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed;
	}
	
	UE_LOG(LogRomanEmpire, Verbose, TEXT("Unit spawned: %s"), *UnitData.DisplayName.ToString());
	ApplyFactionColor();  // Apply faction color on spawn
}

void AUnitBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsPossessedByPlayer)
	{
		// RTS mode - AI movement
		UpdateAIMovement(DeltaSeconds);

		// Direct movement toward destination (no NavMesh needed)
		if (bHasMoveCommand)
		{
			FVector CurrentLoc = GetActorLocation();
			FVector Dir = MoveDestination - CurrentLoc;
			Dir.Z = 0.0f;
			float Dist = Dir.Size();

			if (Dist > 80.0f)
			{
				Dir.Normalize();
				AddMovementInput(Dir, 1.0f);

				// Face movement direction
				FRotator TargetRot = Dir.Rotation();
				SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaSeconds, 10.0f));
			}
			else
			{
				bHasMoveCommand = false;
			}
		}
	}

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

	UpdateCombatCooldowns(DeltaSeconds);
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

	FLinearColor FactionColor;
	switch (OwnerFaction)
	{
		case EFactionID::Rome:     FactionColor = FLinearColor(0.8f, 0.15f, 0.1f); break;  // Red
		case EFactionID::Carthage: FactionColor = FLinearColor(0.5f, 0.1f, 0.6f); break;   // Purple
		case EFactionID::Gaul:     FactionColor = FLinearColor(0.2f, 0.5f, 0.15f); break;  // Green
		default:                   FactionColor = FLinearColor(0.5f, 0.5f, 0.5f); break;   // Gray
	}

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
	bIsSelected = bNewSelected;
	
	// Show/hide selection decal
	if (SelectionDecal)
	{
		SelectionDecal->SetVisibility(bNewSelected);
	}

	// Show/hide selection ring
	if (SelectionRingMesh)
	{
		SelectionRingMesh->SetVisibility(bNewSelected);
		if (bNewSelected)
		{
			// Apply green glowing material to selection ring
			UMaterial* BaseMat = LoadObject<UMaterial>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial"));
			if (BaseMat)
			{
				UMaterialInstanceDynamic* RingMat = UMaterialInstanceDynamic::Create(BaseMat, this);
				if (RingMat)
				{
					RingMat->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.0f, 1.0f, 0.0f, 0.7f)); // Green
					SelectionRingMesh->SetMaterial(0, RingMat);
				}
			}
		}
	}
}

void AUnitBase::CommandMoveTo(const FVector& Destination)
{
	MoveDestination = Destination;
	bHasMoveCommand = true;
	AttackTarget = nullptr;
}

void AUnitBase::CommandAttack(AActor* Target)
{
	AttackTarget = Target;
	if (Target)
	{
		CommandMoveTo(Target->GetActorLocation());
	}
}

void AUnitBase::CommandStop()
{
	bHasMoveCommand = false;
	AttackTarget = nullptr;
}

void AUnitBase::CommandHold()
{
	CommandStop();
	CurrentStance = EUnitStance::StandGround;
}

void AUnitBase::MoveInput(const FVector2D& InputVector)
{
	if (!bIsPossessedByPlayer)
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
	if (!bIsPossessedByPlayer)
	{
		return;
	}

	// Add rotation from mouse input
	AddControllerYawInput(InputVector.X);
	AddControllerPitchInput(InputVector.Y);
}

void AUnitBase::PerformAttack()
{
	if (AttackCooldownRemaining > 0.0f)
	{
		return; // Still on cooldown
	}

	if (CurrentStamina < 10.0f)
	{
		return; // Not enough stamina
	}

	bIsAttacking = true;
	AttackCooldownRemaining = AttackCooldown / UnitData.BaseStats.AttackSpeed;
	CurrentStamina -= 10.0f;

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
	if (!UnitData.bHasShield)
	{
		return;
	}

	bIsBlocking = true;
	
	// Reduce movement speed while blocking
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed * 0.5f;
	}
}

void AUnitBase::StopBlocking()
{
	bIsBlocking = false;
	
	// Restore movement speed
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	if (Movement)
	{
		Movement->MaxWalkSpeed = UnitData.BaseStats.Speed;
	}
}

void AUnitBase::PerformDodge(const FVector2D& Direction)
{
	if (CurrentStamina < 20.0f)
	{
		return; // Not enough stamina
	}

	CurrentStamina -= 20.0f;

	// Launch in direction
	FVector DodgeVector = GetActorRightVector() * Direction.X + GetActorForwardVector() * Direction.Y;
	DodgeVector.Normalize();
	DodgeVector *= 500.0f * UnitData.BaseStats.DodgeSpeed;
	DodgeVector.Z = 100.0f; // Slight upward push

	LaunchCharacter(DodgeVector, true, true);
}

void AUnitBase::TakeCombatDamage(float Damage, AActor* DamageSource, bool bIsRanged)
{
	if (!IsAlive())
	{
		return;
	}

	// Calculate damage reduction from armor and blocking
	float ActualDamage = CalculateDamageReduction(Damage, bIsRanged);

	CurrentHealth = FMath::Max(0, CurrentHealth - FMath::RoundToInt(ActualDamage));
	
	// Reduce morale when taking damage
	CurrentMorale = FMath::Max(0, CurrentMorale - FMath::RoundToInt(ActualDamage * 0.1f));

	OnUnitDamaged.Broadcast(this, ActualDamage);

	if (CurrentHealth <= 0)
	{
		OnDeath();
	}
}

float AUnitBase::CalculateDamageReduction(float RawDamage, bool bIsRanged) const
{
	float Defense = bIsRanged ? UnitData.BaseStats.RangedDefense : UnitData.BaseStats.MeleeDefense;
	float ArmorReduction = UnitData.BaseStats.Armor;

	// Blocking reduces damage significantly
	if (bIsBlocking && !bIsRanged)
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
	bIsPossessedByPlayer = bPossessed;

	// Enable/disable FPS mode settings
	bUseControllerRotationYaw = bPossessed;
	bUseControllerRotationPitch = bPossessed;

	// Activate/deactivate FPS camera
	if (FPSCamera)
	{
		FPSCamera->SetActive(bPossessed);
	}

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
}

void AUnitBase::UpdateAIMovement(float DeltaSeconds)
{
	// Check if we have an attack target and are in range
	if (AttackTarget && IsValid(AttackTarget))
	{
		float Distance = FVector::Distance(GetActorLocation(), AttackTarget->GetActorLocation());
		
		if (Distance <= UnitData.AttackRange)
		{
			// In attack range - stop and attack
			bHasMoveCommand = false;
			PerformAttack();
			return;
		}
		else
		{
			// Move toward target
			CommandMoveTo(AttackTarget->GetActorLocation());
			return;
		}
	}

	// Auto-detect enemies (only for non-player-controlled units)
	if (OwnerFaction != EFactionID::None && OwnerFaction != EFactionID::Rome)
	{
		// Enemy AI: scan for nearby Roman units
		const float DetectRange = 2000.0f;
		AUnitBase* ClosestEnemy = nullptr;
		float ClosestDist = DetectRange;

		TArray<AActor*> AllActors;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), AUnitBase::StaticClass(), AllActors);

		for (AActor* Actor : AllActors)
		{
			AUnitBase* OtherUnit = Cast<AUnitBase>(Actor);
			if (!OtherUnit || OtherUnit == this || !OtherUnit->IsAlive()) continue;
			if (OtherUnit->GetOwnerFaction() == OwnerFaction) continue; // Same team

			float Dist = FVector::Distance(GetActorLocation(), OtherUnit->GetActorLocation());
			if (Dist < ClosestDist)
			{
				ClosestDist = Dist;
				ClosestEnemy = OtherUnit;
			}
		}

		if (ClosestEnemy)
		{
			CommandAttack(ClosestEnemy);
			return;
		}
	}

	// If no attack target and no move command, patrol randomly
	if (!bHasMoveCommand && FMath::FRand() < 0.005f) // ~0.5% chance per frame = patrol occasionally
	{
		FVector PatrolOffset = FVector(FMath::RandRange(-500.0f, 500.0f), FMath::RandRange(-500.0f, 500.0f), 0.0f);
		CommandMoveTo(GetActorLocation() + PatrolOffset);
	}
}

void AUnitBase::UpdateCombatCooldowns(float DeltaSeconds)
{
	if (AttackCooldownRemaining > 0.0f)
	{
		AttackCooldownRemaining -= DeltaSeconds;
		
		if (AttackCooldownRemaining <= 0.0f)
		{
			bIsAttacking = false;
		}
	}
}

void AUnitBase::UpdateStamina(float DeltaSeconds)
{
	// Regenerate stamina when not blocking or attacking
	if (!bIsBlocking && !bIsAttacking)
	{
		float RegenRate = 10.0f; // Stamina per second
		CurrentStamina = FMath::Min(UnitData.BaseStats.Stamina, CurrentStamina + RegenRate * DeltaSeconds);
	}
}

void AUnitBase::OnDeath()
{
	UE_LOG(LogRomanEmpire, Log, TEXT("Unit died: %s"), *GetName());
	
	OnUnitDied.Broadcast(this);
	
	// TODO: Play death animation, spawn ragdoll
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	
	// Destroy after delay
	SetLifeSpan(5.0f);
}

