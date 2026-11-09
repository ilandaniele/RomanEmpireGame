// Copyright Roman Empire Game. All Rights Reserved.
#include "Misc/AutomationTest.h"
#include "Building/BuildingPresentation.h"
#include "Building/BuildingTypes.h"
#include "Building/TrainingReservation.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Content/RomanContentRegistry.h"
#include "Content/RomanContentDefinitions.h"
#include "Core/RomanEmpireGameState.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/ScopeExit.h"
#include "Misc/DataValidation.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/GarbageCollection.h"
#include "UObject/Package.h"
#include "UObject/StructOnScope.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#include <limits>

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuildingPresentationPolicyTest,
	"RomanEmpireGame.BuildingPresentation.Policy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FBuildingPresentationPolicyTest::RunTest(const FString& Parameters)
{
	using namespace RomanEmpireBuildingPresentation;
	FBuildingData Data;
	TestTrue(TEXT("Null opt-in preserves legacy definition"),Validate(Data)==EFailure::None);
	Data.PresentationMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Real engine mesh"),Data.PresentationMesh.Get())) return false;
	Data.FootprintSize=FVector2D(4,4); Data.PrototypeScale=FVector(99,99,99);
	Data.PresentationTransform=FTransform(FRotator(0,15,0),FVector(17,25,-150));
	TestTrue(TEXT("Authored transform ignores prototype scale"),Validate(Data)==EFailure::None);
	Data.PresentationTransform.SetTranslation(FVector(400,0,-150));
	TestTrue(TEXT("Outside footprint rejected"),Validate(Data)==EFailure::OutsideFootprint);
	Data.PresentationTransform.SetTranslation(FVector(0,0,-100));
	TestTrue(TEXT("Floating mesh rejected"),Validate(Data)==EFailure::NotGrounded);
	Data.PresentationTransform.SetTranslation(FVector(0,0,-250));
	TestTrue(TEXT("Buried mesh rejected"),Validate(Data)==EFailure::NotGrounded);
	Data.PresentationTransform.SetScale3D(FVector(1,-1,1));
	TestTrue(TEXT("Mirrored transform rejected"),Validate(Data)==EFailure::InvalidTransform);
	Data.PresentationTransform.SetScale3D(FVector(1,0,1));
	TestTrue(TEXT("Collapsed transform rejected"),Validate(Data)==EFailure::InvalidTransform);
	Data.PresentationTransform=FTransform(FVector(0,0,-150));
	Data.FootprintSize.X=std::numeric_limits<double>::infinity();
	TestTrue(TEXT("Non-finite footprint rejected"),Validate(Data)==EFailure::OutsideFootprint);
	Data.FootprintSize=FVector2D(4,4);
	Data.PresentationTransform=FTransform(FQuat::Identity,FVector(0,0,-9.25),FVector(6.686,5.73,3.815));
	TestTrue(TEXT("Camp03-sized envelope cannot silently expand footprint"),Validate(Data)==EFailure::OutsideFootprint);
	return true;
}

namespace RomanBuildingAppearanceTests
{
template<typename T> TArray<T*> MutableDefinitions(const TArray<const T*>& Definitions)
{
	TArray<T*> Result;
	for (const T* Definition : Definitions) Result.Add(const_cast<T*>(Definition));
	return Result;
}
template<typename T> T* Property(UStruct* Owner,const TCHAR* Name)
{
	T* Value=FindFProperty<T>(Owner,Name);
	if (!Value)
		if (FAutomationTestBase* Current=FAutomationTestFramework::Get().GetCurrentTest())
			Current->AddError(FString::Printf(TEXT("Missing reflected contract: %s"),Name));
	return Value;
}
// Public/reflected boundary: no private runtime header or fake initialization.
class FCall
{
	AActor* Actor;
	UFunction* Function;
	FStructOnScope Params;
public:
	FCall(AActor* InActor,const TCHAR* Name) : Actor(InActor),Function(InActor->FindFunctionChecked(Name)),Params(Function) {}
	FCall& Bool(const TCHAR* Name,bool Value) { if(auto* P=Property<FBoolProperty>(Function,Name)) P->SetPropertyValue_InContainer(Params.GetStructMemory(),Value); return *this; }
	FCall& Float(const TCHAR* Name,float Value) { if(auto* P=Property<FFloatProperty>(Function,Name)) P->SetPropertyValue_InContainer(Params.GetStructMemory(),Value); return *this; }
	FCall& Name(const TCHAR* Name,FName Value) { if(auto* P=Property<FNameProperty>(Function,Name)) P->SetPropertyValue_InContainer(Params.GetStructMemory(),Value); return *this; }
	FCall& Owner(EFactionID Value)
	{
		if(auto* P=Property<FEnumProperty>(Function,TEXT("NewOwner")))
			P->GetUnderlyingProperty()->SetIntPropertyValue(P->ContainerPtrToValuePtr<void>(Params.GetStructMemory()),static_cast<int64>(Value)); return *this;
	}
	void Run() { Actor->ProcessEvent(Function,Params.GetStructMemory()); }
	bool RunBool()
	{
		Run();
		if(auto* P=Property<FBoolProperty>(Function,TEXT("ReturnValue")))
			return P->GetPropertyValue_InContainer(Params.GetStructMemory());
		return false;
	}
};
FBuildingData DataOf(AActor* Actor)
{
	if(auto* P=Property<FStructProperty>(Actor->GetClass(),TEXT("BuildingData"))) return *P->ContainerPtrToValuePtr<FBuildingData>(Actor);
	return FBuildingData();
}
EBuildingState StateOf(AActor* Actor)
{
	if(auto* P=Property<FEnumProperty>(Actor->GetClass(),TEXT("CurrentState")))
		return static_cast<EBuildingState>(P->GetUnderlyingProperty()->GetSignedIntPropertyValue(P->ContainerPtrToValuePtr<void>(Actor)));
	return EBuildingState::Destroyed;
}
class FExercise : public IAutomationLatentCommand
{
	FAutomationTestBase& Test;
	double Started=FPlatformTime::Seconds();
public:
	explicit FExercise(FAutomationTestBase& InTest) : Test(InTest) {}
	bool Update() override
	{
		UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
		auto* Registry=World && World->GetGameInstance()?World->GetGameInstance()->GetSubsystem<URomanContentRegistrySubsystem>():nullptr;
		if (!Registry || !Registry->IsReady())
		{
			if (FPlatformTime::Seconds()-Started<20) return false;
			Test.AddError(TEXT("Owned PIE/content registry did not become ready")); return true;
		}
		auto* Definition=const_cast<URomanBuildingDefinition*>(Registry->FindBuilding(TEXT("Barracks")));
		if (!Test.TestNotNull(TEXT("Canonical Barracks definition"),Definition)) return true;
		const FBuildingData Original=Definition->Definition;
		ON_SCOPE_EXIT { Definition->Definition=Original; };
		UClass* Class=LoadClass<AActor>(nullptr,*Definition->BuildingClass.ToSoftObjectPath().ToString());
		if (!Test.TestNotNull(TEXT("Actual canonical factory class"),Class)) return true;
		auto Spawn=[&]() -> AActor*
		{
			const FTransform Transform(FVector(20000,20000,200));
			AActor* Actor=World->SpawnActorDeferred<AActor>(Class,Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Actor) { FCall(Actor,TEXT("SetOwnerFaction")).Owner(EFactionID::Rome).Run(); UGameplayStatics::FinishSpawningActor(Actor,Transform); }
			return Actor;
		};
		// Explicit fallback case, independent of whether the saved catalog adopts final art.
		Definition->Definition.PresentationMesh=nullptr;
		AActor* Legacy=Spawn();
		if (!Test.TestNotNull(TEXT("Legacy producer initializes through BeginPlay"),Legacy)) return true;
		auto* LegacyArt=FindObjectFast<UStaticMeshComponent>(Legacy,TEXT("BuildingMesh"));
		Test.TestTrue(TEXT("Existing definition stays prototype"),!DataOf(Legacy).PresentationMesh);
		Test.TestTrue(TEXT("Legacy cube scale unchanged"),LegacyArt && LegacyArt->GetRelativeScale3D().Equals(Original.PrototypeScale)); Legacy->Destroy();
		Definition->Definition=Original;
		UStaticMesh* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
		UStaticMesh* Mesh=Cube?DuplicateObject<UStaticMesh>(Cube,GetTransientPackage()):nullptr;
		if (!Test.TestNotNull(TEXT("Transient multi-slot asset"),Mesh)) return true;
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Mesh->GetMaterial(0)));
		Definition->Definition.PresentationMesh=Mesh;
		Definition->Definition.PresentationTransform=FTransform(FRotator(0,15,0),FVector(17,25,-150));
		const FTransform Expected=Definition->Definition.PresentationTransform;
		auto ValidateDefinitions=[&]()
		{
			return RomanEmpireContent::ValidateDefinitionSet(MutableDefinitions(Registry->GetAllFactions()),
				MutableDefinitions(Registry->GetAllUnits()),MutableDefinitions(Registry->GetAllBuildings()),
				MutableDefinitions(Registry->GetAllTerritories()));
		};
		Test.TestTrue(TEXT("Registry accepts valid authored definition"),ValidateDefinitions().IsValid());
		Definition->Definition.PresentationTransform.SetTranslation(FVector(1000,0,-150));
		const FContentDefinitionValidationReport InvalidReport=ValidateDefinitions();
		Test.TestTrue(TEXT("Registry rejects exactly the invalid building opt-in"),InvalidReport.Issues.Num()==1 &&
			InvalidReport.Issues[0].Failure==EContentDefinitionFailure::InvalidDefinition &&
			InvalidReport.Issues[0].DefinitionID==Original.BuildingID);
		Test.AddExpectedError(TEXT("has no valid content definition"),EAutomationExpectedErrorFlags::Contains,1);
		AActor* Rejected=Spawn();
		if (!Test.TestNotNull(TEXT("Rejected factory actor exists for inspection"),Rejected)) return true;
		Test.TestTrue(TEXT("Unsafe initialization installs no authored mesh and disables ticking"),
			!DataOf(Rejected).PresentationMesh && !Rejected->IsActorTickEnabled());
		Rejected->Destroy();
		Definition->Definition.PresentationTransform=Expected;
		AActor* Actor=Spawn();
		if (!Test.TestNotNull(TEXT("Opt-in producer"),Actor)) return true;
		ON_SCOPE_EXIT { if (IsValid(Actor)) Actor->Destroy(); };
		Definition->Definition=Original;
		auto* Art=FindObjectFast<UStaticMeshComponent>(Actor,TEXT("BuildingMesh"));
		auto* Feedback=FindObjectFast<UStaticMeshComponent>(Actor,TEXT("ConstructionScaffolding"));
		auto* Outline=FindObjectFast<UStaticMeshComponent>(Actor,TEXT("SelectionOutline0"));
		auto* Box=FindObjectFast<UBoxComponent>(Actor,TEXT("CollisionBox"));
		if (!Test.TestTrue(TEXT("Production components present"),Art && Feedback && Outline && Box)) return true;
		Test.TestTrue(TEXT("Authored asset installed"),Art->GetStaticMesh()==Mesh);
		Test.TestTrue(TEXT("Exact authored transform at BeginPlay"),Art->GetRelativeTransform().Equals(Expected));
		Test.TestTrue(TEXT("Footprint remains canonical"),Box->GetScaledBoxExtent().Equals(FVector(Original.FootprintSize.X*50,Original.FootprintSize.Y*50,200)));
		Test.TestTrue(TEXT("Art/feedback non-blocking and non-navigating"),Art->GetCollisionEnabled()==ECollisionEnabled::NoCollision && !Art->CanEverAffectNavigation() && Feedback->GetCollisionEnabled()==ECollisionEnabled::NoCollision && !Feedback->CanEverAffectNavigation());
		auto CheckSlots=[&]() { for (int32 I=0;I<Mesh->GetStaticMaterials().Num();++I) Test.TestEqual(TEXT("All PBR slots preserved"),Art->GetMaterial(I),Mesh->GetMaterial(I)); };
		CheckSlots();
		FCall(Actor,TEXT("SetPlacementMode")).Bool(TEXT("bPlacing"),true).Bool(TEXT("bValidPlacement"),false).Run();
		UMaterialInterface* Invalid=Feedback->GetMaterial(0);
		FCall(Actor,TEXT("SetPlacementMode")).Bool(TEXT("bPlacing"),true).Bool(TEXT("bValidPlacement"),true).Run();
		Test.TestTrue(TEXT("Valid/invalid feedback distinct"),Invalid!=Feedback->GetMaterial(0)); CheckSlots();
		FCall(Actor,TEXT("SetSelected")).Bool(TEXT("bNewSelected"),true).Run();
		Test.TestTrue(TEXT("Outline visible on canonical ground"),Outline->IsVisible() && FMath::IsNearlyEqual(Outline->GetRelativeLocation().Z,-197.0));
		FCall(Actor,TEXT("SetPlacementMode")).Bool(TEXT("bPlacing"),false).Bool(TEXT("bValidPlacement"),true).Run();
		Test.TestTrue(TEXT("Confirmed placement restores canonical actor collision"),Actor->GetActorEnableCollision());
		FCall(Actor,TEXT("StartConstruction")).Run(); Actor->Tick(Original.ConstructionTime*.5f);
		Test.TestTrue(TEXT("Construction retains pivot"),Art->GetRelativeLocation().Equals(Expected.GetTranslation()+FVector(0,0,-100)));
		Actor->Tick(Original.ConstructionTime*2);
		Test.TestTrue(TEXT("Overshoot completes at exact authored transform"),Art->GetRelativeTransform().Equals(Expected));
		Test.TestEqual(TEXT("Construction state completes"),StateOf(Actor),EBuildingState::Complete); CheckSlots();
		FCall(Actor,TEXT("ApplyBuildingDamage")).Float(TEXT("Damage"),Original.MaxHealth*.6f+Original.Armor).Run();
		Test.TestEqual(TEXT("Damage state retained"),StateOf(Actor),EBuildingState::Damaged); CheckSlots();
		auto* DamageMID=Cast<UMaterialInstanceDynamic>(Feedback->GetMaterial(0));
		Test.TestTrue(TEXT("Damage feedback visible"),Feedback->IsVisible() && DamageMID && DamageMID->K2_GetScalarParameterValue(TEXT("Damage"))>.5f);
		FCall(Actor,TEXT("Repair")).Float(TEXT("Amount"),Original.MaxHealth).Run();
		Test.TestEqual(TEXT("Repair restores complete"),StateOf(Actor),EBuildingState::Complete); CheckSlots();
		auto* MID=Cast<UMaterialInstanceDynamic>(Feedback->GetMaterial(0));
		const FLinearColor OldColor=MID?MID->K2_GetVectorParameterValue(TEXT("Color")):FLinearColor();
		FCall(Actor,TEXT("SetOwnerFaction")).Owner(EFactionID::Carthage).Run();
		Test.TestTrue(TEXT("Faction affects feedback not art"),MID && !MID->K2_GetVectorParameterValue(TEXT("Color")).Equals(OldColor)); CheckSlots();
		TWeakObjectPtr<UStaticMesh> WeakMesh(Mesh); CollectGarbage(RF_NoFlags);
		Test.TestTrue(TEXT("Actor/component own mesh after DA restore and GC"),WeakMesh.IsValid());
		Actor->Destroy();
		if(auto* Selected=Property<FBoolProperty>(Actor->GetClass(),TEXT("bIsSelected")))
			Test.TestFalse(TEXT("EndPlay clears retained selected state"),Selected->GetPropertyValue_InContainer(Actor));
		return true;
	}
};

// Retained by every latent step: restore the shared catalog even if PIE fails.
// Strong refs cover the pre-PIE interval, where no world/component owns the mesh.
class FInitialCampFixture
{
	TStrongObjectPtr<URomanBuildingDefinition> Definition;
	TStrongObjectPtr<UStaticMesh> OriginalMesh;
	bool bRestored = false;
public:
	TStrongObjectPtr<UStaticMesh> Mesh;
	FBuildingData Original;
	FTransform Expected = FTransform(FRotator(0,15,0),FVector(17,25,-150));

	FInitialCampFixture(URomanBuildingDefinition* InDefinition,UStaticMesh* InMesh)
		: Definition(InDefinition),OriginalMesh(InDefinition->Definition.PresentationMesh.Get()),
		  Mesh(InMesh),Original(InDefinition->Definition)
	{
		Definition->Definition.PresentationMesh = Mesh.Get();
		Definition->Definition.PresentationTransform = Expected;
	}
	~FInitialCampFixture() { Restore(); }
	void Restore()
	{
		if (!bRestored)
		{
			Definition->Definition = Original;
			bRestored = true;
		}
	}
};

class FCheckInitialCamp : public IAutomationLatentCommand
{
	FAutomationTestBase& Test;
	TSharedRef<FInitialCampFixture> Fixture;
	double Started = FPlatformTime::Seconds();
public:
	FCheckInitialCamp(FAutomationTestBase& InTest,TSharedRef<FInitialCampFixture> InFixture)
		: Test(InTest),Fixture(InFixture) {}
	bool Update() override
	{
		UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
		auto* State = World ? World->GetGameState<ARomanEmpireGameState>() : nullptr;
		if (!World || !World->HasBegunPlay() || !State || !State->IsGameplayInitialized())
		{
			if (FPlatformTime::Seconds()-Started < 20) return false;
			Test.AddError(TEXT("Real initial camp did not become ready"));
			Fixture->Restore();
			return true;
		}
		ON_SCOPE_EXIT { Fixture->Restore(); };
		AActor* Producer = nullptr;
		int32 Producers = 0;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->GetOwner() != World->GetAuthGameMode() ||
				!FindFProperty<FStructProperty>(It->GetClass(),TEXT("BuildingData"))) continue;
			if (DataOf(*It).BuildingID != TEXT("Barracks")) continue;
			Producer = *It;
			++Producers;
		}
		Test.TestEqual(TEXT("Exactly one real GameMode-owned starter producer"),Producers,1);
		if (!Test.TestNotNull(TEXT("Initial camp producer"),Producer)) return true;
		auto* Art = FindObjectFast<UStaticMeshComponent>(Producer,TEXT("BuildingMesh"));
		auto* Box = FindObjectFast<UBoxComponent>(Producer,TEXT("CollisionBox"));
		if (!Test.TestTrue(TEXT("Initial producer components"),Art && Box)) return true;
		Test.TestEqual(TEXT("Initial completion did not use timed construction"),StateOf(Producer),EBuildingState::Complete);
		Test.TestTrue(TEXT("Starter camp uses canonical spawn"),Producer->GetActorLocation().Equals(FVector(-600,-800,200)));
		Test.TestTrue(TEXT("Initial completion retains authored pivot/rotation/scale"),Art->GetRelativeTransform().Equals(Fixture->Expected));
		Test.TestTrue(TEXT("Initial completion retains definition footprint"),Box->GetScaledBoxExtent().Equals(
			FVector(Fixture->Original.FootprintSize.X*50,Fixture->Original.FootprintSize.Y*50,200)));
		UStaticMesh* Mesh = Fixture->Mesh.Get();
		Test.TestTrue(TEXT("Initial factory installed exact opt-in mesh"),Art->GetStaticMesh()==Mesh);
		auto CheckSlots = [&]()
		{
			for (int32 I=0; I<Mesh->GetStaticMaterials().Num(); ++I)
				Test.TestEqual(TEXT("Initial producer PBR slots untouched"),Art->GetMaterial(I),Mesh->GetMaterial(I));
		};
		CheckSlots();

		// Restore catalog before GC; neither fixture nor catalog may pin the test mesh.
		Fixture->Restore();
		TWeakObjectPtr<UStaticMesh> WeakMesh(Mesh);
		Fixture->Mesh.Reset();
		CollectGarbage(RF_NoFlags);
		if (!Test.TestTrue(TEXT("Real starter actor retains mesh after restoration/GC"),WeakMesh.IsValid())) return true;
		CheckSlots();

		const auto* Registry = World->GetGameInstance()->GetSubsystem<URomanContentRegistrySubsystem>();
		const auto* Unit = Registry ? Registry->FindUnit(TEXT("Legionary")) : nullptr;
		if (!Test.TestNotNull(TEXT("Canonical training definition"),Unit)) return true;
		const FFactionResourceAmounts Before = State->GetFactionResources(EFactionID::Rome);
		const FFactionResourceAmounts Cost = Unit->Definition.TrainingCost;
		if (!Test.TestTrue(TEXT("Initial authored producer accepts real training command"),
			FCall(Producer,TEXT("TrainUnit")).Name(TEXT("UnitID"),TEXT("Legionary")).RunBool())) return true;
		ON_SCOPE_EXIT { FCall(Producer,TEXT("CancelTraining")).Run(); };
		const FFactionResourceAmounts After = State->GetFactionResources(EFactionID::Rome);
		Test.TestEqual(TEXT("Canonical gold debited once"),After.Gold,Before.Gold-Cost.Gold);
		Test.TestEqual(TEXT("Canonical food debited once"),After.Food,Before.Food-Cost.Food);
		Test.TestEqual(TEXT("Canonical iron debited once"),After.Iron,Before.Iron-Cost.Iron);
		Test.TestEqual(TEXT("Canonical wood debited once"),After.Wood,Before.Wood-Cost.Wood);
		Test.TestEqual(TEXT("Canonical stone debited once"),After.Stone,Before.Stone-Cost.Stone);
		Test.TestEqual(TEXT("Canonical population reserved once"),After.Population,Before.Population-Cost.Population);
		if (auto* P=Property<FStructProperty>(Producer->GetClass(),TEXT("ActiveTrainingOrder")))
		{
			const auto& Receipt = *P->ContainerPtrToValuePtr<FTrainingReservation>(Producer);
			Test.TestTrue(TEXT("Existing producer owns canonical training receipt"),Receipt.ReceiptID.IsValid() &&
				Receipt.UnitID==TEXT("Legionary") && Receipt.PayingFaction==EFactionID::Rome &&
				FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Receipt.PaidCost,&Cost,0));
		}
		Test.TestTrue(TEXT("Cancellation uses existing refund authority"),FCall(Producer,TEXT("TryCancelTraining")).RunBool());
		const FFactionResourceAmounts Refunded = State->GetFactionResources(EFactionID::Rome);
		Test.TestTrue(TEXT("Canonical wallet exactly restored"),FFactionResourceAmounts::StaticStruct()->CompareScriptStruct(&Refunded,&Before,0));
		CheckSlots();
		return true;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuildingPresentationPIETest,
	"RomanEmpireGame.BuildingPresentation.LifecyclePIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FBuildingPresentationPIETest::RunTest(const FString& Parameters)
{
	if (!TestNotNull(TEXT("Editor available"),GEditor) || !TestNull(TEXT("Test owns PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_Testbed"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(RomanBuildingAppearanceTests::FExercise(*this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInitialBuildingPresentationTest,
	"RomanEmpireGame.BuildingPresentation.InitialCampPIE",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FInitialBuildingPresentationTest::RunTest(const FString& Parameters)
{
	using namespace RomanBuildingAppearanceTests;
	if (!TestNotNull(TEXT("Editor available"),GEditor) || !TestNull(TEXT("Test owns PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_VerticalSlice"));
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> IDs;
	Manager.GetPrimaryAssetIdList(GetDefault<URomanBuildingDefinition>()->GetPrimaryAssetId().PrimaryAssetType,IDs);
	URomanBuildingDefinition* Definition = nullptr;
	for (const FPrimaryAssetId& ID : IDs)
	{
		auto* Candidate = Cast<URomanBuildingDefinition>(Manager.GetPrimaryAssetPath(ID).TryLoad());
		if (Candidate && Candidate->Definition.BuildingID==TEXT("Barracks")) { Definition=Candidate; break; }
	}
	if (!TestNotNull(TEXT("Actual catalog Barracks before PIE"),Definition)) return false;
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* Mesh = Cube ? DuplicateObject<UStaticMesh>(Cube,GetTransientPackage()) : nullptr;
	if (!TestNotNull(TEXT("Transient multi-slot mesh"),Mesh)) return false;
	Mesh->GetStaticMaterials().Add(FStaticMaterial(Mesh->GetMaterial(0)));
	TSharedRef<FInitialCampFixture> Fixture = MakeShared<FInitialCampFixture>(Definition,Mesh);
	if (!TestTrue(TEXT("Pre-PIE opt-in obeys existing bounds contract"),
		RomanEmpireBuildingPresentation::Validate(Definition->Definition)==RomanEmpireBuildingPresentation::EFailure::None)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckInitialCamp(*this,Fixture));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBuildingAssetPresentationValidationTest,
	"RomanEmpireGame.BuildingPresentation.EditorAssetValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FBuildingAssetPresentationValidationTest::RunTest(const FString& Parameters)
{
	auto* Asset = NewObject<URomanBuildingDefinition>(GetTransientPackage());
	Asset->Definition.BuildingID = TEXT("PresentationProbe");
	Asset->Definition.BuildingType = EBuildingType::Barracks;
	Asset->Definition.DisplayName = FText::FromString(TEXT("Presentation probe"));
	Asset->Definition.ConstructionTime = 1;
	Asset->Definition.MaxHealth = 100;
	Asset->Definition.FootprintSize = FVector2D(4,4);
	Asset->Definition.PrototypeScale = FVector::OneVector;
	Asset->BuildingClass = TSoftClassPtr<ABuildingBase>(FSoftObjectPath(TEXT("/Script/RomanEmpireGame.Barracks")));
	auto ValidateAsset = [&](const TCHAR* Label,bool bExpectedValid)
	{
		FDataValidationContext Context;
		TestEqual(Label,Asset->IsDataValid(Context),bExpectedValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid);
		TestEqual(FString(Label)+TEXT(" diagnostic count"),Context.GetNumErrors(),bExpectedValid ? 0u : 1u);
	};
	ValidateAsset(TEXT("Editor accepts null legacy presentation"),true);
	Asset->Definition.PresentationMesh = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!TestNotNull(TEXT("Actual engine mesh"),Asset->Definition.PresentationMesh.Get())) return false;
	Asset->Definition.PresentationTransform = FTransform(FVector(0,0,-150));
	ValidateAsset(TEXT("Editor accepts fitting grounded presentation"),true);
	Asset->Definition.PresentationTransform.SetTranslation(FVector(400,0,-150));
	ValidateAsset(TEXT("Editor rejects footprint overhang"),false);
	Asset->Definition.PresentationTransform.SetTranslation(FVector(0,0,-100));
	ValidateAsset(TEXT("Editor rejects floating presentation"),false);
	Asset->Definition.PresentationTransform.SetTranslation(FVector(0,0,-150));
	Asset->Definition.PresentationTransform.SetScale3D(FVector(1,-1,1));
	ValidateAsset(TEXT("Editor rejects mirrored presentation"),false);
	Asset->Definition.PresentationTransform = FTransform(FVector(0,0,-150));
	Asset->Definition.PresentationMesh = NewObject<UStaticMesh>(GetTransientPackage());
	ValidateAsset(TEXT("Editor rejects empty geometry"),false);
	Asset->Definition.PresentationMesh = nullptr;
	ValidateAsset(TEXT("Removing invalid opt-in restores legacy validity"),true);

	// Cross-definition validation alone does not exercise the editor's virtual hook.
	UAssetManager& Manager = UAssetManager::Get();
	TArray<FPrimaryAssetId> IDs;
	Manager.GetPrimaryAssetIdList(RomanEmpireContent::BuildingAssetType,IDs);
	TestEqual(TEXT("All six saved building definitions checked individually"),IDs.Num(),6);
	for (const FPrimaryAssetId& ID : IDs)
	{
		auto* Saved = Cast<URomanBuildingDefinition>(Manager.GetPrimaryAssetPath(ID).TryLoad());
		if (!TestNotNull(TEXT("Saved building definition loads"),Saved)) continue;
		FDataValidationContext Context;
		TestEqual(FString::Printf(TEXT("Saved %s passes editor validation"),*ID.ToString()),
			Saved->IsDataValid(Context),EDataValidationResult::Valid);
		TestEqual(TEXT("Saved building produces no validation error"),Context.GetNumErrors(),0u);
	}
	return true;
}
#endif
