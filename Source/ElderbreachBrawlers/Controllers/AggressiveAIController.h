#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AggressiveAIController.generated.h"

class AIO89Character;

UCLASS()
class ELDERBREACHBRAWLERS_API AAggressiveAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	AAggressiveAIController();

	void StartCombatMode();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	float TimeUntilNextAction;
	int32 CurrentAction;

	float LastAttackTime;
	float AttackCooldown;
	float LastBlockTime;
	float BlockCooldown;
	int32 ConsecutiveAttacks;
	float ReactionTime;
	bool bIsReactingToAttack;
	
	// Variables para comportamiento más natural
	float LastJumpTime;
	float JumpCooldown;
	float LastLateralMoveTime;
	float LateralMoveCooldown;
	float ActionVariationTimer;
	float NaturalPauseTimer;
	bool bInNaturalPause;

	float LastDashTime;
	float DashCooldown;
	float CrouchDodgeEndTime;
	float LastCrouchDodgeTime; // when we finished crouch dodge, to avoid dash right after

	/** Si > 0, bloqueo proactivo: soltar guardia cuando CurrentTime >= BlockReleaseTime */
	float BlockReleaseTime;

	static constexpr int32 MaxConsecutiveAttacks = 2;
	static constexpr float DashJumpSeparation = 2.0f;  // no dash after jump / no jump after dash
	static constexpr float DashCrouchSeparation = 1.8f; // no dash after crouch dodge

	// Funciones auxiliares
	void EvaluateCombatSituation(AIO89Character* MyChar, AIO89Character* Enemy, float Distance);
	bool ShouldBlock(AIO89Character* Enemy);
	bool ShouldRetreat(AIO89Character* Enemy, float Distance);
	/** True if enemy is standing and hittable: idle stand or blocking (not punching, dashing or crouching). */
	bool IsEnemyIdleStand(AIO89Character* Enemy) const;
	void PerformStrategicMovement(AIO89Character* MyChar, const FVector& Dir, float Distance);
};