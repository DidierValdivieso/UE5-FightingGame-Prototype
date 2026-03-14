#include "MainGameState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Controllers/ThirdCharacterPlayerController.h"
#include "Controllers/AggressiveAIController.h"
#include "Controllers/DefensiveAIController.h"
#include "Characters/IO89Character.h"
#include "Widgets/HUDWidget.h"
#include "Widgets/FinalFightWidget.h"
#include "Subsystems/WidgetSequenceManager.h"
#include "Subsystems/CollisionDebugSubsystem.h"
#include "Subsystems/CombatTestSubsystem.h"
#include "Materials/MaterialInterface.h"
#include <Kismet/GameplayStatics.h>

namespace
{
    /** Offset por encima del suelo para round 2 (evita quedar dentro del piso en Elderbreach y Garbage). */
    const float Round2FloorOffset = 20.f;
    /** Mitad de altura minima por si la capsula devuelve 0 o valor raro cuando el personaje esta muerto (ragdoll). */
    const float MinCapsuleHalfHeight = 85.f;

    /** Coloca al personaje en la posicion de suelo manual (SpawnLoc.Z = suelo; sin traceline).
     *  Cuando esta muerto la capsula sigue existiendo pero con colision desactivada; usamos altura minima por seguridad. */
    void ApplyManualFloorPosition(AIO89Character* Char, const FVector& SpawnLoc)
    {
        if (!Char) return;
        UCapsuleComponent* Capsule = Char->GetCapsuleComponent();
        if (!Capsule) return;
        float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        if (HalfHeight < 1.f) HalfHeight = MinCapsuleHalfHeight;
        FVector FinalLoc(SpawnLoc.X, SpawnLoc.Y, SpawnLoc.Z + HalfHeight + Round2FloorOffset);
        Char->SetActorLocation(FinalLoc, false);
        if (USceneComponent* Root = Char->GetRootComponent())
            Root->UpdateComponentToWorld();
    }
}

void AMainGameState::BeginPlay()
{
    Super::BeginPlay();
}

void AMainGameState::SetInitialSpawnPositions(const FVector& P1Loc, const FRotator& P1Rot, const FVector& P2Loc, const FRotator& P2Rot)
{
    PlayerOneSpawnLocation = P1Loc;
    PlayerOneSpawnRotation = P1Rot;
    PlayerTwoSpawnLocation = P2Loc;
    PlayerTwoSpawnRotation = P2Rot;
}

void AMainGameState::SetParticipantRespawnInfo(TSubclassOf<APawn> P1Class, TSubclassOf<APawn> P2Class,
    UMaterialInterface* P1BodyMat, int32 P1BodySlot, UMaterialInterface* P2BodyMat, int32 P2BodySlot,
    ECombatControllerType P1ControllerType, ECombatControllerType P2ControllerType)
{
    PlayerOnePawnClass = P1Class;
    PlayerTwoPawnClass = P2Class;
    PlayerOneBodyMaterialOverride = P1BodyMat;
    PlayerTwoBodyMaterialOverride = P2BodyMat;
    PlayerOneBodyMaterialSlotIndex = P1BodySlot;
    PlayerTwoBodyMaterialSlotIndex = P2BodySlot;
    PlayerOneControllerType = P1ControllerType;
    PlayerTwoControllerType = P2ControllerType;
}

void AMainGameState::ClearAllTimers()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PreCountdownTimerHandle);
        World->GetTimerManager().ClearTimer(CountdownTimerHandle);
    }
}

void AMainGameState::ResetTestData()
{
    // Limpiar todos los timers activos antes de resetear los datos
    ClearAllTimers();

    PreCountdown = 2;  // 2 seg total: seg 0-1 ROUND #, seg 1-2 READY
    Countdown = 60;
    bIsRunning = false;
    bFightStarted = false;
    CurrentRound = 1;
    PlayerOneRoundsWon = 0;
    PlayerTwoRoundsWon = 0;
    RoundOneDuration = 0.f;
    RoundTwoDuration = 0.f;
    PlayerOneRoundOneDamage = 0.f;
    PlayerOneRoundTwoDamage = 0.f;
    PlayerTwoRoundOneDamage = 0.f;
    PlayerTwoRoundTwoDamage = 0.f;
    RoundOneStartTime = 0.f;
    RoundTwoStartTime = 0.f;
    PlayerOneHealthAtRoundStart = 0.f;
    PlayerTwoHealthAtRoundStart = 0.f;
    LastRoundNumber = 0;
    LastRoundWinner = 0;

    // Reiniciar posiciones iniciales y referencias a personajes (se destruyen al ir al main menu)
    PlayerOneSpawnLocation = FVector::ZeroVector;
    PlayerOneSpawnRotation = FRotator::ZeroRotator;
    PlayerTwoSpawnLocation = FVector::ZeroVector;
    PlayerTwoSpawnRotation = FRotator::ZeroRotator;
    PlayerOneCharacter = nullptr;
    PlayerTwoCharacter = nullptr;
}

void AMainGameState::StartPreCountdown()
{
    if (!HasAuthority()) return;

    GetWorld()->GetTimerManager().SetTimer(
        PreCountdownTimerHandle,
        this,
        &AMainGameState::UpdatePreCountdown,
        1.0f,
        true
    );
}

void AMainGameState::UpdatePreCountdown()
{
    if (PreCountdown > 0)
    {
        PreCountdown--;
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(PreCountdownTimerHandle);
        StartRound(CurrentRound);
    }
}

void AMainGameState::StartRound(int32 RoundNumber)
{
    if (!HasAuthority()) return;

    // Limpiar timers activos antes de iniciar un nuevo round (evita bugs al hacer restart)
    ClearAllTimers();

    // Al iniciar round 1 (nueva pelea) resetear cajas F para que el toggle funcione (p. ej. tras volver del menú)
    if (RoundNumber == 1)
    {
        MulticastResetCollisionDebug();
    }

    CurrentRound = RoundNumber;
    bFightStarted = true;
    bIsRunning = true;
    Countdown = 60;

    // Store round start time
    if (RoundNumber == 1)
    {
        RoundOneStartTime = GetWorld()->GetTimeSeconds();
    }
    else if (RoundNumber == 2)
    {
        RoundTwoStartTime = GetWorld()->GetTimeSeconds();
    }

    // Store initial health for damage calculation
    if (PlayerOneCharacter)
    {
        PlayerOneHealthAtRoundStart = PlayerOneCharacter->Health;
    }
    if (PlayerTwoCharacter)
    {
        PlayerTwoHealthAtRoundStart = PlayerTwoCharacter->Health;
    }

    GetWorld()->GetTimerManager().SetTimer(
        CountdownTimerHandle,
        this,
        &AMainGameState::UpdateCountdown,
        1.0f,
        true
    );
}

void AMainGameState::UpdateCountdown()
{
    if (Countdown > 0)
    {
        Countdown--;
    }
    else
    {
        GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);
        HandleCountdownFinished();
        bIsRunning = false;
    }
}

void AMainGameState::OnRep_Countdown()
{
}

void AMainGameState::OnRep_FightCam()
{
    if (FightCamRef)
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            FViewTargetTransitionParams Params;
            Params.BlendTime = 0.f;
            PC->SetViewTarget(FightCamRef, Params);
        }
    }
}

void AMainGameState::DetermineRoundWinner()
{
    if (!HasAuthority()) return;

    if (!PlayerOneCharacter || !PlayerTwoCharacter) return;

    int32 RoundWinner = 0; // 0 = tie, 1 = PlayerOne, 2 = PlayerTwo

    // Compare health - player with more health wins
    if (PlayerOneCharacter->Health > PlayerTwoCharacter->Health)
    {
        RoundWinner = 1;
    }
    else if (PlayerTwoCharacter->Health > PlayerOneCharacter->Health)
    {
        RoundWinner = 2;
    }
    // else tie (RoundWinner = 0)

    EndRound(RoundWinner);
}

void AMainGameState::EndRound(int32 RoundWinner)
{
    if (!HasAuthority()) return;

    // Update round wins
    if (RoundWinner == 1)
    {
        PlayerOneRoundsWon++;
    }
    else if (RoundWinner == 2)
    {
        PlayerTwoRoundsWon++;
    }

    // Mismo flujo para round 1 y round 2: ralentizado + timer 0.5f. No llamar MulticastOnCountdownFinished aquí
    // (hace SetPause y dejaría el juego pegado; solo lo llamamos al mostrar el widget final, tras el 0.5f).
    const int32 RoundThatEnded = CurrentRound;
    LastRoundNumber = CurrentRound;
    LastRoundWinner = RoundWinner;
    bFightStarted = false;
    bIsRunning = false;
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.2f);

    // Ultimo round: guardar posicion de la camara ANTES de destruir personajes para mantener la vista
    if (RoundThatEnded >= 2 && FightCamRef)
    {
        FightCamRef->FreezeCameraForFightEnd();
    }

    // Quien murio: dejar ragdoll 0.49s, luego destruir (0.01s antes del callback de respawn a 0.5s).
    PendingRespawnSlot = 0;
    if (RoundWinner == 1 && PlayerTwoCharacter && PlayerTwoCharacter->IsDead())
    {
        PendingRespawnSlot = 2;
        UE_LOG(LogTemp, Log, TEXT("[MainGameState] Round end: P2 murio (derecha). Ragdoll 0.49s, luego destruir. Respawn slot 2 en round 2."));
        FTimerHandle DestroyDeadHandle;
        GetWorld()->GetTimerManager().SetTimer(DestroyDeadHandle, [this]()
            {
                if (PlayerTwoCharacter && PlayerTwoCharacter->IsDead())
                {
                    AController* OldController = PlayerTwoCharacter->GetController();
                    PlayerTwoCharacter->Destroy();
                    if (OldController && !OldController->IsA(APlayerController::StaticClass()))
                        OldController->Destroy();
                    PlayerTwoCharacter = nullptr;
                }
            }, 0.49f, false);
    }
    else if (RoundWinner == 2 && PlayerOneCharacter && PlayerOneCharacter->IsDead())
    {
        PendingRespawnSlot = 1;
        UE_LOG(LogTemp, Log, TEXT("[MainGameState] Round end: P1 murio (izquierda). Ragdoll 0.49s, luego destruir. Respawn slot 1 en round 2."));
        FTimerHandle DestroyDeadHandle;
        GetWorld()->GetTimerManager().SetTimer(DestroyDeadHandle, [this]()
            {
                if (PlayerOneCharacter && PlayerOneCharacter->IsDead())
                {
                    AController* OldController = PlayerOneCharacter->GetController();
                    PlayerOneCharacter->Destroy();
                    if (OldController && !OldController->IsA(APlayerController::StaticClass()))
                        OldController->Destroy();
                    PlayerOneCharacter = nullptr;
                }
            }, 0.49f, false);
    }

    FTimerHandle RoundEndEffectHandle;
    GetWorld()->GetTimerManager().SetTimer(RoundEndEffectHandle, [this, RoundThatEnded]()
        {
            LastRoundNumber = 0;
            UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);

            if (RoundThatEnded >= 2)
            {
                MulticastOnCountdownFinished(); // pausa al mostrar resultado final
                ShowFinalFightWidget();
                return;
            }

            // Tras round 1: el muerto ya se destruyo en EndRound. Spawneamos el nuevo personaje y controller; reposicionamos al vivo.
            UWorld* World = GetWorld();
            AGameModeBase* GM = World ? World->GetAuthGameMode() : nullptr;
            const bool bAuthority = HasAuthority();

            UCombatTestSubsystem* Sub = World->GetSubsystem<UCombatTestSubsystem>();
            if (bAuthority && PendingRespawnSlot != 0 && Sub)
            {
                if (PendingRespawnSlot == 1 && PlayerOnePawnClass)
                {
                    AController* NewController = Sub->SpawnControllerForType(PlayerOneControllerType);
                    if (NewController)
                    {
                        FActorSpawnParameters Params;
                        if (GM) Params.Owner = GM;
                        APawn* NewPawn = World->SpawnActor<APawn>(PlayerOnePawnClass, PlayerOneSpawnLocation, PlayerOneSpawnRotation, Params);
                        AIO89Character* NewChar = Cast<AIO89Character>(NewPawn);
                        if (NewChar)
                        {
                            ApplyManualFloorPosition(NewChar, PlayerOneSpawnLocation);
                            NewChar->SetActorRotation(PlayerOneSpawnRotation);
                            if (UCharacterMovementComponent* Mov = NewChar->GetCharacterMovement())
                            {
                                Mov->Velocity = FVector::ZeroVector;
                                Mov->SetMovementMode(MOVE_Walking);
                                Mov->SetComponentTickEnabled(true);
                            }
                            if (PlayerOneBodyMaterialOverride && NewChar->GetMesh())
                            {
                                const int32 Slot = FMath::Clamp(PlayerOneBodyMaterialSlotIndex, 0, NewChar->GetMesh()->GetNumMaterials() - 1);
                                NewChar->GetMesh()->SetMaterial(Slot, PlayerOneBodyMaterialOverride);
                            }
                            NewChar->Health = NewChar->MaxHealth;
                            NewChar->Energy = 10.f;
                            NewChar->Stamina = 10.f;
                            NewController->Possess(NewChar);
                            PlayerOneCharacter = NewChar;
                            if (APlayerController* PC = Cast<APlayerController>(NewController))
                            {
                                if (FightCamRef)
                                {
                                    FViewTargetTransitionParams ViewParams;
                                    ViewParams.BlendTime = 0.f;
                                    PC->SetViewTarget(FightCamRef, ViewParams);
                                }
                            }
                            UE_LOG(LogTemp, Log, TEXT("[MainGameState] Round 2: P1 respawneado con NUEVO controller %s."), *NewController->GetName());
                        }
                        else if (NewPawn)
                            NewPawn->Destroy();
                    }
                    PendingRespawnSlot = 0;
                }
                else if (PendingRespawnSlot == 2 && PlayerTwoPawnClass)
                {
                    AController* NewController = Sub->SpawnControllerForType(PlayerTwoControllerType);
                    if (NewController)
                    {
                        FActorSpawnParameters Params;
                        if (GM) Params.Owner = GM;
                        APawn* NewPawn = World->SpawnActor<APawn>(PlayerTwoPawnClass, PlayerTwoSpawnLocation, PlayerTwoSpawnRotation, Params);
                        AIO89Character* NewChar = Cast<AIO89Character>(NewPawn);
                        if (NewChar)
                        {
                            ApplyManualFloorPosition(NewChar, PlayerTwoSpawnLocation);
                            NewChar->SetActorRotation(PlayerTwoSpawnRotation);
                            if (UCharacterMovementComponent* Mov = NewChar->GetCharacterMovement())
                            {
                                Mov->Velocity = FVector::ZeroVector;
                                Mov->SetMovementMode(MOVE_Walking);
                                Mov->SetComponentTickEnabled(true);
                            }
                            if (PlayerTwoBodyMaterialOverride && NewChar->GetMesh())
                            {
                                const int32 Slot = FMath::Clamp(PlayerTwoBodyMaterialSlotIndex, 0, NewChar->GetMesh()->GetNumMaterials() - 1);
                                NewChar->GetMesh()->SetMaterial(Slot, PlayerTwoBodyMaterialOverride);
                            }
                            NewChar->Health = NewChar->MaxHealth;
                            NewChar->Energy = 10.f;
                            NewChar->Stamina = 10.f;
                            NewController->Possess(NewChar);
                            PlayerTwoCharacter = NewChar;
                            if (APlayerController* PC = Cast<APlayerController>(NewController))
                            {
                                if (FightCamRef)
                                {
                                    FViewTargetTransitionParams ViewParams;
                                    ViewParams.BlendTime = 0.f;
                                    PC->SetViewTarget(FightCamRef, ViewParams);
                                }
                            }
                            UE_LOG(LogTemp, Log, TEXT("[MainGameState] Round 2: P2 respawneado con NUEVO controller %s."), *NewController->GetName());
                        }
                        else if (NewPawn)
                            NewPawn->Destroy();
                    }
                    PendingRespawnSlot = 0;
                }
            }

            // Reposicionar al que sigue vivo (o a ambos si fue empate).
            if (IsValid(PlayerOneCharacter) && !PlayerOneCharacter->IsDead())
            {
                ApplyManualFloorPosition(PlayerOneCharacter, PlayerOneSpawnLocation);
                PlayerOneCharacter->SetActorRotation(PlayerOneSpawnRotation);
                if (UCharacterMovementComponent* Mov = PlayerOneCharacter->GetCharacterMovement())
                    Mov->Velocity = FVector::ZeroVector;
            }
            if (IsValid(PlayerTwoCharacter) && !PlayerTwoCharacter->IsDead())
            {
                ApplyManualFloorPosition(PlayerTwoCharacter, PlayerTwoSpawnLocation);
                PlayerTwoCharacter->SetActorRotation(PlayerTwoSpawnRotation);
                if (UCharacterMovementComponent* Mov = PlayerTwoCharacter->GetCharacterMovement())
                    Mov->Velocity = FVector::ZeroVector;
            }

            // Restaurar vida/stamina/energy en ambos para round 2 (si el round termino por tiempo, nadie murio y las barras quedan bajas si no se resetean).
            for (AIO89Character* Char : { PlayerOneCharacter, PlayerTwoCharacter })
            {
                if (Char)
                {
                    Char->Health = Char->MaxHealth;
                    Char->Energy = 10.f;
                    Char->Stamina = 10.f;
                }
            }

            if (PlayerOneCharacter && PlayerTwoCharacter)
            {
                PlayerOneCharacter->OtherPlayer = PlayerTwoCharacter;
                PlayerTwoCharacter->OtherPlayer = PlayerOneCharacter;
                UE_LOG(LogTemp, Log, TEXT("[MainGameState] Round 2: OtherPlayer asignado. P1=%s -> OtherPlayer=%s; P2=%s -> OtherPlayer=%s"),
                    *PlayerOneCharacter->GetName(), PlayerOneCharacter->OtherPlayer ? *PlayerOneCharacter->OtherPlayer->GetName() : TEXT("null"),
                    *PlayerTwoCharacter->GetName(), PlayerTwoCharacter->OtherPlayer ? *PlayerTwoCharacter->OtherPlayer->GetName() : TEXT("null"));
            }
            // No activar pelea aqui: esperar al PreCountdown (ROUND 2 -> READY) y que StartRound(2) ponga bFightStarted=true.
            if (FightCamRef)
            {
                FightCamRef->Player1 = PlayerOneCharacter;
                FightCamRef->Player2 = PlayerTwoCharacter;
            }
            if (Sub)
                Sub->UpdateHUDCharacters(PlayerOneCharacter, PlayerTwoCharacter);

            CurrentRound++;
            Countdown = 60;
            GetWorld()->GetTimerManager().ClearTimer(CountdownTimerHandle);

            FTimerHandle NextRoundHandle;
            GetWorld()->GetTimerManager().SetTimer(NextRoundHandle, [this]()
                {
                    PreCountdown = 2;
                    Countdown = 60;
                    StartPreCountdown();
                }, 0.5f, false);
        }, 0.5f, false);
}

void AMainGameState::ShowFinalFightWidget()
{
    if (!HasAuthority()) return;

    MulticastShowFinalFightWidget();
}

void AMainGameState::MulticastResetCollisionDebug_Implementation()
{
    if (UCollisionDebugSubsystem* CollisionDebug = GetWorld()->GetSubsystem<UCollisionDebugSubsystem>())
    {
        CollisionDebug->ResetToDefault();
    }
}

void AMainGameState::MulticastShowFinalFightWidget_Implementation()
{
    // Reset cajas de colisión (F) al terminar la pelea para que el toggle funcione en la siguiente
    MulticastResetCollisionDebug_Implementation();

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        if (TSubclassOf<UFinalFightWidget> FinalFightWidgetClass = LoadClass<UFinalFightWidget>(
            nullptr, TEXT("/Game/Blueprints/Widgets/WBP_FinalFightWidget.WBP_FinalFightWidget_C")))
        {
            UFinalFightWidget* FinalWidget = CreateWidget<UFinalFightWidget>(PC, FinalFightWidgetClass);
            if (FinalWidget)
            {
                FinalWidget->AddToViewport(999);
                PC->bShowMouseCursor = true;
                PC->SetInputMode(FInputModeUIOnly());
            }
        }
    }
}

void AMainGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMainGameState, FightCamRef);
    DOREPLIFETIME(AMainGameState, PlayerOneCharacter);
    DOREPLIFETIME(AMainGameState, PlayerTwoCharacter);
    DOREPLIFETIME(AMainGameState, Countdown);
    DOREPLIFETIME(AMainGameState, PreCountdown);
    DOREPLIFETIME(AMainGameState, bFightStarted);
    DOREPLIFETIME(AMainGameState, CurrentRound);
    DOREPLIFETIME(AMainGameState, PlayerOneRoundsWon);
    DOREPLIFETIME(AMainGameState, PlayerTwoRoundsWon);
    DOREPLIFETIME(AMainGameState, LastRoundNumber);
    DOREPLIFETIME(AMainGameState, LastRoundWinner);
    DOREPLIFETIME(AMainGameState, RoundOneDuration);
    DOREPLIFETIME(AMainGameState, RoundTwoDuration);
    DOREPLIFETIME(AMainGameState, PlayerOneRoundOneDamage);
    DOREPLIFETIME(AMainGameState, PlayerOneRoundTwoDamage);
    DOREPLIFETIME(AMainGameState, PlayerTwoRoundOneDamage);
    DOREPLIFETIME(AMainGameState, PlayerTwoRoundTwoDamage);
}

void AMainGameState::OnRep_Players()
{
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        if (AThirdCharacterPlayerController* PC = Cast<AThirdCharacterPlayerController>(*It))
        {
            if (PC->HUDWidgetRef)
            {
                AIO89Character* MyChar = Cast<AIO89Character>(PC->GetPawn());
                AIO89Character* Opponent = nullptr;

                if (MyChar == PlayerOneCharacter)
                    Opponent = PlayerTwoCharacter;
                else if (MyChar == PlayerTwoCharacter)
                    Opponent = PlayerOneCharacter;

                PC->HUDWidgetRef->SetCharacters(MyChar, Opponent);
            }
        }
    }
}

void AMainGameState::HandleCountdownFinished()
{
    if (!HasAuthority()) return;

    // Calculate round duration
    float RoundDuration = 0.f;
    if (CurrentRound == 1)
    {
        RoundDuration = GetWorld()->GetTimeSeconds() - RoundOneStartTime;
        RoundOneDuration = RoundDuration;
    }
    else if (CurrentRound == 2)
    {
        RoundDuration = GetWorld()->GetTimeSeconds() - RoundTwoStartTime;
        RoundTwoDuration = RoundDuration;
    }

    // Calculate damage dealt this round
    // PlayerOneRoundXDamage = damage dealt BY PlayerOne (health lost by PlayerTwo)
    // PlayerTwoRoundXDamage = damage dealt BY PlayerTwo (health lost by PlayerOne)
    if (PlayerOneCharacter && PlayerTwoCharacter)
    {
        float PlayerOneDamageDealt = PlayerTwoHealthAtRoundStart - PlayerTwoCharacter->Health;
        float PlayerTwoDamageDealt = PlayerOneHealthAtRoundStart - PlayerOneCharacter->Health;

        if (CurrentRound == 1)
        {
            PlayerOneRoundOneDamage = PlayerOneDamageDealt;
            PlayerTwoRoundOneDamage = PlayerTwoDamageDealt;
        }
        else if (CurrentRound == 2)
        {
            PlayerOneRoundTwoDamage = PlayerOneDamageDealt;
            PlayerTwoRoundTwoDamage = PlayerTwoDamageDealt;
        }
    }

    DetermineRoundWinner();
}

void AMainGameState::MulticastOnCountdownFinished_Implementation()
{
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
        PC->SetPause(true);
}

void AMainGameState::HandlePlayerDeath(AMainCharacter* DeadPlayer)
{
    if (!HasAuthority() || !DeadPlayer) return;

    UE_LOG(LogTemp, Warning, TEXT("[DEATH_VIEW] HandlePlayerDeath DeadPlayer=%s (Authority), calling MulticastOnPlayerDeath"), DeadPlayer ? *DeadPlayer->GetName() : TEXT("null"));

    // Calculate round duration
    float RoundDuration = 0.f;
    if (CurrentRound == 1)
    {
        RoundDuration = GetWorld()->GetTimeSeconds() - RoundOneStartTime;
        RoundOneDuration = RoundDuration;
    }
    else if (CurrentRound == 2)
    {
        RoundDuration = GetWorld()->GetTimeSeconds() - RoundTwoStartTime;
        RoundTwoDuration = RoundDuration;
    }

    // Calculate damage dealt this round
    // PlayerOneRoundXDamage = damage dealt BY PlayerOne (health lost by PlayerTwo)
    // PlayerTwoRoundXDamage = damage dealt BY PlayerTwo (health lost by PlayerOne)
    if (PlayerOneCharacter && PlayerTwoCharacter)
    {
        float PlayerOneDamageDealt = PlayerTwoHealthAtRoundStart - PlayerTwoCharacter->Health;
        float PlayerTwoDamageDealt = PlayerOneHealthAtRoundStart - PlayerOneCharacter->Health;

        if (CurrentRound == 1)
        {
            PlayerOneRoundOneDamage = PlayerOneDamageDealt;
            PlayerTwoRoundOneDamage = PlayerTwoDamageDealt;
        }
        else if (CurrentRound == 2)
        {
            PlayerOneRoundTwoDamage = PlayerOneDamageDealt;
            PlayerTwoRoundTwoDamage = PlayerTwoDamageDealt;
        }
    }

    // Notificar a todos los clientes para que quien controlaba al muerto cambie la vista a la cámara de combate
    MulticastOnPlayerDeath(DeadPlayer);

    // Determine winner based on who died
    int32 RoundWinner = 0;
    if (DeadPlayer == PlayerOneCharacter)
    {
        RoundWinner = 2; // Player Two wins
    }
    else if (DeadPlayer == PlayerTwoCharacter)
    {
        RoundWinner = 1; // Player One wins
    }

    EndRound(RoundWinner);
}

void AMainGameState::MulticastOnPlayerDeath_Implementation(AMainCharacter* DeadPlayer)
{
    UE_LOG(LogTemp, Warning, TEXT("[DEATH_VIEW] MulticastOnPlayerDeath DeadPlayer=%s FightCamRef=%s (Role=%d)"),
        DeadPlayer ? *DeadPlayer->GetName() : TEXT("null"),
        FightCamRef ? *FightCamRef->GetName() : TEXT("null"),
        (int32)GetLocalRole());

    if (!DeadPlayer || !FightCamRef)
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEATH_VIEW] Salida temprana: DeadPlayer o FightCamRef null"));
        return;
    }

    AController* Ctrl = DeadPlayer->GetController();
    APlayerController* PC = Cast<APlayerController>(Ctrl);
    const bool bIsLocal = PC && PC->IsLocalController();

    UE_LOG(LogTemp, Warning, TEXT("[DEATH_VIEW] Controller=%s PC=%s IsLocalController=%d"),
        Ctrl ? *Ctrl->GetName() : TEXT("null"),
        PC ? *PC->GetName() : TEXT("null"),
        bIsLocal ? 1 : 0);

    if (PC && bIsLocal)
    {
        FViewTargetTransitionParams Params;
        Params.BlendTime = 0.25f;
        PC->SetViewTarget(FightCamRef, Params);
        UE_LOG(LogTemp, Warning, TEXT("[DEATH_VIEW] SetViewTarget(FightCamRef) ejecutado en este PC"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[DEATH_VIEW] NO se llama SetViewTarget (no es el PC local del muerto)"));
    }
}