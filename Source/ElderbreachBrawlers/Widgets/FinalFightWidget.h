// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FinalFightWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 
 */
UCLASS()
class ELDERBREACHBRAWLERS_API UFinalFightWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	// Text blocks for displaying fight data
	UPROPERTY(meta = (BindWidget))
	UTextBlock* WinnerText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoundOneDurationText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* RoundTwoDurationText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerOneRoundOneDamageText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerOneRoundTwoDamageText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerTwoRoundOneDamageText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerTwoRoundTwoDamageText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerOneRoundsWonText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* PlayerTwoRoundsWonText;

	UPROPERTY(meta = (BindWidget))
	UButton* MainMenuButton;

	UPROPERTY(meta = (BindWidget))
	UButton* RestartFightButton;

	UFUNCTION()
	void OnMainMenuButtonClicked();

	UFUNCTION()
	void OnRestartFightButtonClicked();

private:
	void UpdateFightData();
};
