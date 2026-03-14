#include "FightingCameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"
#include "../Characters/IO89Character.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"
#include "CineCameraComponent.h"
#include <ElderbreachBrawlers/MainGameState.h>
#include "../Controllers/IdleAIController.h"
#include "../Subsystems/CombatTestSubsystem.h"
#include "../Structs/CombatTestStructs.h"


AFightingCameraActor::AFightingCameraActor()
{
    PrimaryActorTick.bCanEverTick = true;

    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);
    bPlayersFound = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    CameraComponent = CreateDefaultSubobject<UCineCameraComponent>(TEXT("CineCamera"));
    CameraComponent->SetupAttachment(RootComponent);
    CameraComponent->SetRelativeRotation(FRotator(5.f, 0.f, 0.f));

    UCineCameraComponent* CineCam = CameraComponent;

    if (CineCam)
    {
        // Filmback
        CineCam->SetFilmbackPresetByName(TEXT("16:9 Digital Film"));

        // Lens preset
        CineCam->SetLensPresetByName(TEXT("12mm Prime f/2.8"));

        // Focal y apertura
        CineCam->CurrentFocalLength = 12.0f;
        CineCam->CurrentAperture = 2.8f;

        // Auto-focus manual inicial
        CineCam->FocusSettings.FocusMethod = ECameraFocusMethod::Manual;
        CineCam->FocusSettings.ManualFocusDistance = 300.f;
    }


    MinZoom = 270.f;
    MaxZoom = 550.f;
    HeightOffset = -20.f;    
}

void AFightingCameraActor::BeginPlay()
{
    Super::BeginPlay();

    // No asignar FightCamRef aquí: el MainGameMode y CombatTestSubsystem lo hacen.
    // Si cada cámara pusiera FightCamRef=this, la que ejecute BeginPlay después
    // (p. ej. la del GM al replicar) robaba la vista y la cámara "caía".

    TryFindPlayers();

    if (!Player1 || !Player2)
    {
        GetWorldTimerManager().SetTimer(TimerHandle_FindPlayers, this, &AFightingCameraActor::TryFindPlayers, 0.25f, true);
    }
}

void AFightingCameraActor::TryFindPlayers()
{
    // No sobrescribir si ya nos asignaron jugadores (ej. CombatTestSubsystem::SetupFightCamera)
    if (Player1 && Player2)
    {
        GetWorldTimerManager().ClearTimer(TimerHandle_FindPlayers);
        return;
    }

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AIO89Character::StaticClass(), FoundActors);

    if (FoundActors.Num() >= 2)
    {
        Player1 = Cast<AIO89Character>(FoundActors[0]);
        Player2 = Cast<AIO89Character>(FoundActors[1]);

        if (Player1 && Player2)
        {
            GetWorldTimerManager().ClearTimer(TimerHandle_FindPlayers);
        }
    }
}

void AFightingCameraActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Si estamos ocultos o desactivados, no hacer nada
    if (IsHidden() || !IsActorTickEnabled()) return;

    // Si la cámara está congelada (fin de pelea), mantener posición guardada
    if (bFrozenForFightEnd)
    {
        SetActorLocation(FrozenLocation);
        SetActorRotation(FrozenRotation);
        return;
    }

    if (!Player1 || !Player2) return;

    // Calcular límites si aún no se han calculado
    if (!bLevelBoundsCalculated)
    {
        CalculateLevelBounds();
    }

    // Verificar si ambos controladores son IdleAI
    bool bBothIdle = AreBothControllersIdleAI();
    
    // Si acabamos de entrar en modo manual, inicializar posición
    if (bBothIdle && !bManualCameraMode)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FightingCameraActor] Entering manual camera mode - both controllers are IdleAI"));
        FVector MidPoint = (Player1->GetActorLocation() + Player2->GetActorLocation()) * 0.5f;
        float Distance = FVector::Dist(Player1->GetActorLocation(), Player2->GetActorLocation());
        ManualZoom = FMath::Clamp(Distance, MinZoom, MaxZoom);
        ManualCameraLocation = MidPoint + FVector(-ManualZoom, 0.f, HeightOffset);
        ManualCameraRotation = (MidPoint - ManualCameraLocation).Rotation();
    }
    
    bManualCameraMode = bBothIdle;

    // Actualizar vibración por hit
    if (ShakeTimeRemaining > 0.f)
    {
        ShakeTimeRemaining = FMath::Max(0.f, ShakeTimeRemaining - DeltaTime);
        float T = ShakeTimeRemaining / HitShakeDuration;
        ShakeOffset = FVector(
            FMath::RandRange(-1.f, 1.f),
            FMath::RandRange(-1.f, 1.f),
            FMath::RandRange(-1.f, 1.f)
        ) * HitShakeMagnitude * T;
    }
    else
    {
        ShakeOffset = FVector::ZeroVector;
    }

    if (bManualCameraMode)
    {
        // Modo manual: usar posición y rotación manuales
        // NO actualizar la posición automáticamente, solo aplicar la posición manual
        FVector ClampedLocation = ClampCameraLocation(ManualCameraLocation) + ShakeOffset;
        SetActorLocation(ClampedLocation, false);
        SetActorRotation(ManualCameraRotation);
        
        // Log periódico para debug (cada segundo aproximadamente)
        static float LogTimer = 0.f;
        LogTimer += DeltaTime;
        if (LogTimer >= 1.0f)
        {
            LogTimer = 0.f;
            UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] Manual mode active - Location: (%.2f, %.2f, %.2f), Rotation: (%.2f, %.2f, %.2f)"), 
                ClampedLocation.X, ClampedLocation.Y, ClampedLocation.Z,
                ManualCameraRotation.Pitch, ManualCameraRotation.Yaw, ManualCameraRotation.Roll);
        }
    }
    else
    {
        // Modo automático: punto medio en altura de referencia (pies + offset) para que la cámara no baje al agacharse
        const float StandingCapsuleHalfHeight = 96.f;
        auto GetReferencePosition = [StandingCapsuleHalfHeight](ACharacter* Char) -> FVector
        {
            if (!Char || !Char->GetCapsuleComponent()) return Char ? Char->GetActorLocation() : FVector::ZeroVector;
            FVector Loc = Char->GetActorLocation();
            float FeetZ = Loc.Z - Char->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            return FVector(Loc.X, Loc.Y, FeetZ + StandingCapsuleHalfHeight);
        };
        FVector Ref1 = GetReferencePosition(Player1);
        FVector Ref2 = GetReferencePosition(Player2);
        FVector MidPoint = (Ref1 + Ref2) * 0.5f;
        float Distance = FVector::Dist(Ref1, Ref2);
        float TargetZoom = FMath::Clamp(Distance, MinZoom, MaxZoom);
        FVector CameraLocation = MidPoint + FVector(-TargetZoom, 0.f, HeightOffset) + ShakeOffset;
        SetActorLocation(CameraLocation);
        FRotator LookAtRotation = (MidPoint - GetActorLocation()).Rotation();
        SetActorRotation(LookAtRotation);
    }
}

bool AFightingCameraActor::AreBothControllersIdleAI() const
{
    if (!Player1 || !Player2) return false;

    AController* Controller1 = Player1->GetController();
    AController* Controller2 = Player2->GetController();

    if (!Controller1 || !Controller2) return false;

    return Cast<AIdleAIController>(Controller1) != nullptr && 
           Cast<AIdleAIController>(Controller2) != nullptr;
}

void AFightingCameraActor::CalculateLevelBounds()
{
    if (bLevelBoundsCalculated) return;

    // Obtener las posiciones de spawn desde el CombatTestSubsystem
    if (UWorld* World = GetWorld())
    {
        if (UCombatTestSubsystem* Subsystem = World->GetSubsystem<UCombatTestSubsystem>())
        {
            const FCombatTestConfig& Config = Subsystem->ActiveConfig;
            
            if (Config.Participants.Num() >= 2)
            {
                FVector P1Spawn = Config.Participants[0].SpawnLocation;
                FVector P2Spawn = Config.Participants[1].SpawnLocation;

                // Calcular límites basados en las posiciones de spawn con un margen
                float Margin = 2000.f; // Margen en unidades
                
                LevelMinBounds.X = FMath::Min(P1Spawn.X, P2Spawn.X) - Margin;
                LevelMinBounds.Y = FMath::Min(P1Spawn.Y, P2Spawn.Y) - Margin;
                LevelMinBounds.Z = FMath::Min(P1Spawn.Z, P2Spawn.Z) - Margin;

                LevelMaxBounds.X = FMath::Max(P1Spawn.X, P2Spawn.X) + Margin;
                LevelMaxBounds.Y = FMath::Max(P1Spawn.Y, P2Spawn.Y) + Margin;
                LevelMaxBounds.Z = FMath::Max(P1Spawn.Z, P2Spawn.Z) + Margin;

                bLevelBoundsCalculated = true;
                return;
            }
        }
    }

    // Si no se pueden obtener los límites, usar límites por defecto basados en las posiciones actuales
    if (Player1 && Player2)
    {
        FVector P1Loc = Player1->GetActorLocation();
        FVector P2Loc = Player2->GetActorLocation();
        float Margin = 2000.f;

        LevelMinBounds.X = FMath::Min(P1Loc.X, P2Loc.X) - Margin;
        LevelMinBounds.Y = FMath::Min(P1Loc.Y, P2Loc.Y) - Margin;
        LevelMinBounds.Z = FMath::Min(P1Loc.Z, P2Loc.Z) - Margin;

        LevelMaxBounds.X = FMath::Max(P1Loc.X, P2Loc.X) + Margin;
        LevelMaxBounds.Y = FMath::Max(P1Loc.Y, P2Loc.Y) + Margin;
        LevelMaxBounds.Z = FMath::Max(P1Loc.Z, P2Loc.Z) + Margin;

        bLevelBoundsCalculated = true;
    }
}

FVector AFightingCameraActor::ClampCameraLocation(const FVector& Location) const
{
    FVector Clamped;
    Clamped.X = FMath::Clamp(Location.X, LevelMinBounds.X, LevelMaxBounds.X);
    Clamped.Y = FMath::Clamp(Location.Y, LevelMinBounds.Y, LevelMaxBounds.Y);
    Clamped.Z = FMath::Clamp(Location.Z, LevelMinBounds.Z, LevelMaxBounds.Z);
    return Clamped;
}

void AFightingCameraActor::MoveCamera(FVector2D MovementInput)
{
    // Verificar en tiempo real si ambos controladores son IdleAI
    bool bBothIdle = AreBothControllersIdleAI();
    
    if (!bBothIdle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FightingCameraActor] MoveCamera called but controllers are not both IdleAI"));
        return;
    }

    // Asegurar que estamos en modo manual
    if (!bManualCameraMode)
    {
        bManualCameraMode = true;
        // Inicializar posición si no estaba en modo manual
        if (Player1 && Player2)
        {
            FVector MidPoint = (Player1->GetActorLocation() + Player2->GetActorLocation()) * 0.5f;
            float Distance = FVector::Dist(Player1->GetActorLocation(), Player2->GetActorLocation());
            ManualZoom = FMath::Clamp(Distance, MinZoom, MaxZoom);
            ManualCameraLocation = MidPoint + FVector(-ManualZoom, 0.f, HeightOffset);
            ManualCameraRotation = (MidPoint - ManualCameraLocation).Rotation();
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] MoveCamera: X=%.2f, Y=%.2f, Location=(%.2f, %.2f, %.2f)"), 
        MovementInput.X, MovementInput.Y, ManualCameraLocation.X, ManualCameraLocation.Y, ManualCameraLocation.Z);

    // Calcular dirección de movimiento basada en la rotación actual de la cámara
    FRotator CameraRot = ManualCameraRotation;
    
    // Obtener direcciones relativas a la cámara
    // En Unreal, X es forward (adelante), Y es right (derecha), Z is up (arriba)
    FVector Forward = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::X);
    FVector Right = FRotationMatrix(CameraRot).GetUnitAxis(EAxis::Y);
    
    // Forward apunta hacia atrás desde la cámara (X negativo en el sistema de la cámara), así que invertimos
    Forward = -Forward;
    
    // MovementInput.X = izquierda/derecha (Right)
    // MovementInput.Y = adelante/atrás (Forward)
    float DeltaTime = GetWorld()->GetDeltaSeconds();
    FVector Movement = (Forward * MovementInput.Y + Right * MovementInput.X) * CameraMoveSpeed * DeltaTime;
    
    UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] Movement vector: (%.2f, %.2f, %.2f), Speed: %.2f, DeltaTime: %.4f"), 
        Movement.X, Movement.Y, Movement.Z, CameraMoveSpeed, DeltaTime);
    
    ManualCameraLocation += Movement;
    
    UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] New Location: (%.2f, %.2f, %.2f)"), 
        ManualCameraLocation.X, ManualCameraLocation.Y, ManualCameraLocation.Z);
}

void AFightingCameraActor::MoveCameraVertical(float VerticalInput)
{
    // Verificar en tiempo real si ambos controladores son IdleAI
    bool bBothIdle = AreBothControllersIdleAI();
    
    if (!bBothIdle) return;

    // Asegurar que estamos en modo manual
    if (!bManualCameraMode)
    {
        bManualCameraMode = true;
    }

    // Movimiento vertical (Z) - arriba/abajo
    float DeltaTime = GetWorld()->GetDeltaSeconds();
    FVector VerticalMovement = FVector(0.f, 0.f, VerticalInput * CameraMoveSpeed * DeltaTime);
    ManualCameraLocation += VerticalMovement;
    
    UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] MoveCameraVertical: Input=%.2f, New Z=%.2f"), 
        VerticalInput, ManualCameraLocation.Z);
}

void AFightingCameraActor::RotateCamera(FVector2D RotationInput)
{
    // Verificar en tiempo real si ambos controladores son IdleAI
    bool bBothIdle = AreBothControllersIdleAI();
    
    if (!bBothIdle) return;

    // Asegurar que estamos en modo manual
    if (!bManualCameraMode)
    {
        bManualCameraMode = true;
    }

    float DeltaTime = GetWorld()->GetDeltaSeconds();
    float YawDelta = RotationInput.X * CameraRotationSpeed * DeltaTime;
    float PitchDelta = RotationInput.Y * CameraRotationSpeed * DeltaTime;

    ManualCameraRotation.Yaw += YawDelta;
    ManualCameraRotation.Pitch = FMath::Clamp(ManualCameraRotation.Pitch + PitchDelta, -89.f, 89.f);
    
    UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] RotateCamera: Yaw=%.2f, Pitch=%.2f"), 
        ManualCameraRotation.Yaw, ManualCameraRotation.Pitch);
}

void AFightingCameraActor::ZoomCamera(float ZoomInput)
{
    // Verificar en tiempo real si ambos controladores son IdleAI
    bool bBothIdle = AreBothControllersIdleAI();
    
    if (!bBothIdle) return;

    // Asegurar que estamos en modo manual
    if (!bManualCameraMode)
    {
        bManualCameraMode = true;
    }

    // Zoom ajustando la distancia en el eje X (hacia atrás/adelante)
    float DeltaTime = GetWorld()->GetDeltaSeconds();
    float ZoomDelta = ZoomInput * CameraZoomSpeed * DeltaTime;
    
    // Calcular dirección hacia atrás desde la cámara
    FVector Backward = -FRotationMatrix(ManualCameraRotation).GetUnitAxis(EAxis::X);
    
    ManualCameraLocation += Backward * ZoomDelta;
    
    // También actualizar el zoom manual para referencia
    ManualZoom = FMath::Clamp(ManualZoom + ZoomDelta, MinZoom, MaxZoom);
    
    UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] ZoomCamera: Input=%.2f, Zoom=%.2f"), 
        ZoomInput, ManualZoom);
}

void AFightingCameraActor::TriggerHitShake()
{
    ShakeTimeRemaining = HitShakeDuration;
}

void AFightingCameraActor::FreezeCameraForFightEnd()
{
    FrozenLocation = GetActorLocation();
    FrozenRotation = GetActorRotation();
    bFrozenForFightEnd = true;
    UE_LOG(LogTemp, Log, TEXT("[FightingCameraActor] Camera frozen for fight end at (%.2f, %.2f, %.2f)"), 
        FrozenLocation.X, FrozenLocation.Y, FrozenLocation.Z);
}

void AFightingCameraActor::UnfreezeCamera()
{
    bFrozenForFightEnd = false;
}