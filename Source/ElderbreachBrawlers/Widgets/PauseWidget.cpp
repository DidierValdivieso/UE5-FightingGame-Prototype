// Fill out your copyright notice in the Description page of Project Settings.

#include "PauseWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

#include "../MainGameState.h"
#include "../Subsystems/WidgetSequenceManager.h"
#include "../Subsystems/CombatTestSubsystem.h"
#include "../Subsystems/CollisionDebugSubsystem.h"

void UPauseWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UPauseWidget::OnCloseClicked);
	}
	if (RestartFightButton)
	{
		RestartFightButton->OnClicked.AddDynamic(this, &UPauseWidget::OnRestartFightClicked);
	}
	if (ExitToFrontendButton)
	{
		ExitToFrontendButton->OnClicked.AddDynamic(this, &UPauseWidget::OnExitToFrontendClicked);
	}
}

void UPauseWidget::OnCloseClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;
	APlayerController* PC = GetOwningPlayer();
	if (UCombatTestSubsystem* CombatSubsystem = World->GetSubsystem<UCombatTestSubsystem>())
	{
		CombatSubsystem->ClosePauseMenu(PC);
	}
}

void UPauseWidget::OnRestartFightClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);

	if (UCombatTestSubsystem* CombatSubsystem = World->GetSubsystem<UCombatTestSubsystem>())
	{
		CombatSubsystem->ClearPauseWidgetRef();
	}

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		PC->SetPause(false);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}

	if (UCombatTestSubsystem* CombatSubsystem = World->GetSubsystem<UCombatTestSubsystem>())
	{
		CombatSubsystem->StopTest();
	}

	if (UCollisionDebugSubsystem* CollisionDebug = World->GetSubsystem<UCollisionDebugSubsystem>())
	{
		CollisionDebug->ResetToDefault();
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		if (UWidgetSequenceManager* Manager = GameInstance->GetSubsystem<UWidgetSequenceManager>())
		{
			FCombatTestConfig Config = Manager->GetCurrentConfig();
			if (UCombatTestSubsystem* CombatSubsystem = World->GetSubsystem<UCombatTestSubsystem>())
			{
				CombatSubsystem->StartTest(Config);
			}
		}
	}

	RemoveFromParent();
}

void UPauseWidget::OnExitToFrontendClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;

	if (UCombatTestSubsystem* CombatSubsystem = World->GetSubsystem<UCombatTestSubsystem>())
	{
		CombatSubsystem->ClearPauseWidgetRef();
	}

	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		PC->SetPause(false);
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly());
	}

	UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);

	if (UCombatTestSubsystem* CombatSubsystem = World->GetSubsystem<UCombatTestSubsystem>())
	{
		CombatSubsystem->StopTest();
	}

	if (UCollisionDebugSubsystem* CollisionDebug = World->GetSubsystem<UCollisionDebugSubsystem>())
	{
		CollisionDebug->ResetToDefault();
	}

	if (AMainGameState* GS = World->GetGameState<AMainGameState>())
	{
		GS->ResetTestData();
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UWidgetSequenceManager* Manager = GameInstance->GetSubsystem<UWidgetSequenceManager>())
		{
			Manager->StartWidgetSequence(PC ? PC : UGameplayStatics::GetPlayerController(World, 0));
		}
	}
	RemoveFromParent();
}
