#include "WidgetSequenceManager.h"
#include "../Widgets/FrontendWidget.h"
#include "../Widgets/StageSelectWidget.h"
#include "../Widgets/ControllerSelectWidget.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "../Subsystems/CombatTestSubsystem.h"
#include "../Subsystems/AudioManagerSubsystem.h"
#include "../MainGameMode.h"
#include "../Actors/LevelManagerActor.h"

void UWidgetSequenceManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ResetSession();
    VerifyRequiredWidgetsLoaded();
}

void UWidgetSequenceManager::VerifyRequiredWidgetsLoaded()
{
    UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] ===== Verificando widgets para el .exe ====="));
    int32 Ok = 0, Missing = 0;

    if (LoadClass<UFrontendWidget>(nullptr, TEXT("/Game/Blueprints/Widgets/WBP_FrontendWidget.WBP_FrontendWidget_C")))
    { UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager]   [OK] WBP_FrontendWidget")); Ok++; }
    else
    { UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager]   [MISSING] WBP_FrontendWidget")); Missing++; }

    if (LoadClass<UStageSelectWidget>(nullptr, TEXT("/Game/Blueprints/Widgets/WBP_StageSelectWidget.WBP_StageSelectWidget_C")))
    { UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager]   [OK] WBP_StageSelectWidget")); Ok++; }
    else
    { UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager]   [MISSING] WBP_StageSelectWidget")); Missing++; }

    if (LoadClass<UControllerSelectWidget>(nullptr, TEXT("/Game/Blueprints/Widgets/WBP_ControllerSelectWidget.WBP_ControllerSelectWidget_C")))
    { UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager]   [OK] WBP_ControllerSelectWidget")); Ok++; }
    else
    { UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager]   [MISSING] WBP_ControllerSelectWidget")); Missing++; }

    if (LoadClass<UUserWidget>(nullptr, TEXT("/Game/Blueprints/Widgets/WBP_FinalFightWidget.WBP_FinalFightWidget_C")))
    { UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager]   [OK] WBP_FinalFightWidget")); Ok++; }
    else
    { UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager]   [MISSING] WBP_FinalFightWidget")); Missing++; }

    if (LoadClass<UUserWidget>(nullptr, TEXT("/Game/Blueprints/Widgets/WBP_HUDWidget.WBP_HUDWidget_C")))
    { UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager]   [OK] WBP_HUDWidget")); Ok++; }
    else
    { UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager]   [MISSING] WBP_HUDWidget")); Missing++; }

    UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] Total: %d OK, %d MISSING. Si hay MISSING, incluye los widgets al empaquetar (ver MAPA_MENU_LEEME)."), Ok, Missing);
}

void UWidgetSequenceManager::Deinitialize()
{
    CleanupCurrentWidget();
    Super::Deinitialize();
}

void UWidgetSequenceManager::StartWidgetSequence(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("[WidgetSequenceManager] Invalid PlayerController"));
        return;
    }

    if (IsFrontendWidgetVisible())
    {
        UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] StartWidgetSequence skipped - Frontend already visible."));
        return;
    }

    CachedPlayerController = PlayerController;
    ResetSession();

    if (TSubclassOf<UFrontendWidget> FrontendWidgetClass = LoadClass<UFrontendWidget>(
        nullptr, TEXT("/Game/Blueprints/Widgets/WBP_FrontendWidget.WBP_FrontendWidget_C")))
    {
        FrontendWidgetRef = CreateWidget<UFrontendWidget>(PlayerController, FrontendWidgetClass);
        if (FrontendWidgetRef)
        {
            FrontendWidgetRef->AddToViewport(999);
            PlayerController->bShowMouseCursor = true;
            PlayerController->SetInputMode(FInputModeUIOnly());
            if (UAudioManagerSubsystem* AudioMgr = GetGameInstance()->GetSubsystem<UAudioManagerSubsystem>())
            {
                AudioMgr->PlayUIMusic();
            }
            UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] Menu (Frontend) shown - select stage next."));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager] Could not load FrontendWidget class - check /Game/Blueprints/Widgets/WBP_FrontendWidget exists and is cooked."));
    }
}

void UWidgetSequenceManager::ShowStageSelection()
{
    if (!CachedPlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("[WidgetSequenceManager] No cached PlayerController"));
        return;
    }

    CleanupCurrentWidget();

    if (TSubclassOf<UStageSelectWidget> StageSelectWidgetClass = LoadClass<UStageSelectWidget>(
        nullptr, TEXT("/Game/Blueprints/Widgets/WBP_StageSelectWidget.WBP_StageSelectWidget_C")))
    {
        StageSelectWidgetRef = CreateWidget<UStageSelectWidget>(CachedPlayerController, StageSelectWidgetClass);
        if (StageSelectWidgetRef)
        {
            StageSelectWidgetRef->AddToViewport(999);
            CachedPlayerController->bShowMouseCursor = true;
            CachedPlayerController->SetInputMode(FInputModeUIOnly());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager] Could not load StageSelectWidget class"));
    }
}

void UWidgetSequenceManager::ShowCombatConfig()
{
    if (!CachedPlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("[WidgetSequenceManager] No cached PlayerController"));
        return;
    }

    CleanupCurrentWidget();

    if (TSubclassOf<UControllerSelectWidget> CombatConfigWidgetClass = LoadClass<UControllerSelectWidget>(
        nullptr, TEXT("/Game/Blueprints/Widgets/WBP_ControllerSelectWidget.WBP_ControllerSelectWidget_C")))
    {
        CombatConfigWidgetRef = CreateWidget<UControllerSelectWidget>(CachedPlayerController, CombatConfigWidgetClass);
        if (CombatConfigWidgetRef)
        {
            CombatConfigWidgetRef->AddToViewport(999);
            CachedPlayerController->bShowMouseCursor = true;
            CachedPlayerController->SetInputMode(FInputModeUIOnly());
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[WidgetSequenceManager] Could not load ControllerSelectWidget class"));
    }
}

void UWidgetSequenceManager::OnStageSelected(FName SelectedStage)
{
    CurrentSessionConfig.InitialMap = SelectedStage;
    UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] Stage selected: %s"), *SelectedStage.ToString());
    OnStageSelectedDelegate.Broadcast(SelectedStage);

    if (!CachedPlayerController) return;

    UWorld* World = CachedPlayerController->GetWorld();
    AMainGameMode* GM = World ? Cast<AMainGameMode>(UGameplayStatics::GetGameMode(World)) : nullptr;

    if (GM && GM->IsFrontendMap())
    {
        // Estamos en el menú: viajar al mapa arena ya; la pantalla de controller se mostrará al cargar
        bShowControllerConfigOnArenaLoad = true;
        const FName ArenaMapName = GM->ArenaHubMapName;
        UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] Loading arena map: %s (controller config will show when ready)"), *ArenaMapName.ToString());
        UGameplayStatics::OpenLevel(World, ArenaMapName);
        return;
    }

    // Ya estamos en la arena: cargar stage y mostrar controller config aquí
    if (GM && GM->LevelManager)
    {
        GM->LevelManager->LoadStage(SelectedStage);
    }
    ShowCombatConfig();
}

bool UWidgetSequenceManager::IsFrontendWidgetVisible() const
{
    return FrontendWidgetRef != nullptr && FrontendWidgetRef->IsInViewport();
}

void UWidgetSequenceManager::ShowControllerConfigOnArena(APlayerController* ArenaPC)
{
    if (!ArenaPC)
    {
        bShowControllerConfigOnArenaLoad = false;
        return;
    }
    CachedPlayerController = ArenaPC;
    bShowControllerConfigOnArenaLoad = false;
    ShowCombatConfig();
}

void UWidgetSequenceManager::OnCombatConfigComplete(const FCombatTestConfig& Config)
{
    CurrentSessionConfig.Participants = Config.Participants;
    CurrentSessionConfig.bUseSpectator = Config.bUseSpectator;
    CurrentSessionConfig.MaxTestDuration = Config.MaxTestDuration;
    CurrentSessionConfig.bDebugMode = Config.bDebugMode;

    UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] Combat config complete"));
    OnCombatConfigCompleteDelegate.Broadcast(CurrentSessionConfig);
    
    FinalizeCombatSession();
}

void UWidgetSequenceManager::FinalizeCombatSession()
{
    CleanupCurrentWidget();

    if (!CachedPlayerController)
    {
        UE_LOG(LogTemp, Error, TEXT("[WidgetSequenceManager] No cached PlayerController"));
        return;
    }

    CachedPlayerController->bShowMouseCursor = false;
    CachedPlayerController->SetInputMode(FInputModeGameOnly());

    UWorld* World = CachedPlayerController->GetWorld();
    if (!World) return;

    AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(World));

    // Si estamos en el mapa de menú, guardar config y viajar al mapa arena; el combate se inicia al cargar ese mapa
    if (GM && GM->IsFrontendMap())
    {
        PendingCombatConfig = CurrentSessionConfig;
        bPendingCombatFromMenu = true;
        const FName ArenaMapName = GM->ArenaHubMapName;
        UE_LOG(LogTemp, Log, TEXT("[WidgetSequenceManager] Traveling to arena map: %s"), *ArenaMapName.ToString());
        UGameplayStatics::OpenLevel(World, ArenaMapName);
        return;
    }

    // Ya estamos en la arena: iniciar combate aquí
    if (UCombatTestSubsystem* Subsystem = World->GetSubsystem<UCombatTestSubsystem>())
    {
        OnCombatSessionReadyDelegate.Broadcast(CurrentSessionConfig);
        Subsystem->StartTest(CurrentSessionConfig);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[WidgetSequenceManager] CombatTestSubsystem not found"));
    }
}

void UWidgetSequenceManager::ResetSession()
{
    CurrentSessionConfig = FCombatTestConfig();
    CurrentSessionConfig.bDebugMode = true;
    CurrentSessionConfig.bUseSpectator = true;
    CurrentSessionConfig.MaxTestDuration = 60.f;
}

void UWidgetSequenceManager::CleanupCurrentWidget()
{
    if (FrontendWidgetRef && FrontendWidgetRef->IsInViewport())
    {
        FrontendWidgetRef->RemoveFromParent();
        FrontendWidgetRef = nullptr;
    }

    if (StageSelectWidgetRef && StageSelectWidgetRef->IsInViewport())
    {
        StageSelectWidgetRef->RemoveFromParent();
        StageSelectWidgetRef = nullptr;
    }

    if (CombatConfigWidgetRef && CombatConfigWidgetRef->IsInViewport())
    {
        CombatConfigWidgetRef->RemoveFromParent();
        CombatConfigWidgetRef = nullptr;
    }
}

