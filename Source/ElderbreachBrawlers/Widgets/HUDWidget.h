#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "HUDWidget.generated.h"

class AIO89Character;

UCLASS()
class ELDERBREACHBRAWLERS_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CountdownValue;

	/** Solo visible durante el pre-countdown al inicio de la pelea */
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* GetReadyText;

	/** Quién ganó el round al terminar (Round 1: Player One wins! / etc.) */
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* DecisionText;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	void SetCharacters(AIO89Character* InCharOne, AIO89Character* InCharTwo);

	UPROPERTY()
	AIO89Character* CharacterOne;

	UPROPERTY()
	AIO89Character* CharacterTwo;

protected:
	UPROPERTY(meta = (BindWidget))
	UProgressBar* LifeCharacterOneBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* LifeCharacterTwoBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* EnergyCharacterOneBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* EnergyCharacterTwoBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaCharacterOneBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaCharacterTwoBar;

private:
	bool bIsRunning = false;
	/** Tiempo en que se mostró "FIGHT!" para limitar a 0.3 s; -1 = no mostrando. */
	float FightTextShowStartTime = -1.f;
	/** True si acabamos de estar en PreCountdown 2 o 1 (para rellenar READY hasta FIGHT! en round 1 y 2, sin mostrar READY en la espera de 2 s). */
	bool bWasInPreCountdown = false;
};