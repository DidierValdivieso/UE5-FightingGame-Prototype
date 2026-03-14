// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PauseWidget.generated.h"

class UButton;

/**
 * Widget de pausa durante la pelea: botones Cerrar, Reiniciar, Salir al menú.
 */
UCLASS()
class ELDERBREACHBRAWLERS_API UPauseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* CloseButton;

	UPROPERTY(meta = (BindWidget))
	UButton* RestartFightButton;

	UPROPERTY(meta = (BindWidget))
	UButton* ExitToFrontendButton;

	UFUNCTION()
	void OnCloseClicked();

	UFUNCTION()
	void OnRestartFightClicked();

	UFUNCTION()
	void OnExitToFrontendClicked();
};
