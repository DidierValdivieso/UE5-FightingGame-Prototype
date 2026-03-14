#include "ButtonStageWidget.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "UObject/UObjectIterator.h"

void UButtonStageWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (StageButton)
	{
		StageButton->OnClicked.AddDynamic(this, &UButtonStageWidget::OnStageButtonClicked);
	}

	if (StageBorder)
	{
		DefaultBorderColor = StageBorder->GetBrushColor();
	}

	// Zoom "cover" en el ScaleBox que envuelve StageImage
	if (StageImage)
	{
		for (UPanelWidget* Parent = StageImage->GetParent(); Parent; Parent = Parent->GetParent())
		{
			if (UScaleBox* ScaleBox = Cast<UScaleBox>(Parent))
			{
				ScaleBox->SetStretch(EStretch::ScaleToFill);
				ScaleBox->SetStretchDirection(EStretchDirection::Both);
				ScaleBox->SynchronizeProperties();
				break;
			}
		}
	}
}

void UButtonStageWidget::InitializeStageButton(const FString& InStageName, UTexture2D* StageTexture, bool bInStageAvailable, const FString& UnavailableMessage)
{
	StageName = InStageName;
	bStageAvailable = bInStageAvailable;

	if (StageImage && StageTexture)
	{
		FSlateBrush Brush = StageImage->GetBrush();
		
		UMaterialInterface* BaseMaterial = Cast<UMaterialInterface>(Brush.GetResourceObject());
		
		if (BaseMaterial)
		{			
			UMaterialInstanceDynamic* DynamicMaterial = Cast<UMaterialInstanceDynamic>(BaseMaterial);
			
			if (!DynamicMaterial)
			{
				DynamicMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
			}
			else
			{
			}
			
			if (DynamicMaterial)
			{
				UMaterial* MaterialBase = BaseMaterial->GetMaterial();
				if (MaterialBase)
				{
				// Obtener todos los parámetros de textura del material
				TArray<FMaterialParameterInfo> TextureParameterInfos;
				TArray<FGuid> TextureParameterIds;
				MaterialBase->GetAllTextureParameterInfo(TextureParameterInfos, TextureParameterIds);
								
				bool bTextureSet = false;
				for (const FMaterialParameterInfo& ParamInfo : TextureParameterInfos)
				{
					DynamicMaterial->SetTextureParameterValue(ParamInfo.Name, StageTexture);
					bTextureSet = true;
				}
				
				// Si no se encontraron parámetros, intentar con nombres comunes
				if (!bTextureSet || TextureParameterInfos.Num() == 0)
				{
					TArray<FName> CommonTextureParamNames = {
						FName("Texture"),
						FName("BaseTexture"),
						FName("MainTexture"),
						FName("ButtonTexture"),
						FName("StageTexture"),
						FName("ImageTexture"),
						FName("DiffuseTexture"),
						FName("AlbedoTexture"),
						FName("BaseColorTexture")
					};
					
					for (const FName& ParamName : CommonTextureParamNames)
					{
						DynamicMaterial->SetTextureParameterValue(ParamName, StageTexture);
					}
				}
				else
				{
				}
				}
				else
				{
					TArray<FName> CommonTextureParamNames = {
						FName("Texture"),
						FName("BaseTexture"),
						FName("MainTexture"),
						FName("ButtonTexture"),
						FName("ImageTexture")
					};
					
					for (const FName& ParamName : CommonTextureParamNames)
					{
						DynamicMaterial->SetTextureParameterValue(ParamName, StageTexture);
					}
				}
				
				// Aspect ratio para el material "cover" (evita estiramiento)
				const float TexAspect = (StageTexture->GetSizeY() > 0)
					? static_cast<float>(StageTexture->GetSizeX()) / StageTexture->GetSizeY()
					: 1.0f;
				DynamicMaterial->SetScalarParameterValue(FName("TextureAspect"), TexAspect);

				Brush.SetResourceObject(DynamicMaterial);
				Brush.SetImageSize(FVector2D(StageTexture->GetSizeX(), StageTexture->GetSizeY()));
				StageImage->SetBrush(Brush);
			}
			else
			{
			}
		}
		else
		{
			StageImage->SetBrushFromTexture(StageTexture, true);
		}
	}
	else
	{
		if (!StageImage)
		{
		}
		if (!StageTexture)
		{
		}
	}

	if (StageText)
	{
		if (bStageAvailable)
		{
			// Cuando el nivel está disponible (true), no mostrar el nombre del nivel (vacío)
			StageText->SetText(FText::FromString(FString()));
			// Restaurar el color por defecto
			StageText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		}
		else
		{
			// Cuando no está disponible (false), mostrar el letrero (Coming Soon, etc.)
			StageText->SetText(FText::FromString(UnavailableMessage));
			// Texto blanco con opacidad completa para que "Coming Soon" se note bien sobre la imagen oscurecida
			StageText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
		}
	}

	// Oscurecer la imagen del botón cuando no está disponible
	if (StageImage)
	{
		if (bStageAvailable)
		{
			// Restaurar el color y opacidad normal de la imagen
			StageImage->SetColorAndOpacity(FLinearColor::White);
		}
		else
		{
			// Oscurecer la imagen cuando muestra "Coming Soon"
			StageImage->SetColorAndOpacity(FLinearColor(0.15f, 0.15f, 0.15f, 1.0f));
		}
	}

	if (StageButton)
	{
		StageButton->SetIsEnabled(bStageAvailable);
	}

}

void UButtonStageWidget::SetSelected(bool bSelected)
{
	if (StageBorder)
	{
		StageBorder->SetBrushColor(bSelected ? FLinearColor(0.0f, 0.35f, 1.0f, 1.0f) : DefaultBorderColor);
	}
}

void UButtonStageWidget::OnStageButtonClicked()
{	
	if (bStageAvailable && !StageName.IsEmpty())
	{
		OnStageSelected.Broadcast(StageName);
	}
}

