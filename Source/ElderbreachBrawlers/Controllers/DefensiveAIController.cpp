#include "DefensiveAIController.h"
#include "../Characters/IO89Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <ElderbreachBrawlers/MainGameState.h>
#include "TimerManager.h"

ADefensiveAIController::ADefensiveAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    TimeUntilNextAction = 0.f;
    bIsBlockingAI = false;
}

void ADefensiveAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>();
    if (GS && !GS->bFightStarted)
    {
        return;
    }

    AIO89Character* MyChar = Cast<AIO89Character>(GetPawn());
    if (!MyChar || !MyChar->OtherPlayer) return;

    AIO89Character* Enemy = MyChar->OtherPlayer;

    FVector TargetLoc = Enemy->GetActorLocation();
    FVector MyLoc = MyChar->GetActorLocation();

    FVector Dir = TargetLoc - MyLoc;
    Dir.Z = 0.f;
    float Distance = Dir.Size();
    Dir.Normalize();

    FRotator LookRot = (TargetLoc - MyLoc).Rotation();
    LookRot.Pitch = 0.f;
    FRotator SmoothRot = FMath::RInterpTo(MyChar->GetActorRotation(), LookRot, DeltaSeconds, 6.0f);
    MyChar->SetActorRotation(SmoothRot);

    if (Distance > 180.f)
    {
        if (MyChar->CanMoveTowards(Dir))
        {
            MyChar->AddMovementInput(Dir, 0.8f);
        }
    }

    if (bIsBlockingAI)
    {
        return;
    }

    TimeUntilNextAction -= DeltaSeconds;
    if (TimeUntilNextAction <= 0.f)
    {
        
        if (Enemy->bIsPunching && bCanBlock && FMath::RandRange(0, 100) < 60)
        {
            CurrentAction = 99;
            TimeUntilNextAction = 1.0f;
        }
        else
        {
            if (Distance < 250.f)
            {
                int32 Rand = FMath::RandRange(0, 99);
                if (Rand < 15) CurrentAction = 2;
                else if (Rand < 50) CurrentAction = 1;
                else CurrentAction = 3;
            }
            else if (Distance >= 250.f && Distance < 600.f)
            {
                int32 Rand = FMath::RandRange(0, 99);
                if (Rand < 30) CurrentAction = 1;
                else if (Rand < 60) CurrentAction = 3;
                else CurrentAction = 0;
            }
            else
            {
                CurrentAction = (FMath::RandRange(0, 100) < 70) ? 3 : 1;
            }

            TimeUntilNextAction = FMath::RandRange(0.8f, 1.5f);
        }
    }

    switch (CurrentAction)
    {
    case 0: // Idle (side movement)
    {
        FVector SideDir = FVector::CrossProduct(FVector::UpVector, Dir);
        if (FMath::RandBool()) SideDir *= -1;
        MyChar->AddMovementInput(SideDir, 0.3f);
    }
    break;

    case 1: // backward
    {
        FVector BackDir = -Dir;
        MyChar->AddMovementInput(BackDir, 1.0f);

        if (FMath::RandRange(0, 100) < 30)
        {
            FVector SideDir = FVector::CrossProduct(FVector::UpVector, Dir);
            if (FMath::RandBool()) SideDir *= -1;
            MyChar->AddMovementInput(SideDir, 0.5f);
        }
    }
    break;

    case 2: // jump
        if (MyChar->GetCharacterMovement()->IsMovingOnGround())
        {
            MyChar->Jump();
        }
        break;

    case 3: // move slow forward
        MyChar->AddMovementInput(Dir, 0.4f);
        break;

    case 99:
        StartAIDefense(MyChar);
        CurrentAction = -1;
        break;
    }
}

void ADefensiveAIController::StartAIDefense(AIO89Character* MyChar)
{
    if (!MyChar || bIsBlockingAI || !bCanBlock) return;

    bIsBlockingAI = true;
    bCanBlock = false;
    MyChar->ServerSetBlocking(true);

    MyChar->GetCharacterMovement()->StopMovementImmediately();

    float BlockDuration = 2.0f;
    GetWorld()->GetTimerManager().SetTimer(BlockTimerHandle, [this, MyChar]()
        {
            MyChar->ServerSetBlocking(false);
            bIsBlockingAI = false;

            GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
                {
                    GetWorld()->GetTimerManager().SetTimer(
                        CooldownTimerHandle,
                        [this]() { bCanBlock = true; },
                        BlockCooldown,
                        false
                    );
                });
        }, BlockDuration, false);
}

void ADefensiveAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
}

void ADefensiveAIController::StartCombatMode()
{
    AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>();
    if (GS)
    {
        GS->bFightStarted = true;
    }

    TimeUntilNextAction = FMath::RandRange(0.2f, 0.8f);
}
