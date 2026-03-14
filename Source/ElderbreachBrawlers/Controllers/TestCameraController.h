// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "TestCameraController.generated.h"

class ATestCameraActor;
class UInputMappingContext;
class UInputAction;

/**
 * 
 */
UCLASS()
class ELDERBREACHBRAWLERS_API ATestCameraController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ATestCameraController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void Tick(float DeltaTime) override;

	// Referencia al actor de cámara
	UPROPERTY(BlueprintReadOnly)
	ATestCameraActor* TestCameraActor;

public:
	// Getter para acceder al TestCameraActor desde fuera
	ATestCameraActor* GetTestCameraActor() const { return TestCameraActor; }

	// Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputMappingContext* FreeCameraMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* UpDownAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* ToggleCollisionAction;

	// Sensibilidad de rotación
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float LookSensitivity = 1.0f;

	// Funciones de input
	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnUpDown(const FInputActionValue& Value);
	void OnToggleCollisionVisibility(const FInputActionValue& Value);

	// Estado de la cámara
	bool bFreeCameraMode = false;
	void EnableFreeCamera();
	void DisableFreeCamera();
};
