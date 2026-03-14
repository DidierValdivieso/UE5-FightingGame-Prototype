// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Camera/CameraComponent.h"
#include "TestCameraActor.generated.h"

UCLASS()
class ELDERBREACHBRAWLERS_API ATestCameraActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestCameraActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Componente de cámara
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* CameraComponent;

	// Velocidad de movimiento
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float MoveSpeed = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float UpDownSpeed = 500.0f;

	// Funciones de movimiento
	void MoveCamera(const FVector2D& MovementVector, const FRotator& CameraRotation);
	void MoveUpDown(float UpDownValue);

};
