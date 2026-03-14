#pragma once

#include "CoreMinimal.h"
#include "MainCharacter.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "IO89Character.generated.h"

class UInputMappingContext;
class UInputAction;
class UBoxComponent;
class AProximityAttackActor;
class ABlockHitboxActor;

UCLASS()
class ELDERBREACHBRAWLERS_API AIO89Character : public AMainCharacter
{
	GENERATED_BODY()
	
public:
	AIO89Character();

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

	UPROPERTY(EditDefaultsOnly, Category = "Combat", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float AttackCooldown = 0.4f;

	float LastAttackTime = -100.f;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	void OnMoveReleased(const FInputActionValue& Value);
	void StartBlockByInput(const FInputActionValue& Value);
	void OnBlockReleased(const FInputActionValue& Value);
	void OnCrouchStarted(const FInputActionValue& Value);
	void OnCrouchCompleted(const FInputActionValue& Value);
	virtual void Move(const FInputActionValue& Value) override;
	void StartJump(const FInputActionValue& Value) override;
	virtual void Jump() override;

	bool CanMoveTowards(const FVector& Direction) const;

	/** Devuelve true si el personaje esta muerto (para logica de respawn round 2). */
	UFUNCTION(BlueprintCallable, Category = "State")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(Server, Reliable)
	void ServerSetBlocking(bool bNewBlocking);

	UFUNCTION(Server, Reliable)
	void ServerEnableBlockHitbox(FVector SocketLocation, FRotator SocketRotation, float Duration);

	UFUNCTION(Server, Reliable)
	void ServerDisableBlockHitbox();

	/** Activa el bloqueo por input (tecla Q). Habilita hitbox sin ventana de tiempo; se desactiva al soltar Q. */
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
	/** Quita cualquier velocidad vertical positiva tras el knockback (evita elevación por motor/animación). */
	void ClampKnockbackVerticalVelocity();
	void ClearBlockImpact();
	/** Resetea la variable bIsHitReacting después de que termine la animación de reacción. */
	void ClearHitReact();
	/** True cuando el bloqueo se activó con Q (ServerStartBlockByInput). Evita que el NotifyEnd del BlockNotifyState desactive el bloqueo. */
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

	/** Duracion del punch para el timer. Ajustar a la duracion real de low_punch (reducir si tarda en volver a idle). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.2", ClampMax = "2.0"))
	float PunchAnimationDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* IA_Punch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* IA_Block;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* IA_Crouch;

	/** Input para alternar visibilidad de cajas de colisión (ej. F). Asignar en Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Input")
	UInputAction* IA_ToggleCollisionVisibility;

	/** Alterna mostrar/ocultar las cajas de colisión (Pushbox, TorsoHitbox, etc.). */
	void ToggleCollisionVisibility();

	UPROPERTY(ReplicatedUsing = OnRep_OtherPlayer)
	AIO89Character* OtherPlayer;

	UFUNCTION()
	void OnRep_OtherPlayer();

	void PlayBlockAnimation();

	void OnSuccessfulBlock(AActor* Attacker);

	bool ArePushBoxesOverlapping(AIO89Character* Other) const;
	void ResolvePush(AIO89Character* Other);

	void Punch();

	void ResetPunch();
	void OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void HandlePunch_Server();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayPunch();

	bool TryReceiveHit(AIO89Character* Attacker, float Damage);

	UFUNCTION(Server, Reliable)
	void ServerTryReceiveHit(AIO89Character* Attacker, float Damage);

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

	UFUNCTION(NetMulticast, Reliable)
	void MulticastRevive();

	/** Offset relativo del mesh respecto a la cápsula al revivir tras ragdoll (evita que salga muy alto). Coincide con el offset del mesh en el Blueprint (ej. -98). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat|Ragdoll")
	FVector ReviveMeshRelativeOffset = FVector(0.f, 0.f, -98.f);

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
	FTimerHandle DeathMontageDebugHandle;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsBlocking = false;

	/** True brevemente cuando se bloquea un golpe; el AnimInstance lo usa para defense_block_hit. */
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

	/** Animación de muerte (reemplaza ragdoll). Asignar AM_Death en Blueprint. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	UAnimMontage* DeathMontage;

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
	AIO89Character* IncomingFrom;

	void NotifyIncomingAttack(AIO89Character* From);
	void OnIncomingAttackNow(AIO89Character* From);
	void ClearIncomingAttack();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bIsPunchStarting = false;

	/** Velocidad de caminata hacia atrás (retroceder). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeedBackward = 100.f;

	/** Velocidad de caminata hacia adelante (avanzar). */
	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeedForward = 150.f;

	/** True cuando el personaje se aleja del enemigo (retrocediendo). Para blend space walk forward/backward. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsRetreating = false;

	/** Fuerza del impulso del salto (Z). Controla qué tan alto salta el personaje. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Jump", meta = (ClampMin = "0", ClampMax = "2000"))
	float JumpZVelocity = 400.f;

	/** Dash lógico: doble toque adelante/atrás. Ventana en segundos para considerar doble tap. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash", meta = (ClampMin = "0.1", ClampMax = "0.5"))
	float DoubleTapWindow = 0.25f;
	/** Velocidad horizontal aplicada al hacer dash. Más alto = más distancia. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash", meta = (ClampMin = "200", ClampMax = "2500"))
	float DashSpeed = 1200.f;
	/** Duración del dash en segundos; tras este tiempo se detiene el impulso. Más largo = más distancia. */
	UPROPERTY(EditDefaultsOnly, Category = "Movement|Dash", meta = (ClampMin = "0.08", ClampMax = "0.6"))
	float DashDuration = 0.28f;

	/** True mientras el personaje está en dash. Usar en state machine (estado Dash, vacío o con animación). */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Movement|Dash")
	bool bIsDashing = false;

	void PerformDash(int32 Direction);

	UFUNCTION(Server, Reliable)
	void ServerPerformDash(int32 Direction);

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
	int32 LastTapDirection = 0;
	float LastTapTime = -1.f;
	bool bHasReleasedSinceLastTap = false;
	FTimerHandle DashStopHandle;
	void OnDashDurationEnded();
	/** Para log periódico del estado de bloqueo (consola). */
	float LastBlockStateLogTime = -1.f;
};
