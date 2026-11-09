// Copyright Roman Empire Game. All Rights Reserved.
// Private Editor-only integration. Never copied into the production runtime module.
#include "Misc/AutomationTest.h"
#include "Building/BuildingPresentation.h"
#include "Content/RomanContentDefinitions.h"
#include "Content/RomanContentRegistry.h"
#include "Core/RomanEmpireGameState.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Serialization/JsonSerializer.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/Package.h"
#include "UObject/ObjectKey.h"
#include "UObject/SavePackage.h"
#include "UObject/StructOnScope.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
namespace RomanCampBuildingIntegration
{
constexpr const TCHAR* MeshPath=TEXT("/Game/ArtCandidates/Camp03/SM_CampBarracks.SM_CampBarracks");
constexpr const TCHAR* DefinitionPath=TEXT("/Game/Data/Buildings/PDA_Building_Barracks.PDA_Building_Barracks");
bool Authorized(FAutomationTestBase& Test)
{
	const FString Project=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()).Replace(TEXT("\\"),TEXT("/"));
	return Test.TestTrue(TEXT("Only existing private GamePreview may run this fixture"),
		Project.EndsWith(TEXT("/Artifacts/VQ01_Semireal1/GamePreview/")));
}
bool Report(FAutomationTestBase& Test,const TCHAR* Name,TSharedRef<FJsonObject> Object)
{
	FString Directory;
	if (!Test.TestTrue(TEXT("Explicit output supplied"),FParse::Value(FCommandLine::Get(),TEXT("RomanCampIntegrationOutput="),Directory))) return false;
	const FString Filename=FPaths::Combine(Directory,Name);
	if (!Test.TestFalse(TEXT("Receipt is create-only"),IFileManager::Get().FileExists(*Filename))) return false;
	Object->SetStringField(TEXT("scope"),TEXT("Private native Camp03 building integration; not player/visual/release approval"));
	Object->SetStringField(TEXT("project"),FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
	FString Text;
	FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text));
	return Test.TestTrue(TEXT("Receipt written"),FFileHelper::SaveStringToFile(Text,*Filename));
}
class FCall
{
	AActor* Actor;
	UFunction* Function;
	FStructOnScope Params;
public:
	FCall(AActor* A,const TCHAR* Name):Actor(A),Function(A->FindFunctionChecked(Name)),Params(Function) {}
	FCall& Name(const TCHAR* Field,FName Value)
	{
		if (auto* P=FindFProperty<FNameProperty>(Function,Field)) P->SetPropertyValue_InContainer(Params.GetStructMemory(),Value);
		return *this;
	}
	FCall& Vector(const TCHAR* Field,const FVector& Value)
	{
		if (auto* P=FindFProperty<FStructProperty>(Function,Field))
			P->CopyCompleteValue(P->ContainerPtrToValuePtr<void>(Params.GetStructMemory()),&Value);
		return *this;
	}
	void Run() { Actor->ProcessEvent(Function,Params.GetStructMemory()); }
	bool Bool()
	{
		Actor->ProcessEvent(Function,Params.GetStructMemory());
		auto* P=FindFProperty<FBoolProperty>(Function,TEXT("ReturnValue"));
		return P && P->GetPropertyValue_InContainer(Params.GetStructMemory());
	}
};
FBuildingData Data(AActor* Actor)
{
	if (auto* P=FindFProperty<FStructProperty>(Actor->GetClass(),TEXT("BuildingData")))
		return *P->ContainerPtrToValuePtr<FBuildingData>(Actor);
	return FBuildingData();
}
class FCheckProducer : public IAutomationLatentCommand
{
	FAutomationTestBase& Test;
	double Started=FPlatformTime::Seconds();
	bool bMoveRequested=false;
	TWeakObjectPtr<ACharacter> MovingRecruit;
	FVector MoveStart=FVector::ZeroVector;
	FVector MoveTarget=FVector::ZeroVector;
	double MoveStarted=0;
	TSharedPtr<FJsonObject> MovementReceipt;
public:
	explicit FCheckProducer(FAutomationTestBase& InTest):Test(InTest) {}
	bool Update() override
	{
		if (bMoveRequested)
		{
			ACharacter* Recruit=MovingRecruit.Get();
			if (!Recruit)
			{
				Test.AddError(TEXT("Trained recruit disappeared during physical exit"));
				MovementReceipt->SetBoolField(TEXT("passed"),false);
				Report(Test,TEXT("producer.json"),MovementReceipt.ToSharedRef());return true;
			}
			const FVector Feet=Recruit->GetNavAgentLocation();
			const double Distance=FVector::Dist2D(Feet,MoveStart);
			const double Remaining=FVector::Dist2D(Feet,MoveTarget);
			const bool bCommandActive=FCall(Recruit,TEXT("HasMoveCommand")).Bool();
			const bool bArrived=Distance>250 && Remaining<45 && !bCommandActive && FMath::Abs(Feet.Z-MoveStart.Z)<60;
			if (!bArrived && FPlatformTime::Seconds()-MoveStarted<15) return false;
			Test.TestTrue(TEXT("Actual recruit physically exits along the canonical move command"),bArrived);
			MovementReceipt->SetNumberField(TEXT("physical_distance_cm"),Distance);
			MovementReceipt->SetNumberField(TEXT("remaining_distance_cm"),Remaining);
			MovementReceipt->SetStringField(TEXT("physical_end_feet"),Feet.ToString());
			MovementReceipt->SetBoolField(TEXT("physical_move_completed"),bArrived);
			MovementReceipt->SetBoolField(TEXT("move_command_active"),bCommandActive);
			MovementReceipt->SetBoolField(TEXT("passed"),!Test.HasAnyErrors());
			Report(Test,TEXT("producer.json"),MovementReceipt.ToSharedRef());return true;
		}
		UWorld* World=GEditor?GEditor->PlayWorld:nullptr;
		auto* State=World?World->GetGameState<ARomanEmpireGameState>():nullptr;
		auto* Navigation=World?FNavigationSystem::GetCurrent<UNavigationSystemV1>(World):nullptr;
		if (!State || !State->IsGameplayInitialized() || !Navigation || Navigation->IsNavigationBuildInProgress())
		{
			if (FPlatformTime::Seconds()-Started<40) return false;
			Test.AddError(TEXT("Real world/navigation not ready"));return true;
		}
		AActor* Producer=nullptr;
		for (TActorIterator<AActor> It(World);It;++It)
			if (It->GetOwner()==World->GetAuthGameMode() && Data(*It).BuildingID==TEXT("Barracks")) {Producer=*It;break;}
		if (!Test.TestNotNull(TEXT("Real initial Barracks"),Producer)) return true;
		const FBuildingData Definition=Data(Producer);
		auto* Art=FindObjectFast<UStaticMeshComponent>(Producer,TEXT("BuildingMesh"));
		auto* Box=FindObjectFast<UBoxComponent>(Producer,TEXT("CollisionBox"));
		if (!Test.TestTrue(TEXT("Production components"),Art && Box)) return true;
		Test.TestTrue(TEXT("Actual Camp03, not transient cube"),Art->GetStaticMesh() && Art->GetStaticMesh()->GetPathName()==MeshPath);
		Test.TestTrue(TEXT("Approved 7x6 footprint"),Definition.FootprintSize.Equals(FVector2D(7,6)) && Box->GetScaledBoxExtent().Equals(FVector(350,300,200)));
		Test.TestTrue(TEXT("Authored transform survives initial completion"),Art->GetRelativeTransform().Equals(Definition.PresentationTransform));
		UStaticMesh* Mesh=Art->GetStaticMesh();
		if (!Mesh) return true;
		for (int32 I=0;I<Mesh->GetStaticMaterials().Num();++I)
			Test.TestEqual(TEXT("Real PBR material slot preserved"),Art->GetMaterial(I),Mesh->GetMaterial(I));
		const auto* Registry=World->GetGameInstance()->GetSubsystem<URomanContentRegistrySubsystem>();
		const auto* Unit=Registry?Registry->FindUnit(TEXT("Legionary")):nullptr;
		if (!Test.TestNotNull(TEXT("Canonical Legionary definition"),Unit)) return true;
		const FFactionResourceAmounts Before=State->GetFactionResources(EFactionID::Rome);
		TSet<FObjectKey> ExistingCharacters;
		for (TActorIterator<ACharacter> It(World);It;++It) ExistingCharacters.Add(FObjectKey(*It));
		if (!Test.TestTrue(TEXT("Real training accepted"),FCall(Producer,TEXT("TrainUnit")).Name(TEXT("UnitID"),TEXT("Legionary")).Bool())) return true;
		Producer->Tick(Unit->Definition.TrainingTime*1.01f);
		ACharacter* Recruit=nullptr;
		int32 Recruits=0;
		for (TActorIterator<ACharacter> It(World);It;++It)
			if (!ExistingCharacters.Contains(FObjectKey(*It))) { Recruit=*It;++Recruits; }
		Test.TestEqual(TEXT("Exactly one recruit, no forced overlapping/ghost unit"),Recruits,1);
		if (!Test.TestNotNull(TEXT("Actual trained character"),Recruit)) return true;
		const auto* Capsule=Recruit->GetCapsuleComponent();
		const FVector Local=Producer->GetActorRotation().UnrotateVector(Recruit->GetActorLocation()-Producer->GetActorLocation());
		Test.TestTrue(TEXT("Capsule lies beyond approved footprint plus clearance"),Local.X>350+Capsule->GetScaledCapsuleRadius()+9);
		const FFactionResourceAmounts After=State->GetFactionResources(EFactionID::Rome);
		Test.TestEqual(TEXT("Training paid once through canonical wallet"),After.Gold,Before.Gold-Unit->Definition.TrainingCost.Gold);
		UNavigationPath* Path=UNavigationSystemV1::FindPathToLocationSynchronously(World,
			Recruit->GetActorLocation(),Recruit->GetActorLocation()+Producer->GetActorForwardVector()*350,Recruit);
		const FVector Feet=Recruit->GetNavAgentLocation();
		UNavigationPath* FeetPath=UNavigationSystemV1::FindPathToLocationSynchronously(World,
			Feet,Feet+Producer->GetActorForwardVector()*350,Recruit);
		Test.TestTrue(TEXT("Actual recruit has complete onward navigation path"),Path && Path->IsValid() && !Path->IsPartial() && Path->PathPoints.Num()>1);
		auto Receipt=MakeShared<FJsonObject>();
		Receipt->SetStringField(TEXT("mesh"),Mesh->GetPathName());
		Receipt->SetStringField(TEXT("recruit"),Recruit->GetPathName());
		Receipt->SetStringField(TEXT("recruit_owner"),GetPathNameSafe(Recruit->GetOwner()));
		Receipt->SetNumberField(TEXT("recruit_local_x_cm"),Local.X);
		Receipt->SetNumberField(TEXT("capsule_radius_cm"),Capsule->GetScaledCapsuleRadius());
		Receipt->SetNumberField(TEXT("path_points"),Path?Path->PathPoints.Num():0);
		Receipt->SetBoolField(TEXT("path_valid"),Path && Path->IsValid());
		Receipt->SetBoolField(TEXT("path_partial"),Path && Path->IsPartial());
		Receipt->SetStringField(TEXT("requested_center_start"),Recruit->GetActorLocation().ToString());
		Receipt->SetStringField(TEXT("requested_feet_start"),Feet.ToString());
		Receipt->SetStringField(TEXT("requested_direction"),Producer->GetActorForwardVector().ToString());
		if(Path && !Path->PathPoints.IsEmpty()) Receipt->SetStringField(TEXT("path_last"),Path->PathPoints.Last().ToString());
		Receipt->SetBoolField(TEXT("feet_path_valid"),FeetPath && FeetPath->IsValid());
		Receipt->SetBoolField(TEXT("feet_path_partial"),FeetPath && FeetPath->IsPartial());
		if(FeetPath && !FeetPath->PathPoints.IsEmpty()) Receipt->SetStringField(TEXT("feet_path_last"),FeetPath->PathPoints.Last().ToString());
		if (Test.HasAnyErrors())
		{
			Receipt->SetBoolField(TEXT("passed"),false);Report(Test,TEXT("producer.json"),Receipt);return true;
		}
		MoveStart=Feet;MoveTarget=Feet+Producer->GetActorForwardVector()*350;
		MovingRecruit=Recruit;MovementReceipt=Receipt;bMoveRequested=true;MoveStarted=FPlatformTime::Seconds();
		FCall(Recruit,TEXT("CommandMoveTo")).Vector(TEXT("Destination"),MoveTarget).Run();
		return false;
	}
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampBuildingAuthor,"RomanEmpireGame.ArtProbe.CampBuilding.Author",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FCampBuildingAuthor::RunTest(const FString& Parameters)
{
	using namespace RomanCampBuildingIntegration;
	if (!FParse::Param(FCommandLine::Get(),TEXT("RomanCampFunctionalAuthor"))) {AddInfo(TEXT("Authoring requires explicit opt-in and receipt"));return true;}
	if (!Authorized(*this)) return false;
	UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,MeshPath);
	auto* Asset=LoadObject<URomanBuildingDefinition>(nullptr,DefinitionPath);
	if (!TestNotNull(TEXT("Previously verified real Camp03 mesh"),Mesh) || !TestNotNull(TEXT("Actual private definition"),Asset)) return false;
	if (!TestTrue(TEXT("Only pristine 4x4 definition may be adopted once"),!Asset->Definition.PresentationMesh && Asset->Definition.FootprintSize.Equals(FVector2D(4,4)))) return false;
	const FBox Bounds=Mesh->GetBoundingBox();
	const FVector Translation(-Bounds.GetCenter().X,-Bounds.GetCenter().Y,-200-Bounds.Min.Z);
	Asset->Definition.PresentationMesh=Mesh;
	Asset->Definition.FootprintSize=FVector2D(7,6);
	Asset->Definition.PresentationTransform=FTransform(Translation);
	FDataValidationContext Context;
	if (!TestTrue(TEXT("Adopted definition passes same runtime and editor policy"),
		RomanEmpireBuildingPresentation::Validate(Asset->Definition)==RomanEmpireBuildingPresentation::EFailure::None &&
		Asset->IsDataValid(Context)==EDataValidationResult::Valid)) return false;
	UPackage* Package=Asset->GetOutermost();
	Asset->MarkPackageDirty();
	FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
	if (!TestTrue(TEXT("Private adopted definition saved"),UPackage::SavePackage(Package,Asset,
		*FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension()),Args))) return false;
	auto Receipt=MakeShared<FJsonObject>();
	Receipt->SetBoolField(TEXT("passed"),true);
	Receipt->SetStringField(TEXT("definition"),Asset->GetPathName());
	Receipt->SetStringField(TEXT("mesh"),Mesh->GetPathName());
	Receipt->SetStringField(TEXT("translation_cm"),Translation.ToString());
	Receipt->SetStringField(TEXT("mesh_bounds_cm"),Bounds.ToString());
	Receipt->SetNumberField(TEXT("material_slots"),Mesh->GetStaticMaterials().Num());
	return Report(*this,TEXT("author.json"),Receipt);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCampBuildingProducer,"RomanEmpireGame.ArtProbe.CampBuilding.ProducerPIE",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::ProductFilter)
bool FCampBuildingProducer::RunTest(const FString& Parameters)
{
	using namespace RomanCampBuildingIntegration;
	if (!FParse::Param(FCommandLine::Get(),TEXT("RomanCampFunctionalProbe"))) {AddInfo(TEXT("Functional probe requires explicit opt-in and receipt"));return true;}
	if (!Authorized(*this) || !TestNotNull(TEXT("Editor"),GEditor) || !TestNull(TEXT("Own PIE"),GEditor->PlayWorld)) return false;
	FAutomationEditorCommonUtils::LoadMap(TEXT("/Game/Maps/L_VerticalSlice"));
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckProducer(*this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
