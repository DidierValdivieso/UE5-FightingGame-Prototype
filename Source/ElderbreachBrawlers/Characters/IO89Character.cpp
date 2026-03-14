#include "IO89Character.h"
#include "EnhancedInputComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "../Animators/IO89AnimInstance.h"

#include <ElderbreachBrawlers/Actors/ProximityAttackActor.h>
#include <ElderbreachBrawlers/Actors/BlockHitboxActor.h>
#include <ElderbreachBrawlers/MainGameState.h>
#include "../Subsystems/CollisionDebugSubsystem.h"
#include "Kismet/GameplayStatics.h"

AIO89Character::AIO89Character()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    bAlwaysRelevant = true;
    GetMesh()->SetIsReplicated(true);
    GetMesh()->SetReceivesDecals(false);

    Pushbox = CreateDefaultSubobject<UBoxComponent>(TEXT("Pushbox"));
    Pushbox->SetupAttachment(RootComponent);
    Pushbox->SetRelativeLocation(FVector(0.f, 0.f, -30.f));
    Pushbox->SetBoxExtent(FVector(30.f, 30.f, 60.f));
    Pushbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Pushbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    Pushbox->ShapeColor = FColor::Green;
    Pushbox->SetHiddenInGame(true);
    Pushbox->ComponentTags.Add(FName("Pushbox"));
    Pushbox->SetReceivesDecals(false);


    TorsoHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("TorsoHitbox"));
    TorsoHitbox->InitBoxExtent(FVector(10.f, 20.f, 40.f));
    TorsoHitbox->SetupAttachment(GetMesh(), FName("spine_03"));
    TorsoHitbox->SetUsingAbsoluteRotation(true);
    TorsoHitbox->SetRelativeLocation(FVector(0.f, 0.f, 0.f));

    TorsoHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TorsoHitbox->SetCollisionObjectType(ECC_GameTraceChannel1);
    TorsoHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    TorsoHitbox->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
    TorsoHitbox->ShapeColor = FColor::Blue;
    TorsoHitbox->SetHiddenInGame(true);
    TorsoHitbox->SetGenerateOverlapEvents(true);
    TorsoHitbox->ComponentTags.Add(FName("TorsoHitbox"));

    static ConstructorHelpers::FClassFinder<ABlockHitboxActor> HitboxBP(TEXT("/Game/Blueprints/Actors/BP_BlockHitboxActor"));
    if (HitboxBP.Succeeded())
    {
        BlockHitboxClass = HitboxBP.Class;
    }

    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;
}

void AIO89Character::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("[DEATH_ANIM] %s BeginPlay: Class=%s DeathMontage=%s"),
        *GetName(), GetClass() ? *GetClass()->GetName() : TEXT("?"),
        DeathMontage ? *DeathMontage->GetName() : TEXT("NULL"));

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
        MoveComp->ResetMoveState();
        MoveComp->JumpZVelocity = JumpZVelocity;

        MoveComp->bConstrainToPlane = true;
        MoveComp->SetPlaneConstraintNormal(FVector(1.f, 0.f, 0.f));
    }

    MaxHealth = 100.f;
    Health = MaxHealth;

    MaxEnergy = 100.f;
    Energy = 10.f;

    MaxStamina = 100.f;
    Stamina = 10.f;

    if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
    {
        FOnMontageEnded EndDelegate;
        EndDelegate.BindUObject(this, &AIO89Character::OnPunchMontageEnded);
        AnimInstance->Montage_SetEndDelegate(EndDelegate);
    }

    if (UCollisionDebugSubsystem* Sub = GetWorld()->GetSubsystem<UCollisionDebugSubsystem>())
    {
        TArray<UActorComponent*> Components;
        GetComponents(UPrimitiveComponent::StaticClass(), Components);
        for (UActorComponent* Comp : Components)
        {
            UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp);
            if (!PrimComp) continue;
            if (PrimComp->ComponentHasTag(FName("TorsoHitbox")) ||
                PrimComp->ComponentHasTag(FName("Hitbox")) ||
                PrimComp->ComponentHasTag(FName("Hurtbox")) ||
                PrimComp->ComponentHasTag(FName("Pushbox")))
            {
                PrimComp->SetHiddenInGame(!Sub->bShowCollisionDebug);
            }
        }
    }
}

void AIO89Character::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(KnockbackClampHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void AIO89Character::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsDead) return;

    if (OtherPlayer && Controller)
    {
        FVector MyLocation = GetActorLocation();
        FVector OtherLocation = OtherPlayer->GetActorLocation();

        FVector Direction = OtherLocation - MyLocation;
        Direction.Z = 0.f;

        if (!Direction.IsNearlyZero())
        {
            FRotator LookAtRotation = Direction.Rotation();
            Controller->SetControlRotation(FRotator(0.f, LookAtRotation.Yaw, 0.f));
        }
    }

    if (OtherPlayer && ArePushBoxesOverlapping(OtherPlayer))
    {
        ResolvePush(OtherPlayer);
    }

    if (bIsBlocking && GetWorld())
    {
        float t = GetWorld()->GetTimeSeconds();
        if (t - LastBlockStateLogTime >= 1.5f)
        {
            UE_LOG(LogTemp, Log, TEXT("[%s] Block state: BLOCKING (idle)"), *GetName());
            LastBlockStateLogTime = t;
        }
    }
    else
    {
        LastBlockStateLogTime = -1.f;
    }

    RegenerateStamina(DeltaTime);
}

void AIO89Character::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(IA_Punch, ETriggerEvent::Started, this, &AIO89Character::Punch);
        EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AIO89Character::Move);
        EnhancedInput->BindAction(IA_Move, ETriggerEvent::Completed, this, &AIO89Character::OnMoveReleased);
        if (IA_Block)
        {
            EnhancedInput->BindAction(IA_Block, ETriggerEvent::Started, this, &AIO89Character::StartBlockByInput);
            EnhancedInput->BindAction(IA_Block, ETriggerEvent::Completed, this, &AIO89Character::OnBlockReleased);
        }
        if (IA_Crouch)
        {
            EnhancedInput->BindAction(IA_Crouch, ETriggerEvent::Started, this, &AIO89Character::OnCrouchStarted);
            EnhancedInput->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &AIO89Character::OnCrouchCompleted);
        }
        // F (toggle cajas) solo en el controlador (ThirdCharacterPC/SpectatorPC) para evitar doble binding cuando el jugador tiene pawn
    }
}

void AIO89Character::ToggleCollisionVisibility()
{
    if (UWorld* World = GetWorld())
    {
        if (UCollisionDebugSubsystem* Sub = World->GetSubsystem<UCollisionDebugSubsystem>())
            Sub->ToggleAndUpdateAll();
    }
}

void AIO89Character::OnCrouchStarted(const FInputActionValue& Value)
{
    if (bIsDead) return;
    if (const AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (!GS->bFightStarted) return;
    }
    if (GetCharacterMovement() && GetCharacterMovement()->IsFalling())
    {
        return;
    }
    Crouch();
}

void AIO89Character::OnCrouchCompleted(const FInputActionValue& Value)
{
    UnCrouch();
}

void AIO89Character::StartJump(const FInputActionValue& Value)
{
    Super::StartJump(Value);
}

void AIO89Character::Jump()
{
    Super::Jump();
    UE_LOG(LogTemp, Log, TEXT("[%s] REAL_JUMP VelZ=%.0f"), *GetName(), GetVelocity().Z);
}

void AIO89Character::StopBlockAnimation()
{
    if (!HasAuthority())
    {
        ServerStopBlock();
        return;
    }

    if (bIsBlocking)
        UE_LOG(LogTemp, Warning, TEXT("[%s] Block state: NONE (stop animation)"), *GetName());

    bBlockStartedByInput = false;
    bIsBlocking = false;
    bCanBlockDamage = false;
    bHasIncomingAttack = false;
    bCanBlock = false;

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(BlockWindowTimerHandle);
    if (ActiveBlockHitbox)
    {
        ActiveBlockHitbox->Destroy();
        ActiveBlockHitbox = nullptr;
    }

    MulticastStopBlock();

    // Si hay movimiento activo al soltar bloqueo, forzar Walk en el AnimInstance
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        const FVector Velocity = MoveComp->Velocity;
        const FVector VelocityXY = FVector(Velocity.X, Velocity.Y, 0.f);
        const float CurrentSpeed = VelocityXY.Size();
        
        if (CurrentSpeed > 10.f) // Si hay velocidad significativa, hay movimiento activo
        {
            if (UIO89AnimInstance* AnimInstance = Cast<UIO89AnimInstance>(GetMesh()->GetAnimInstance()))
            {
                AnimInstance->bForceWalkAfterBlock = true;
            }
        }
    }

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (BlockMontage && AnimInstance->Montage_IsPlaying(BlockMontage))
        {
            AnimInstance->Montage_Stop(0.2f, BlockMontage);
            UE_LOG(LogTemp, Verbose, TEXT("[%s] StopBlockAnimation: montage detenido (servidor)"), *GetName());
        }
    }
}

void AIO89Character::ServerStopBlock_Implementation()
{
    StopBlockAnimation();
}

void AIO89Character::MulticastStopBlock_Implementation()
{
    if (HasAuthority())
        return;

    bIsBlocking = false;

    if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
    {
        if (BlockMontage && AnimInstance->Montage_IsPlaying(BlockMontage))
        {
            AnimInstance->Montage_Stop(0.2f, BlockMontage);
            UE_LOG(LogTemp, Verbose, TEXT("[%s] MulticastStopBlock: montage detenido (cliente)"), *GetName());
        }
    }
}

void AIO89Character::OnMoveReleased(const FInputActionValue& Value)
{
    bHasReleasedSinceLastTap = true;
    if (bIsBlocking)
        StopBlockAnimation();
}

void AIO89Character::StartBlockByInput(const FInputActionValue& Value)
{
    if (bIsDead || bIsPunching || bIsDashing || bIsBlocking) return;
    if (const AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (!GS->bFightStarted) return;
    }
    ServerSetBlocking(true);
    ServerStartBlockByInput();
    UE_LOG(LogTemp, Warning, TEXT("[%s] Block state: BLOCKING (start)"), *GetName());
}

void AIO89Character::OnBlockReleased(const FInputActionValue& Value)
{
    if (bIsBlocking)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Block state: NONE (released)"), *GetName());
        StopBlockAnimation();
    }
}

bool AIO89Character::IsFacingAttacker(ACharacter* Attacker) const
{
    if (!Attacker)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] IsFacingAttacker: Attacker is null"), *GetName());
        return false;
    }

    FVector ToAttacker = Attacker->GetActorLocation() - GetActorLocation();
    ToAttacker.Z = 0.f;
    ToAttacker.Normalize();

    FVector Forward = GetActorForwardVector();
    float Dot = FVector::DotProduct(Forward, ToAttacker);

    UE_LOG(LogTemp, Warning, TEXT("[%s] FacingAttacker: Dot=%.2f"), *GetName(), Dot);

    return Dot > 0.5f;
}

float AIO89Character::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[%s] TakeDamage: ignorado (ya está muerto)"), *GetName());
        return 0.f;
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] TakeDamage: Recibiendo dano %.2f"), *GetName(), DamageAmount);

    ACharacter* Attacker = nullptr;

    if (DamageCauser)
    {
        Attacker = Cast<ACharacter>(DamageCauser);
        if (!Attacker && DamageCauser->GetOwner())
        {
            Attacker = Cast<ACharacter>(DamageCauser->GetOwner());
        }
    }

    UE_LOG(LogTemp, Verbose, TEXT("[%s] TakeDamage: bIsBlocking=%d bCanBlockDamage=%d Attacker=%s"),
        *GetName(), bIsBlocking ? 1 : 0, bCanBlockDamage ? 1 : 0,
        Attacker ? *Attacker->GetName() : TEXT("nullptr"));

    if (bCanBlockDamage && Attacker && IsFacingAttacker(Attacker))
    {
        const float BlockedDamage = DamageAmount * 0.2f;

        if (HasAuthority())
        {
            MulticastPlayBlockImpact();
        }

        Stamina = FMath::Clamp(Stamina - 10.f, 0.f, MaxStamina);

        ApplyDamage(BlockedDamage);
        return BlockedDamage;
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] BLOQUEO FALLIDO: recibe dano completo %.2f"), *GetName(), DamageAmount);

    const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

    ApplyDamage(ActualDamage);
    OnSuccessfulHit();
    UE_LOG(LogTemp, Warning, TEXT("[KNOCKBACK] %s TakeDamage: NO se aplica knockback aqui (solo TryReceiveHit aplica knockback al recibir golpe)"), *GetName());

    return ActualDamage;
}

bool AIO89Character::TryReceiveHit(AIO89Character* Attacker, float Damage)
{
    if (!Attacker || bIsDead)
        return false;

    if (!HasAuthority())
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] TryReceiveHit called on CLIENT -> forwarding to server (Attacker=%s, Damage=%.1f)"),
            *GetName(),
            Attacker ? *Attacker->GetName() : TEXT("null"),
            Damage);

        ServerTryReceiveHit(Attacker, Damage);
        return false;
    }

    const bool bFacing = IsFacingAttacker(Attacker);

    if (bIsBlocking && bBlockStartedByInput && !bCanBlockDamage)
    {
        bCanBlockDamage = true;
        UE_LOG(LogTemp, Warning, TEXT("[%s] TryReceiveHit: resincronizado bCanBlockDamage=1 (bloqueo por Q)"), *GetName());
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] TryReceiveHit (server): Attacker=%s bIsBlocking=%d bCanBlockDamage=%d IsFacing=%d"),
        *GetName(),
        *Attacker->GetName(),
        bIsBlocking ? 1 : 0,
        bCanBlockDamage ? 1 : 0,
        bFacing ? 1 : 0);

    if (bCanBlockDamage && bFacing)
    {
        if (HasAuthority())
        {
            MulticastPlayBlockImpact();
        }
        Stamina = FMath::Clamp(Stamina - 10.f, 0.f, MaxStamina);
        ClampStats();

        // Empuje del defensor (quien bloqueó) al bloquear efectivo
        if (Attacker && HasAuthority())
        {
            FVector Dir = GetActorLocation() - Attacker->GetActorLocation();
            Dir.Z = 0.f;
            Dir.Normalize();
            const FVector BlockKnockback = Dir * 500.f;
            LaunchCharacter(BlockKnockback, true, true);
            GetWorldTimerManager().SetTimer(KnockbackClampHandle, this, &AIO89Character::ClampKnockbackVerticalVelocity, 0.05f, false);
            UE_LOG(LogTemp, Warning, TEXT("[KNOCKBACK] %s BLOQUEO: knockback 500 aplicado al defensor (TryReceiveHit)"), *GetName());
        }

        UE_LOG(LogTemp, Warning, TEXT("[%s] BLOQUEO exitoso! (Atacante: %s)"), *GetName(), *Attacker->GetName());

        FTimerHandle TimerHandle_StopBlock;
        GetWorldTimerManager().SetTimer(
            TimerHandle_StopBlock,
            this,
            &AIO89Character::StopBlockAnimation,
            0.28f,
            false
        );

        return true;
    }
    else
    {
        StopBlockAnimation();
        ApplyDamage(Damage);

        if (HasAuthority())
        {
            MulticastPlayHitReact();
        }

        // Deslizamiento del enemigo cuando recibe el golpe (solo horizontal, sin salto)
        if (Attacker)
        {
            FVector Dir = GetActorLocation() - Attacker->GetActorLocation();
            Dir.Z = 0.f;
            Dir.Normalize();

            const FVector Knockback = Dir * 320.f;
            LaunchCharacter(Knockback, true, true);
            GetWorldTimerManager().SetTimer(KnockbackClampHandle, this, &AIO89Character::ClampKnockbackVerticalVelocity, 0.05f, false);
            UE_LOG(LogTemp, Warning, TEXT("[KNOCKBACK] %s GOLPE recibido: knockback 320 horizontal aplicado (TryReceiveHit)"), *GetName());
        }

        UE_LOG(LogTemp, Warning, TEXT("[%s] GOLPE recibido de %s -> Daño: %.1f (Vida restante: %.1f)"),
            *GetName(), *Attacker->GetName(), Damage, Health);

        return false;
    }

}

void AIO89Character::ServerTryReceiveHit_Implementation(AIO89Character* Attacker, float Damage)
{
    TryReceiveHit(Attacker, Damage);
}

void AIO89Character::MulticastPlayHitReact_Implementation()
{
	if (bIsDead) return; // No mostrar hit react si ya estamos muertos (dejar que se vea el montage de muerte)
	bIsHitReacting = true;
	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	HitReactStartTime = Now;
	UE_LOG(LogTemp, Warning, TEXT("[HIT_REACT] %s START | tiempo=%.2f | HitReactDuration=%.2fs (bIsHitReacting=true)"), *GetName(), Now, HitReactDuration);

	if (World)
	{
		World->GetTimerManager().SetTimer(HitReactResetHandle, this, &AIO89Character::ClearHitReact, HitReactDuration, false);
	}
}

void AIO89Character::ApplyDamage(float DamageAmount)
{
    if (!HasAuthority()) return;

    Health -= DamageAmount;
    Energy += DamageAmount * 0.5f;
    ClampStats();

    if (Health <= 0.f)
    {
        Die();
    }
}

bool AIO89Character::TryConsumeStamina(float Amount)
{
    if (Stamina >= Amount)
    {
        Stamina -= Amount;
        ClampStats();
        return true;
    }
    return false;
}

bool AIO89Character::TryConsumeEnergy(float Amount)
{
    if (Energy >= Amount)
    {
        Energy -= Amount;
        ClampStats();
        return true;
    }
    return false;
}

void AIO89Character::Die()
{
    if (!HasAuthority()) return;

    MulticastDie();

    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        GS->HandlePlayerDeath(this);
    }
}


void AIO89Character::DisableAllHitboxes()
{
    TArray<UActorComponent*> Components;
    GetComponents(UPrimitiveComponent::StaticClass(), Components);

    for (UActorComponent* Comp : Components)
    {
        UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp);
        if (!PrimComp) continue;

        if (PrimComp->ComponentHasTag(FName("TorsoHitbox")) ||
            PrimComp->ComponentHasTag(FName("Hitbox")) ||
            PrimComp->ComponentHasTag(FName("Hurtbox")) ||
            PrimComp->ComponentHasTag(FName("Pushbox")))
        {
            PrimComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            PrimComp->SetCollisionResponseToAllChannels(ECR_Ignore);
            PrimComp->SetHiddenInGame(true);
        }
    }
}

void AIO89Character::MulticastDie_Implementation()
{
    if (bIsDead) return;

    UE_LOG(LogTemp, Warning, TEXT("[DEATH_VIEW] MulticastDie %s Class=%s DeathMontage=%s (ptr=%p)"),
        *GetName(), GetClass() ? *GetClass()->GetName() : TEXT("?"),
        DeathMontage ? *DeathMontage->GetName() : TEXT("NULL"), (void*)DeathMontage);

    bIsDead = true;

    DisableAllHitboxes();

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
        MoveComp->StopMovementImmediately();
    }

    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // Dejar de mostrar hit react / punch reaction para que se vea solo el montage de muerte
    bIsHitReacting = false;
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(HitReactResetHandle);
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
        {
            if (BlockMontage && AnimInst->Montage_IsPlaying(BlockMontage))
                AnimInst->Montage_Stop(0.f, BlockMontage);
            if (PunchMontage && AnimInst->Montage_IsPlaying(PunchMontage))
                AnimInst->Montage_Stop(0.f, PunchMontage);
        }
    }

    // Reproducir animación de muerte (AM_Death) en lugar de ragdoll
    UAnimMontage* MontageToPlay = DeathMontage;
    if (!MontageToPlay)
    {
        MontageToPlay = LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Blueprints/Animations/IO89/Death/AM_Death.AM_Death"));
        if (MontageToPlay)
        {
            UE_LOG(LogTemp, Log, TEXT("[DEATH_ANIM] DeathMontage era NULL; usando AM_Death cargado por path."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[DEATH_ANIM] DeathMontage es NULL y no se pudo cargar AM_Death por path. Asigna en Blueprint (Combat -> Death Montage)."));
        }
    }

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        if (!MeshComp->GetAnimInstance())
        {
            UE_LOG(LogTemp, Warning, TEXT("[DEATH_ANIM] GetAnimInstance() es NULL. No se puede reproducir el montage de muerte."));
        }
        if (MontageToPlay && MeshComp->GetAnimInstance())
        {
            UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
            AnimInst->Montage_Play(MontageToPlay, 1.0f);

            // Debug: qué montage y duración está configurada
            const float MontageLength = MontageToPlay->GetPlayLength();
            UE_LOG(LogTemp, Warning, TEXT("[DEATH_ANIM] Playing montage: %s | PlayRate=1.0 | Montage GetPlayLength()=%.3fs"),
                *MontageToPlay->GetName(), MontageLength);

            // Debug: logear posición cada 0.35s para ver si recorre 2.03s (animación completa) o menos
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().ClearTimer(DeathMontageDebugHandle);
                TWeakObjectPtr<AIO89Character> WeakThis(this);
                UAnimMontage* MontageRef = MontageToPlay;
                World->GetTimerManager().SetTimer(DeathMontageDebugHandle, [WeakThis, MontageRef, MontageLength]()
                {
                    if (!WeakThis.IsValid()) return;
                    UAnimInstance* Inst = WeakThis->GetMesh() ? WeakThis->GetMesh()->GetAnimInstance() : nullptr;
                    if (!Inst || !MontageRef || !Inst->Montage_IsPlaying(MontageRef))
                    {
                        WeakThis->GetWorld()->GetTimerManager().ClearTimer(WeakThis->DeathMontageDebugHandle);
                        return;
                    }
                    const float Pos = Inst->Montage_GetPosition(MontageRef);
                    UE_LOG(LogTemp, Log, TEXT("[DEATH_ANIM] Montage position=%.3fs / %.3fs (%.0f%%)"),
                        Pos, MontageLength, MontageLength > 0.f ? (Pos / MontageLength * 100.f) : 0.f);
                }, 0.35f, true);
            }
        }
    }
}

void AIO89Character::Revive()
{
    if (!HasAuthority()) return;

    // Restore health (solo servidor; stats se replican)
    Health = MaxHealth;
    Energy = 10.f;
    Stamina = 10.f;
    ClampStats();

    MulticastRevive();
}

void AIO89Character::MulticastRevive_Implementation()
{
    bIsDead = false;

    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
        {
            UAnimMontage* DeathRef = DeathMontage ? DeathMontage : LoadObject<UAnimMontage>(nullptr, TEXT("/Game/Blueprints/Animations/IO89/Death/AM_Death.AM_Death"));
            if (DeathRef && AnimInst->Montage_IsPlaying(DeathRef))
            {
                AnimInst->Montage_Stop(0.f, DeathRef);
            }
        }
    }

    // Desactivar ragdoll (si se usó): volver mesh a animación y restaurar cápsula (todos los clientes)
    if (USkeletalMeshComponent* MeshComp = GetMesh())
    {
        if (MeshComp->IsSimulatingPhysics())
        {
            MeshComp->SetSimulatePhysics(false);
            MeshComp->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
            // Ángulo recto y altura correcta al spawn round 2 (mesh con su offset normal, no centrado en cápsula)
            MeshComp->SetRelativeLocationAndRotation(ReviveMeshRelativeOffset, FRotator::ZeroRotator);
            MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    }
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    }

    // Re-enable hitboxes
    TArray<UActorComponent*> Components;
    GetComponents(UPrimitiveComponent::StaticClass(), Components);

    for (UActorComponent* Comp : Components)
    {
        UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp);
        if (!PrimComp) continue;

        if (PrimComp->ComponentHasTag(FName("TorsoHitbox")))
        {
            PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            PrimComp->SetCollisionObjectType(ECC_GameTraceChannel1);
            PrimComp->SetCollisionResponseToAllChannels(ECR_Ignore);
            PrimComp->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Overlap);
            PrimComp->SetHiddenInGame(true);
        }
        else if (PrimComp->ComponentHasTag(FName("Hitbox")) || 
                 PrimComp->ComponentHasTag(FName("Hurtbox")))
        {
            PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            PrimComp->SetHiddenInGame(true);
        }
        else if (PrimComp->ComponentHasTag(FName("Pushbox")))
        {
            PrimComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            PrimComp->SetCollisionResponseToAllChannels(ECR_Ignore);
            PrimComp->SetHiddenInGame(true);
        }
    }

    // Re-enable movement
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->SetMovementMode(MOVE_Walking);
    }
}


void AIO89Character::OnSuccessfulHit()
{
    if (!HasAuthority()) return;

    Energy += 10.f;
    Stamina -= 5.f;

    ClampStats();
}

void AIO89Character::RegenerateStamina(float DeltaTime)
{
    if (!HasAuthority()) return;

    Stamina += 5.f * DeltaTime;
    ClampStats();
}

void AIO89Character::ClampStats()
{
    Health = FMath::Clamp(Health, 0.f, MaxHealth);
    Energy = FMath::Clamp(Energy, 0.f, MaxEnergy);
    Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina);
}

void AIO89Character::Move(const FInputActionValue& Value)
{
    if (const AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (!GS->bFightStarted) return;
    }

    if (GetCharacterMovement() && GetCharacterMovement()->IsCrouching())
    {
        return;
    }

    FVector2D MovementVector = Value.Get<FVector2D>();
    if (!Controller || !OtherPlayer) return;

    MovementVector.Y = 0.f;

    // Dirección de input: -1 izquierda, 1 derecha, 0 neutro (eje Y del mundo)
    int32 CurrentDir = 0;
    if (MovementVector.X < -0.1f) CurrentDir = -1;
    else if (MovementVector.X > 0.1f) CurrentDir = 1;

    if (CurrentDir != 0 && !bIsDead && !bIsPunching && !bIsDashing)
    {
        const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
        if (bHasReleasedSinceLastTap && CurrentDir == LastTapDirection && (Now - LastTapTime) <= DoubleTapWindow)
        {
            PerformDash(CurrentDir);
            LastTapDirection = 0;
            LastTapTime = -1.f;
            bHasReleasedSinceLastTap = false;
        }
        else
        {
            LastTapDirection = CurrentDir;
            LastTapTime = Now;
            bHasReleasedSinceLastTap = false;
        }
    }

    float EnemyY = OtherPlayer->GetActorLocation().Y;
    float MyY = GetActorLocation().Y;
    bool bEnemyRight = (EnemyY > MyY);

    bool bPressingLeft = MovementVector.X < -0.1f;
    bool bPressingRight = MovementVector.X > 0.1f;

    bool bRetreating = (bEnemyRight && bPressingLeft) || (!bEnemyRight && bPressingRight);

    bIsRetreating = bRetreating;
    bCanBlock = bRetreating;

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = bRetreating ? WalkSpeedBackward : WalkSpeedForward;
    }

    // Si hay movimiento mientras se está bloqueando, desactivar bloqueo automáticamente
    if (bIsBlocking && !FMath::IsNearlyZero(MovementVector.X))
    {
        StopBlockAnimation();
    }

    bool bCanMove = true;

    float DistY = FMath::Abs(GetActorLocation().Y - OtherPlayer->GetActorLocation().Y);
    if (DistY >= MaxAllowedDist)
    {
        bool bTryingToGoOut =
            (MovementVector.X > 0 && GetActorLocation().Y > OtherPlayer->GetActorLocation().Y) ||
            (MovementVector.X < 0 && GetActorLocation().Y < OtherPlayer->GetActorLocation().Y);

        if (bTryingToGoOut)
            bCanMove = false;
    }

    if (bCanMove && !FMath::IsNearlyZero(MovementVector.X))
    {
        AddMovementInput(FVector::RightVector, MovementVector.X);
    }
}

void AIO89Character::PerformDash(int32 Direction)
{
    if (bIsDead || !GetCharacterMovement() || !OtherPlayer) return;
    if (!CanMoveTowards(FVector(0.f, static_cast<float>(Direction), 0.f))) return;

    if (HasAuthority())
    {
        GetWorldTimerManager().ClearTimer(DashStopHandle);
        bIsDashing = true;
        FVector LaunchVel(0.f, static_cast<float>(Direction) * DashSpeed, 0.f);
        LaunchCharacter(LaunchVel, true, false);
        GetWorldTimerManager().SetTimer(DashStopHandle, this, &AIO89Character::OnDashDurationEnded, DashDuration, false);
    }
    else
    {
        ServerPerformDash(Direction);
    }
}

void AIO89Character::ServerPerformDash_Implementation(int32 Direction)
{
    PerformDash(Direction);
}

void AIO89Character::OnDashDurationEnded()
{
    bIsDashing = false;
    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->StopMovementImmediately();
    }
}

bool AIO89Character::CanMoveTowards(const FVector& Direction) const
{
    if (!OtherPlayer) return true;

    float DistY = FMath::Abs(GetActorLocation().Y - OtherPlayer->GetActorLocation().Y);
    if (DistY >= MaxAllowedDist)
    {
        bool bTryingToGoOut =
            (Direction.Y > 0 && GetActorLocation().Y > OtherPlayer->GetActorLocation().Y) ||
            (Direction.Y < 0 && GetActorLocation().Y < OtherPlayer->GetActorLocation().Y);

        return !bTryingToGoOut;
    }

    return true;
}

void AIO89Character::ServerSetBlocking_Implementation(bool bNewBlocking)
{
    bIsBlocking = bNewBlocking;
}

void AIO89Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AIO89Character, Health);
    DOREPLIFETIME(AIO89Character, MaxHealth);
    DOREPLIFETIME(AIO89Character, Energy);
    DOREPLIFETIME(AIO89Character, MaxEnergy);
    DOREPLIFETIME(AIO89Character, Stamina);
    DOREPLIFETIME(AIO89Character, MaxStamina);
    DOREPLIFETIME(AIO89Character, OtherPlayer);
	DOREPLIFETIME(AIO89Character, bIsPunching);
	DOREPLIFETIME(AIO89Character, bIsBlocking);
	DOREPLIFETIME(AIO89Character, bCanBlockDamage);
	DOREPLIFETIME(AIO89Character, bIsDashing);
	DOREPLIFETIME(AIO89Character, bIsHitReacting);
}

bool AIO89Character::ArePushBoxesOverlapping(AIO89Character* Other) const
{
    if (!Pushbox || !Other->Pushbox) return false;

    FBox MyBox = Pushbox->Bounds.GetBox();
    FBox OtherBox = Other->Pushbox->Bounds.GetBox();

    return MyBox.Intersect(OtherBox);
}

void AIO89Character::ResolvePush(AIO89Character* Other)
{
    if (!Pushbox || !Other->Pushbox) return;

    FBox MyBox = Pushbox->Bounds.GetBox();
    FBox OtherBox = Other->Pushbox->Bounds.GetBox();

    FVector MyCenter = MyBox.GetCenter();
    FVector OtherCenter = OtherBox.GetCenter();
    FVector Delta = MyCenter - OtherCenter;

    float OverlapY = (MyBox.GetExtent().Y + OtherBox.GetExtent().Y) - FMath::Abs(Delta.Y);

    if (OverlapY > 0.f)
    {
        float PushAmount = OverlapY * 0.5f;
        float Direction = FMath::Sign(Delta.Y);

        AddActorWorldOffset(FVector(0, PushAmount * Direction, 0), true);
        Other->AddActorWorldOffset(FVector(0, -PushAmount * Direction, 0), true);
    }
}

void AIO89Character::Punch()
{
    if (bIsPunching) return;

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    if (CurrentTime - LastAttackTime < AttackCooldown) return;

    if (const AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (!GS->bFightStarted) return;
    }

    if (!HasAuthority())
    {
        ServerPunch();
        return;
    }

    HandlePunch_Server();
}

void AIO89Character::ServerSpawnProximity_Implementation()
{
    if (IsValid(ActiveProximityAttack))
    {
        UE_LOG(LogTemp, Verbose, TEXT("[%s] ServerSpawnProximity: ActiveProximityAttack ya existe, skipping"), *GetName());
        return;
    }

    UWorld* World = GetWorld();
    if (!World) return;

    FVector Forward = GetActorForwardVector();
    Forward.Z = 0.f;
    Forward.Normalize();

    const float BaseZ = GetMesh() ? GetMesh()->GetComponentLocation().Z : GetActorLocation().Z;

    FVector SpawnLoc = FVector(GetActorLocation().X, GetActorLocation().Y, BaseZ)
        + Forward * ProximitySpawnDistance
        + FVector(0.f, 0.f, ProximitySpawnHeight);

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    TSubclassOf<AProximityAttackActor> ClassToSpawn = ProximityAttackClass ? ProximityAttackClass : TSubclassOf<AProximityAttackActor>(AProximityAttackActor::StaticClass());

    if (AProximityAttackActor* Prox = World->SpawnActor<AProximityAttackActor>(ClassToSpawn, SpawnLoc, GetActorRotation(), Params))
    {
        FVector ProxExtent(30.f, 5.f, 10.f);
        Prox->Init(this, SpawnLoc, ProxExtent);
        Prox->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
        ActiveProximityAttack = Prox;

        UE_LOG(LogTemp, Warning, TEXT("[%s] ServerSpawnProximity: spawned prox actor at %s"), *GetName(), *SpawnLoc.ToString());
    }
}

void AIO89Character::ResetPunch()
{
    bIsPunching = false;
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(PunchResetHandle);
    }
}

void AIO89Character::ServerPunch_Implementation()
{
    HandlePunch_Server();
}

void AIO89Character::HandlePunch_Server()
{
    MulticastPlayPunch();
}

void AIO89Character::MulticastPlayPunch_Implementation()
{
    if (bIsPunching) return;

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(PunchResetHandle);
    }

    bIsPunching = true;
    LastAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    float Duration = 0.f;
    if (PunchMontage && GetMesh())
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            if (!AnimInstance->Montage_IsPlaying(PunchMontage))
            {
                Duration = AnimInstance->Montage_Play(PunchMontage, 1.0f);
            }
        }
    }

    if (Duration <= 0.f)
    {
        Duration = PunchAnimationDuration;
    }

    if (Duration > 0.f && GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(PunchResetHandle, this, &AIO89Character::ResetPunch, Duration, false);
    }
    else if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(PunchResetHandle, this, &AIO89Character::ResetPunch, PunchAnimationDuration, false);
    }
}

void AIO89Character::OnPunchMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage == PunchMontage)
    {
        bIsPunching = false;
    }
}

void AIO89Character::MulticastPlayBlock_Implementation()
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] MulticastPlayBlock llamado (Role=%d)"), *GetName(), (int32)GetLocalRole());

    USkeletalMeshComponent* MeshComp = GetMesh();
    if (!MeshComp)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] NO TIENE MeshComp!"), *GetName());
        return;
    }

    UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] NO TIENE AnimInstance (probablemente servidor dedicado)"), *GetName());
        return;
    }

    if (!BlockMontage)
    {
        UE_LOG(LogTemp, Error, TEXT("[%s] BlockMontage es NULL!"), *GetName());
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] Reproduciendo animacion de BLOQUEO en Role=%d"), *GetName(), (int32)GetLocalRole());
    AnimInstance->Montage_Play(BlockMontage, 2.0f);
}

void AIO89Character::ServerPlayBlock_Implementation()
{
    MulticastPlayBlock();
}

void AIO89Character::OnRep_OtherPlayer()
{
}

void AIO89Character::PlayBlockAnimation()
{
    if (BlockMontage && GetMesh() && GetMesh()->GetAnimInstance())
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (!AnimInstance->Montage_IsPlaying(BlockMontage))
        {
            AnimInstance->Montage_Play(BlockMontage, 2.0f);
            UE_LOG(LogTemp, Warning, TEXT("[%s] BLOQUE animacion reproducida localmente"), *GetName());
        }
    }
}

void AIO89Character::MulticastPlayBlockImpact_Implementation()
{
    bBlockImpact = true;
    UE_LOG(LogTemp, Warning, TEXT("[%s] Block state: BLOCK_IMPACT (golpe bloqueado)"), *GetName());
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimer(BlockImpactResetHandle, this, &AIO89Character::ClearBlockImpact, 0.28f, false);
}

void AIO89Character::ClearBlockImpact()
{
	bBlockImpact = false;
}

void AIO89Character::ClearHitReact()
{
	bIsHitReacting = false;
	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	const float ActualDuration = (HitReactStartTime > 0.f && World) ? (Now - HitReactStartTime) : 0.f;
	UE_LOG(LogTemp, Warning, TEXT("[HIT_REACT] %s END   | tiempo=%.2f | duración real=%.2fs (bIsHitReacting=false)"), *GetName(), Now, ActualDuration);
}

void AIO89Character::ClampKnockbackVerticalVelocity()
{
    if (!IsValid(this)) return;
    UCharacterMovementComponent* M = GetCharacterMovement();
    if (M && M->Velocity.Z > 0.f)
    {
        M->Velocity.Z = 0.f;
    }
}

void AIO89Character::OnSuccessfulBlock(AActor* Attacker)
{
}

void AIO89Character::NotifyIncomingAttack(AIO89Character* From)
{
    if (!From) return;

    if (!bIsBlocking && bCanBlock && bHasIncomingAttack == false)
    {
        IncomingFrom = From;
        bHasIncomingAttack = true;

        if (HasAuthority())
            MulticastPlayBlock();
        else
            ServerPlayBlock();

        UE_LOG(LogTemp, Warning, TEXT("[%s] Bloqueo activado: ataque entrante y retroceso detectado"), *GetName());
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Ataque entrante ignorado: bCanBlock=%d bIsBlocking=%d bHasIncomingAttack=%d"),
            *GetName(), bCanBlock ? 1 : 0, bIsBlocking ? 1 : 0, bHasIncomingAttack ? 1 : 0);
    }
}

void AIO89Character::OnIncomingAttackNow(AIO89Character* From)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] OnIncomingAttackNow: ATTACK HAPPENS from %s"), *GetName(), *From->GetName());
}

void AIO89Character::ClearIncomingAttack()
{
    bHasIncomingAttack = false;
    IncomingFrom = nullptr;
}

void AIO89Character::ServerDestroyProximity_Implementation()
{
    if (!HasAuthority()) return;

    if (IsValid(ActiveProximityAttack))
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] ServerDestroyProximity: destroying ActiveProximityAttack at %s"), *GetName(), *ActiveProximityAttack->GetActorLocation().ToString());
        ActiveProximityAttack->Destroy();
        ActiveProximityAttack = nullptr;
    }
    else
    {
        UE_LOG(LogTemp, Verbose, TEXT("[%s] ServerDestroyProximity: no ActiveProximityAttack to destroy"), *GetName());
    }
}

void AIO89Character::ServerEnableBlockHitbox_Implementation(FVector SocketLocation, FRotator SocketRotation, float Duration)
{
    if (!HasAuthority()) return;

    if (!bCanBlock || !bHasIncomingAttack || !IncomingFrom)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] ServerEnableBlockHitbox: bloqueo no permitido (bCanBlock=%d, bHasIncomingAttack=%d)"),
            *GetName(), bCanBlock ? 1 : 0, bHasIncomingAttack ? 1 : 0);
        return;
    }

    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(BlockWindowTimerHandle);
    }

    if (ActiveBlockHitbox)
    {
        ActiveBlockHitbox->Destroy();
        ActiveBlockHitbox = nullptr;
    }

    bCanBlockDamage = true;
    bIsBlocking = true;

    // Bloqueo resuelto por AttackHitbox overlap con TorsoHitbox + TryReceiveHit; no se spawnea BlockHitboxActor.

    const float UseDuration = (Duration > 0.f) ? Duration : DefaultBlockWindow;
    if (GetWorld())
    {
        GetWorldTimerManager().SetTimer(BlockWindowTimerHandle, this, &AIO89Character::OnBlockWindowExpired, UseDuration, false);
    }
}

void AIO89Character::ServerDisableBlockHitbox_Implementation()
{
    if (!HasAuthority()) return;
    if (bBlockStartedByInput)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[%s] ServerDisableBlockHitbox: ignorado (bloqueo por Q activo)"), *GetName());
        return;
    }

    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(BlockWindowTimerHandle);
    }

    bCanBlockDamage = false;
    bIsBlocking = false;

    if (ActiveBlockHitbox)
    {
        ActiveBlockHitbox->Destroy();
        ActiveBlockHitbox = nullptr;
    }

    MulticastStopBlock();

    UE_LOG(LogTemp, Warning, TEXT("[%s] ServerDisableBlockHitbox executed (server)"), *GetName());
}

void AIO89Character::ServerStartBlockByInput_Implementation()
{
    if (!HasAuthority()) return;

    if (GetWorld())
        GetWorld()->GetTimerManager().ClearTimer(BlockWindowTimerHandle);

    bBlockStartedByInput = true;
    bCanBlock = true;
    bIsBlocking = true;
    bCanBlockDamage = true;

    if (ActiveBlockHitbox)
    {
        ActiveBlockHitbox->Destroy();
        ActiveBlockHitbox = nullptr;
    }

    // Bloqueo resuelto por AttackHitbox overlap con TorsoHitbox + TryReceiveHit; no se spawnea BlockHitboxActor.
}


void AIO89Character::OnBlockWindowExpired()
{
    if (!HasAuthority()) return;
    if (bBlockStartedByInput) return;

    bCanBlockDamage = false;
    bIsBlocking = false;

    if (ActiveBlockHitbox)
    {
        ActiveBlockHitbox->Destroy();
        ActiveBlockHitbox = nullptr;
    }

    MulticastStopBlock();

    UE_LOG(LogTemp, Warning, TEXT("[%s] Block window expired on server"), *GetName());
}
