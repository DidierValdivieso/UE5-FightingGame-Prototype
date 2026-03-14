// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "SpectatorPlayerController.generated.h"

class UHUDWidget;
class AMainGameState;
class UInputAction;
class UInputMappingContext;

UCLASS()
class ELDERBREACHBRAWLERS_API ASpectatorPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
    ASpectatorPlayerController();

    /** Llama el CombatTestSubsystem al iniciar IA vs IA para asegurar que F (toggle cajas) esté enlazada aunque el BP no llame Parent. */
    void EnsureCollisionToggleBinding();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputAction* ToggleCollisionAction;

    UPROPERTY(EditAnywhere, Category = "Input")
    UInputMappingContext* DefaultMappingContext;

    void OnToggleCollisionVisibility(const struct FInputActionValue& Value);
    void OnPausePressed(const struct FInputActionValue& Value);

private:
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<UHUDWidget> HUDWidgetClass;

    UPROPERTY()
    UHUDWidget* HUDWidgetRef;

    FTimerHandle InitHUDTimerHandle;
    /** Evita enlazar F varias veces (cada bind duplicaba el callback y la tecla hacía doble toggle). */
    bool bCollisionToggleBound = false;

    void InitHUD();
};
