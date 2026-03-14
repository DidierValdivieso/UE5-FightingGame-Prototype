#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "../Structs/CombatTestStructs.h"
#include "CombatTestSubsystem.generated.h"

class AMainGameMode;
class AIO89Character;
class AAggressiveAIController;
class ADefensiveAIController;
class AIdleAIController;
class ASpectatorPlayerController;
class AFightingCameraActor;
class ATestCameraController;
class ATestCameraActor;
class UHUDWidget;
class UPauseWidget;
class AMainGameState;
class ALevelManagerActor;
class APlayerController;

UCLASS()
class ELDERBREACHBRAWLERS_API UCombatTestSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UCombatTestSubsystem();

    void StartTest(const FCombatTestConfig& Config);
    void StopTest();

    /** Muestra el menú de pausa y fuerza SetPause. Solo tiene efecto si la pelea está en curso (bTestRunning). Si ya está abierto, lo cierra (misma acción que Close). */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ShowPauseMenu(APlayerController* PC);

    /** Cierra el menú de pausa y reanuda la pelea (misma lógica que el botón Cerrar). */
    UFUNCTION(BlueprintCallable, Category = "Combat")
    void ClosePauseMenu(APlayerController* PC);

    /** Llamar cuando el PauseWidget se cierra para no mantener referencia huérfana. */
    void ClearPauseWidgetRef();

    /** Llamado cuando el escenario en streaming está listo; spawnea personajes y arranca la pelea. */
    UFUNCTION()
    void OnStageLoadedForCombat(FName LoadedStageName);

    /** Actualiza las referencias de personajes en el HUD (p. ej. tras respawn round 2). */
    void UpdateHUDCharacters(AIO89Character* CharA, AIO89Character* CharB);

    /** Spawnea un controller del tipo indicado (para respawn round 2, como primer spawn). */
    AController* SpawnControllerForType(ECombatControllerType Type);

    UPROPERTY()
    bool bTestRunning = false;

    UPROPERTY()
    FCombatTestConfig ActiveConfig;

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    void SpawnParticipants(const FCombatTestConfig& Config);
    void TrySetupFight();
    void SetupFightCamera(AIO89Character* CharA, AIO89Character* CharB);

    void InitializeHUD();

    UPROPERTY()
    UHUDWidget* HUDWidgetRef;

    UPROPERTY()
    UPauseWidget* PauseWidgetRef;

    UPROPERTY(EditAnywhere)
    TSubclassOf<UHUDWidget> HUDWidgetClass;

    UPROPERTY()
    FCombatTestConfig CurrentConfig;

    FTimerHandle TestTimerHandle;
    FTimerHandle StageLoadedFallbackTimerHandle;
    bool bWaitingForStageToLoad = false;

    void TickTest();
    void StartCombat();
    /** Spawnea personajes, HUD y arranca precountdown (solo cuando el escenario ya está cargado). */
    void FinishStartTestAfterStageLoaded();
    /** Fallback si el delegado OnStageLoaded no llega (ej. mapa completo con OpenLevel). */
    void OnStageLoadedFallbackFired();

private:
    UPROPERTY()
    TArray<AIO89Character*> SpawnedCharacters;

    UPROPERTY()
    AFightingCameraActor* FightCam = nullptr;

    UPROPERTY()
    ATestCameraController* TestCameraController = nullptr;

    UPROPERTY()
    ATestCameraActor* TestCameraActor = nullptr;
};