#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StageSelectWidget.generated.h"

class UButton;
class UImage;
class UButtonStageWidget;
class UPanelWidget;
class UMaterialInterface;
class UTexture2D;

UCLASS()
class ELDERBREACHBRAWLERS_API UStageSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UStageSelectWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnToggleVisibilityButtonClicked();

protected:
    UPROPERTY(meta = (BindWidget))
    UButton* ConfirmButton;

    UPROPERTY(meta = (BindWidget))
    UButton* BackButton;

    UPROPERTY(meta = (BindWidget))
    UImage* BackgroundImage;

    UPROPERTY(meta = (BindWidget))
    UButton* ToggleVisibilityButton;

    UPROPERTY(meta = (BindWidget))
    UImage* bgContainer_image;

    UPROPERTY(meta = (BindWidget))
    UPanelWidget* StageButtonsContainer;

    UFUNCTION()
    void OnConfirmButtonClicked();

    UFUNCTION()
    void OnBackButtonClicked();

    UFUNCTION()
    void OnStageButtonSelected(FString InSelectedStageName);

private:
    void NavigateToCombatConfig();
    void NavigateBackToFrontend();
    void UpdateBackgroundImage(const FString& StageName);
    FString GetTexturePathForStage(const FString& StageName);
    bool IsStageAvailable(const FString& StageName) const;
    void CreateStageButtons();
    /** Inicializa los botones de escenario ya colocados en el Blueprint (evita conflicto con BindWidget) */
    void InitializeExistingButtons();
    /** Actualiza el borde azul del botón seleccionado y restaura el resto. */
    void RefreshSelectionBorder();
    void LoadStageSelectionMaterials();
    /** Precarga todas las imágenes de niveles y las guarda en caché para el .exe. */
    void PreloadStageImages();

    /** Material opcional para las imágenes de stage. Transient = no se serializa en Blueprint (evita Bad export index). */
    UPROPERTY(Transient)
    TSoftObjectPtr<UMaterialInterface> StageImageMaterial;

    /** Materiales adicionales a precargar. Transient = no se serializa en Blueprint. */
    UPROPERTY(Transient)
    TArray<TSoftObjectPtr<UMaterialInterface>> MaterialsToLoad;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stage Selection", meta = (AllowPrivateAccess = "true"))
    TSubclassOf<UButtonStageWidget> ButtonStageWidgetClass;

    FString SelectedStageName;
    TArray<UButtonStageWidget*> StageButtons;

    /** Materiales cargados y mantenidos en memoria mientras el widget exista. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> LoadedMaterials;

    /** Caché de texturas de niveles cargadas (para que estén en el build y no se descarguen en el .exe). */
    UPROPERTY(Transient)
    TMap<FString, TObjectPtr<UTexture2D>> LoadedStageTextures;

    /** Referencias en el CDO para que el cooker incluya las texturas en el build (Transient = no serializa en Blueprint). */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTexture2D>> CookerStageTextureRefs;

    bool bIsContainerVisible;
};
