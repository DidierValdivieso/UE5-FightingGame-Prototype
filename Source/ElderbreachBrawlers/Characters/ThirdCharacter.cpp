#include "ThirdCharacter.h"
#include "EnhancedInputComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

#include <ElderbreachBrawlers/Actors/ProximityAttackActor.h>
#include <ElderbreachBrawlers/Actors/BlockHitboxActor.h>
#include <ElderbreachBrawlers/MainGameState.h>
#include "../Animators/IO89AnimInstance.h"

AThirdCharacter::AThirdCharacter()
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
    TorsoHitbox->ShapeColor = FColor::Red;
    TorsoHitbox->SetHiddenInGame(false);
    //TorsoHitbox->SetHiddenInGame(true);
    TorsoHitbox->SetGenerateOverlapEvents(true);
    TorsoHitbox->ComponentTags.Add(FName("TorsoHitbox"));

    static ConstructorHelpers::FClassFinder<ABlockHitboxActor> HitboxBP(TEXT("/Game/Blueprints/Actors/BP_BlockHitboxActor"));
    if (HitboxBP.Succeeded())
    {
        BlockHitboxClass = HitboxBP.Class;
    }
}

void AThirdCharacter::BeginPlay()
{
    Super::BeginPlay();

    MaxHealth = 100.f;
    Health = MaxHealth;

    MaxEnergy = 100.f;
    Energy = 10.f;

    MaxStamina = 100.f;
    Stamina = 10.f;
}

void AThirdCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(KnockbackClampHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void AThirdCharacter::Tick(float DeltaTime)
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

void AThirdCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInput->BindAction(IA_Punch, ETriggerEvent::Started, this, &AThirdCharacter::Punch);
        EnhancedInput->BindAction(IA_Move, ETriggerEvent::Triggered, this, &AThirdCharacter::Move);
        EnhancedInput->BindAction(IA_Move, ETriggerEvent::Completed, this, &AThirdCharacter::OnMoveReleased);
        if (IA_Block)
        {
            EnhancedInput->BindAction(IA_Block, ETriggerEvent::Started, this, &AThirdCharacter::StartBlockByInput);
            EnhancedInput->BindAction(IA_Block, ETriggerEvent::Completed, this, &AThirdCharacter::OnBlockReleased);
        }
    }
}

void AThirdCharacter::StopBlockAnimation()
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

void AThirdCharacter::ServerStopBlock_Implementation()
{
    StopBlockAnimation();
}

void AThirdCharacter::MulticastStopBlock_Implementation()
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

void AThirdCharacter::OnMoveReleased(const FInputActionValue& Value)
{
    if (bIsBlocking)
        StopBlockAnimation();
}

void AThirdCharacter::StartBlockByInput(const FInputActionValue& Value)
{
    if (bIsDead || bIsPunching || bIsBlocking) return;
    if (const AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (!GS->bFightStarted) return;
    }
    ServerSetBlocking(true);
    ServerStartBlockByInput();
    UE_LOG(LogTemp, Warning, TEXT("[%s] Block state: BLOCKING (start)"), *GetName());
}

void AThirdCharacter::OnBlockReleased(const FInputActionValue& Value)
{
    if (bIsBlocking)
    {
        UE_LOG(LogTemp, Warning, TEXT("[%s] Block state: NONE (released)"), *GetName());
        StopBlockAnimation();
    }
}

bool AThirdCharacter::IsFacingAttacker(ACharacter* Attacker) const
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

float AThirdCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
    if (bIsDead)
    {
        UE_LOG(LogTemp, Verbose, TEXT("[%s] TakeDamage: ignorado (ya est? muerto)"), *GetName());
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
    // Knockback on hit is applied only in TryReceiveHit (punch path).

    return ActualDamage;
}

bool AThirdCharacter::TryReceiveHit(AThirdCharacter* Attacker, float Damage)
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
            GetWorldTimerManager().SetTimer(KnockbackClampHandle, this, &AThirdCharacter::ClampKnockbackVerticalVelocity, 0.05f, false);
        }

        UE_LOG(LogTemp, Warning, TEXT("[%s] BLOQUEO exitoso! (Atacante: %s)"), *GetName(), *Attacker->GetName());

        FTimerHandle TimerHandle_StopBlock;
        GetWorldTimerManager().SetTimer(
            TimerHandle_StopBlock,
            this,
            &AThirdCharacter::StopBlockAnimation,
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

        // Deslizamiento del enemigo cuando recibe el golpe (similar a Tekken)
        if (Attacker)
        {
            FVector Dir = GetActorLocation() - Attacker->GetActorLocation();
            Dir.Z = 0.f;
            Dir.Normalize();

            // Deslizamiento m?s notable al recibir el golpe
            const FVector Knockback = Dir * 320.f;
            LaunchCharacter(Knockback, true, true);
            GetWorldTimerManager().SetTimer(KnockbackClampHandle, this, &AThirdCharacter::ClampKnockbackVerticalVelocity, 0.05f, false);
        }

        UE_LOG(LogTemp, Warning, TEXT("[%s] GOLPE recibido de %s -> Da?o: %.1f (Vida restante: %.1f)"),
            *GetName(), *Attacker->GetName(), Damage, Health);

        return false;
    }

}

void AThirdCharacter::ServerTryReceiveHit_Implementation(AThirdCharacter* Attacker, float Damage)
{
    TryReceiveHit(Attacker, Damage);
}

void AThirdCharacter::MulticastPlayHitReact_Implementation()
{
	bIsHitReacting = true;
	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	HitReactStartTime = Now;
	UE_LOG(LogTemp, Warning, TEXT("[HIT_REACT] %s START | tiempo=%.2f | HitReactDuration=%.2fs (bIsHitReacting=true)"), *GetName(), Now, HitReactDuration);

	if (World)
	{
		World->GetTimerManager().SetTimer(HitReactResetHandle, this, &AThirdCharacter::ClearHitReact, HitReactDuration, false);
	}
}

void AThirdCharacter::ApplyDamage(float DamageAmount)
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

bool AThirdCharacter::TryConsumeStamina(float Amount)
{
    if (Stamina >= Amount)
    {
        Stamina -= Amount;
        ClampStats();
        return true;
    }
    return false;
}

bool AThirdCharacter::TryConsumeEnergy(float Amount)
{
    if (Energy >= Amount)
    {
        Energy -= Amount;
        ClampStats();
        return true;
    }
    return false;
}

void AThirdCharacter::Die()
{
    if (!HasAuthority()) return;

    MulticastDie();

    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        GS->HandlePlayerDeath(this);
    }
}


void AThirdCharacter::DisableAllHitboxes()
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

void AThirdCharacter::MulticastDie_Implementation()
{
    if (bIsDead) return;

    bIsDead = true;

    DisableAllHitboxes();

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->DisableMovement();
        MoveComp->StopMovementImmediately();
    }
}

void AThirdCharacter::Revive()
{
    if (!HasAuthority()) return;

    bIsDead = false;

    // Restore health
    Health = MaxHealth;
    Energy = 10.f;
    Stamina = 10.f;
    ClampStats();

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
            PrimComp->SetHiddenInGame(false);
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


void AThirdCharacter::OnSuccessfulHit()
{
    if (!HasAuthority()) return;

    Energy += 10.f;
    Stamina -= 5.f;

    ClampStats();
}

void AThirdCharacter::RegenerateStamina(float DeltaTime)
{
    if (!HasAuthority()) return;

    Stamina += 5.f * DeltaTime;
    ClampStats();
}

void AThirdCharacter::ClampStats()
{
    Health = FMath::Clamp(Health, 0.f, MaxHealth);
    Energy = FMath::Clamp(Energy, 0.f, MaxEnergy);
    Stamina = FMath::Clamp(Stamina, 0.f, MaxStamina);
}

void AThirdCharacter::Move(const FInputActionValue& Value)
{
    if (const AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (!GS->bFightStarted) return;
    }

    FVector2D MovementVector = Value.Get<FVector2D>();
    if (!Controller || !OtherPlayer) return;

    MovementVector.Y = 0.f;

    float EnemyY = OtherPlayer->GetActorLocation().Y;
    float MyY = GetActorLocation().Y;
    bool bEnemyRight = (EnemyY > MyY);

    bool bPressingLeft = MovementVector.X < -0.1f;
    bool bPressingRight = MovementVector.X > 0.1f;

    bool bRetreating = (bEnemyRight && bPressingLeft) || (!bEnemyRight && bPressingRight);

    bCanBlock = bRetreating;

    // Si hay movimiento mientras se está bloqueando, desactivar bloqueo automáticamente
    if (bIsBlocking && !FMath::IsNearlyZero(MovementVector.X))
    {
        StopBlockAnimation();
    }

    if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
    {
        MoveComp->MaxWalkSpeed = bRetreating ? 280.f : 250.f;
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

bool AThirdCharacter::CanMoveTowards(const FVector& Direction) const
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

void AThirdCharacter::ServerSetBlocking_Implementation(bool bNewBlocking)
{
    bIsBlocking = bNewBlocking;
}

void AThirdCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AThirdCharacter, Health);
    DOREPLIFETIME(AThirdCharacter, MaxHealth);
    DOREPLIFETIME(AThirdCharacter, Energy);
    DOREPLIFETIME(AThirdCharacter, MaxEnergy);
    DOREPLIFETIME(AThirdCharacter, Stamina);
    DOREPLIFETIME(AThirdCharacter, MaxStamina);
    DOREPLIFETIME(AThirdCharacter, OtherPlayer);
	DOREPLIFETIME(AThirdCharacter, bIsPunching);
	DOREPLIFETIME(AThirdCharacter, bIsBlocking);
	DOREPLIFETIME(AThirdCharacter, bCanBlockDamage);
	DOREPLIFETIME(AThirdCharacter, bIsHitReacting);
}

bool AThirdCharacter::ArePushBoxesOverlapping(AThirdCharacter* Other) const
{
    if (!Pushbox || !Other->Pushbox) return false;

    FBox MyBox = Pushbox->Bounds.GetBox();
    FBox OtherBox = Other->Pushbox->Bounds.GetBox();

    return MyBox.Intersect(OtherBox);
}

void AThirdCharacter::ResolvePush(AThirdCharacter* Other)
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

void AThirdCharacter::Punch()
{
    if (bIsPunching) return;

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

void AThirdCharacter::ServerSpawnProximity_Implementation()
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

void AThirdCharacter::ResetPunch()
{
    bIsPunching = false;
}

void AThirdCharacter::ServerPunch_Implementation()
{
    HandlePunch_Server();
}

void AThirdCharacter::HandlePunch_Server()
{
    MulticastPlayPunch();
}

void AThirdCharacter::MulticastPlayPunch_Implementation()
{
    if (PunchMontage && GetMesh())
    {
        if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
        {
            if (!AnimInstance->Montage_IsPlaying(PunchMontage))
            {
                AnimInstance->Montage_Play(PunchMontage, 1.0f);
            }
        }
    }
}

void AThirdCharacter::MulticastPlayBlock_Implementation()
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

void AThirdCharacter::ServerPlayBlock_Implementation()
{
    MulticastPlayBlock();
}

void AThirdCharacter::OnRep_OtherPlayer()
{
}

void AThirdCharacter::PlayBlockAnimation()
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

void AThirdCharacter::MulticastPlayBlockImpact_Implementation()
{
    bBlockImpact = true;
    UE_LOG(LogTemp, Warning, TEXT("[%s] Block state: BLOCK_IMPACT (golpe bloqueado)"), *GetName());
    if (GetWorld())
        GetWorld()->GetTimerManager().SetTimer(BlockImpactResetHandle, this, &AThirdCharacter::ClearBlockImpact, 0.28f, false);
}

void AThirdCharacter::ClearBlockImpact()
{
	bBlockImpact = false;
}

void AThirdCharacter::ClearHitReact()
{
	bIsHitReacting = false;
	UWorld* World = GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;
	const float ActualDuration = (HitReactStartTime > 0.f && World) ? (Now - HitReactStartTime) : 0.f;
	UE_LOG(LogTemp, Warning, TEXT("[HIT_REACT] %s END   | tiempo=%.2f | duración real=%.2fs (bIsHitReacting=false)"), *GetName(), Now, ActualDuration);
}

void AThirdCharacter::ClampKnockbackVerticalVelocity()
{
    if (!IsValid(this)) return;
    UCharacterMovementComponent* M = GetCharacterMovement();
    if (M && M->Velocity.Z > 0.f)
    {
        M->Velocity.Z = 0.f;
    }
}

void AThirdCharacter::OnSuccessfulBlock(AActor* Attacker)
{
}

void AThirdCharacter::NotifyIncomingAttack(AThirdCharacter* From)
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

void AThirdCharacter::OnIncomingAttackNow(AThirdCharacter* From)
{
    UE_LOG(LogTemp, Warning, TEXT("[%s] OnIncomingAttackNow: ATTACK HAPPENS from %s"), *GetName(), *From->GetName());
}

void AThirdCharacter::ClearIncomingAttack()
{
    bHasIncomingAttack = false;
    IncomingFrom = nullptr;
}

void AThirdCharacter::ServerDestroyProximity_Implementation()
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

void AThirdCharacter::ServerEnableBlockHitbox_Implementation(FVector SocketLocation, FRotator SocketRotation, float Duration)
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
        GetWorldTimerManager().SetTimer(BlockWindowTimerHandle, this, &AThirdCharacter::OnBlockWindowExpired, UseDuration, false);
    }
}

void AThirdCharacter::ServerDisableBlockHitbox_Implementation()
{
    if (!HasAuthority()) return;

    if (bBlockStartedByInput)
        return;

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

void AThirdCharacter::ServerStartBlockByInput_Implementation()
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


void AThirdCharacter::OnBlockWindowExpired()
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
