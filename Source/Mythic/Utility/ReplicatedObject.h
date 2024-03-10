#pragma once
#include "Net/UnrealNetwork.h"
#include "Mythic/Mythic.h"
#include "ReplicatedObject.generated.h"

UCLASS(BlueprintType, Blueprintable, Abstract)

class MYTHIC_API UReplicatedObject : public UObject
{
	GENERATED_BODY()

	UPROPERTY(Replicated)
	AActor* OwningActor;

public:
	virtual bool IsSupportedForNetworking() const override { return true; };
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override {
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		DOREPLIFETIME(UReplicatedObject, OwningActor);
	}

	// Returns the owner of this object (the actor responsible for replicating this object)
	UFUNCTION(BlueprintPure)
	AActor* GetOwningActor() const {
		return OwningActor;
	}

	// If no owning actor exists, sets the owning actor to the passed in actor and adds this object to the owning actor's replicated subobject list.
	// If an owning actor already exists, updates the owning actor for this object and adds this object and all other
	// replicatedObject's with this object as an outer to the new actor's replicated subobject list.
	// If using a component's replication list, pass in the component as the second parameter.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void SetOwner(AActor* NewOwner) {
		if (!NewOwner->IsUsingRegisteredSubObjectList()) {
			UE_LOG(Mythic, Error, TEXT("SetOwner:: Owner %s is not using RegisteredSubObjectList for %s!"), *NewOwner->GetName(), *GetName());
		}
		
		// if an owner already exists, update all child ReplicatedObjects with the same owning actor as this object
		if (OwningActor) {
			checkf(OwningActor->HasAuthority(), TEXT("SetOwner:: SetOwner should only be called with authority!"));

			// Get all replicated objects that have this object as an outer and update their owning actor
			TArray<UObject*> children;
			GetObjectsWithOuter(this, children, true, RF_NoFlags);
			for (auto child : children) {
				auto repObj = Cast<UReplicatedObject>(child);
				if (IsValid(repObj)) {
					if (GetOwningActor() != OwningActor) {
						UE_LOG(Mythic, Warning, TEXT("SetOwner:: Skipping %s because it has a different owning actor!"), *repObj->GetName());
						continue;
					}
					if (OwningActor->IsReplicatedSubObjectRegistered(repObj)) {
							repObj->OwningActor->RemoveReplicatedSubObject(repObj);
							repObj->OwningActor = NewOwner;
							repObj->OwningActor->AddReplicatedSubObject(repObj);
					}
				}
			}

			// Remove this object from the old owning actor's replicated subobject list
			if (OwningActor->IsReplicatedSubObjectRegistered(this)) {
				OwningActor->RemoveReplicatedSubObject(this);
			}
		}
		else {
			checkf(NewOwner->HasAuthority(), TEXT("SetOwner:: SetOwner should only be called with authority!"));
		}
		
		OwningActor = NewOwner;
		OwningActor->AddReplicatedSubObject(this);
	}

	void SetOwner(UActorComponent* NewComponentOwner) {
		auto NewOwner = NewComponentOwner->GetOwner();
		checkf(NewOwner, TEXT("SetOwner:: NewComponentOwner does not have an owner!"));
		checkf(NewOwner->IsUsingRegisteredSubObjectList(), TEXT("SetOwner:: NewComponentOwner is not using a registered subobject list!"));
		
		// if an owner already exists, update all child ReplicatedObjects with the same owning actor as this object
		if (OwningActor) {
			checkf(OwningActor->HasAuthority(), TEXT("SetOwner:: SetOwner should only be called with authority!"));

			// Get all replicated objects that have this object as an outer and update their owning actor
			TArray<UObject*> children;
			GetObjectsWithOuter(this, children, true, RF_NoFlags);
			for (auto child : children) {
				auto repObj = Cast<UReplicatedObject>(child);
				if (IsValid(repObj)) {
					if (NewOwner != OwningActor) {
						UE_LOG(Mythic, Warning, TEXT("SetOwner:: Skipping %s because it has a different owning actor!"), *repObj->GetName());
						continue;
					}
					if (OwningActor->IsActorComponentReplicatedSubObjectRegistered(NewComponentOwner, repObj)) {
						repObj->OwningActor->RemoveActorComponentReplicatedSubObject(NewComponentOwner, this);
						repObj->OwningActor = NewOwner;
						repObj->OwningActor->AddActorComponentReplicatedSubObject(NewComponentOwner, this);
					}
				}
			}

			// Remove this object from the old owning actor's replicated subobject list
			if (OwningActor->IsActorComponentReplicatedSubObjectRegistered(NewComponentOwner, this)) {
				OwningActor->RemoveActorComponentReplicatedSubObject(NewComponentOwner, this);
			}
		}
		else {
			checkf(NewOwner->HasAuthority(), TEXT("SetOwner:: SetOwner should only be called with authority!"));
		}
		
		OwningActor = NewOwner;
		OwningActor->AddActorComponentReplicatedSubObject(NewComponentOwner, this);
	}

	// Marks the object as garbage so it will be destroyed and removes it from owning actor's replicated subobject list
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	void Destroy() {
		if (!IsValid(this)) {
			checkf(GetOwningActor()->HasAuthority() == true,
			       TEXT("Destroy:: Object does not have authority to destroy itself!"));

			OnDestroyed();
			MarkAsGarbage(); // Mark the object as garbage so it will be destroyed

			auto owner = GetOwningActor();
			if (IsValid(owner) && owner->IsReplicatedSubObjectRegistered(this)) {
				owner->RemoveReplicatedSubObject(this);
			}
		}
	}

protected:
	// Called before the object is marked as garbage and removed from owning actor's replicated subobject list
	virtual void OnDestroyed() {}
};
