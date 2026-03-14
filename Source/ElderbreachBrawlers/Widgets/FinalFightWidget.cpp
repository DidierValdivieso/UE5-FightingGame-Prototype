// Fill out your copyright notice in the Description page of Project Settings.

#include "FinalFightWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "../MainGameState.h"
#include "../Subsystems/WidgetSequenceManager.h"
#include "../Subsystems/CombatTestSubsystem.h"
#include "../Subsystems/CollisionDebugSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UFinalFightWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (MainMenuButton)
	{
		MainMenuButton->OnClicked.AddDynamic(this, &UFinalFightWidget::OnMainMenuButtonClicked);
	}
	if (RestartFightButton)
	{
		RestartFightButton->OnClicked.AddDynamic(this, &UFinalFightWidget::OnRestartFightButtonClicked);
	}

	UpdateFightData();
}

void UFinalFightWidget::UpdateFightData()
{
	if (UWorld* World = GetWorld())
	{
		if (AMainGameState* GS = World->GetGameState<AMainGameState>())
		{
			// Determine winner
			FString WinnerString;
			if (GS->PlayerOneRoundsWon > GS->PlayerTwoRoundsWon)
			{
				WinnerString = TEXT("Winner: Player One!");
			}
			else if (GS->PlayerTwoRoundsWon > GS->PlayerOneRoundsWon)
			{
				WinnerString = TEXT("Winner: Player Two!");
			}
			else
			{
				WinnerString = TEXT("Draw!");
			}

			if (WinnerText)
			{
				WinnerText->SetText(FText::FromString(WinnerString));
			}

			// Round durations
			if (RoundOneDurationText)
			{
				FString RoundOneText = FString::Printf(TEXT("Round 1 Duration: %.1f seconds"), GS->RoundOneDuration);
				RoundOneDurationText->SetText(FText::FromString(RoundOneText));
			}

			if (RoundTwoDurationText)
			{
				FString RoundTwoText = FString::Printf(TEXT("Round 2 Duration: %.1f seconds"), GS->RoundTwoDuration);
				RoundTwoDurationText->SetText(FText::FromString(RoundTwoText));
			}

			// Player One damage
			if (PlayerOneRoundOneDamageText)
			{
				FString DamageText = FString::Printf(TEXT("Round 1 Damage: %.1f"), GS->PlayerOneRoundOneDamage);
				PlayerOneRoundOneDamageText->SetText(FText::FromString(DamageText));
			}

			if (PlayerOneRoundTwoDamageText)
			{
				FString DamageText = FString::Printf(TEXT("Round 2 Damage: %.1f"), GS->PlayerOneRoundTwoDamage);
				PlayerOneRoundTwoDamageText->SetText(FText::FromString(DamageText));
			}

			// Player Two damage
			if (PlayerTwoRoundOneDamageText)
			{
				FString DamageText = FString::Printf(TEXT("Round 1 Damage: %.1f"), GS->PlayerTwoRoundOneDamage);
				PlayerTwoRoundOneDamageText->SetText(FText::FromString(DamageText));
			}

			if (PlayerTwoRoundTwoDamageText)
			{
				FString DamageText = FString::Printf(TEXT("Round 2 Damage: %.1f"), GS->PlayerTwoRoundTwoDamage);
				PlayerTwoRoundTwoDamageText->SetText(FText::FromString(DamageText));
			}

			// Rounds won
			if (PlayerOneRoundsWonText)
			{
				FString RoundsText = FString::Printf(TEXT("Rounds Won: %d"), GS->PlayerOneRoundsWon);
				PlayerOneRoundsWonText->SetText(FText::FromString(RoundsText));
			}

			if (PlayerTwoRoundsWonText)
			{
				FString RoundsText = FString::Printf(TEXT("Rounds Won: %d"), GS->PlayerTwoRoundsWon);
				PlayerTwoRoundsWonText->SetText(FText::FromString(RoundsText));
			}
		}
	}
}

void UFinalFightWidget::OnMainMenuButtonClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (PC)
	{
		PC->SetPause(false);
		UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
	}

	// Limpiar la pelea por completo: destruir personajes, quitar HUD (como la primera vez)
	if (UCombatTestSubsystem* CombatSubsystem = World->GetSubsystem<UCombatTestSubsystem>())
	{
		CombatSubsystem->StopTest();
	}

	// Resetear cajas de colisión (F) para que en la siguiente pelea el toggle funcione (CPU vs CPU, etc.)
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
			RemoveFromParent();
		}
	}
}

void UFinalFightWidget::OnRestartFightButtonClicked()
{
	UWorld* World = GetWorld();
	if (!World) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	if (PC)
	{
		PC->SetPause(false);
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
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

