#include "CombatTestSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"

#include "../MainGameMode.h"
#include "AudioManagerSubsystem.h"
#include "../MainGameState.h"
#include "../Controllers/AggressiveAIController.h"
#include "../Controllers/DefensiveAIController.h"
#include "../Controllers/IdleAIController.h"
#include "../Controllers/SpectatorPlayerController.h"
#include "../Controllers/TestCameraController.h"
#include "../Characters/IO89Character.h"
#include "../Actors/FightingCameraActor.h"
#include "../Actors/TestCameraActor.h"
#include "../Widgets/HUDWidget.h"
#include "../Widgets/PauseWidget.h"
#include "../Controllers/ThirdCharacterPlayerController.h"
#include "../Actors/LevelManagerActor.h"
#include "../Subsystems/CollisionDebugSubsystem.h"
#include "TimerManager.h"
#include "GenericPlatform/GenericPlatformProcess.h"

UCombatTestSubsystem::UCombatTestSubsystem()
{
    static ConstructorHelpers::FClassFinder<UUserWidget> HUDClassFinder(TEXT("/Game/Blueprints/Widgets/WBP_HUDWidget"));
    if (HUDClassFinder.Succeeded())
    {
        HUDWidgetClass = HUDClassFinder.Class;
    }
}

void UCombatTestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UCombatTestSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

void UCombatTestSubsystem::StartTest(const FCombatTestConfig& Config)
{
    UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] ===== Starting Combat Test ====="));
    ActiveConfig = Config;

    UE_LOG(LogTemp, Warning, TEXT("   - Debug Mode: %s"), Config.bDebugMode ? TEXT("TRUE") : TEXT("FALSE"));
    UE_LOG(LogTemp, Warning, TEXT("   - Max Duration: %.2f s"), Config.MaxTestDuration);
    UE_LOG(LogTemp, Warning, TEXT("   - Total Participants: %d"), Config.Participants.Num());

    if (!GetWorld()) return;

    CurrentConfig = Config;
    bTestRunning = true;

    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        GS->ResetTestData();
        GS->bIsRunning = true;
        GS->PreCountdown = 2;  // 2 seg: ROUND # (1s) -> READY (1s)
        GS->Countdown = Config.MaxTestDuration;
    }

    // Verificar si hay dos IdleAI antes de spawneear FightingCameraActor
    int32 IdleAICount = 0;
    for (const FParticipantConfig& PData : Config.Participants)
    {
        if (PData.ControllerType == ECombatControllerType::IdleAI)
        {
            IdleAICount++;
        }
    }
    bool bTwoIdleAI = (IdleAICount == 2);

    // Cuando no son dos IdleAI: usar una sola cámara de combate (la del subsistema).
    // Destruir la cámara del GameMode para que no queden dos y una robe la vista al replicar.
    if (!bTwoIdleAI)
    {
        if (AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
        {
            if (GM->FightCam)
            {
                GM->FightCam->Destroy();
                GM->FightCam = nullptr;
            }
        }
        if (!FightCam)
        {
            FActorSpawnParameters CamParams;
            CamParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            FightCam = GetWorld()->SpawnActor<AFightingCameraActor>(
                AFightingCameraActor::StaticClass(),
                FVector::ZeroVector,
                FRotator::ZeroRotator,
                CamParams
            );
        }
    }

    AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    if (GM && GM->LevelManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] LevelManager presente -> suscribiendo a OnStageLoaded y luego LoadStage(%s)."), *Config.InitialMap.ToString());
        bWaitingForStageToLoad = true;
        GM->LevelManager->OnStageLoaded.AddDynamic(this, &UCombatTestSubsystem::OnStageLoadedForCombat);
        GetWorld()->GetTimerManager().SetTimer(StageLoadedFallbackTimerHandle, this, &UCombatTestSubsystem::OnStageLoadedFallbackFired, 1.5f, false);
        UE_LOG(LogTemp, Log, TEXT("[CombatTestSubsystem] Timer fallback 1.5s por si OnStageLoaded no llega."));
        GM->LevelManager->LoadStage(Config.InitialMap);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] Sin LevelManager -> spawneando participantes YA (sin esperar escenario)."));
        FinishStartTestAfterStageLoaded();
    }
}

void UCombatTestSubsystem::OnStageLoadedForCombat(FName LoadedStageName)
{
    if (!bWaitingForStageToLoad) return;
    bWaitingForStageToLoad = false;
    GetWorld()->GetTimerManager().ClearTimer(StageLoadedFallbackTimerHandle);
    if (AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        if (GM->LevelManager)
        {
            GM->LevelManager->OnStageLoaded.RemoveDynamic(this, &UCombatTestSubsystem::OnStageLoadedForCombat);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] *** OnStageLoaded recibido (%s) -> spawneando participantes ahora ***"), *LoadedStageName.ToString());
    UE_LOG(LogTemp, Log, TEXT("[PERF] [CombatTestSubsystem] Punto revision 4: OnStageLoaded recibido | escenario=%s"), *LoadedStageName.ToString());
    {
        FProcHandle ProcHandle = FPlatformProcess::OpenProcess(FPlatformProcess::GetCurrentProcessId());
        if (ProcHandle.IsValid())
        {
            FPlatformProcessMemoryStats Mem = {};
            if (FPlatformProcess::TryGetMemoryUsage(ProcHandle, Mem))
            {
                const double UsedMB = static_cast<double>(Mem.UsedPhysical) / (1024.0 * 1024.0);
                const double PeakMB = static_cast<double>(Mem.PeakUsedPhysical) / (1024.0 * 1024.0);
                UE_LOG(LogTemp, Log, TEXT("[PERF] [CombatTestSubsystem] Punto 4 memoria: | RAM_proceso=%.1f MB | RAM_peak=%.1f MB"), UsedMB, PeakMB);
            }
            FPlatformProcess::CloseProc(ProcHandle);
        }
    }
    FinishStartTestAfterStageLoaded();
}

void UCombatTestSubsystem::OnStageLoadedFallbackFired()
{
    if (!bWaitingForStageToLoad) return;
    bWaitingForStageToLoad = false;
    if (AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        if (GM->LevelManager)
        {
            GM->LevelManager->OnStageLoaded.RemoveDynamic(this, &UCombatTestSubsystem::OnStageLoadedForCombat);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] *** FALLBACK 1.5s: OnStageLoaded no llego -> spawneando participantes igual (puede que el suelo aun no este). ***"));
    FinishStartTestAfterStageLoaded();
}

void UCombatTestSubsystem::FinishStartTestAfterStageLoaded()
{
    UE_LOG(LogTemp, Log, TEXT("[PERF] [CombatTestSubsystem] Punto revision 5: FinishStartTestAfterStageLoaded INICIO | participantes=%d"), CurrentConfig.Participants.Num());
    {
        FProcHandle ProcHandle = FPlatformProcess::OpenProcess(FPlatformProcess::GetCurrentProcessId());
        if (ProcHandle.IsValid())
        {
            FPlatformProcessMemoryStats Mem = {};
            if (FPlatformProcess::TryGetMemoryUsage(ProcHandle, Mem))
            {
                const double UsedMB = static_cast<double>(Mem.UsedPhysical) / (1024.0 * 1024.0);
                const double PeakMB = static_cast<double>(Mem.PeakUsedPhysical) / (1024.0 * 1024.0);
                UE_LOG(LogTemp, Log, TEXT("[PERF] [CombatTestSubsystem] Punto 5 memoria: | RAM_proceso=%.1f MB | RAM_peak=%.1f MB"), UsedMB, PeakMB);
            }
            FPlatformProcess::CloseProc(ProcHandle);
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] FinishStartTestAfterStageLoaded -> SpawnParticipants(%d), HUD, PreCountdown."), CurrentConfig.Participants.Num());

    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UAudioManagerSubsystem* AudioMgr = GI->GetSubsystem<UAudioManagerSubsystem>())
        {
            AudioMgr->PlayGameplayMusic(CurrentConfig.InitialMap);
        }
    }

    SpawnParticipants(CurrentConfig);

    // Reset cajas F DESPUÉS de spawn: así ApplyVisibilityToAll(false) encuentra a los personajes y los oculta (evita desincronía).
    if (UCollisionDebugSubsystem* CollisionDebug = GetWorld()->GetSubsystem<UCollisionDebugSubsystem>())
    {
        CollisionDebug->ResetToDefault();
    }

    InitializeHUD();
    if (SpawnedCharacters.Num() >= 2)
    {
        UpdateHUDCharacters(SpawnedCharacters[0], SpawnedCharacters[1]);
    }

    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        GS->StartPreCountdown();
    }
    UE_LOG(LogTemp, Log, TEXT("[PERF] [CombatTestSubsystem] Punto revision 6: FinishStartTestAfterStageLoaded FIN | spawn+HUD+PreCountdown listos."));
    {
        FProcHandle ProcHandle = FPlatformProcess::OpenProcess(FPlatformProcess::GetCurrentProcessId());
        if (ProcHandle.IsValid())
        {
            FPlatformProcessMemoryStats Mem = {};
            if (FPlatformProcess::TryGetMemoryUsage(ProcHandle, Mem))
            {
                const double UsedMB = static_cast<double>(Mem.UsedPhysical) / (1024.0 * 1024.0);
                const double PeakMB = static_cast<double>(Mem.PeakUsedPhysical) / (1024.0 * 1024.0);
                UE_LOG(LogTemp, Log, TEXT("[PERF] [CombatTestSubsystem] Punto 6 memoria: | RAM_proceso=%.1f MB | RAM_peak=%.1f MB"), UsedMB, PeakMB);
            }
            FPlatformProcess::CloseProc(ProcHandle);
        }
    }
}

void UCombatTestSubsystem::TickTest()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AMainGameState* GS = World->GetGameState<AMainGameState>();
    if (!GS)
    {
        return;
    }

    if (GS->PreCountdown > 0)
    {
        GS->PreCountdown--;
    }
    else if (!GS->bFightStarted)
    {
        StartCombat();
    }

    if (!GS->bIsRunning)
    {
        GS->bIsRunning = true;
    }

    if (GS->Countdown > 0.f)
    {
        GS->Countdown--;
        FString CountdownText = FString::Printf(TEXT("%d"), GS->Countdown);
    }
    else
    {
        StopTest();
        World->GetTimerManager().ClearTimer(TestTimerHandle);
        bTestRunning = false;
    }
}

void UCombatTestSubsystem::StartCombat()
{
    for (AIO89Character* Char : SpawnedCharacters)
    {
        if (!Char) continue;

        AController* Controller = Char->GetController();
        if (!Controller) continue;

        if (AAggressiveAIController* AggressiveAI = Cast<AAggressiveAIController>(Controller))
        {
            AggressiveAI->StartCombatMode();
        }
        else if (ADefensiveAIController* DefensiveAI = Cast<ADefensiveAIController>(Controller))
        {
            DefensiveAI->StartCombatMode();
        }
    }

    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        GS->bFightStarted = true;
        GS->bIsRunning = true;
    }
}

void UCombatTestSubsystem::StopTest()
{
    if (UGameInstance* GI = GetWorld()->GetGameInstance())
    {
        if (UAudioManagerSubsystem* AudioMgr = GI->GetSubsystem<UAudioManagerSubsystem>())
        {
            AudioMgr->StopGameplayMusic();
        }
    }

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TestTimerHandle);
        World->GetTimerManager().ClearTimer(StageLoadedFallbackTimerHandle);
    }
    bWaitingForStageToLoad = false;
    if (AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(GetWorld())))
    {
        if (GM->LevelManager)
        {
            GM->LevelManager->OnStageLoaded.RemoveDynamic(this, &UCombatTestSubsystem::OnStageLoadedForCombat);
        }
    }

    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        GS->bIsRunning = false;
        // Limpiar timers del GameState antes de destruir personajes
        GS->ClearAllTimers();
        // Destruir los personajes actuales (incl. el respawneado en round 2 que no esta en SpawnedCharacters)
        for (AIO89Character* Char : { GS->PlayerOneCharacter, GS->PlayerTwoCharacter })
        {
            if (IsValid(Char))
            {
                AController* C = Char->GetController();
                Char->Destroy();
                if (C && !C->IsA(APlayerController::StaticClass()))
                    C->Destroy();
            }
        }
        GS->PlayerOneCharacter = nullptr;
        GS->PlayerTwoCharacter = nullptr;
    }

    for (AIO89Character* Char : SpawnedCharacters)
    {
        if (Char && Char->IsValidLowLevel())
        {
            Char->Destroy();
        }
    }
    SpawnedCharacters.Empty();

    bTestRunning = false;

    if (HUDWidgetRef)
    {
        HUDWidgetRef->RemoveFromParent();
        HUDWidgetRef = nullptr;
    }

    if (PauseWidgetRef)
    {
        PauseWidgetRef->RemoveFromParent();
        PauseWidgetRef = nullptr;
    }
}

void UCombatTestSubsystem::ShowPauseMenu(APlayerController* PC)
{
    UWorld* World = GetWorld();
    if (!World || !PC || !bTestRunning)
    {
        return;
    }

    if (PauseWidgetRef && PauseWidgetRef->IsInViewport())
    {
        ClosePauseMenu(PC);
        return;
    }
    if (PauseWidgetRef && !PauseWidgetRef->IsInViewport())
    {
        PauseWidgetRef = nullptr;
    }

    TSubclassOf<UPauseWidget> PauseWidgetClass = LoadClass<UPauseWidget>(nullptr, TEXT("/Game/Blueprints/Widgets/WBP_PauseWidget.WBP_PauseWidget_C"));
    if (!PauseWidgetClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] ShowPauseMenu: no se pudo cargar WBP_PauseWidget"));
        return;
    }

    UPauseWidget* PauseWidget = CreateWidget<UPauseWidget>(PC, PauseWidgetClass);
    if (!PauseWidget)
    {
        return;
    }

    PauseWidget->AddToViewport(999);
    PauseWidgetRef = PauseWidget;

    if (UGameInstance* GI = World->GetGameInstance())
    {
        if (UAudioManagerSubsystem* AudioMgr = GI->GetSubsystem<UAudioManagerSubsystem>())
        {
            AudioMgr->PlayPauseMusic();
        }
    }

    // Congelar la pelea con time dilation en vez de SetPause, para que el input (IA_Pause) siga procesándose
    UGameplayStatics::SetGlobalTimeDilation(World, 0.0f);
    PC->bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    PC->SetInputMode(InputMode);
}

void UCombatTestSubsystem::ClosePauseMenu(APlayerController* PC)
{
    if (!PC) return;

    if (UWorld* World = PC->GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            if (UAudioManagerSubsystem* AudioMgr = GI->GetSubsystem<UAudioManagerSubsystem>())
            {
                AudioMgr->StopPauseMusic();
                AudioMgr->ResumeGameplayMusic();
            }
        }
    }

    if (PauseWidgetRef && PauseWidgetRef->IsInViewport())
    {
        PauseWidgetRef->RemoveFromParent();
        PauseWidgetRef = nullptr;
    }
    if (UWorld* World = PC->GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
    }
    PC->bShowMouseCursor = false;
    PC->SetInputMode(FInputModeGameOnly());
}

void UCombatTestSubsystem::ClearPauseWidgetRef()
{
    PauseWidgetRef = nullptr;
}

/** Coloca al personaje en la posicion de suelo manual (SpawnPos viene de GetSpawnPositionsForMap con Z = suelo). Sin traceline. */
static void ApplyManualFloorPosition(const FVector& SpawnPos, AIO89Character* Char)
{
    if (!Char) return;
    UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
    if (!Capsule) return;
    const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
    FVector FinalLoc(SpawnPos.X, SpawnPos.Y, SpawnPos.Z + HalfHeight);
    Char->SetActorLocation(FinalLoc);
    UE_LOG(LogTemp, Log, TEXT("[CombatTestSubsystem] ApplyManualFloorPosition: %s -> (%.0f, %.0f, %.1f)"), *Char->GetName(), FinalLoc.X, FinalLoc.Y, FinalLoc.Z);
}

void UCombatTestSubsystem::SpawnParticipants(const FCombatTestConfig& Config)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("[CombatTestSubsystem] SpawnParticipants: World null."));
        return;
    }
    UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] SpawnParticipants INICIO -> %d participantes."), Config.Participants.Num());

    AMainGameMode* GM = Cast<AMainGameMode>(UGameplayStatics::GetGameMode(World));
    if (!GM)
    {
        return;
    }

    // Verificar si hay dos IdleAI
    int32 IdleAICount = 0;
    for (const FParticipantConfig& PData : Config.Participants)
    {
        if (PData.ControllerType == ECombatControllerType::IdleAI)
        {
            IdleAICount++;
        }
    }

    bool bTwoIdleAI = (IdleAICount == 2);

    FActorSpawnParameters Params;
    Params.Owner = GM;

    for (const FParticipantConfig& PData : Config.Participants)
    {
        if (!PData.PawnClass)
        {
            continue;
        }

        FVector SpawnPos = PData.SpawnLocation;
        AIO89Character* SpawnedChar = World->SpawnActor<AIO89Character>(
            PData.PawnClass,
            SpawnPos,
            PData.SpawnRotation,
            Params
        );

        if (!SpawnedChar)
        {
            UE_LOG(LogTemp, Error, TEXT("[CombatTestSubsystem] SpawnActor fallo para clase %s."), PData.PawnClass ? *PData.PawnClass->GetName() : TEXT("null"));
            continue;
        }

        UE_LOG(LogTemp, Log, TEXT("[CombatTestSubsystem] Spawneado %s en (%.0f, %.0f, %.0f)."), *SpawnedChar->GetName(), SpawnPos.X, SpawnPos.Y, SpawnPos.Z);
        ApplyManualFloorPosition(PData.SpawnLocation, SpawnedChar);

        AController* Controller = SpawnControllerForType(PData.ControllerType);
        if (Controller)
        {
            Controller->Possess(SpawnedChar);
            SpawnedCharacters.Add(SpawnedChar);

            if (PData.BodyMaterialOverride && SpawnedChar->GetMesh())
            {
                const int32 SlotIndex = FMath::Clamp(PData.BodyMaterialSlotIndex, 0, SpawnedChar->GetMesh()->GetNumMaterials() - 1);
                SpawnedChar->GetMesh()->SetMaterial(SlotIndex, PData.BodyMaterialOverride);
            }

            FString ControllerTypeName;
            switch (PData.ControllerType)
            {
            case ECombatControllerType::AggressiveAI: ControllerTypeName = TEXT("AggressiveAI"); break;
            case ECombatControllerType::DefensiveAI:  ControllerTypeName = TEXT("DefensiveAI");  break;
            case ECombatControllerType::IdleAI:       ControllerTypeName = TEXT("IdleAI");       break;
            case ECombatControllerType::Player:       ControllerTypeName = TEXT("Player");       break;
            default: ControllerTypeName = TEXT("Unknown"); break;
            }

            if (PData.ControllerType == ECombatControllerType::Player)
            {
                APlayerController* PC = Cast<APlayerController>(Controller);
                if (PC && FightCam && !bTwoIdleAI)
                {
                    PC->SetViewTargetWithBlend(FightCam, 0.f);
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[CombatTestSubsystem] Could not create a controller for the character %s."), *SpawnedChar->GetName());
        }
    }

    // Si hay dos IdleAI, configurar TestCameraController y TestCameraActor para cámara libre
    if (bTwoIdleAI)
    {
        // Desactivar o destruir el FightingCameraActor si existe (spawneado en MainGameMode)
        // Reutilizar la variable GM que ya existe en el scope
        if (GM && GM->FightCam)
        {
            GM->FightCam->SetActorTickEnabled(false);
            GM->FightCam->SetActorHiddenInGame(true);
            GM->FightCam->SetActorEnableCollision(false);
            UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] Disabled FightingCameraActor for free camera mode"));
        }
        
        // También desactivar el FightCam del subsistema si existe
        if (FightCam)
        {
            FightCam->SetActorTickEnabled(false);
            FightCam->SetActorHiddenInGame(true);
            FightCam->SetActorEnableCollision(false);
        }
        
        // Limpiar FightCamRef del GameState
        if (AMainGameState* GS = World->GetGameState<AMainGameState>())
        {
            GS->FightCamRef = nullptr;
        }
        
        APlayerController* LocalPC = UGameplayStatics::GetPlayerController(World, 0);
        if (LocalPC)
        {
            // Spawnear TestCameraController si no existe (esto también spawnea TestCameraActor)
            if (!TestCameraController)
            {
                FActorSpawnParameters CameraSpawnParams;
                CameraSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
                TestCameraController = World->SpawnActor<ATestCameraController>(ATestCameraController::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, CameraSpawnParams);
                
                if (TestCameraController)
                {
                    TestCameraActor = TestCameraController->GetTestCameraActor();
                    
                    if (TestCameraActor)
                    {
                        // Hacer que el TestCameraController sea el PlayerController activo
                        // Esto permite que reciba input
                        if (ULocalPlayer* LocalPlayer = LocalPC->GetLocalPlayer())
                        {
                            LocalPC->Player = nullptr;
                            TestCameraController->SetPlayer(LocalPlayer);
                            TestCameraController->Player = LocalPlayer;
                            
                            UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] 2 IdleAI: Spawned TestCameraController, LocalPlayer asignado a el. LA TECLA F DEBE PULSARSE CON ESTE CONTROLADOR (no SpectatorPC)."));
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("[CombatTestSubsystem] 2 IdleAI: LocalPlayer null, TestCameraController no recibira input (F no funcionara)."));
                        }
                    }
                }
            }
            
            // Cambiar la vista al TestCameraActor (usar TestCameraController si está disponible)
            APlayerController* ActivePC = TestCameraController ? TestCameraController : LocalPC;
            if (TestCameraActor && ActivePC)
            {
                FViewTargetTransitionParams ViewParams;
                ViewParams.BlendTime = 0.f;
                ActivePC->SetViewTarget(TestCameraActor, ViewParams);
                UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] Switched view target to TestCameraActor"));
            }
        }
    }

    // F (toggle cajas) siempre en el controlador local: así solo hay un binding (evita doble en Player vs CPU: controlador + personaje).
    APlayerController* LocalPC = UGameplayStatics::GetPlayerController(World, 0);
    if (LocalPC)
    {
        if (ASpectatorPlayerController* SpectatorPC = Cast<ASpectatorPlayerController>(LocalPC))
        {
            SpectatorPC->EnsureCollisionToggleBinding();
        }
        else if (AThirdCharacterPlayerController* ThirdPC = Cast<AThirdCharacterPlayerController>(LocalPC))
        {
            ThirdPC->EnsureCollisionToggleBinding();
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] SpawnParticipants FIN -> %d personajes en escena."), SpawnedCharacters.Num());
    TrySetupFight();

}

void UCombatTestSubsystem::TrySetupFight()
{
    if (SpawnedCharacters.Num() == 2)
    {
        SpawnedCharacters[0]->OtherPlayer = SpawnedCharacters[1];
        SpawnedCharacters[1]->OtherPlayer = SpawnedCharacters[0];
        
        // Solo configurar la cámara de combate si no hay dos IdleAI
        bool bTwoIdleAI = false;
        if (SpawnedCharacters[0]->GetController() && SpawnedCharacters[1]->GetController())
        {
            AIdleAIController* Idle1 = Cast<AIdleAIController>(SpawnedCharacters[0]->GetController());
            AIdleAIController* Idle2 = Cast<AIdleAIController>(SpawnedCharacters[1]->GetController());
            bTwoIdleAI = (Idle1 != nullptr && Idle2 != nullptr);
        }
        
        if (!bTwoIdleAI)
        {
            SetupFightCamera(SpawnedCharacters[0], SpawnedCharacters[1]);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatTestSubsystem] Two IdleAI detected - using free camera mode"));
        }
    }
}

AController* UCombatTestSubsystem::SpawnControllerForType(ECombatControllerType Type)
{
    UWorld* World = GetWorld();
    if (!World) return nullptr;

    switch (Type)
    {
    case ECombatControllerType::AggressiveAI:
        return World->SpawnActor<AAggressiveAIController>();

    case ECombatControllerType::DefensiveAI:
        return World->SpawnActor<ADefensiveAIController>();

    case ECombatControllerType::IdleAI:
        return World->SpawnActor<AIdleAIController>();

    case ECombatControllerType::Spectator:
        return World->SpawnActor<ASpectatorPlayerController>();

    case ECombatControllerType::Player:
        return UGameplayStatics::GetPlayerController(World, 0);

    default:
        return nullptr;
    }
}

void UCombatTestSubsystem::SetupFightCamera(AIO89Character* CharA, AIO89Character* CharB)
{
    if (!FightCam)
    {
        return;
    }

    FightCam->UnfreezeCamera();
    FightCam->Player1 = CharA;
    FightCam->Player2 = CharB;

    AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>();
    if (GS)
    {
        // Solo configurar FightCamRef si NO estamos usando TestCameraActor
        // (es decir, si no hay dos IdleAI)
        if (!TestCameraActor)
        {
            GS->FightCamRef = FightCam;
            // Forzar view target del espectador a esta cámara (InitHUD pudo haber corrido antes de StartTest)
            if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
            {
                FViewTargetTransitionParams Params;
                Params.BlendTime = 0.f;
                PC->SetViewTarget(FightCam, Params);
            }
        }
        GS->PlayerOneCharacter = CharA;
        GS->PlayerTwoCharacter = CharB;
        if (CurrentConfig.Participants.Num() >= 2)
        {
            GS->SetInitialSpawnPositions(
                CurrentConfig.Participants[0].SpawnLocation,
                CurrentConfig.Participants[0].SpawnRotation,
                CurrentConfig.Participants[1].SpawnLocation,
                CurrentConfig.Participants[1].SpawnRotation
            );
            const FParticipantConfig& P1 = CurrentConfig.Participants[0];
            const FParticipantConfig& P2 = CurrentConfig.Participants[1];
            GS->SetParticipantRespawnInfo(
                P1.PawnClass, P2.PawnClass,
                P1.BodyMaterialOverride, P1.BodyMaterialSlotIndex,
                P2.BodyMaterialOverride, P2.BodyMaterialSlotIndex,
                P1.ControllerType, P2.ControllerType
            );
        }
    }
}

void UCombatTestSubsystem::InitializeHUD()
{
    if (HUDWidgetRef)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World || !GEngine || !GEngine->GameViewport)
    {
        return;
    }

    if (HUDWidgetClass)
    {
        HUDWidgetRef = CreateWidget<UHUDWidget>(World, HUDWidgetClass);
        if (HUDWidgetRef)
        {
            HUDWidgetRef->AddToViewport();
        }
    }
}

void UCombatTestSubsystem::UpdateHUDCharacters(AIO89Character* CharA, AIO89Character* CharB)
{
    if (HUDWidgetRef && CharA && CharB)
    {
        HUDWidgetRef->SetCharacters(CharA, CharB);
    }
}

