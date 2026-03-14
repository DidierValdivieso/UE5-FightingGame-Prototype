#include "MainGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"

#include "MainGameState.h"
#include "Characters/IO89Character.h"
#include "Actors/FightingCameraActor.h"
#include "Controllers/SpectatorPlayerController.h" 
#include "Controllers/DefensiveAIController.h"
#include "Controllers/AggressiveAIController.h"
#include "Subsystems/CombatTestSubsystem.h"
#include "Subsystems/WidgetSequenceManager.h"
#include "Structs/CombatTestStructs.h"
#include "Actors/LevelManagerActor.h"

AMainGameMode::AMainGameMode()
{
    DefaultPawnClass = nullptr;
    PlayerControllerClass = ASpectatorPlayerController::StaticClass();
    GameStateClass = AMainGameState::StaticClass();
    FrontendMapName = FName("L_MainMenu");
    ArenaHubMapName = FName("L_Brawlers_Base");

    static ConstructorHelpers::FClassFinder<AIO89Character> PlayerBPObj(TEXT("/Game/Blueprints/Characters/BP_IO89Character"));
    if (PlayerBPObj.Succeeded())
    {
        PlayerBPClass = PlayerBPObj.Class;
    }

    static ConstructorHelpers::FClassFinder<AFightingCameraActor> CamBPObj(TEXT("/Game/Blueprints/Actors/BP_FightingCameraActor"));
    if (CamBPObj.Succeeded())
    {
        FightingCameraClass = CamBPObj.Class;
    }
}

void AMainGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        for (TActorIterator<ALevelManagerActor> It(GetWorld()); It; ++It)
        {
            LevelManager = *It;
            break;
        }

        if (LevelManager && !IsFrontendMap())
        {
            UCombatTestSubsystem* Subsystem = GetWorld()->GetSubsystem<UCombatTestSubsystem>();
            UWidgetSequenceManager* WidgetMgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWidgetSequenceManager>() : nullptr;
            const bool bPendingFromMenu = WidgetMgr && WidgetMgr->HasPendingCombatFromMenu();
            const bool bWaitingControllerConfig = WidgetMgr && WidgetMgr->ShouldShowControllerConfigOnArenaLoad();

            if (bPendingFromMenu)
            {
                // Usuario ya confirmó en el menú: iniciar combate (StartTest hace LoadStage)
                const FCombatTestConfig PendingConfig = WidgetMgr->GetPendingCombatConfig();
                WidgetMgr->ClearPendingCombatFromMenu();
                if (Subsystem)
                {
                    Subsystem->StartTest(PendingConfig);
                }
            }
            else if (bWaitingControllerConfig)
            {
                // Se mostrará la pantalla de controller al hacer PostLogin
            }
            else
            {
                // Arena abierta al inicio (ej. .exe que arranca en L_Brawlers_Base): mostrar secuencia de widgets
                // para que el usuario vea Frontend -> Stage -> Controller igual que en L_MainMenu
                FTimerHandle ArenaMenuHandle;
                GetWorld()->GetTimerManager().SetTimer(ArenaMenuHandle, [this]()
                {
                    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
                    {
                        if (UWidgetSequenceManager* Mgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWidgetSequenceManager>() : nullptr)
                        {
                            if (!Mgr->IsFrontendWidgetVisible())
                            {
                                if (FightCam)
                                {
                                    FViewTargetTransitionParams Params;
                                    Params.BlendTime = 0.f;
                                    PC->SetViewTarget(FightCam, Params);
                                }
                                Mgr->StartWidgetSequence(PC);
                            }
                        }
                    }
                }, 0.5f, false);
            }
        }
        else if (!LevelManager && !IsFrontendMap())
        {
            UE_LOG(LogTemp, Warning, TEXT("[MainGameMode] LevelManager not found in world!"));
            // Aun así intentar mostrar el menú para que el .exe no quede en negro
            StartMenuWidgetsWithDelay();
        }

        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = this;
        FightCam = GetWorld()->SpawnActor<AFightingCameraActor>(
            FightingCameraClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
        if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
        {
            GS->FightCamRef = FightCam;
        }
    }

    // Fallback: si estamos en el menú y PostLogin no llegó a iniciar los widgets (ej. BP no llama Super),
    // intentar iniciar la secuencia un poco después para que el .exe siempre muestre el menú
    if (HasAuthority() && IsFrontendMap())
    {
        FTimerHandle FallbackHandle;
        GetWorld()->GetTimerManager().SetTimer(FallbackHandle, [this]()
        {
            if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
            {
                if (UWidgetSequenceManager* Mgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWidgetSequenceManager>() : nullptr)
                {
                    if (!Mgr->IsFrontendWidgetVisible())
                    {
                        if (FightCam)
                        {
                            FViewTargetTransitionParams Params;
                            Params.BlendTime = 0.f;
                            PC->SetViewTarget(FightCam, Params);
                        }
                        Mgr->StartWidgetSequence(PC);
                    }
                }
            }
        }, 0.5f, false);
    }
}

void AMainGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer || !HasAuthority()) return;

    UWidgetSequenceManager* WidgetMgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWidgetSequenceManager>() : nullptr;
    if (!WidgetMgr) return;

    if (IsFrontendMap())
    {
        // Asegurar que el jugador tenga una cámara (evitar pantalla negra)
        if (FightCam)
        {
            FViewTargetTransitionParams Params;
            Params.BlendTime = 0.f;
            NewPlayer->SetViewTarget(FightCam, Params);
        }

        // Menú: iniciar secuencia de widgets (solo si aún no está visible, evita duplicar con GameInstance/BeginPlay)
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, NewPlayer]()
        {
            if (UWidgetSequenceManager* Mgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWidgetSequenceManager>() : nullptr)
            {
                if (!Mgr->IsFrontendWidgetVisible())
                {
                    Mgr->StartWidgetSequence(NewPlayer);
                }
            }
        }, 0.15f, false);
    }
    else if (WidgetMgr->ShouldShowControllerConfigOnArenaLoad())
    {
        // Arena cargada tras elegir etapa: mostrar pantalla de controller (el mapa ya cargó mientras el usuario la ve)
        FTimerHandle TimerHandle;
        GetWorld()->GetTimerManager().SetTimer(TimerHandle, [this, NewPlayer]()
        {
            if (UWidgetSequenceManager* Mgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWidgetSequenceManager>() : nullptr)
            {
                Mgr->ShowControllerConfigOnArena(NewPlayer);
            }
        }, 0.15f, false);
    }
}

bool AMainGameMode::IsFrontendMap() const
{
    const FString CurrentLevelName = GetWorld() ? UGameplayStatics::GetCurrentLevelName(this, true) : FString();
    return CurrentLevelName.Equals(FrontendMapName.ToString(), ESearchCase::IgnoreCase);
}

void AMainGameMode::StartMenuWidgetsWithDelay()
{
    if (!GetWorld()) return;
    FTimerHandle H;
    GetWorld()->GetTimerManager().SetTimer(H, [this]()
    {
        if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
        {
            if (UWidgetSequenceManager* Mgr = GetGameInstance() ? GetGameInstance()->GetSubsystem<UWidgetSequenceManager>() : nullptr)
            {
                if (!Mgr->IsFrontendWidgetVisible())
                {
                    if (FightCam)
                    {
                        FViewTargetTransitionParams Params;
                        Params.BlendTime = 0.f;
                        PC->SetViewTarget(FightCam, Params);
                    }
                    Mgr->StartWidgetSequence(PC);
                }
            }
        }
    }, 1.0f, false);
}