#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ButtonStageWidget.generated.h"

class UButton;
class UBorder;
class UImage;
class UTextBlock;

UCLASS()
class ELDERBREACHBRAWLERS_API UButtonStageWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	// Inicializa el botón con el nombre del mapa, su textura y si está disponible
	UFUNCTION(BlueprintCallable)
	void InitializeStageButton(const FString& StageName, UTexture2D* StageTexture, bool bStageAvailable = true, const FString& UnavailableMessage = TEXT("Coming Soon"));

	// Obtiene el nombre del mapa asociado a este botón
	UFUNCTION(BlueprintCallable)
	FString GetStageName() const { return StageName; }

	// Indica si el escenario está disponible para jugar
	UFUNCTION(BlueprintCallable)
	bool IsStageAvailable() const { return bStageAvailable; }

	/** Marca el botón como seleccionado (borde azul) o no seleccionado (borde por defecto). */
	UFUNCTION(BlueprintCallable)
	void SetSelected(bool bSelected);

	// Delegate para notificar cuando se selecciona un mapa
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStageSelected, FString, SelectedStageName);
	
	UPROPERTY(BlueprintAssignable)
	FOnStageSelected OnStageSelected;

protected:
	UPROPERTY(meta = (BindWidget))
	UButton* StageButton;

	UPROPERTY(meta = (BindWidget))
	UImage* StageImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StageText;

	/** Borde del botón; si existe, se pondrá azul al seleccionar. Nombre en el Blueprint: StageBorder. */
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* StageBorder;

	UFUNCTION()
	void OnStageButtonClicked();

private:
	FString StageName;
	bool bStageAvailable = true;
	FLinearColor DefaultBorderColor;
};
