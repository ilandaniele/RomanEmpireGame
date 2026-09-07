// Copyright Roman Empire Game. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RomanEmpireHUD.generated.h"

class ARomanEmpireGameMode;

/** Optional Canvas diagnostics only; all production game UI lives in UMG. */
UCLASS()
class ROMANEMPIREGAME_API ARomanEmpireHUD : public AHUD
{
	GENERATED_BODY()

public:
	ARomanEmpireHUD();

	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	UFUNCTION(BlueprintCallable, Category = "HUD|Diagnostics")
	void SetDebugOverlayVisible(bool bVisible) { bShowDebugOverlay = bVisible; }

	UFUNCTION(BlueprintCallable, Category = "HUD|Diagnostics")
	void ToggleDebugOverlay() { bShowDebugOverlay = !bShowDebugOverlay; }

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Diagnostics")
	bool bShowDebugOverlay;

	UPROPERTY(Transient)
	TObjectPtr<ARomanEmpireGameMode> CachedGameMode;
};
