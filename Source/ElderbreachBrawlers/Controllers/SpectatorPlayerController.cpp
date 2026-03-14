#include "SpectatorPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "../Widgets/HUDWidget.h"
#include "../MainGameState.h"
#include "../Characters/IO89Character.h"
#include "../MainGameMode.h"
#include "../Subsystems/CombatTestSubsystem.h"
#include "../Subsystems/CollisionDebugSubsystem.h"

ASpectatorPlayerController::ASpectatorPlayerController()
{
    bAutoManageActiveCameraTarget = false;
    PrimaryActorTick.bCanEverTick = true;
    UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] Constructor called (C++ base)"));
}

void ASpectatorPlayerController::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] BeginPlay - %s Class=%s IsLocal=%d Pawn=%s"),
        *GetName(), GetClass() ? *GetClass()->GetName() : TEXT("?"), IsLocalController(),
        GetPawn() ? *GetPawn()->GetName() : TEXT("null"));

    // No mostrar el HUD en el mapa de menú (ahí solo van los widgets Frontend -> Stage -> Controller)
    if (AMainGameMode* GM = GetWorld() ? Cast<AMainGameMode>(UGameplayStatics::GetGameMode(GetWorld())) : nullptr)
    {
        if (GM->IsFrontendMap())
        {
            return;
        }
    }

    // En la arena: NUNCA crear el HUD aquí. CombatTestSubsystem lo crea en StartTest() cuando el usuario
    // termina el menú. Si CombatSub es null o el combate no ha empezado, no mostramos HUD (evita pantalla negra + HUD).
    UCombatTestSubsystem* CombatSub = GetWorld() ? GetWorld()->GetSubsystem<UCombatTestSubsystem>() : nullptr;
    if (!CombatSub || !CombatSub->bTestRunning)
    {
        return;
    }

    if (IsLocalController() && HUDWidgetClass)
    {
        HUDWidgetRef = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
        if (HUDWidgetRef)
        {
            HUDWidgetRef->AddToViewport();

            GetWorld()->GetTimerManager().SetTimer(InitHUDTimerHandle, this, &ASpectatorPlayerController::InitHUD, 0.1f, true);
        }
    }
}

void ASpectatorPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] SetupInputComponent - %s Class=%s IsLocal=%d InputComponent=%s"),
        *GetName(), GetClass() ? *GetClass()->GetName() : TEXT("?"), IsLocalController(), InputComponent ? TEXT("ok") : TEXT("null"));

    // Cargar assets en runtime (evita ConstructorHelpers con UInputMappingContext en UE 5.6)
    if (!ToggleCollisionAction)
    {
        ToggleCollisionAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_ToggleCollisionVisibility.IA_ToggleCollisionVisibility"));
        UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] ToggleCollisionAction loaded: %s"), ToggleCollisionAction ? *ToggleCollisionAction->GetName() : TEXT("FAILED"));
    }
    UInputAction* PauseAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Pause.IA_Pause"));
    if (!DefaultMappingContext)
    {
        DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default.IMC_Default"));
        UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] DefaultMappingContext loaded: %s"), DefaultMappingContext ? *DefaultMappingContext->GetName() : TEXT("FAILED"));
    }

    if (IsLocalController() && DefaultMappingContext)
    {
        if (ULocalPlayer* LP = GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Sub->AddMappingContext(DefaultMappingContext, 0);
                UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] MappingContext added to EnhancedInput"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] EnhancedInputLocalPlayerSubsystem is null"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] GetLocalPlayer() is null"));
        }
    }

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
    if (EIC)
    {
        if (ToggleCollisionAction)
        {
            EIC->BindAction(ToggleCollisionAction, ETriggerEvent::Started, this, &ASpectatorPlayerController::OnToggleCollisionVisibility);
            UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] F (ToggleCollision) BOUND - press F in IA vs IA to toggle boxes"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] ToggleCollisionAction null - NOT bound"));
        }
        if (PauseAction)
        {
            EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &ASpectatorPlayerController::OnPausePressed);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] InputComponent is not EnhancedInputComponent - bind skipped"));
    }
}

void ASpectatorPlayerController::EnsureCollisionToggleBinding()
{
    if (!IsLocalController()) return;

    if (!ToggleCollisionAction)
    {
        ToggleCollisionAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_ToggleCollisionVisibility.IA_ToggleCollisionVisibility"));
    }
    if (!DefaultMappingContext)
    {
        DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default.IMC_Default"));
    }

    // Mapping context cada vez (por si se quitó al abrir menú)
    if (IsLocalController() && DefaultMappingContext)
    {
        if (ULocalPlayer* LP = GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Sub = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Sub->AddMappingContext(DefaultMappingContext, 0);
            }
        }
    }

    // Bind solo una vez: evita que una pulsación dispare varios callbacks
    if (bCollisionToggleBound) return;
    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (ToggleCollisionAction)
        {
            EIC->BindAction(ToggleCollisionAction, ETriggerEvent::Started, this, &ASpectatorPlayerController::OnToggleCollisionVisibility);
            bCollisionToggleBound = true;
        }
    }
}

void ASpectatorPlayerController::OnPausePressed(const FInputActionValue& Value)
{
    UWorld* World = GetWorld();
    if (!World) return;

    UCombatTestSubsystem* CombatSub = World->GetSubsystem<UCombatTestSubsystem>();
    if (CombatSub && CombatSub->bTestRunning)
    {
        CombatSub->ShowPauseMenu(this);
    }
}

void ASpectatorPlayerController::OnToggleCollisionVisibility(const FInputActionValue& Value)
{
    UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] >>> F DETECTADA <<< OnToggleCollisionVisibility (CPU vs CPU sin 2 IdleAI - input aqui) Controller=%s"),
        *GetName());

    UWorld* World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] World null"));
        return;
    }

    UCollisionDebugSubsystem* Sub = World->GetSubsystem<UCollisionDebugSubsystem>();
    if (Sub)
    {
        Sub->ToggleAndUpdateAll();
        UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] ToggleAndUpdateAll() called"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[SpectatorPC] CollisionDebugSubsystem null"));
    }
}

void ASpectatorPlayerController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

void ASpectatorPlayerController::InitHUD()
{
    AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>();
    if (!GS) return;

    if (GS->PlayerOneCharacter && GS->PlayerTwoCharacter && HUDWidgetRef)
    {
        HUDWidgetRef->SetCharacters(GS->PlayerOneCharacter, GS->PlayerTwoCharacter);
    }

    // Poner vista en FightCamRef cuando esté asignada
    if (GS->FightCamRef)
    {
        FViewTargetTransitionParams Params;
        Params.BlendTime = 0.f;
        SetViewTarget(GS->FightCamRef, Params);
    }

    GetWorld()->GetTimerManager().ClearTimer(InitHUDTimerHandle);
}
