#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Actors/FightingCameraActor.h"
#include "Structs/CombatTestStructs.h"
#include "MainGameState.generated.h"

class UHUDWidget;
class AIO89Character;
class AMainCharacter;
class UMaterialInterface;

UCLASS()
class ELDERBREACHBRAWLERS_API AMainGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
    UPROPERTY(ReplicatedUsing = OnRep_FightCam)
    AFightingCameraActor* FightCamRef;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(ReplicatedUsing = OnRep_Players)
    AIO89Character* PlayerOneCharacter;

    UPROPERTY(ReplicatedUsing = OnRep_Players)
    AIO89Character* PlayerTwoCharacter;

    UFUNCTION()
    void OnRep_Players();

    void HandleCountdownFinished();

    void HandlePlayerDeath(AMainCharacter* DeadPlayer);

    void UpdateCountdown();

    UPROPERTY(ReplicatedUsing = OnRep_Countdown, VisibleAnywhere, BlueprintReadOnly, Category = "Countdown")
    int32 Countdown;

    UPROPERTY(Replicated)
    bool bIsRunning = false;

    UFUNCTION()
    void OnRep_Countdown();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnCountdownFinished();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastOnPlayerDeath(AMainCharacter* DeadPlayer);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastShowFinalFightWidget();

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly)
    int32 PreCountdown;

    FTimerHandle PreCountdownTimerHandle;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Fight")
    bool bFightStarted;

    // Round System
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    int32 CurrentRound = 1;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    int32 PlayerOneRoundsWon = 0;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    int32 PlayerTwoRoundsWon = 0;

    /** Último round que terminó (1 o 2); 0 = no mostrar decisión */
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    int32 LastRoundNumber = 0;

    /** Ganador del último round: 0=draw, 1=PlayerOne, 2=PlayerTwo */
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    int32 LastRoundWinner = 0;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float RoundOneDuration = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float RoundTwoDuration = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float PlayerOneRoundOneDamage = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float PlayerOneRoundTwoDamage = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float PlayerTwoRoundOneDamage = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float PlayerTwoRoundTwoDamage = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float RoundOneStartTime = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float RoundTwoStartTime = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float PlayerOneHealthAtRoundStart = 0.f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    float PlayerTwoHealthAtRoundStart = 0.f;

    /** Posiciones iniciales para resetear al empezar cada round */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    FVector PlayerOneSpawnLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    FRotator PlayerOneSpawnRotation = FRotator::ZeroRotator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    FVector PlayerTwoSpawnLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Rounds")
    FRotator PlayerTwoSpawnRotation = FRotator::ZeroRotator;

    void SetInitialSpawnPositions(const FVector& P1Loc, const FRotator& P1Rot, const FVector& P2Loc, const FRotator& P2Rot);

    /** Guarda clase, material y tipo de controller por jugador para respawn round 2 (nuevo controller como primer spawn). */
    void SetParticipantRespawnInfo(TSubclassOf<APawn> P1Class, TSubclassOf<APawn> P2Class,
        UMaterialInterface* P1BodyMat, int32 P1BodySlot, UMaterialInterface* P2BodyMat, int32 P2BodySlot,
        ECombatControllerType P1ControllerType, ECombatControllerType P2ControllerType);

private:
    UPROPERTY()
    TSubclassOf<APawn> PlayerOnePawnClass;

    UPROPERTY()
    TSubclassOf<APawn> PlayerTwoPawnClass;

    UPROPERTY()
    UMaterialInterface* PlayerOneBodyMaterialOverride = nullptr;

    UPROPERTY()
    UMaterialInterface* PlayerTwoBodyMaterialOverride = nullptr;

    int32 PlayerOneBodyMaterialSlotIndex = 0;
    int32 PlayerTwoBodyMaterialSlotIndex = 0;

    ECombatControllerType PlayerOneControllerType = ECombatControllerType::AggressiveAI;
    ECombatControllerType PlayerTwoControllerType = ECombatControllerType::AggressiveAI;

    /** 1 = respawn P1, 2 = respawn P2, 0 = nadie murio (empate). */
    int32 PendingRespawnSlot = 0;

public:

    UFUNCTION()
    void ResetTestData();

    /** Limpia todos los timers activos del contador (útil para restart) */
    void ClearAllTimers();

    void StartPreCountdown();

    void UpdatePreCountdown();

    void StartRound(int32 RoundNumber);

    void EndRound(int32 RoundWinner); // 0 = tie, 1 = PlayerOne, 2 = PlayerTwo

    void DetermineRoundWinner();

    void ShowFinalFightWidget();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastResetCollisionDebug();

protected:
    UFUNCTION()
    void OnRep_FightCam();

    virtual void BeginPlay() override;

    

    

    FTimerHandle CountdownTimerHandle;
};
