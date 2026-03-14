#include "ProximityAttackActor.h"
#include "Components/BoxComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "../Characters/IO89Character.h"
#include "../Subsystems/CollisionDebugSubsystem.h"

AProximityAttackActor::AProximityAttackActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    ProxBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ProximityBox"));
    RootComponent = ProxBox;    

    ProxBox->SetCollisionObjectType(ECC_GameTraceChannel2);
    ProxBox->SetCollisionResponseToAllChannels(ECR_Ignore);
    ProxBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
    ProxBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    ProxBox->SetGenerateOverlapEvents(true);

    DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMesh"));
    DebugMesh->SetupAttachment(ProxBox);
    DebugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugMesh->SetHiddenInGame(true);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        DebugMesh->SetStaticMesh(CubeFinder.Object);
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> MatFinder(TEXT("/Game/Blueprints/Materials/M_Debug_Blue"));
    if (MatFinder.Succeeded())
    {
        DebugMesh->SetMaterial(0, MatFinder.Object);
    }

    ProxBox->SetHiddenInGame(true);

    bDetectFrontOnly = true;
    FrontDotThreshold = 0.5f;    
}

void AProximityAttackActor::BeginPlay()
{
    Super::BeginPlay();
    ProxBox->OnComponentBeginOverlap.AddDynamic(this, &AProximityAttackActor::OnProxBeginOverlap);
}

void AProximityAttackActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    for (AActor* A : NotifiedActors)
    {
        if (AIO89Character* Victim = Cast<AIO89Character>(A))
        {
            Victim->StopBlockAnimation();
        }
    }
    NotifiedActors.Empty();

    if (OwnerCharacter)
    {
        if (AIO89Character* OwnerChar = Cast<AIO89Character>(OwnerCharacter))
        {
            if (OwnerChar->ActiveProximityAttack == this)
            {
                OwnerChar->ActiveProximityAttack = nullptr;
            }
        }
    }

    Super::EndPlay(EndPlayReason);
}

void AProximityAttackActor::Init(AActor* InOwner, const FVector& Location, const FVector& ProxExtent)
{
    OwnerCharacter = InOwner;
    SetActorLocation(Location);
    ProxBox->SetBoxExtent(ProxExtent);

    FVector Scale = (ProxExtent * 2.f) / 100.f;
    DebugMesh->SetWorldScale3D(Scale);
    if (UCollisionDebugSubsystem* Sub = GetWorld()->GetSubsystem<UCollisionDebugSubsystem>())
    {
        DebugMesh->SetHiddenInGame(!Sub->bShowCollisionDebug);
    }
    else
    {
        DebugMesh->SetHiddenInGame(true);
    }

    NotifiedActors.Empty();

    ProxBox->UpdateOverlaps();
    TArray<UPrimitiveComponent*> OverlappingComps;
    ProxBox->GetOverlappingComponents(OverlappingComps);

    AIO89Character* Attacker = Cast<AIO89Character>(OwnerCharacter);
    if (!Attacker) return;

    for (UPrimitiveComponent* Comp : OverlappingComps)
    {
        if (!Comp || !Comp->ComponentHasTag(FName("TorsoHitbox"))) continue;

        AActor* OtherActor = Comp->GetOwner();
        if (!OtherActor || OtherActor == OwnerCharacter || NotifiedActors.Contains(OtherActor))
            continue;

        AIO89Character* Victim = Cast<AIO89Character>(OtherActor);
        if (!Victim) continue;

        if (bDetectFrontOnly)
        {
            FVector DirToVictim = (Victim->GetActorLocation() - Attacker->GetActorLocation()).GetSafeNormal();
            float Dot = FVector::DotProduct(Attacker->GetActorForwardVector(), DirToVictim);
            if (Dot < FrontDotThreshold)
                continue;
        }

        NotifiedActors.Add(OtherActor);
        Victim->NotifyIncomingAttack(Attacker);
    }
}


void AProximityAttackActor::OnProxBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || !OtherComp) return;
    if (!OwnerCharacter) return;
    if (OtherActor == OwnerCharacter) return;
    if (!OtherComp->ComponentHasTag(FName("TorsoHitbox"))) return;
    if (NotifiedActors.Contains(OtherActor)) return;

    AIO89Character* Victim = Cast<AIO89Character>(OtherActor);
    AIO89Character* Attacker = Cast<AIO89Character>(OwnerCharacter);
    if (!Victim || !Attacker) return;

    NotifiedActors.Add(OtherActor);

    Victim->NotifyIncomingAttack(Attacker);
}

void AProximityAttackActor::DestroySelf()
{
    for (AActor* A : NotifiedActors)
    {
        if (AIO89Character* Victim = Cast<AIO89Character>(A))
        {
            Victim->StopBlockAnimation();
        }
    }
    NotifiedActors.Empty();

    if (OwnerCharacter)
    {
        if (AIO89Character* OwnerChar = Cast<AIO89Character>(OwnerCharacter))
        {
            if (OwnerChar->ActiveProximityAttack == this)
            {
                OwnerChar->ActiveProximityAttack = nullptr;
            }
        }
    }

    Destroy();
}