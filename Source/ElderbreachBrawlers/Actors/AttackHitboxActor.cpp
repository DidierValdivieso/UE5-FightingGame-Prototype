#include "AttackHitboxActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"
#include "../Characters/IO89Character.h"
#include "FightingCameraActor.h"
#include "../Subsystems/CollisionDebugSubsystem.h"

AAttackHitboxActor::AAttackHitboxActor()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);

    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    RootComponent = Box;

    Box->SetCollisionObjectType(ECC_GameTraceChannel2);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore);
    Box->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Overlap);
    Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    Box->SetGenerateOverlapEvents(false);
    Box->SetHiddenInGame(true);

    DebugMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugMesh"));
    DebugMesh->SetupAttachment(Box);
    DebugMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugMesh->SetHiddenInGame(true);
    DebugMesh->SetVisibility(false);
    DebugMesh->SetIsReplicated(true);
    DebugMesh->SetMobility(EComponentMobility::Movable);
    DebugMesh->bCastDynamicShadow = false;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeFinder.Succeeded())
    {
        DebugMesh->SetStaticMesh(CubeFinder.Object);
    }

    // Cargar materiales según estado: Yellow (activo), Green (hit), Red (blocked)
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> YellowMat(TEXT("/Game/Blueprints/Materials/M_Debug_Yellow"));
    if (YellowMat.Succeeded())
    {
        DebugMaterialYellow = YellowMat.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GreenMat(TEXT("/Game/Blueprints/Materials/M_Debug_Green"));
    if (GreenMat.Succeeded())
    {
        DebugMaterialGreen = GreenMat.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> RedMat(TEXT("/Game/Blueprints/Materials/M_Debug_Red"));
    if (RedMat.Succeeded())
    {
        DebugMaterialRed = RedMat.Object;
    }

    // Material por defecto: amarillo (activo)
    if (DebugMaterialYellow)
    {
        DebugMesh->SetMaterial(0, DebugMaterialYellow);
    }
    else
    {
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMat(TEXT("/Engine/EngineMaterials/DefaultMaterial"));
        if (DefaultMat.Succeeded())
            DebugMesh->SetMaterial(0, DefaultMat.Object);
    }

    DamageAmount = 10.f;
    DamageTypeClass = UDamageType::StaticClass();
    OwnerCharacter = nullptr;
    bOverlapBound = false;
    bHitSomething = false;
}

void AAttackHitboxActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAttackHitboxActor, HitState);
}

void AAttackHitboxActor::OnRep_HitState()
{
    UpdateDebugMaterial();
}

void AAttackHitboxActor::MulticastSetHitResult_Implementation(EHitboxDebugState ResolvedState)
{
    HitState = ResolvedState;
}

void AAttackHitboxActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    UWorld* World = GetWorld();
    if (!World) return;
    
    // Actualizar visibilidad del DebugMesh según el subsistema de debug (tecla F)
    UCollisionDebugSubsystem* Sub = World->GetSubsystem<UCollisionDebugSubsystem>();
    const bool bShouldShow = Sub && Sub->bShowCollisionDebug;
    
    if (DebugMesh)
    {
        DebugMesh->SetHiddenInGame(!bShouldShow);
        DebugMesh->SetVisibility(bShouldShow);
        
        // Actualizar material según estado (solo si está visible)
        if (bShouldShow)
        {
            UpdateDebugMaterial();
        }
    }

    // DrawDebugBox solo en builds de desarrollo (para debug adicional)
    #if !UE_BUILD_SHIPPING
    if (bDrawDebug && bShouldShow)
    {
        FColor Color;
        switch (HitState)
        {
        case EHitboxDebugState::Active:  Color = FColor::Yellow; break;
        case EHitboxDebugState::Hit:     Color = FColor::Green; break;
        case EHitboxDebugState::Blocked: Color = FColor::Red; break;
        default: Color = FColor::White; break;
        }

        const FVector Center = Box->GetComponentLocation();
        const FVector Extent = Box->GetScaledBoxExtent();
        const FQuat Rotation = Box->GetComponentQuat();
        DrawDebugBox(World, Center, Extent, Rotation, Color, false, 0.f, 0, 2.f);
    }
    #endif
}

void AAttackHitboxActor::BeginPlay()
{
    Super::BeginPlay();

    if (!OwnerCharacter)
    {
        AActor* OwnerFromGet = GetOwner();
        if (OwnerFromGet)
            OwnerCharacter = OwnerFromGet;
    }
}

void AAttackHitboxActor::InitHitbox(AActor* InOwner, const FVector& Location, const FVector& Extent, float Duration)
{
    OwnerCharacter = InOwner;
    SetOwner(InOwner);
    SetActorLocation(Location);

    Box->SetBoxExtent(Extent);

    // Ajustar escala del DebugMesh al tamaño del Box
    if (DebugMesh && Box)
    {
        FVector Scale = (Extent * 2.f) / 100.f;
        DebugMesh->SetRelativeScale3D(Scale);
        
        // Verificar estado del subsistema de debug al inicializar
        if (UWorld* World = GetWorld())
        {
            if (UCollisionDebugSubsystem* Sub = World->GetSubsystem<UCollisionDebugSubsystem>())
            {
                DebugMesh->SetHiddenInGame(!Sub->bShowCollisionDebug);
                DebugMesh->SetVisibility(Sub->bShowCollisionDebug);
            }
        }
    }

    HitState = EHitboxDebugState::Active;
    UpdateDebugMaterial(); // Asegurar material inicial (amarillo)

    UE_LOG(LogTemp, Warning, TEXT("InitHitbox: Owner=%s Loc=%s Extent=%s Duration=%.2f"),
        OwnerCharacter ? *OwnerCharacter->GetName() : TEXT("null"),
        *Location.ToString(),
        *Extent.ToString(),
        Duration);

    if (!bOverlapBound)
    {
        Box->OnComponentBeginOverlap.AddDynamic(this, &AAttackHitboxActor::OnOverlap);
        bOverlapBound = true;
    }

    Box->SetGenerateOverlapEvents(true); 
    Box->UpdateOverlaps();
    // No volver a asignar HitState aquí: UpdateOverlaps() puede haber llamado OnOverlap y puesto Hit/Blocked.

    if (Duration > 0.f)
        GetWorldTimerManager().SetTimer(DestroyHandle, this, &AAttackHitboxActor::DestroyHitbox, Duration, false);
}

void AAttackHitboxActor::OnOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || OtherActor == OwnerCharacter) return;
    if (ActorsHit.Contains(OtherActor)) return;

    AIO89Character* Victim = Cast<AIO89Character>(OtherActor);
    if (!Victim) return;

    ActorsHit.Add(OtherActor);
    bHitSomething = true;

    AIO89Character* Attacker = Cast<AIO89Character>(OwnerCharacter);
    if (Attacker)
    {
        bool bBlocked = Victim->TryReceiveHit(Attacker, DamageAmount);
        UE_LOG(LogTemp, Warning, TEXT("OnOverlap: Victim=%s Blocked=%d"), *Victim->GetName(), bBlocked ? 1 : 0);

        HitState = bBlocked ? EHitboxDebugState::Blocked : EHitboxDebugState::Hit;
        MulticastSetHitResult(HitState);
        UpdateDebugMaterial();

        // Vibración de cámara en hit efectivo
        if (!bBlocked && GetWorld())
        {
            TArray<AActor*> FoundCameras;
            UGameplayStatics::GetAllActorsOfClass(GetWorld(), AFightingCameraActor::StaticClass(), FoundCameras);
            for (AActor* CamActor : FoundCameras)
            {
                if (AFightingCameraActor* FightCam = Cast<AFightingCameraActor>(CamActor))
                {
                    FightCam->TriggerHitShake();
                    break;
                }
            }
        }

        // Retroceso del atacante: mismo efecto en golpe y en bloqueo (similar a Tekken)
        if (Attacker->HasAuthority())
        {
            FVector Dir = Attacker->GetActorLocation() - Victim->GetActorLocation();
            Dir.Z = 0.f;
            Dir.Normalize();

            const FVector Pushback = Dir * 60.f;
            Attacker->LaunchCharacter(Pushback, false, false);
        }
    }

    const float PostHitVisibleTime = 0.25f;
    GetWorldTimerManager().ClearTimer(DestroyHandle);
    GetWorldTimerManager().SetTimer(DestroyHandle, this, &AAttackHitboxActor::DestroyHitbox, PostHitVisibleTime, false);
}

void AAttackHitboxActor::DestroyHitbox()
{
    DestroyNow();
}

void AAttackHitboxActor::DestroyNow()
{
    Destroy();
}

void AAttackHitboxActor::UpdateDebugMaterial()
{
    if (!DebugMesh) return;

    UMaterialInterface* MaterialToUse = nullptr;
    switch (HitState)
    {
    case EHitboxDebugState::Active:
        MaterialToUse = DebugMaterialYellow;
        break;
    case EHitboxDebugState::Hit:
        MaterialToUse = DebugMaterialGreen;
        break;
    case EHitboxDebugState::Blocked:
        MaterialToUse = DebugMaterialRed;
        break;
    default:
        MaterialToUse = DebugMaterialYellow;
        break;
    }

    if (MaterialToUse)
    {
        DebugMesh->SetMaterial(0, MaterialToUse);
    }
}