#include "BlockHitboxActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "../Characters/MainCharacter.h"
#include "../Actors/AttackHitboxActor.h"
#include "../Subsystems/CollisionDebugSubsystem.h"

ABlockHitboxActor::ABlockHitboxActor()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(true);

    BlockBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockBox"));
    RootComponent = BlockBox;

    BlockBox->SetBoxExtent(FVector(50.f));
    // Si en el viewport tienes "Show → Collision" activado, el motor dibujará esta caja (wireframe).
    // Para no verla: desactiva "Show → Collision" en el viewport; no hay API para ocultar solo este componente.
    BlockBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BlockBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    BlockBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    BlockBox->SetHiddenInGame(true);
    BlockBox->SetGenerateOverlapEvents(true);

    BlockBox->OnComponentBeginOverlap.AddDynamic(this, &ABlockHitboxActor::OnBlockOverlapBegin);

    DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMesh"));
    DebugMesh->SetupAttachment(BlockBox);
    DebugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugMesh->SetHiddenInGame(true);
    DebugMesh->SetVisibility(true);

    DebugMesh->SetIsReplicated(true);
    DebugMesh->SetMobility(EComponentMobility::Movable);
    DebugMesh->bCastDynamicShadow = false;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        DebugMesh->SetStaticMesh(CubeFinder.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Game/Blueprints/Materials/M_Debug_Yellow"));
    if (MatFinder.Succeeded())
    {
        DebugMesh->SetMaterial(0, MatFinder.Object);
    }
    else
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMat(TEXT("/Engine/EngineMaterials/DefaultMaterial"));
        if (DefaultMat.Succeeded())
            DebugMesh->SetMaterial(0, DefaultMat.Object);
    }
}

void ABlockHitboxActor::BeginPlay()
{
    Super::BeginPlay();

    if (BlockBox && DebugMesh)
    {
        FVector Scale = (BlockBox->GetUnscaledBoxExtent() * 2.f) / 100.f;
        DebugMesh->SetRelativeScale3D(Scale);
        // Visibilidad se fuerza en ActivateBlock cuando el bloqueo está activo
    }

    UE_LOG(LogTemp, Warning, TEXT("[%s] BlockHitbox BeginPlay: Role=%d, NetMode=%d, MeshVisible=%d"),
        *GetName(), (int32)GetLocalRole(), (int32)GetNetMode(), DebugMesh->IsVisible());
}

void ABlockHitboxActor::ActivateBlock(AMainCharacter* InOwner)
{
    OwnerCharacter = InOwner;
    BlockBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

    // No pintar la caja al subir la guardia; solo se verá en el impacto (OnBlockOverlapBegin).
    if (DebugMesh && BlockBox)
    {
        FVector Scale = (BlockBox->GetUnscaledBoxExtent() * 2.f) / 100.f;
        DebugMesh->SetRelativeScale3D(Scale);
        DebugMesh->SetHiddenInGame(true);
        DebugMesh->SetVisibility(false);
    }

    UE_LOG(LogTemp, Warning, TEXT("[BlockHitbox] ActivateBlock spawned (Role=%d, NetMode=%d) at %s"),
        (int32)GetLocalRole(), (int32)GetNetMode(), *GetActorLocation().ToString());

    FVector Offset = GetActorForwardVector() * 20.f;
    SetActorLocation(GetActorLocation() + Offset);
}

void ABlockHitboxActor::SetDebugVisibility(bool bVisible)
{
    if (DebugMesh)
    {
        DebugMesh->SetHiddenInGame(!bVisible);
        DebugMesh->SetVisibility(bVisible);
        DebugMesh->MarkRenderStateDirty();
    }
}

void ABlockHitboxActor::DeactivateBlock()
{
    BlockBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (DebugMesh)
    {
        DebugMesh->SetHiddenInGame(true);
        DebugMesh->SetVisibility(false);
        DebugMesh->MarkRenderStateDirty();
    }
}

void ABlockHitboxActor::OnBlockOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OwnerCharacter || !OtherActor) return;

    if (AAttackHitboxActor* EnemyHitbox = Cast<AAttackHitboxActor>(OtherActor))
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("Ataque bloqueado!"));
        OwnerCharacter->OnSuccessfulBlock(EnemyHitbox->GetOwner());

        // Pintar la caja solo en el impacto si el debug de colisiones está activo (tecla F).
        if (UCollisionDebugSubsystem* Sub = GetWorld()->GetSubsystem<UCollisionDebugSubsystem>())
        {
            if (Sub->bShowCollisionDebug && DebugMesh)
            {
                SetDebugVisibility(true);
                ABlockHitboxActor* Self = this;
                GetWorld()->GetTimerManager().SetTimerForNextTick([Self]()
                {
                    if (IsValid(Self)) Self->DeactivateBlock();
                });
                return;
            }
        }
        DeactivateBlock();
    }
}
