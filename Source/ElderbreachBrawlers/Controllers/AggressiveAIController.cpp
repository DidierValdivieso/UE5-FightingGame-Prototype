#include "AggressiveAIController.h"
#include "../Characters/IO89Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <ElderbreachBrawlers/MainGameState.h>

AAggressiveAIController::AAggressiveAIController()
{
    PrimaryActorTick.bCanEverTick = true;
    TimeUntilNextAction = 0.f;
    LastAttackTime = -1000.f;
    AttackCooldown = 0.0f;
    LastBlockTime = -1000.f;
    BlockCooldown = 0.0f;
    ConsecutiveAttacks = 0;
    ReactionTime = 0.0f;
    bIsReactingToAttack = false;
    LastJumpTime = -1000.f;
    JumpCooldown = 0.0f;
    LastLateralMoveTime = -1000.f;
    LateralMoveCooldown = 0.0f;
    ActionVariationTimer = 0.0f;
    NaturalPauseTimer = 0.0f;
    bInNaturalPause = false;
    LastDashTime = -1000.f;
    DashCooldown = 0.0f;
    CrouchDodgeEndTime = 0.0f;
    LastCrouchDodgeTime = -1000.f;
    BlockReleaseTime = 0.0f;
}

void AAggressiveAIController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>();
    if (GS && !GS->bFightStarted)
    {
        static float LastLogTime = -999.f;
        if (GetWorld() && GetWorld()->GetTimeSeconds() - LastLogTime > 2.f)
        {
            LastLogTime = GetWorld()->GetTimeSeconds();
            UE_LOG(LogTemp, Warning, TEXT("[AggressiveAI] %s: Tick sale porque bFightStarted=false (CurrentRound=%d). GetPawn()=%s"), *GetName(), GS->CurrentRound, GetPawn() ? *GetPawn()->GetName() : TEXT("null"));
        }
        return;
    }

    AIO89Character* MyChar = Cast<AIO89Character>(GetPawn());
    if (!MyChar)
    {
        static float LastPawnLogTime = -999.f;
        if (GetWorld() && GetWorld()->GetTimeSeconds() - LastPawnLogTime > 2.f)
        {
            LastPawnLogTime = GetWorld()->GetTimeSeconds();
            UE_LOG(LogTemp, Warning, TEXT("[AggressiveAI] %s: Tick sale porque MyChar=null (GetPawn()=%s, no es IO89?)"), *GetName(), GetPawn() ? *GetPawn()->GetName() : TEXT("null"));
        }
        return;
    }
    if (!MyChar->OtherPlayer)
    {
        static float LastOtherLogTime = -999.f;
        if (GetWorld() && GetWorld()->GetTimeSeconds() - LastOtherLogTime > 2.f)
        {
            LastOtherLogTime = GetWorld()->GetTimeSeconds();
            UE_LOG(LogTemp, Warning, TEXT("[AggressiveAI] %s: Tick sale porque MyChar->OtherPlayer=null (MyChar=%s). Round 2 respawn sin enemigo?"), *GetName(), *MyChar->GetName());
        }
        return;
    }

    AIO89Character* Enemy = MyChar->OtherPlayer;
    static float LastRunLogTime = -999.f;
    if (GetWorld() && GetWorld()->GetTimeSeconds() - LastRunLogTime > 3.f)
    {
        LastRunLogTime = GetWorld()->GetTimeSeconds();
        UE_LOG(LogTemp, Log, TEXT("[AggressiveAI] %s: Tick EJECUTANDO (MyChar=%s Enemy=%s Distance=%.0f)"), *GetName(), *MyChar->GetName(), Enemy ? *Enemy->GetName() : TEXT("null"), Enemy ? FVector::Dist(MyChar->GetActorLocation(), Enemy->GetActorLocation()) : 0.f);
    }
    FVector TargetLoc = Enemy->GetActorLocation();
    FVector MyLoc = MyChar->GetActorLocation();

    FVector Dir = TargetLoc - MyLoc;
    Dir.Z = 0.f;
    float Distance = Dir.Size();
    Dir.Normalize();

    // Rotaci�n suave hacia el enemigo
    FRotator LookRot = (TargetLoc - MyLoc).Rotation();
    LookRot.Pitch = 0.f;
    FRotator SmoothRot = FMath::RInterpTo(MyChar->GetActorRotation(), LookRot, DeltaSeconds, 8.0f);
    MyChar->SetActorRotation(SmoothRot);

    // Actualizar cooldowns
    float CurrentTime = GetWorld()->GetTimeSeconds();
    AttackCooldown = FMath::Max(0.0f, AttackCooldown - DeltaSeconds);
    BlockCooldown = FMath::Max(0.0f, BlockCooldown - DeltaSeconds);
    ReactionTime = FMath::Max(0.0f, ReactionTime - DeltaSeconds);
    JumpCooldown = FMath::Max(0.0f, JumpCooldown - DeltaSeconds);
    LateralMoveCooldown = FMath::Max(0.0f, LateralMoveCooldown - DeltaSeconds);
    DashCooldown = FMath::Max(0.0f, DashCooldown - DeltaSeconds);
    ActionVariationTimer -= DeltaSeconds;
    NaturalPauseTimer -= DeltaSeconds;

    if (CrouchDodgeEndTime > 0.0f && CurrentTime >= CrouchDodgeEndTime)
    {
        MyChar->UnCrouch();
        LastCrouchDodgeTime = CurrentTime;
        CrouchDodgeEndTime = 0.0f;
    }

    // Evaluar situaci�n de combate
    EvaluateCombatSituation(MyChar, Enemy, Distance);

    // Si estamos reaccionando a un ataque, priorizar la reacci�n
    if (bIsReactingToAttack && ReactionTime > 0.0f)
    {
        if (ShouldBlock(Enemy))
        {
            if (!MyChar->bIsBlocking && BlockCooldown <= 0.0f)
            {
                MyChar->ServerSetBlocking(true);
                BlockCooldown = 1.6f;
                ReactionTime = 0.5f;
            }
        }
        else if (ShouldRetreat(Enemy, Distance))
        {
            FVector BackDir = -Dir;
            FVector SideDir = FVector::CrossProduct(FVector::UpVector, Dir);
            if (FMath::RandBool()) SideDir *= -1;
            MyChar->AddMovementInput(BackDir, 0.7f);
            MyChar->AddMovementInput(SideDir, 0.4f);
            ReactionTime = 0.0f;
            bIsReactingToAttack = false;
        }
        else if (!MyChar->GetCharacterMovement()->IsCrouching() && (CurrentTime - LastDashTime) >= DashCrouchSeparation && FMath::RandRange(0, 99) < 45)
        {
            MyChar->Crouch();
            CrouchDodgeEndTime = CurrentTime + 0.42f;
            ReactionTime = 0.0f;
            bIsReactingToAttack = false;
        }
        else
        {
            FVector BackDir = -Dir;
            MyChar->AddMovementInput(BackDir, 0.6f);
            ReactionTime = 0.0f;
            bIsReactingToAttack = false;
        }

        if (ReactionTime <= 0.0f && MyChar->bIsBlocking)
        {
            MyChar->ServerSetBlocking(false);
            bIsReactingToAttack = false;
        }

        TimeUntilNextAction -= DeltaSeconds;
        return;
    }

    // Bloqueo proactivo: soltar la guardia cuando toque
    if (MyChar->bIsBlocking && BlockReleaseTime > 0.f && CurrentTime >= BlockReleaseTime)
    {
        MyChar->ServerSetBlocking(false);
        BlockReleaseTime = 0.f;
    }

    // Si est� bloqueando, no hacer nada m�s
    if (MyChar->bIsBlocking)
    {
        TimeUntilNextAction -= DeltaSeconds;
        return;
    }

    // L�gica de decisi�n mejorada
    TimeUntilNextAction -= DeltaSeconds;
    if (TimeUntilNextAction <= 0.f)
    {
        // Reducir ataques consecutivos si hay demasiados (anti-spam)
        if (ConsecutiveAttacks >= MaxConsecutiveAttacks)
        {
            ConsecutiveAttacks = 0;
            AttackCooldown = FMath::RandRange(1.4f, 2.2f);
            CurrentAction = 0;
            TimeUntilNextAction = FMath::RandRange(0.9f, 1.5f);
            return;
        }

        // Decisi�n basada en distancia y situaci�n
        bool bCanDash = (DashCooldown <= 0.0f && !MyChar->bIsDashing
            && (CurrentTime - LastJumpTime) >= DashJumpSeparation
            && (CurrentTime - LastCrouchDodgeTime) >= DashCrouchSeparation);
        bool bCanJump = (JumpCooldown <= 0.0f && (CurrentTime - LastDashTime) >= DashJumpSeparation);

        if (Distance > 450.f)
        {
            CurrentAction = (bCanDash && FMath::RandRange(0, 99) < 32) ? 6 : 1;
            TimeUntilNextAction = FMath::RandRange(0.4f, 0.85f);
        }
        else if (Distance > 250.f && Distance <= 450.f)
        {
            // Rango medio: variar entre acercarse, saltar, o movimiento lateral
            int32 Rand = FMath::RandRange(0, 99);
            if (Rand < 38) CurrentAction = 1;
            else if (bCanDash && Rand < 52) CurrentAction = 6;
            else if (Rand < 68) CurrentAction = 5;
            else if (bCanJump && Rand < 80) CurrentAction = 3;
            else if (Rand < 88) CurrentAction = 0;
            else if (Rand < 96) CurrentAction = 4;
            else CurrentAction = 7; // Ataque (si est� en cooldown, se ignorar�)
            TimeUntilNextAction = FMath::RandRange(0.5f, 1.1f);
        }
        else if (Distance > 180.f && Distance <= 250.f)
        {
            // Rango de ataque cercano: m�s variado
            int32 Rand = FMath::RandRange(0, 99);
            if (AttackCooldown <= 0.0f && Rand < 42) CurrentAction = 4;
            else if (Rand < 58) CurrentAction = 1; // Acercarse m�s
            else if (bCanDash && Rand < 70) CurrentAction = 6;
            else if (Rand < 82) CurrentAction = 5;
            else if (Rand < 88) CurrentAction = 2;
            else if (Rand < 95) CurrentAction = 0;
            else CurrentAction = 7;
            TimeUntilNextAction = FMath::RandRange(0.4f, 0.9f);
        }
        else
        {
            int32 Rand = FMath::RandRange(0, 99);
            if (AttackCooldown <= 0.0f && Rand < 48) CurrentAction = 4;
            else if (Rand < 74) CurrentAction = 2;
            else if (Rand < 86) CurrentAction = 5;
            else if (Rand < 94) CurrentAction = 0;
            else CurrentAction = 7;
            TimeUntilNextAction = FMath::RandRange(0.35f, 0.75f);
        }
    }

    // Ejecutar acci�n actual
    switch (CurrentAction)
    {
    case 0: // Idle: slight approach so we don't get stuck standing
        if (Distance > 80.f && MyChar->CanMoveTowards(Dir))
        {
            if (UCharacterMovementComponent* MoveComp = MyChar->GetCharacterMovement())
            {
                MoveComp->MaxWalkSpeed = MyChar->WalkSpeedForward * 0.4f;
            }
            MyChar->AddMovementInput(Dir, 0.25f);
        }
        break;

    case 1: // Approach at full walk speed
        if (Distance > 60.f && MyChar->CanMoveTowards(Dir))
        {
            if (UCharacterMovementComponent* MoveComp = MyChar->GetCharacterMovement())
            {
                MoveComp->MaxWalkSpeed = MyChar->WalkSpeedForward;
            }
            MyChar->AddMovementInput(Dir, 1.0f);
        }
        break;

    case 2: // Retroceder estrat�gicamente
        if (Distance < 100.f)
        {
            FVector BackDir = -Dir;
            MyChar->AddMovementInput(BackDir, 0.5f);
        }
        break;

    case 3: // Jump occasionally (not right after dash)
        if (MyChar->GetCharacterMovement()->IsMovingOnGround() && JumpCooldown <= 0.0f
            && (CurrentTime - LastDashTime) >= DashJumpSeparation)
        {
            MyChar->Jump();
            LastJumpTime = CurrentTime;
            JumpCooldown = FMath::RandRange(1.2f, 2.6f);
        }
        break;

    case 4: // Punch only when enemy is idle stand
        if (Distance < 150.f && !MyChar->bIsPunching && AttackCooldown <= 0.0f && IsEnemyIdleStand(Enemy))
        {
            MyChar->Punch();
            ConsecutiveAttacks++;
            LastAttackTime = CurrentTime;
            AttackCooldown = FMath::RandRange(0.9f, 1.7f);
            TimeUntilNextAction = FMath::RandRange(0.7f, 1.3f);
        }
        break;

    case 7: // Subir guardia un momento (bloqueo proactivo), mantenerla y soltarla
        if (!MyChar->bIsBlocking && BlockCooldown <= 0.0f)
        {
            float HoldTime = FMath::RandRange(0.5f, 1.2f);
            MyChar->ServerSetBlocking(true);
            BlockReleaseTime = CurrentTime + HoldTime;
            BlockCooldown = HoldTime + 0.4f;
            TimeUntilNextAction = HoldTime;
        }
        break;

    case 6: // Dash toward enemy (then we re-decide soon to punch)
        if (DashCooldown <= 0.0f && !MyChar->bIsDashing && Distance > 80.f
            && (CurrentTime - LastJumpTime) >= DashJumpSeparation
            && (CurrentTime - LastCrouchDodgeTime) >= DashCrouchSeparation)
        {
            float EnemyY = Enemy->GetActorLocation().Y;
            float MyY = MyChar->GetActorLocation().Y;
            int32 DashDir = (EnemyY > MyY) ? 1 : -1;
            if (MyChar->CanMoveTowards(FVector(0.f, static_cast<float>(DashDir), 0.f)))
            {
                MyChar->PerformDash(DashDir);
                DashCooldown = 3.0f;
                LastDashTime = CurrentTime;
                TimeUntilNextAction = 0.45f; // re-decide soon after dash to favor punch
            }
        }
        break;

    case 5: // Lateral movement estrat�gico
        if (LateralMoveCooldown <= 0.0f)
        {
            PerformStrategicMovement(MyChar, Dir, Distance);
            LastLateralMoveTime = CurrentTime;
            LateralMoveCooldown = FMath::RandRange(1.0f, 2.0f); // Cooldown para variar movimiento
        }
        break;
    }
}

void AAggressiveAIController::EvaluateCombatSituation(AIO89Character* MyChar, AIO89Character* Enemy, float Distance)
{
    if (!MyChar || !Enemy) return;

    // Detectar si el enemigo est� atacando
    if (Enemy->bIsPunching && Distance < 200.f && !bIsReactingToAttack)
    {
        // Calcular probabilidad de reacci�n basada en distancia
        float ReactionChance = FMath::Clamp((200.f - Distance) / 200.f, 0.0f, 1.0f) * 100.f;

        if (FMath::RandRange(0, 100) < ReactionChance * 0.88f) // ~88% para intentar bloquear/evadir ms
        {
            bIsReactingToAttack = true;
            ReactionTime = FMath::RandRange(0.35f, 0.6f);
        }
    }
}

bool AAggressiveAIController::ShouldBlock(AIO89Character* Enemy)
{
    if (!Enemy) return false;

    // Bloquear si el enemigo est� atacando y estamos cerca
    if (Enemy->bIsPunching && BlockCooldown <= 0.0f)
    {
        FVector MyLoc = GetPawn()->GetActorLocation();
        FVector EnemyLoc = Enemy->GetActorLocation();
        float Dist = FVector::Dist2D(MyLoc, EnemyLoc);
        return Dist < 120.f;
    }

    return false;
}

bool AAggressiveAIController::IsEnemyIdleStand(AIO89Character* Enemy) const
{
    if (!Enemy) return false;
    if (Enemy->bIsPunching || Enemy->bIsDashing) return false;
    UCharacterMovementComponent* MoveComp = Enemy->GetCharacterMovement();
    if (MoveComp && MoveComp->IsCrouching()) return false;
    return true; // idle stand or blocking (both standing)
}

bool AAggressiveAIController::ShouldRetreat(AIO89Character* Enemy, float Distance)
{
    if (!Enemy) return false;

    // Retroceder si el enemigo est� atacando y estamos muy cerca
    if (Enemy->bIsPunching && Distance < 60.f)
    {
        return true;
    }

    return false;
}

void AAggressiveAIController::PerformStrategicMovement(AIO89Character* MyChar, const FVector& Dir, float Distance)
{
    if (!MyChar) return;

    // Movimiento lateral con variaci�n
    FVector SideDir = FVector::CrossProduct(FVector::UpVector, Dir);

    // A veces moverse hacia adelante mientras se mueve lateralmente
    if (FMath::RandRange(0, 100) < 40 && Distance > 200.f)
    {
        MyChar->AddMovementInput(Dir, 0.3f);
    }

    // Movimiento lateral con intensidad variable
    float SideIntensity = FMath::RandRange(0.4f, 0.7f);
    if (FMath::RandBool())
    {
        SideDir *= -1; // Cambiar direcci�n lateral aleatoriamente
    }

    MyChar->AddMovementInput(SideDir, SideIntensity);
}

void AAggressiveAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
}

void AAggressiveAIController::StartCombatMode()
{
    AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>();
    if (GS)
    {
        GS->bFightStarted = true;
    }
}
