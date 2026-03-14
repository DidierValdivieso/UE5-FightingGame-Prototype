#include "TestCameraActor.h"
#include "Components/SceneComponent.h"
#include "../MainGameState.h"
#include "../Characters/IO89Character.h"
#include "../Subsystems/CombatTestSubsystem.h"

// Sets default values
ATestCameraActor::ATestCameraActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Crear root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Crear componente de cámara
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
}

// Called when the game starts or when spawned
void ATestCameraActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Configurar la posición inicial de la cámara basada en el nivel
	FVector StartLocation = FVector(-500, 0, 200);
	FRotator StartRotation = FRotator(-10, 0, 0);
	
	// Obtener el nombre del nivel desde CombatTestSubsystem
	FName LevelName;
	if (UCombatTestSubsystem* Subsystem = GetWorld()->GetSubsystem<UCombatTestSubsystem>())
	{
		LevelName = Subsystem->ActiveConfig.InitialMap;
	}
	
	// Si hay personajes spawnados, usar sus posiciones
	if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
	{
		if (GS->PlayerOneCharacter && GS->PlayerTwoCharacter)
		{
			// Calcular el punto medio entre los dos personajes
			FVector Player1Loc = GS->PlayerOneCharacter->GetActorLocation();
			FVector Player2Loc = GS->PlayerTwoCharacter->GetActorLocation();
			FVector MidPoint = (Player1Loc + Player2Loc) * 0.5f;
			
			// Calcular distancia entre personajes para ajustar la distancia de la cámara
			float Distance = FVector::Dist(Player1Loc, Player2Loc);
			float CameraDistance = FMath::Max(500.0f, Distance * 0.5f);
			
			// Posicionar la cámara al frente (en X negativo) y ligeramente arriba
			// La altura depende del nivel
			float CameraHeight = 200.0f;
			FString LevelString = LevelName.ToString();
			
			// Ajustar altura según el nivel (basado en las posiciones Z de los personajes)
			if (LevelString == "L_ElderBreach")
			{
				CameraHeight = 200.0f; // Nivel bajo
			}
			else if (LevelString == "L_GarbageEjection")
			{
				CameraHeight = -3200.0f; // Nivel muy bajo
			}
			else
			{
				CameraHeight = MidPoint.Z + 200.0f;
			}
			
			StartLocation = MidPoint + FVector(-CameraDistance, 0, CameraHeight - MidPoint.Z);
		}
		else if (GS->PlayerOneCharacter)
		{
			FVector PlayerLoc = GS->PlayerOneCharacter->GetActorLocation();
			StartLocation = PlayerLoc + FVector(-500, 0, 200);
		}
		else
		{
			FString LevelString = LevelName.ToString();
			
			if (LevelString == "L_ElderBreach")
			{
				StartLocation = FVector(-2300, 0, 300);
			}
			else if (LevelString == "L_GarbageEjection")
			{
				StartLocation = FVector(-560, 5750, -3200);
			}
			else if (LevelString == "L_Maintenance")
			{
				StartLocation = FVector(-750, -6600, 3230);
			}
			else if (LevelString == "L_PaddockLeft")
			{
				StartLocation = FVector(-750, -6600, 4330);
			}
			else if (LevelString == "L_PaddockRight")
			{
				StartLocation = FVector(-750, -6600, 5430);
			}
			else if (LevelString == "L_ConcourseLeft")
			{
				StartLocation = FVector(-750, -4400, 3230);
			}
			else if (LevelString == "L_VendorCorridor_Left")
			{
				StartLocation = FVector(-750, -4400, 4330);
			}
			else if (LevelString == "L_Arena")
			{
				StartLocation = FVector(-750, -4400, 5430);
			}			
		}
	}
	
	SetActorLocation(StartLocation);
	SetActorRotation(StartRotation);
}

void ATestCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ATestCameraActor::MoveCamera(const FVector2D& MovementVector, const FRotator& CameraRotation)
{
	FVector ForwardVector = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::X);
	FVector RightVector = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);
	
	FVector Movement = (ForwardVector * MovementVector.Y + RightVector * MovementVector.X) * MoveSpeed * GetWorld()->GetDeltaSeconds();
	
	FVector NewLocation = GetActorLocation() + Movement;
	SetActorLocation(NewLocation);
}

void ATestCameraActor::MoveUpDown(float UpDownValue)
{
	FVector UpVector = FVector::UpVector;
	FVector Movement = UpVector * UpDownValue * UpDownSpeed * GetWorld()->GetDeltaSeconds();
	
	FVector NewLocation = GetActorLocation() + Movement;
	SetActorLocation(NewLocation);
}

