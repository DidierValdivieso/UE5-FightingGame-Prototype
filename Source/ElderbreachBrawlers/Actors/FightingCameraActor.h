#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "../Characters/IO89Character.h"
#include "FightingCameraActor.generated.h"

class UCineCameraComponent;

UCLASS()
class ELDERBREACHBRAWLERS_API AFightingCameraActor : public AActor
{
	GENERATED_BODY()
	
public:
    AFightingCameraActor();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
    UCineCameraComponent* CameraComponent;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MinZoom;

    UPROPERTY(EditAnywhere, Category = "Camera")
    float MaxZoom;

    UPROPERTY()
    AIO89Character* Player1;

    UPROPERTY()
    AIO89Character* Player2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float HeightOffset;

    // Funciones para movimiento manual de cámara
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void MoveCamera(FVector2D MovementInput);

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void MoveCameraVertical(float VerticalInput);

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void RotateCamera(FVector2D RotationInput);

    UFUNCTION(BlueprintCallable, Category = "Camera")
    void ZoomCamera(float ZoomInput);

    /** Llamar cuando ocurre un hit efectivo para vibrar la cámara. */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void TriggerHitShake();

    /** Congela la cámara en su posición/rotación actual (útil al final de la pelea para evitar rotación al destruir personajes). */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void FreezeCameraForFightEnd();

    /** Descongela la cámara para que vuelva a seguir a los jugadores. */
    UFUNCTION(BlueprintCallable, Category = "Camera")
    void UnfreezeCamera();

protected:
    virtual void BeginPlay() override;

private:
    FTimerHandle TimerHandle_FindPlayers;
    bool bPlayersFound = false;

    void TryFindPlayers();
    bool AreBothControllersIdleAI() const;
    void CalculateLevelBounds();
    FVector ClampCameraLocation(const FVector& Location) const;

    // Variables para movimiento manual
    bool bManualCameraMode = false;
    FVector ManualCameraLocation;
    FRotator ManualCameraRotation;
    float ManualZoom = 400.f;

    // Límites del nivel
    FVector LevelMinBounds;
    FVector LevelMaxBounds;
    bool bLevelBoundsCalculated = false;

    // Velocidad de movimiento y rotación
    UPROPERTY(EditAnywhere, Category = "Camera Movement")
    float CameraMoveSpeed = 500.f;

    UPROPERTY(EditAnywhere, Category = "Camera Movement")
    float CameraRotationSpeed = 50.f;

    UPROPERTY(EditAnywhere, Category = "Camera Movement")
    float CameraZoomSpeed = 100.f;

    // Vibración de cámara en hit efectivo
    UPROPERTY(EditAnywhere, Category = "Camera Shake")
    float HitShakeDuration = 0.1f;
    UPROPERTY(EditAnywhere, Category = "Camera Shake")
    float HitShakeMagnitude = 10.0f;
    float ShakeTimeRemaining = 0.f;
    FVector ShakeOffset = FVector::ZeroVector;

    // Congelar cámara al final de la pelea (último round) para mantener la vista
    bool bFrozenForFightEnd = false;
    FVector FrozenLocation = FVector::ZeroVector;
    FRotator FrozenRotation = FRotator::ZeroRotator;

};
