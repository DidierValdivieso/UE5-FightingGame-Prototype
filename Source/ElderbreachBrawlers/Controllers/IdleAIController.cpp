#include "IdleAIController.h"
#include "../Characters/IO89Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AIdleAIController::AIdleAIController()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AIdleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	// Detener completamente el movimiento del personaje
	if (AIO89Character* ThirdChar = Cast<AIO89Character>(InPawn))
	{
		if (UCharacterMovementComponent* MovementComp = ThirdChar->GetCharacterMovement())
		{
			MovementComp->StopMovementImmediately();
			MovementComp->DisableMovement();
			MovementComp->SetComponentTickEnabled(false);
		}
	}
}

