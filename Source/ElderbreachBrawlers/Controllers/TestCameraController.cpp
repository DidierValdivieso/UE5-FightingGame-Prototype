// Fill out your copyright notice in the Description page of Project Settings.


#include "TestCameraController.h"
#include "../Actors/TestCameraActor.h"
#include "../Subsystems/CollisionDebugSubsystem.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "InputAction.h"
#include "InputMappingContext.h"

ATestCameraController::ATestCameraController()
{
	bAutoManageActiveCameraTarget = false;
	PrimaryActorTick.bCanEverTick = true;
	TestCameraActor = nullptr;

	// Intentar cargar los InputActions y InputMappingContext desde assets
	// Usar las rutas correctas basadas en los assets existentes en el proyecto
	static ConstructorHelpers::FObjectFinder<UInputAction> MoveActionAsset(TEXT("/Game/Input/Actions/IA_Move"));
	if (MoveActionAsset.Succeeded())
	{
		MoveAction = MoveActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> LookActionAsset(TEXT("/Game/Input/Actions/IA_Look"));
	if (LookActionAsset.Succeeded())
	{
		LookAction = LookActionAsset.Object;
	}
	else
	{
		// Intentar con IA_MouseLook como alternativa
		static ConstructorHelpers::FObjectFinder<UInputAction> MouseLookActionAsset(TEXT("/Game/Input/Actions/IA_MouseLook"));
		if (MouseLookActionAsset.Succeeded())
		{
			LookAction = MouseLookActionAsset.Object;
		}
	}

	// Para UpDown, podemos usar IA_Jump o crear uno nuevo
	// Por ahora, intentamos cargar IA_Jump que ya existe
	static ConstructorHelpers::FObjectFinder<UInputAction> UpDownActionAsset(TEXT("/Game/Input/Actions/IA_Jump"));
	if (UpDownActionAsset.Succeeded())
	{
		UpDownAction = UpDownActionAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextAsset(TEXT("/Game/Input/IMC_Default"));
	if (MappingContextAsset.Succeeded())
	{
		FreeCameraMappingContext = MappingContextAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> ToggleCollisionAsset(TEXT("/Game/Input/Actions/IA_ToggleCollisionVisibility"));
	if (ToggleCollisionAsset.Succeeded())
	{
		ToggleCollisionAction = ToggleCollisionAsset.Object;
	}
}

void ATestCameraController::BeginPlay()
{
	Super::BeginPlay();

	// Spawnear el actor de cámara
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	TestCameraActor = GetWorld()->SpawnActor<ATestCameraActor>(ATestCameraActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

	if (TestCameraActor)
	{
		// Sincronizar la rotación inicial del controlador con la del actor
		SetControlRotation(TestCameraActor->GetActorRotation());
	}

	// Habilitar cámara libre por defecto
	EnableFreeCamera();
}

void ATestCameraController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] SetupInputComponent - MoveAction: %s, LookAction: %s, UpDownAction: %s, MappingContext: %s"),
		MoveAction ? *MoveAction->GetName() : TEXT("NULL"),
		LookAction ? *LookAction->GetName() : TEXT("NULL"),
		UpDownAction ? *UpDownAction->GetName() : TEXT("NULL"),
		FreeCameraMappingContext ? *FreeCameraMappingContext->GetName() : TEXT("NULL"));

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATestCameraController::OnMove);
			UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] Bound MoveAction"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[TestCameraController] MoveAction is NULL! Input will not work."));
		}

		// Desactivar LookAction - no usamos mouse para rotar
		// if (LookAction)
		// {
		// 	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATestCameraController::OnLook);
		// 	UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] Bound LookAction"));
		// }
		// else
		// {
		// 	UE_LOG(LogTemp, Error, TEXT("[TestCameraController] LookAction is NULL! Look input will not work."));
		// }

		if (UpDownAction)
		{
			EnhancedInputComponent->BindAction(UpDownAction, ETriggerEvent::Triggered, this, &ATestCameraController::OnUpDown);
			UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] Bound UpDownAction"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[TestCameraController] UpDownAction is NULL! Up/Down input will not work."));
		}

		if (ToggleCollisionAction)
		{
			EnhancedInputComponent->BindAction(ToggleCollisionAction, ETriggerEvent::Started, this, &ATestCameraController::OnToggleCollisionVisibility);
			UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] F (ToggleCollision) BOUND - CPU vs CPU con 2 IdleAI usa este controlador para F."));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[TestCameraController] ToggleCollisionAction NULL - F no enlazada."));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[TestCameraController] InputComponent is not UEnhancedInputComponent!"));
	}

	// Agregar el mapping context
	if (IsLocalController())
	{
		if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
			{
				if (FreeCameraMappingContext)
				{
					Subsystem->AddMappingContext(FreeCameraMappingContext, 0);
					UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] Added MappingContext"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("[TestCameraController] FreeCameraMappingContext is NULL! Input mapping will not work."));
				}
			}
		}
	}
}

void ATestCameraController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ATestCameraController::EnableFreeCamera()
{
	bFreeCameraMode = true;

	if (TestCameraActor)
	{
		SetViewTarget(TestCameraActor);
		// Mostrar el cursor y permitir interacción con UI
		bShowMouseCursor = true;
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;

		// Configurar input mode para permitir UI y juego
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
}

void ATestCameraController::DisableFreeCamera()
{
	bFreeCameraMode = false;
}

void ATestCameraController::OnMove(const FInputActionValue& Value)
{
	if (!bFreeCameraMode || !TestCameraActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] OnMove called but bFreeCameraMode=%d, TestCameraActor=%s"),
			bFreeCameraMode, TestCameraActor ? *TestCameraActor->GetName() : TEXT("NULL"));
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();
	
	// Movimiento simplificado: usar direcciones del mundo
	// A/D = adelante/atrás en Y (A adelante = Y positivo, D atrás = Y negativo)
	// W/S = izquierda/derecha en X (W derecha = X positivo, S izquierda = X negativo)
	// MovementVector.X es A/D (A negativo, D positivo) -> usar directamente para Y del mundo
	// MovementVector.Y es W/S (W positivo, S negativo) -> usar directamente para X del mundo
	FVector WorldMovement = FVector(MovementVector.Y, MovementVector.X, 0.0f) * TestCameraActor->MoveSpeed * GetWorld()->GetDeltaSeconds();
	
	FVector NewLocation = TestCameraActor->GetActorLocation() + WorldMovement;
	TestCameraActor->SetActorLocation(NewLocation);
	
	UE_LOG(LogTemp, VeryVerbose, TEXT("[TestCameraController] OnMove: X=%.2f, Y=%.2f"), MovementVector.X, MovementVector.Y);
}

void ATestCameraController::OnLook(const FInputActionValue& Value)
{
	if (!bFreeCameraMode || !TestCameraActor) return;

	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// Rotación horizontal (Yaw)
	AddYawInput(LookAxisVector.X * LookSensitivity);

	// Rotación vertical (Pitch)
	AddPitchInput(-LookAxisVector.Y * LookSensitivity);

	// Actualizar la rotación del actor de cámara
	FRotator NewRotation = GetControlRotation();
	TestCameraActor->SetActorRotation(NewRotation);
}

void ATestCameraController::OnUpDown(const FInputActionValue& Value)
{
	if (!bFreeCameraMode || !TestCameraActor) return;

	float UpDownValue = Value.Get<float>();
	TestCameraActor->MoveUpDown(UpDownValue);
}

void ATestCameraController::OnToggleCollisionVisibility(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Warning, TEXT("[TestCameraController] >>> F DETECTADA <<< OnToggleCollisionVisibility (CPU vs CPU con 2 IdleAI - input aqui)"));
	if (UWorld* World = GetWorld())
	{
		if (UCollisionDebugSubsystem* Sub = World->GetSubsystem<UCollisionDebugSubsystem>())
		{
			Sub->ToggleAndUpdateAll();
		}
	}
}

