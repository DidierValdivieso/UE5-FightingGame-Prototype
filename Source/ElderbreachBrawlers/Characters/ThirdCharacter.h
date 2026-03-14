#pragma once

#include "CoreMinimal.h"
#include "MainCharacter.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "ThirdCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
class UBoxComponent;
class AProximityAttackActor;
class ABlockHitboxActor;

UCLASS()
class ELDERBREACHBRAWLERS_API AThirdCharacter : public AMainCharacter
{
	GENERATED_BODY()
	
public:
	AThirdCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	void ClampStats();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		class AController* EventInstigator,
		AActor* DamageCauser
	) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
	bool bIsDead = false;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float AttackCooldown = 1.0f;

	float LastAttackTime = -100.f;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void OnMoveReleased(const FInputActionValue& Value);
	void StartBlockByInput(const FInputActionValue& Value);
	void OnBlockReleased(const FInputActionValue& Value);
	virtual void Move(const FInputActionValue& Value) override;

	bool CanMoveTowards(const FVector& Direction) const;

	UFUNCTION(Server, Reliable)
	void ServerSetBlocking(bool bNewBlocking);

	UFUNCTION(Server, Reliable)
	void ServerEnableBlockHitbox(FVector SocketLocation, FRotator SocketRotation, float Duration);

	UFUNCTION(Server, Reliable)
	void ServerDisableBlockHitbox();

	UFUNCTION(Server, Reliable)
	void ServerStartBlockByInput();

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Block")
	float DefaultBlockWindow = 0.28f;

	/** Duración de la animación de reacción al golpe (hit react). Debe ser >= duración real del anim (ej. 14 frames @ 30fps = 14/30 ≈ 0.47s). Si es menor, se corta a mitad y se ve un salto. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|HitReact", meta = (ClampMin = "0.1", ClampMax = "2.0"))
	float HitReactDuration = 0.47f;

protected:
	FTimerHandle BlockWindowTimerHandle;
	FTimerHandle BlockImpactResetHandle;
	FTimerHandle KnockbackClampHandle;
	FTimerHandle HitReactResetHandle;
	/** Tiempo (GetWorld()->GetTimeSeconds()) cuando empezó la hit reaction; solo para log de duración. */
	float HitReactStartTime = 0.f;
	void OnBlockWindowExpired();
	void ClearBlockImpact();
	void ClampKnockbackVerticalVelocity();
	/** Resetea la variable bIsHitReacting después de que termine la animación de reacción. */
	void ClearHitReact();
	bool bBlockStartedByInput = false;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UBoxComponent* Pushbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UBoxComponent* TorsoHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UBoxComponent* BlockHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UBoxComponent* RightFistHurtbox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float MaxAllowedDist = 700.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	UAnimMontage* PunchMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* IA_Punch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* IA_Block;

	UPROPERTY(ReplicatedUsing = OnRep_OtherPlayer)
	AThirdCharacter* OtherPlayer;

	UFUNCTION()
	void OnRep_OtherPlayer();

	void PlayBlockAnimation();

	void OnSuccessfulBlock(AActor* Attacker);

	bool ArePushBoxesOverlapping(AThirdCharacter* Other) const;
	void ResolvePush(AThirdCharacter* Other);

	void Punch();

	void ResetPunch();

	void HandlePunch_Server();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayPunch();

	bool TryReceiveHit(AThirdCharacter* Attacker, float Damage);

	UFUNCTION(Server, Reliable)
	void ServerTryReceiveHit(AThirdCharacter* Attacker, float Damage);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitReact();

	void StopBlockAnimation();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Combat|Block")
	bool bCanBlockDamage = false;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Combat|Block")
	bool bCanBlock = false;

	TSet<AActor*> AlreadyHitActors;

	UPROPERTY()
	UBoxComponent* AttackHitbox;

	UFUNCTION(Server, Reliable)
	void ServerPunch();

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Health;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxHealth;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Energy;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxEnergy;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float Stamina;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Stats")
	float MaxStamina;

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetHealthPercent() const { return MaxHealth > 0 ? static_cast<float>(Health) / static_cast<float>(MaxHealth) : 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetEnergyPercent() const { return MaxEnergy > 0 ? static_cast<float>(Energy) / static_cast<float>(MaxEnergy) : 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Stats")
	float GetStaminaPercent() const { return MaxStamina > 0 ? static_cast<float>(Stamina) / static_cast<float>(MaxStamina) : 0.0f; }

	UFUNCTION(BlueprintCallable)
	void ApplyDamage(float DamageAmount);

	bool TryConsumeStamina(float Amount);

	bool TryConsumeEnergy(float Amount);

	void Die();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void Revive();

	void DisableAllHitboxes();

	UFUNCTION(BlueprintCallable)
	void OnSuccessfulHit();

	UFUNCTION(BlueprintCallable)
	void RegenerateStamina(float DeltaTime);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastDie();

	bool IsFacingAttacker(ACharacter* Attacker) const;

	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsPunching;

	UFUNCTION(Server, Reliable)
	void ServerPlayBlock();

	FTimerHandle PunchResetHandle;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsBlocking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bBlockImpact = false;

	/** True cuando el personaje está reproduciendo una animación de reacción al golpe (hit react). Se activa en MulticastPlayHitReact y se resetea automáticamente. */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsHitReacting = false;

	UFUNCTION()
	bool IsBlocking() const { return bIsBlocking; }

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayBlock();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	UAnimMontage* BlockMontage;

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayBlockImpact();

	UFUNCTION(Server, Reliable)
	void ServerStopBlock();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopBlock();

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Block")
	TSubclassOf<class ABlockHitboxActor> BlockHitboxClass;

	UPROPERTY()
	ABlockHitboxActor* ActiveBlockHitbox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bHasIncomingAttack;

	UPROPERTY(VisibleAnywhere)
	AThirdCharacter* IncomingFrom;

	void NotifyIncomingAttack(AThirdCharacter* From);
	void OnIncomingAttackNow(AThirdCharacter* From);
	void ClearIncomingAttack();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsPunchStarting = false;

	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TSubclassOf<AProximityAttackActor> ProximityAttackClass;

	UPROPERTY()
	AProximityAttackActor* ActiveProximityAttack;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Proximity")
	float ProximitySpawnDistance = 60.f;

	UPROPERTY(EditDefaultsOnly, Category = "Combat|Proximity")
	float ProximitySpawnHeight = 125.f;

	UFUNCTION(Server, Reliable)
	void ServerSpawnProximity();

	UFUNCTION(Server, Reliable)
	void ServerDestroyProximity();

private:
	float LastBlockStateLogTime = -1.f;
};