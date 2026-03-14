#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include <ElderbreachBrawlers/Widgets/HUDWidget.h>
#include "ThirdCharacterPlayerController.generated.h"

class UInputAction;

UCLASS()
class ELDERBREACHBRAWLERS_API AThirdCharacterPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AThirdCharacterPlayerController();

	/** Llama CombatTestSubsystem al iniciar IA vs IA para enlazar F (toggle cajas) cuando este PC es espectador (sin pawn). */
	void EnsureCollisionToggleBinding();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* DefaultMappingContext = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleCollisionAction = nullptr;

	UFUNCTION()
	void TryAddMappingContext();

	void OnToggleCollisionVisibility(const struct FInputActionValue& Value);
	void OnPausePressed(const struct FInputActionValue& Value);

	virtual void Tick(float DeltaSeconds) override;

	FTimerHandle FightCamTimerHandle;

private:
	FTimerHandle RetryMappingTimerHandle;
	/** Evita enlazar F varias veces (cada bind duplicaba el callback y la tecla hacía doble toggle). */
	bool bCollisionToggleBound = false;

public:
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UHUDWidget* HUDWidgetRef;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UHUDWidget> HUDWidgetClass;
protected:
	FTimerHandle HUDInitTimerHandle;
};