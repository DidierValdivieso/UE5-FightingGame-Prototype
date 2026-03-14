#include "HUDWidget.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include <ElderbreachBrawlers/Characters/IO89Character.h>
#include <ElderbreachBrawlers/MainGameState.h>

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (AMainGameState* GS = GetWorld()->GetGameState<AMainGameState>())
    {
        if (CountdownValue)
        {
            // Durante pre-countdown se mantiene en 60; al arrancar el round, el contador real
            int32 ValueToShow = GS->PreCountdown > 0 ? 60 : GS->Countdown;
            FString CountdownText = FString::Printf(TEXT("%02d"), ValueToShow);
            CountdownValue->SetText(FText::FromString(CountdownText));
        }

        if (GetReadyText)
        {
            // PreCountdown 2 seg: seg 0-1 ROUND #, seg 1-2 READY, luego FIGHT! 0.3 s. Igual en round 1 y round 2.
            if (GS->PreCountdown > 0)
            {
                FightTextShowStartTime = -1.f;
                bWasInPreCountdown = true;  // estamos en pre-countdown (ROUND # o READY)
            }
            if (GS->bFightStarted)
            {
                bWasInPreCountdown = false;  // ya no rellenar con READY
            }

            if (GS->PreCountdown == 2)
            {
                GetReadyText->SetVisibility(ESlateVisibility::HitTestInvisible);
                GetReadyText->SetText(FText::FromString(FString::Printf(TEXT("ROUND %d"), GS->CurrentRound)));
            }
            else if (GS->PreCountdown == 1)
            {
                GetReadyText->SetVisibility(ESlateVisibility::HitTestInvisible);
                GetReadyText->SetText(FText::FromString(TEXT("READY")));
            }
            else if (GS->bFightStarted)
            {
                const float Now = GetWorld()->GetTimeSeconds();
                if (FightTextShowStartTime < 0.f)
                {
                    FightTextShowStartTime = Now;
                }
                if ((Now - FightTextShowStartTime) < 0.3f)
                {
                    GetReadyText->SetVisibility(ESlateVisibility::HitTestInvisible);
                    GetReadyText->SetText(FText::FromString(TEXT("FIGHT!")));
                }
                else
                {
                    GetReadyText->SetVisibility(ESlateVisibility::Collapsed);
                }
            }
            else if (GS->PreCountdown <= 0 && bWasInPreCountdown)
            {
                // Hueco entre READY y FIGHT!: mantener READY hasta que salga FIGHT! (round 1 y round 2)
                GetReadyText->SetVisibility(ESlateVisibility::HitTestInvisible);
                GetReadyText->SetText(FText::FromString(TEXT("READY")));
            }
            else
            {
                // Espera de 2 s antes del round 2 (bWasInPreCountdown false): no mostrar nada
                GetReadyText->SetVisibility(ESlateVisibility::Collapsed);
            }
        }

        if (DecisionText)
        {
            if (GS->LastRoundNumber > 0)
            {
                FString DecisionString;
                if (GS->LastRoundWinner == 1)
                    DecisionString = TEXT("Player One wins!");
                else if (GS->LastRoundWinner == 2)
                    DecisionString = TEXT("Player Two wins!");
                else
                    DecisionString = TEXT("Draw!");
                DecisionText->SetVisibility(ESlateVisibility::HitTestInvisible);
                DecisionText->SetText(FText::FromString(DecisionString));
            }
            else
            {
                DecisionText->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    if (CharacterOne)
    {
        LifeCharacterOneBar->SetPercent(CharacterOne->GetHealthPercent());
        EnergyCharacterOneBar->SetPercent(CharacterOne->GetEnergyPercent());
        StaminaCharacterOneBar->SetPercent(CharacterOne->GetStaminaPercent());
    }

    if (CharacterTwo)
    {
        LifeCharacterTwoBar->SetPercent(CharacterTwo->GetHealthPercent());
        EnergyCharacterTwoBar->SetPercent(CharacterTwo->GetEnergyPercent());
        StaminaCharacterTwoBar->SetPercent(CharacterTwo->GetStaminaPercent());
    }
}

void UHUDWidget::SetCharacters(AIO89Character* InCharOne, AIO89Character* InCharTwo)
{
    CharacterOne = InCharOne;
    CharacterTwo = InCharTwo;
}