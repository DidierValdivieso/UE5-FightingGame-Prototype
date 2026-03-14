#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Structs/CombatTestStructs.h"
#include "WidgetSequenceManager.generated.h"

class UFrontendWidget;
class UStageSelectWidget;
class UControllerSelectWidget;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageSelected, FName, SelectedStage);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatConfigComplete, const FCombatTestConfig&, Config);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatSessionReady, const FCombatTestConfig&, FinalConfig);

UCLASS()
class ELDERBREACHBRAWLERS_API UWidgetSequenceManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Widget Sequence")
    void StartWidgetSequence(APlayerController* PlayerController);

    UFUNCTION(BlueprintCallable, Category = "Widget Sequence")
    void ShowStageSelection();

    UFUNCTION(BlueprintCallable, Category = "Widget Sequence")
    void ShowCombatConfig();

    UFUNCTION(BlueprintCallable, Category = "Widget Sequence")
    void FinalizeCombatSession();

    void OnStageSelected(FName SelectedStage);
    void OnCombatConfigComplete(const FCombatTestConfig& Config);

    UPROPERTY(BlueprintAssignable, Category = "Widget Sequence")
    FOnStageSelected OnStageSelectedDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Widget Sequence")
    FOnCombatConfigComplete OnCombatConfigCompleteDelegate;

    UPROPERTY(BlueprintAssignable, Category = "Widget Sequence")
    FOnCombatSessionReady OnCombatSessionReadyDelegate;

    UFUNCTION(BlueprintPure, Category = "Widget Sequence")
    FCombatTestConfig GetCurrentConfig() const { return CurrentSessionConfig; }

    UFUNCTION(BlueprintCallable, Category = "Widget Sequence")
    void ResetSession();

    /** Si hay una pelea pendiente por haber confirmado desde el menú (viaje al mapa arena). */
    bool HasPendingCombatFromMenu() const { return bPendingCombatFromMenu; }
    FCombatTestConfig GetPendingCombatConfig() const { return PendingCombatConfig; }
    void ClearPendingCombatFromMenu() { bPendingCombatFromMenu = false; }

    /** True si el widget de menú principal está visible (para fallback de inicio). */
    bool IsFrontendWidgetVisible() const;

    /** Tras elegir etapa en el menú: al cargar la arena hay que mostrar la pantalla de controller. */
    bool ShouldShowControllerConfigOnArenaLoad() const { return bShowControllerConfigOnArenaLoad; }
    /** Llamar cuando la arena ya cargó: muestra la UI de controller y usa el PC del mapa arena. */
    void ShowControllerConfigOnArena(APlayerController* ArenaPC);

private:
    UPROPERTY()
    FCombatTestConfig CurrentSessionConfig;

    UPROPERTY()
    UFrontendWidget* FrontendWidgetRef;

    UPROPERTY()
    UStageSelectWidget* StageSelectWidgetRef;

    UPROPERTY()
    UControllerSelectWidget* CombatConfigWidgetRef;

    UPROPERTY()
    APlayerController* CachedPlayerController;

    bool bPendingCombatFromMenu = false;
    FCombatTestConfig PendingCombatConfig;

    /** True si acabamos de viajar al mapa arena tras elegir etapa (mostrar controller config al cargar). */
    bool bShowControllerConfigOnArenaLoad = false;

    void CleanupCurrentWidget();
    /** Al arrancar, comprueba que todos los widgets del menú estén cocinados (se escribe en log). */
    void VerifyRequiredWidgetsLoaded();
};





