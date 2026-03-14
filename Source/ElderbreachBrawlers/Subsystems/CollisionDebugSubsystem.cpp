#include "CollisionDebugSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "../Characters/IO89Character.h"
#include "../Actors/BlockHitboxActor.h"
#include "../Actors/ProximityAttackActor.h"
#include "../Actors/AttackHitboxActor.h"
#include "Components/PrimitiveComponent.h"

void UCollisionDebugSubsystem::ToggleAndUpdateAll()
{
	UWorld* World = GetWorld();
	if (!World) return;

	bShowCollisionDebug = !bShowCollisionDebug;
	UE_LOG(LogTemp, Warning, TEXT("[CollisionDebug] ToggleAndUpdateAll -> bShowCollisionDebug=%d"), bShowCollisionDebug ? 1 : 0);
	ApplyVisibilityToAll(bShowCollisionDebug);
}

void UCollisionDebugSubsystem::ResetToDefault()
{
	bShowCollisionDebug = false;
	ApplyVisibilityToAll(false);
}

void UCollisionDebugSubsystem::ApplyVisibilityToAll(bool bShow)
{
	UWorld* World = GetWorld();
	if (!World) return;

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(World, AIO89Character::StaticClass(), Found);
	for (AActor* A : Found)
	{
		if (!IsValid(A)) continue;
		if (AIO89Character* Char = Cast<AIO89Character>(A))
		{
			TArray<UActorComponent*> Components;
			Char->GetComponents(UPrimitiveComponent::StaticClass(), Components);
			for (UActorComponent* Comp : Components)
			{
				UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Comp);
				if (!PrimComp) continue;
				if (PrimComp->ComponentHasTag(FName("TorsoHitbox")) ||
					PrimComp->ComponentHasTag(FName("Hitbox")) ||
					PrimComp->ComponentHasTag(FName("Hurtbox")) ||
					PrimComp->ComponentHasTag(FName("Pushbox")))
				{
					PrimComp->SetHiddenInGame(!bShow);
					PrimComp->SetVisibility(bShow);
					PrimComp->MarkRenderStateDirty();
				}
			}
			// No mostrar la caja de bloqueo cuando solo está subida la guardia; se pinta solo en el impacto (en BlockHitboxActor).
			if (IsValid(Char->ActiveProximityAttack) && Char->ActiveProximityAttack->DebugMesh)
			{
				Char->ActiveProximityAttack->DebugMesh->SetHiddenInGame(!bShow);
				Char->ActiveProximityAttack->DebugMesh->SetVisibility(bShow);
				Char->ActiveProximityAttack->DebugMesh->MarkRenderStateDirty();
			}
		}
	}

	Found.Empty();
	UGameplayStatics::GetAllActorsOfClass(World, AProximityAttackActor::StaticClass(), Found);
	for (AActor* A : Found)
	{
		if (!IsValid(A)) continue;
		if (AProximityAttackActor* Prox = Cast<AProximityAttackActor>(A))
		{
			if (Prox->DebugMesh)
			{
				Prox->DebugMesh->SetHiddenInGame(!bShow);
				Prox->DebugMesh->SetVisibility(bShow);
				Prox->DebugMesh->MarkRenderStateDirty();
			}
		}
	}

	// Actualizar visibilidad de DebugMesh en todos los AttackHitboxActor
	Found.Empty();
	UGameplayStatics::GetAllActorsOfClass(World, AAttackHitboxActor::StaticClass(), Found);
	for (AActor* A : Found)
	{
		if (!IsValid(A)) continue;
		if (AAttackHitboxActor* AttackHitbox = Cast<AAttackHitboxActor>(A))
		{
			if (AttackHitbox->DebugMesh)
			{
				AttackHitbox->DebugMesh->SetHiddenInGame(!bShow);
				AttackHitbox->DebugMesh->SetVisibility(bShow);
				AttackHitbox->DebugMesh->MarkRenderStateDirty();
			}
		}
	}
}
