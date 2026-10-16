// Copyright Roman Empire Game. All Rights Reserved.
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/RomanTextScaleProjection.h"
#include "RomanSettingsWidget.generated.h"
class USpinBox;
class UCheckBox;
class UTextBlock;
class UButton;
class URomanPlayerSettingsSubsystem;
class UEnhancedInputUserSettings;
class UVerticalBox;
class IInputProcessor;

/** Detached form draft. Controller owns input/focus/pause; GI subsystem alone commits preferences. */
UCLASS()
class ROMANEMPIREGAME_API URomanSettingsWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UWidget* GetInitialFocus() const;
	/** Native binding buttons route their stable row ID, never a cached key value. */
	void BeginBindingCapture(FName Name,UButton* Button);
protected:
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
	virtual FReply NativeOnKeyUp(const FGeometry& Geometry,const FKeyEvent& Event) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& Geometry,const FPointerEvent& Event) override;
private:
	UPROPERTY(Transient) TObjectPtr<USpinBox> Master;
	UPROPERTY(Transient) TObjectPtr<USpinBox> Effects;
	UPROPERTY(Transient) TObjectPtr<USpinBox> Look;
	UPROPERTY(Transient) TObjectPtr<USpinBox> Pan;
	UPROPERTY(Transient) TObjectPtr<USpinBox> TextScale;
	UPROPERTY(Transient) TObjectPtr<UCheckBox> Invert;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Status;
	UPROPERTY(Transient) TObjectPtr<UButton> Resume;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> BindingsBox;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> BindingButtons;
	TWeakObjectPtr<UEnhancedInputUserSettings> InputSettings;
	TSharedPtr<IInputProcessor> ReleaseObserver;
	FName CapturingBinding;
	TWeakObjectPtr<UButton> CaptureButton;
	TWeakObjectPtr<URomanPlayerSettingsSubsystem> Source;
	FDelegateHandle ChangedHandle;
	FRomanTextScaleProjection Typography;
	void RefreshTypography();
	void LoadDraft();
	void BuildBindings();
	void FinishBindingCapture(FKey Key);
	void CancelBindingCapture();
	UFUNCTION() void RefreshBindings(UEnhancedInputUserSettings* Settings);
	UFUNCTION() void SaveBindingsClicked();
	UFUNCTION() void DefaultsBindingsClicked();
	UFUNCTION() void ApplyClicked();
	UFUNCTION() void DefaultsClicked();
	UFUNCTION() void ResumeClicked();
};
