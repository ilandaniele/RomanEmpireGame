// Copyright Roman Empire Game. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Core/GameplayMode.h"
#include "EnhancedActionKeyMapping.h"
#include "Engine/AssetManager.h"
#include "Input/RomanInputConfig.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Misc/DataValidation.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

namespace RomanInputConfigurationTests
{
	URomanInputConfig* MakeValidConfiguration()
	{
		// Transient editor-test data, never runtime mapping generation or saved asset mutation.
		URomanInputConfig* Config = NewObject<URomanInputConfig>(GetTransientPackage());
		const auto Action = [Config](const TCHAR* Name, EInputActionValueType Type)
		{
			UInputAction* Result = NewObject<UInputAction>(Config, Name);
			Result->ValueType = Type;
			return Result;
		};
		Config->CameraPan = Action(TEXT("CameraPan"), EInputActionValueType::Axis2D);
		Config->Zoom = Action(TEXT("Zoom"), EInputActionValueType::Axis1D);
		Config->Select = Action(TEXT("Select"), EInputActionValueType::Boolean);
		Config->Command = Action(TEXT("Command"), EInputActionValueType::Boolean);
		Config->EnterPossession = Action(TEXT("EnterPossession"), EInputActionValueType::Boolean);
		Config->SoldierMove = Action(TEXT("SoldierMove"), EInputActionValueType::Axis2D);
		Config->Look = Action(TEXT("Look"), EInputActionValueType::Axis2D);
		Config->Attack = Action(TEXT("Attack"), EInputActionValueType::Boolean);
		Config->Block = Action(TEXT("Block"), EInputActionValueType::Boolean);
		Config->ExitPossession = Action(TEXT("ExitPossession"), EInputActionValueType::Boolean);
		Config->BuildMenu = Action(TEXT("BuildMenu"), EInputActionValueType::Boolean);
		Config->EndTurn = Action(TEXT("EndTurn"), EInputActionValueType::Boolean);
		Config->Cancel = Action(TEXT("Cancel"), EInputActionValueType::Boolean);
		Config->StrategicContext = NewObject<UInputMappingContext>(Config, TEXT("Strategic"));
		Config->TacticalContext = NewObject<UInputMappingContext>(Config, TEXT("Tactical"));
		Config->PossessionContext = NewObject<UInputMappingContext>(Config, TEXT("Possession"));
		Config->SharedContext = NewObject<UInputMappingContext>(Config, TEXT("Shared"));
		for (UInputMappingContext* Context : { Config->StrategicContext.Get(), Config->TacticalContext.Get() })
		{
			Context->MapKey(Config->CameraPan, EKeys::W);
			Context->MapKey(Config->Zoom, EKeys::MouseWheelAxis);
			Context->MapKey(Config->EndTurn, EKeys::T);
		}
		Config->TacticalContext->MapKey(Config->Select, EKeys::LeftMouseButton);
		Config->TacticalContext->MapKey(Config->Command, EKeys::RightMouseButton);
		Config->TacticalContext->MapKey(Config->EnterPossession, EKeys::F);
		Config->TacticalContext->MapKey(Config->BuildMenu, EKeys::B);
		const FKey SlotKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
		for (int32 Index = 0; Index < URomanInputConfig::BuildingSlotCount; ++Index)
		{
			UInputAction* Slot = Action(*FString::Printf(TEXT("Slot%d"), Index + 1), EInputActionValueType::Boolean);
			Config->BuildingSlots.Add(Slot);
			Config->TacticalContext->MapKey(Slot, SlotKeys[Index]);
		}
		Config->PossessionContext->MapKey(Config->SoldierMove, EKeys::W);
		Config->PossessionContext->MapKey(Config->Look, EKeys::Mouse2D);
		Config->PossessionContext->MapKey(Config->Attack, EKeys::LeftMouseButton);
		Config->PossessionContext->MapKey(Config->Block, EKeys::RightMouseButton);
		Config->PossessionContext->MapKey(Config->ExitPossession, EKeys::F);
		Config->SharedContext->MapKey(Config->Cancel, EKeys::Escape);
		return Config;
	}

	void ExpectInvalid(FAutomationTestBase& Test, URomanInputConfig* Config, const TCHAR* Reason)
	{
		TArray<FText> Errors;
		Test.TestFalse(Reason, Config->ValidateConfiguration(Errors));
		Test.TestTrue(TEXT("Invalid configuration produces an actionable error"), Errors.Num() > 0);
		FDataValidationContext Context(false, EDataValidationUsecase::Manual, {});
		Test.TestEqual(TEXT("Editor validation also rejects the invalid configuration"),
			Config->IsDataValid(Context), EDataValidationResult::Invalid);
	}

	bool HasMapping(const UInputMappingContext* Context, const UInputAction* Action, const FKey& Key)
	{
		return Context && Context->GetMappings().ContainsByPredicate([Action, Key](const FEnhancedActionKeyMapping& Mapping)
		{
			return Mapping.Action == Action && Mapping.Key == Key;
		});
	}

	void CheckWASD(FAutomationTestBase& Test, const UInputMappingContext* Context, const UInputAction* Action)
	{
		Test.TestEqual(TEXT("Opposing keyboard directions accumulate and cancel"),
			Action->AccumulationBehavior, EInputActionAccumulationBehavior::Cumulative);
		const FKey Keys[] = { EKeys::W, EKeys::A, EKeys::S, EKeys::D };
		const FVector2D Directions[] = { FVector2D(0, 1), FVector2D(-1, 0), FVector2D(0, -1), FVector2D(1, 0) };
		FVector2D Sum = FVector2D::ZeroVector;
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Keys); ++Index)
		{
			const FEnhancedActionKeyMapping* Mapping = Context->GetMappings().FindByPredicate(
				[Action, &Keys, Index](const FEnhancedActionKeyMapping& Entry) { return Entry.Action == Action && Entry.Key == Keys[Index]; });
			if (!Test.TestNotNull(*FString::Printf(TEXT("%s maps %s"), *Context->GetName(), *Keys[Index].ToString()), Mapping)) continue;
			FInputActionValue Value(FVector2D(1.0, 0.0));
			for (const TObjectPtr<UInputModifier>& Modifier : Mapping->Modifiers)
			{
				if (Test.TestNotNull(TEXT("Keyboard modifier is a valid authored object"), Modifier.Get()))
				{
					Value = Modifier->ModifyRaw(nullptr, Value, 1.0f / 60.0f);
				}
			}
			Test.TestTrue(TEXT("Authored modifiers produce the intended signed axis"), Value.Get<FVector2D>().Equals(Directions[Index]));
			Sum += Value.Get<FVector2D>();
		}
		Test.TestTrue(TEXT("W+A+S+D cancels to a zero vector"), Sum.IsNearlyZero());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanInputConfigurationValidationTest,
	"RomanEmpireGame.Input.ConfigurationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRomanInputConfigurationValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RomanInputConfigurationTests;
	URomanInputConfig* Config = MakeValidConfiguration();
	TArray<FText> Errors = { FText::FromString(TEXT("Existing caller error")) };
	TestTrue(TEXT("Complete transient configuration validates independently of previous errors"), Config->ValidateConfiguration(Errors));
	TestEqual(TEXT("Validation preserves caller-owned error entries"), Errors.Num(), 1);
	TestEqual(TEXT("Primary asset type is stable"), Config->GetPrimaryAssetId().PrimaryAssetType, URomanInputConfig::AssetType);
	TestEqual(TEXT("Strategic resolves only its context"), Config->GetContextForMode(EGameplayMode::Strategic), Config->StrategicContext.Get());
	TestEqual(TEXT("Tactical resolves only its context"), Config->GetContextForMode(EGameplayMode::Tactical), Config->TacticalContext.Get());
	TestEqual(TEXT("Possession resolves only its context"), Config->GetContextForMode(EGameplayMode::Possessed), Config->PossessionContext.Get());
	TestNull(TEXT("Unknown mode cannot select an input context"), Config->GetContextForMode(static_cast<EGameplayMode>(255)));

	Config->SharedContext->UnmapAllKeysFromAction(Config->Cancel);
	Config->SharedContext->MapKey(Config->Cancel, EKeys::BackSpace);
	Errors.Reset();
	TestTrue(TEXT("Logical contract permits a valid alternate physical key"), Config->ValidateConfiguration(Errors));

	Config = MakeValidConfiguration();
	Config->StrategicContext = nullptr;
	ExpectInvalid(*this, Config, TEXT("Missing context is rejected"));
	Config = MakeValidConfiguration();
	Config->SharedContext = Config->TacticalContext;
	ExpectInvalid(*this, Config, TEXT("Aliased contexts are rejected"));
	Config = MakeValidConfiguration();
	Config->Block = nullptr;
	ExpectInvalid(*this, Config, TEXT("Missing logical action is rejected"));
	Config = MakeValidConfiguration();
	Config->ExitPossession = Config->EnterPossession;
	ExpectInvalid(*this, Config, TEXT("Enter/exit possession must not share an action identity"));
	Config = MakeValidConfiguration();
	Config->CameraPan->ValueType = EInputActionValueType::Boolean;
	Config->Zoom->ValueType = EInputActionValueType::Axis2D;
	Config->Attack->ValueType = EInputActionValueType::Axis1D;
	ExpectInvalid(*this, Config, TEXT("Incompatible Boolean/1D/2D action types are rejected"));
	Config = MakeValidConfiguration();
	Config->BuildingSlots.Pop();
	ExpectInvalid(*this, Config, TEXT("Fewer than six building slots are rejected"));
	Config = MakeValidConfiguration();
	Config->BuildingSlots.Add(NewObject<UInputAction>(Config));
	ExpectInvalid(*this, Config, TEXT("More than six building slots are rejected"));
	Config = MakeValidConfiguration();
	Config->BuildingSlots[1] = Config->BuildingSlots[0];
	ExpectInvalid(*this, Config, TEXT("Duplicate quick-slot identities are rejected"));
	Config = MakeValidConfiguration();
	Config->BuildingSlots[0] = Config->BuildMenu;
	ExpectInvalid(*this, Config, TEXT("Quick slot cannot alias a different logical command"));
	Config = MakeValidConfiguration();
	Config->BuildingSlots[0] = nullptr;
	ExpectInvalid(*this, Config, TEXT("Null quick slot is rejected"));
	Config = MakeValidConfiguration();
	Config->BuildingSlots[0]->ValueType = EInputActionValueType::Axis3D;
	ExpectInvalid(*this, Config, TEXT("Non-Boolean quick slot is rejected"));
	Config = MakeValidConfiguration();
	Config->PossessionContext->UnmapAllKeysFromAction(Config->Block);
	ExpectInvalid(*this, Config, TEXT("Every required action needs a usable mapping"));
	Config = MakeValidConfiguration();
	Config->PossessionContext->MapKey(Config->EndTurn, EKeys::T);
	ExpectInvalid(*this, Config, TEXT("Possession cannot gain campaign-turn actions"));
	Config = MakeValidConfiguration();
	Config->StrategicContext->MapKey(Config->BuildMenu, EKeys::B);
	ExpectInvalid(*this, Config, TEXT("Strategic cannot gain tactical building actions"));
	Config = MakeValidConfiguration();
	Config->SharedContext->MapKey(Config->Attack, EKeys::LeftMouseButton);
	ExpectInvalid(*this, Config, TEXT("Shared context cannot bypass per-mode command isolation"));
	Config = MakeValidConfiguration();
	Config->SharedContext->GetMapping(0).Action = nullptr;
	ExpectInvalid(*this, Config, TEXT("Unassigned mapping action is rejected"));
	Config = MakeValidConfiguration();
	Config->SharedContext->GetMapping(0).Key = FKey();
	ExpectInvalid(*this, Config, TEXT("Invalid physical key does not satisfy a required action"));

	Config = MakeValidConfiguration();
	FMapProperty* ProfilesProperty = FindFProperty<FMapProperty>(UInputMappingContext::StaticClass(), TEXT("MappingProfileOverrides"));
	FStructProperty* ProfileValue = ProfilesProperty ? CastField<FStructProperty>(ProfilesProperty->ValueProp) : nullptr;
	if (TestTrue(TEXT("Engine profile-override storage has the expected typed contract"), ProfilesProperty &&
		CastField<FStrProperty>(ProfilesProperty->KeyProp) && ProfileValue && ProfileValue->Struct == FInputMappingContextMappingData::StaticStruct()))
	{
		auto* Profiles = ProfilesProperty->ContainerPtrToValuePtr<TMap<FString, FInputMappingContextMappingData>>(Config->SharedContext.Get());
		FInputMappingContextMappingData Override;
		Override.Mappings.Emplace(Config->Cancel, EKeys::BackSpace);
		Profiles->Add(TEXT("Alternate"), Override);
		Errors.Reset();
		TestTrue(TEXT("Valid authored remapping profile preserves the action contract"), Config->ValidateConfiguration(Errors));
		Profiles->FindChecked(TEXT("Alternate")).Mappings.Reset();
		ExpectInvalid(*this, Config, TEXT("Empty replacement profile cannot silently drop required commands"));
		Profiles->FindChecked(TEXT("Alternate")).Mappings.Emplace(Config->Attack, EKeys::LeftMouseButton);
		ExpectInvalid(*this, Config, TEXT("Profile override cannot introduce a cross-mode command"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRomanAuthoredInputAssetsTest,
	"RomanEmpireGame.Input.AuthoredDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRomanAuthoredInputAssetsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace RomanInputConfigurationTests;
	URomanInputConfig* Config = LoadObject<URomanInputConfig>(nullptr, TEXT("/Game/Input/DA_RomanInputConfig.DA_RomanInputConfig"));
	if (!TestNotNull(TEXT("Committed input configuration loads from its canonical asset path"), Config)) return false;
	TArray<FText> Errors;
	if (!TestTrue(TEXT("Committed input configuration passes the complete logical contract"), Config->ValidateConfiguration(Errors)))
	{
		for (const FText& Error : Errors) AddError(Error.ToString());
		return false;
	}
	TestEqual(TEXT("Asset Manager discovers the cookable input configuration"),
		UAssetManager::Get().GetPrimaryAssetPath(Config->GetPrimaryAssetId()), FSoftObjectPath(Config));
	TestEqual(TEXT("Strategic has WASD, zoom, turn and nothing else"), Config->StrategicContext->GetMappings().Num(), 6);
	TestEqual(TEXT("Tactical has WASD, zoom, selection, command, possession, building menu, turn, and six slots"), Config->TacticalContext->GetMappings().Num(), 16);
	TestEqual(TEXT("Possession has WASD, mouse look, attack, block, and exit"), Config->PossessionContext->GetMappings().Num(), 8);
	TestEqual(TEXT("Shared contains only cancellation"), Config->SharedContext->GetMappings().Num(), 1);
	CheckWASD(*this, Config->StrategicContext, Config->CameraPan);
	CheckWASD(*this, Config->TacticalContext, Config->CameraPan);
	CheckWASD(*this, Config->PossessionContext, Config->SoldierMove);
	for (UInputMappingContext* Context : { Config->StrategicContext.Get(), Config->TacticalContext.Get() })
	{
		TestTrue(TEXT("Exploration zoom uses mouse wheel"), HasMapping(Context, Config->Zoom, EKeys::MouseWheelAxis));
		TestTrue(TEXT("Exploration turn uses T"), HasMapping(Context, Config->EndTurn, EKeys::T));
	}
	TestTrue(TEXT("Tactical LMB selects"), HasMapping(Config->TacticalContext, Config->Select, EKeys::LeftMouseButton));
	TestTrue(TEXT("Tactical RMB commands"), HasMapping(Config->TacticalContext, Config->Command, EKeys::RightMouseButton));
	TestTrue(TEXT("Tactical F enters possession"), HasMapping(Config->TacticalContext, Config->EnterPossession, EKeys::F));
	TestTrue(TEXT("Tactical B opens construction"), HasMapping(Config->TacticalContext, Config->BuildMenu, EKeys::B));
	TestTrue(TEXT("Possession mouse delta drives look"), HasMapping(Config->PossessionContext, Config->Look, EKeys::Mouse2D));
	TestTrue(TEXT("Possession LMB attacks"), HasMapping(Config->PossessionContext, Config->Attack, EKeys::LeftMouseButton));
	TestTrue(TEXT("Possession RMB blocks"), HasMapping(Config->PossessionContext, Config->Block, EKeys::RightMouseButton));
	TestTrue(TEXT("Possession F exits using a separate action"), HasMapping(Config->PossessionContext, Config->ExitPossession, EKeys::F));
	TestTrue(TEXT("Escape is common cancellation"), HasMapping(Config->SharedContext, Config->Cancel, EKeys::Escape));
	const FKey SlotKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six };
	for (int32 Index = 0; Index < URomanInputConfig::BuildingSlotCount; ++Index)
	{
		TestTrue(TEXT("Building slots preserve ordered 1-6 defaults"), HasMapping(Config->TacticalContext, Config->BuildingSlots[Index], SlotKeys[Index]));
	}
	return true;
}

#endif
