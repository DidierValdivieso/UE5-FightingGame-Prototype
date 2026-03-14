#include "FrontendWidget.h"
#include "Components/Button.h"
#include "Kismet/GameplayStatics.h"
#include "../Subsystems/WidgetSequenceManager.h"

void UFrontendWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ArcadeButton)
    {
        ArcadeButton->OnClicked.AddDynamic(this, &UFrontendWidget::OnStartButtonClicked);
    }
    if (ExitButton)
    {
        ExitButton->OnClicked.AddDynamic(this, &UFrontendWidget::OnExitButtonClicked);
    }
}

void UFrontendWidget::OnStartButtonClicked()
{
    NavigateToStageSelection();
}

void UFrontendWidget::OnExitButtonClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->ConsoleCommand(TEXT("quit"), true);
    }
}

void UFrontendWidget::NavigateToStageSelection()
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            if (UWidgetSequenceManager* Manager = GameInstance->GetSubsystem<UWidgetSequenceManager>())
            {
                if (APlayerController* PC = GetOwningPlayer())
                {
                    Manager->ShowStageSelection();
                    RemoveFromParent();
                }
            }
        }
    }
}

