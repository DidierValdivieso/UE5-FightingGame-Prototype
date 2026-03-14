#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "../Structs/CombatTestStructs.h"
#include "ControllerSelectWidget.generated.h"

class UButton;

UCLASS()
class ELDERBREACHBRAWLERS_API UControllerSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

protected:
    UPROPERTY(meta = (BindWidget))
    UComboBoxString* ControllerType1;

    UPROPERTY(meta = (BindWidget))
    UComboBoxString* ControllerType2;

    UPROPERTY(meta = (BindWidget))
    UButton* StartButton;

    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;

    UFUNCTION()
    void OnStartTestClicked();

    UFUNCTION()
    void OnBackButtonClicked();

    UFUNCTION()
    void OnControllerType1SelectionChanged(FString SelectedOption, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void OnControllerType2SelectionChanged(FString SelectedOption, ESelectInfo::Type SelectionType);

private:
    void GetSpawnPositionsForMap(FName MapName, FVector& OutP1Location, FRotator& OutP1Rotation,
        FVector& OutP2Location, FRotator& OutP2Rotation);

    void NavigateBackToStageSelection();
};


