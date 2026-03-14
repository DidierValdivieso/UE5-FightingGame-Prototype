#include "IO89AnimInstance.h"
#include "../Characters/IO89Character.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

void UIO89AnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ACharacter* Character = Cast<ACharacter>(TryGetPawnOwner());
	if (!Character)
	{
		Speed = 0.f;
		ForwardSpeed = 0.f;
		bIsMoving = false;
		bIsPunching = false;
		bIsDashing = false;
		bIsInAir = false;
		VelocityZ = 0.f;
		bIsAscending = false;
		bIsDescending = false;
		bIsOnGround = true;
		TimeOnGroundAccum = 0.f;
		bCanExitJumpToIdle = false;
		bIsAtApex = false;
		bPrevIsAscending = false;
		bPrevIsDescending = false;
		bPrevIsAtApex = false;
		bPrevCanExitJumpToIdle = false;
		bPrevIsMoving = false;
		bIsNearGround = false;
		bShouldPlayLanding = false;
		bPrevShouldPlayLanding = false;
		bIsHitReacting = false;
		return;
	}

	if (AIO89Character* IO89Char = Cast<AIO89Character>(Character))
	{
		bIsPunching = IO89Char->bIsPunching;
		bIsDashing = IO89Char->bIsDashing;
		bIsBlocking = IO89Char->bIsBlocking;
		bBlockImpact = IO89Char->bBlockImpact;
		bIsHitReacting = IO89Char->bIsHitReacting;
	}
	else
	{
		bIsDashing = false;
		bIsBlocking = false;
		bBlockImpact = false;
		bIsHitReacting = false;
	}

	if (UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement())
	{
		bIsCrouching = MoveComp->IsCrouching();

		// Velocidad para state machine y blend space
		const FVector Velocity = Character->GetVelocity();
		const FVector VelocityXY = FVector(Velocity.X, Velocity.Y, 0.f);
		Speed = VelocityXY.Size();

		// ForwardSpeed: positivo = hacia adelante, negativo = hacia atrás (para blend space walk)
		FVector Forward = Character->GetActorForwardVector();
		Forward.Z = 0.f;
		Forward.Normalize();
		const float DotForward = (Speed > 0.f) ? FVector::DotProduct(VelocityXY.GetSafeNormal(), Forward) : 0.f;
		if (AIO89Character* IO89Char = Cast<AIO89Character>(Character))
		{
			// Movimiento lateral (ej. jugador con Left/Right): dot ≈ 0 → usar intención retreating/advancing
			const float LateralThreshold = 0.35f;
			if (Speed > 0.f && FMath::Abs(DotForward) < LateralThreshold)
			{
				ForwardSpeed = IO89Char->bIsRetreating ? -Speed : Speed;
			}
			else
			{
				ForwardSpeed = DotForward * Speed;
			}
		}
		else
		{
			ForwardSpeed = DotForward * Speed;
		}

		bIsMoving = Speed > MovingThreshold;

		// Limpiar bForceWalkAfterBlock después de 2 frames (para forzar Walk solo brevemente al salir de bloqueo)
		if (bForceWalkAfterBlock)
		{
			ForceWalkFrameCount++;
			if (ForceWalkFrameCount >= 2)
			{
				bForceWalkAfterBlock = false;
				ForceWalkFrameCount = 0;
			}
		}
		else
		{
			ForceWalkFrameCount = 0; // Reset si no está activo
		}

		// Jump
		bIsInAir = MoveComp->IsFalling();
		VelocityZ = Velocity.Z;
		// Si |VelocityZ| es grande, estamos claramente en el aire - evita flicker de IsMovingOnGround
		const bool bVelocitySaysInAir = FMath::Abs(VelocityZ) > InAirVelocityThreshold;
		bIsOnGround = bVelocitySaysInAir ? false : MoveComp->IsMovingOnGround();
		bIsAscending = bIsInAir && VelocityZ > JumpVelocityThreshold;
		bIsDescending = bIsInAir && VelocityZ < -JumpVelocityThreshold;
		bIsAtApex = bIsInAir && !bIsAscending && !bIsDescending;

		// Cerca del suelo? (trace hacia abajo) - para activar landing solo al llegar
		bIsNearGround = false;
		if (bIsInAir && bIsDescending)
		{
			UWorld* World = Character->GetWorld();
			if (World)
			{
				const FVector Start = Character->GetActorLocation();
				const FVector End = Start - FVector(0.f, 0.f, LandingActivationDistance + 100.f);
				FHitResult Hit;
				FCollisionQueryParams Params;
				Params.AddIgnoredActor(Character);
				if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
				{
					const float DistToGround = Start.Z - Hit.ImpactPoint.Z;
					bIsNearGround = DistToGround <= LandingActivationDistance && DistToGround >= 0.f;
				}
			}
		}
		bShouldPlayLanding = bIsDescending && bIsNearGround;

		// Tiempo en suelo para permitir transición jump_landing → idle (evita corte al spamear)
		if (bIsOnGround)
		{
			TimeOnGroundAccum += DeltaSeconds;
			bCanExitJumpToIdle = TimeOnGroundAccum >= MinGroundTimeBeforeIdle;
		}
		else
		{
			TimeOnGroundAccum = 0.f;
			bCanExitJumpToIdle = false;
		}

		// Log transiciones de estado (JUMP_START, JUMP_UP, JUMP_LANDING, → IDLE)
		if (bLogStateTransitions)
		{
			const FString OwnerName = Character->GetName();
			if (bIsAscending != bPrevIsAscending && bIsAscending)
			{
				UE_LOG(LogTemp, Log, TEXT("[%s] JUMP_START (inicio) | VelZ=%.0f"), *OwnerName, VelocityZ);
			}
			if (bIsAtApex != bPrevIsAtApex && bIsAtApex)
			{
				UE_LOG(LogTemp, Log, TEXT("[%s] JUMP_UP | VelZ=%.0f"), *OwnerName, VelocityZ);
			}
			if (bShouldPlayLanding != bPrevShouldPlayLanding && bShouldPlayLanding)
			{
				UE_LOG(LogTemp, Log, TEXT("[%s] JUMP_LANDING (cerca suelo) | VelZ=%.0f"), *OwnerName, VelocityZ);
			}
			if (bCanExitJumpToIdle != bPrevCanExitJumpToIdle && bCanExitJumpToIdle)
			{
				UE_LOG(LogTemp, Log, TEXT("[%s] → IDLE (desde landing)"), *OwnerName);
			}
			if (bIsMoving != bPrevIsMoving && !bIsMoving && bIsOnGround)
			{
				UE_LOG(LogTemp, Log, TEXT("[%s] → IDLE (desde walk)"), *OwnerName);
			}
			bPrevIsAscending = bIsAscending;
			bPrevIsDescending = bIsDescending;
			bPrevShouldPlayLanding = bShouldPlayLanding;
			bPrevIsAtApex = bIsAtApex;
			bPrevCanExitJumpToIdle = bCanExitJumpToIdle;
			bPrevIsMoving = bIsMoving;
		}
	}
}

