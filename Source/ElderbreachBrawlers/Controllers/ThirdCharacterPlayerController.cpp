#include "ThirdCharacterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "../Widgets/HUDWidget.h"
#include "../Characters/IO89Character.h"
#include "../Subsystems/CollisionDebugSubsystem.h"
#include "../Subsystems/CombatTestSubsystem.h"
#include "Engine/Engine.h"
#include "../MainGameMode.h"
#include <ElderbreachBrawlers/MainGameState.h>
#include <Kismet/GameplayStatics.h>
#include <Engine/LevelScriptActor.h>

AThirdCharacterPlayerController::AThirdCharacterPlayerController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AThirdCharacterPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        UInputAction* PauseAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/Actions/IA_Pause.IA_Pause"));
        if (PauseAction)
        {
            EIC->BindAction(PauseAction, ETriggerEvent::Started, this, &AThirdCharacterPlayerController::OnPausePressed);
        }
    }
}

void AThirdCharacterPlayerController::EnsureCollisionToggleBinding()
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
    if (DefaultMappingContext)
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
            EIC->BindAction(ToggleCollisionAction, ETriggerEvent::Started, this, &AThirdCharacterPlayerController::OnToggleCollisionVisibility);
            bCollisionToggleBound = true;
        }
    }
}

void AThirdCharacterPlayerController::OnPausePressed(const FInputActionValue& Value)
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (UCombatTestSubsystem* CombatSub = World->GetSubsystem<UCombatTestSubsystem>())
    {
        if (CombatSub->bTestRunning)
        {
            CombatSub->ShowPauseMenu(this);
        }
    }
}

void AThirdCharacterPlayerController::OnToggleCollisionVisibility(const FInputActionValue& Value)
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (UCollisionDebugSubsystem* Sub = World->GetSubsystem<UCollisionDebugSubsystem>())
    {
        Sub->ToggleAndUpdateAll();
    }
}

void AThirdCharacterPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void AThirdCharacterPlayerController::BeginPlay()
{
    Super::BeginPlay();

    TryAddMappingContext();

    // No mostrar el HUD de combate en el mapa de menú (solo en la arena)
    bool bIsFrontendMap = false;
    if (AMainGameMode* GM = GetWorld() ? Cast<AMainGameMode>(UGameplayStatics::GetGameMode(GetWorld())) : nullptr)
    {
        bIsFrontendMap = GM->IsFrontendMap();
    }

    if (IsLocalController() && !bIsFrontendMap)
    {
        if (HUDWidgetClass)
        {
            HUDWidgetRef = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
            if (HUDWidgetRef)
            {
                HUDWidgetRef->AddToViewport();
                // Timer TEMPORAL!!
                GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
                    {
                        GetWorld()->GetTimerManager().SetTimer(HUDInitTimerHandle, [this]()
                            {
                                AIO89Character* MyChar = Cast<AIO89Character>(GetPawn());
                                if (MyChar && MyChar->OtherPlayer)
                                {
                                    HUDWidgetRef->SetCharacters(MyChar, MyChar->OtherPlayer);
                                    GetWorld()->GetTimerManager().ClearTimer(HUDInitTimerHandle);
                                }
                            }, 0.05f, true);
                    });
            }
        }
    }
    AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>();
    if (GS)
    {
        AIO89Character* MyChar = Cast<AIO89Character>(GetPawn());
        AIO89Character* Opponent = nullptr;

        if (MyChar == GS->PlayerOneCharacter)
            Opponent = GS->PlayerTwoCharacter;
        else if (MyChar == GS->PlayerTwoCharacter)
            Opponent = GS->PlayerOneCharacter;

        if (HUDWidgetRef)
        {
            HUDWidgetRef->SetCharacters(MyChar, Opponent);
        }
    }

    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
        {
            GetWorld()->GetTimerManager().SetTimer(FightCamTimerHandle, [this]()
                {
                    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
                    {
                        if (GS->FightCamRef)
                        {
                            FViewTargetTransitionParams Params;
                            Params.BlendTime = 0.f;
                            SetViewTarget(GS->FightCamRef, Params);
                            GetWorld()->GetTimerManager().ClearTimer(FightCamTimerHandle);
                        }
                    }
                }, 0.2f, true);
        });
}

void AThirdCharacterPlayerController::TryAddMappingContext()
{
    ULocalPlayer* LP = GetLocalPlayer();
    if (!LP)
    {
        if (GetWorld())
        {
            GetWorld()->GetTimerManager().SetTimer(RetryMappingTimerHandle, this,
                &AThirdCharacterPlayerController::TryAddMappingContext, 0.12f, false);
        }
        return;
    }

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}