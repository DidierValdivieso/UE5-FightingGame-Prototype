#include "MainCharacter.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include <ElderbreachBrawlers/MainGameState.h>

AMainCharacter::AMainCharacter()
{
    PrimaryActorTick.bCanEverTick = true;

    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    GetCharacterMovement()->MaxWalkSpeed = 600.f;
    GetCharacterMovement()->JumpZVelocity = 500.f;
    GetCharacterMovement()->AirControl = 0.3f;
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->SetPlaneConstraintNormal(FVector(1, 0, 0));

    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetCapsuleComponent()->SetCollisionObjectType(ECC_Pawn);
    GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECR_Block);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
    GetCapsuleComponent()->SetReceivesDecals(false);
}

void AMainCharacter::BeginPlay()
{
    Super::BeginPlay();
}

void AMainCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void AMainCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (IA_Move)
        {
            EIC->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AMainCharacter::Move);
        }

        if (IA_Jump)
        {
            EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &AMainCharacter::StartJump);
            EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &AMainCharacter::StopJump);
        }
    }
}

void AMainCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();
    if (!Controller) return;

    MovementVector.Y = 0.f;

    if (!FMath::IsNearlyZero(MovementVector.X))
    {
        AddMovementInput(FVector::RightVector, MovementVector.X);
    }
}

void AMainCharacter::StartJump(const FInputActionValue& Value)
{
    if (const AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (!GS->bFightStarted) return;
    }

    Jump();
}

void AMainCharacter::StopJump(const FInputActionValue& Value)
{
    StopJumping();
}

void AMainCharacter::OnSuccessfulBlock(AActor* Attacker)
{
}