#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CollisionDebugSubsystem.generated.h"

/**
 * World Subsystem que centraliza si se muestran las cajas de colisión (debug).
 * Por defecto false: empiezan ocultas. Cualquier controlador (jugador o cámara libre) puede alternar con F.
 */
UCLASS()
class ELDERBREACHBRAWLERS_API UCollisionDebugSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** false = cajas ocultas al iniciar. true = mostradas (tras toggle con F). */
	UPROPERTY(BlueprintReadWrite, Category = "Debug")
	bool bShowCollisionDebug = false;

	/** Alterna bShowCollisionDebug y actualiza la visibilidad de todas las cajas (personajes, BlockHitbox, Proximity). Llamar desde input F. */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ToggleAndUpdateAll();

	/** Pone cajas ocultas y bShowCollisionDebug = false. Llamar al terminar la pelea para que en la siguiente el toggle F funcione bien. */
	UFUNCTION(BlueprintCallable, Category = "Debug")
	void ResetToDefault();

private:
	void ApplyVisibilityToAll(bool bShow);
};
