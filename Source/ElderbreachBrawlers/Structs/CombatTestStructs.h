#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Materials/MaterialInterface.h"
#include "CombatTestStructs.generated.h"

UENUM(BlueprintType)
enum class ECombatControllerType : uint8
{
    AggressiveAI UMETA(DisplayName = "Aggressive AI"),
    DefensiveAI UMETA(DisplayName = "Defensive AI"),
    IdleAI      UMETA(DisplayName = "Idle AI"),
    Player      UMETA(DisplayName = "Player"),
    Spectator   UMETA(DisplayName = "Spectator")
};

USTRUCT(BlueprintType)
struct FParticipantConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ECombatControllerType ControllerType = ECombatControllerType::AggressiveAI;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<APawn> PawnClass = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector SpawnLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRotator SpawnRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 DifficultyLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsHumanPlayer = false;

    /** Material opcional para el body del personaje (ej. para diferenciar enemigo). Slot 0 por defecto. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    UMaterialInterface* BodyMaterialOverride = nullptr;

    /** Índice del material slot del body en el SkeletalMesh (0 por defecto). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
    int32 BodyMaterialSlotIndex = 0;
};

USTRUCT(BlueprintType)
struct FCombatTestConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FParticipantConfig> Participants;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bUseSpectator = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxTestDuration = 60.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bDebugMode = true;

    UPROPERTY()
    FName InitialMap;
};
