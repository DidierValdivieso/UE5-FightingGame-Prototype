#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "IO89AnimInstance.generated.h"

UCLASS()
class ELDERBREACHBRAWLERS_API UIO89AnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsCrouching = false;

	/** Magnitud de la velocidad horizontal. Usar en condiciones del state machine (ej: Idle->Walk cuando Speed > 1). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float Speed = 0.f;

	/** Velocidad con signo en dirección del personaje: positivo = adelante, negativo = atrás. Para blend space. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float ForwardSpeed = 0.f;

	/** True cuando el personaje se mueve (Speed > umbral). Para transiciones Idle/Walk. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

	/** Umbral mínimo de velocidad para considerar movimiento. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement", meta = (ClampMin = "0.1", ClampMax = "50"))
	float MovingThreshold = 3.f;

	// --- Jump (para state machine) ---

	/** True cuando el personaje está en el aire (saltando o cayendo). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bIsInAir = false;

	/** Velocidad vertical (Z). Positivo = subiendo, negativo = bajando. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	float VelocityZ = 0.f;

	/** True cuando está en el aire subiendo (VelocityZ > umbral). Para jump_start y jump_up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bIsAscending = false;

	/** True cuando está en el aire bajando (VelocityZ < -umbral). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bIsDescending = false;

	/** True cuando está cerca del suelo (trace hacia abajo). Para activar jump_landing solo al llegar. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bIsNearGround = false;

	/** True cuando debe mostrarse jump_landing: cayendo Y cerca del suelo. Usar en transición jump_up→jump_landing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bShouldPlayLanding = false;

	/** Distancia al suelo (desde centro) para activar landing. ~100 = pies casi tocando. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (ClampMin = "50", ClampMax = "400"))
	float LandingActivationDistance = 120.f;

	/** True cuando está en suelo. Para transición jump_landing → idle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bIsOnGround = true;

	/** Umbral de VelocityZ para considerar ascending/descending (evita jitter en el pico). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (ClampMin = "0", ClampMax = "50"))
	float JumpVelocityThreshold = 10.f;

	/** Si |VelocityZ| > este valor, forzar bIsOnGround=false (evita flicker de IsMovingOnGround durante el salto). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (ClampMin = "50", ClampMax = "1000"))
	float InAirVelocityThreshold = 150.f;

	/** Tiempo mínimo en suelo antes de permitir jump_landing → idle. Evita que al spamear salto la animación se corte. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Jump", meta = (ClampMin = "0", ClampMax = "0.5"))
	float MinGroundTimeBeforeIdle = 0.08f;

	/** True cuando podemos pasar de jump_landing a idle (estuvimos en suelo el tiempo mínimo). Usar en transición jump_landing→idle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bCanExitJumpToIdle = false;

	/** True en el pico del salto (ni subiendo ni bajando). Mantener jump_up en esta zona. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Jump")
	bool bIsAtApex = false;

	/** Si true, imprime en consola las transiciones: JUMP_START, JUMP_UP, JUMP_LANDING, → IDLE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
	bool bLogStateTransitions = false;

	/** True cuando el personaje está puncheando. Copiado desde IO89Character en NativeUpdateAnimation. Usar en Blend/State Machine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsPunching = false;

	/** True cuando el personaje está bloqueando (tecla Q). Usar en state machine: defense_block_start, defense_block_idle. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsBlocking = false;

	/** True durante un frame cuando se recibe un golpe bloqueado (reacción defense_block_hit). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bBlockImpact = false;

	/** True cuando el personaje está en dash. Usar en state machine: estado Dash (vacío o con animación). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsDashing = false;

	/** True brevemente (1-2 frames) cuando se sale de bloqueo con movimiento activo. Fuerza transición Idle→Walk en Movement State Machine. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bForceWalkAfterBlock = false;

	/** True cuando el personaje está reproduciendo una animación de reacción al golpe (hit react). Usar en state machine de hit reactions. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsHitReacting = false;

private:
	float TimeOnGroundAccum = 0.f;
	bool bPrevIsAscending = false;
	bool bPrevIsDescending = false;
	bool bPrevShouldPlayLanding = false;
	bool bPrevIsAtApex = false;
	bool bPrevCanExitJumpToIdle = false;
	bool bPrevIsMoving = false;
	int32 ForceWalkFrameCount = 0;
};
