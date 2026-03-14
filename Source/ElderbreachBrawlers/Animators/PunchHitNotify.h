#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "PunchHitNotify.generated.h"

UCLASS()
class ELDERBREACHBRAWLERS_API UPunchHitNotify : public UAnimNotify
{
	GENERATED_BODY()

public:
	/** Socket donde spawear el hitbox (mano que golpea). Por defecto hand_l. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punch Hit")
	FName HitboxSocketName = FName("hand_l");

	/** Socket alternativo si HitboxSocketName no existe en el skeleton. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Punch Hit")
	FName FallbackSocketName = FName("hand_r");

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
