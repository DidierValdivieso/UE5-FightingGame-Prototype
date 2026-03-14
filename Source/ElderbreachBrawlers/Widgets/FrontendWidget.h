#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FrontendWidget.generated.h"

class UButton;

UCLASS()
class ELDERBREACHBRAWLERS_API UFrontendWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
    virtual void NativeConstruct() override;

protected:
    UPROPERTY(meta = (BindWidget))
    UButton* ArcadeButton;

    UPROPERTY(meta = (BindWidget))
    UButton* ExitButton;

    UFUNCTION()
    void OnStartButtonClicked();

    UFUNCTION()
    void OnExitButtonClicked();

private:
    void NavigateToStageSelection();
};
