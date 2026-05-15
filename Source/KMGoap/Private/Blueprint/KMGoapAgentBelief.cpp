// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/KMGoapAgentBelief.h"

bool UKMGoapAgentBelief::Native_Condition(const UKMGoapAgentComponent* Agent) const
{
	// Native callers use this wrapper so Blueprint-authored beliefs participate in the same evaluation flow
	// as C++ beliefs.
	return Condition(Agent);
}

bool UKMGoapAgentBelief::Condition_Implementation(const UKMGoapAgentComponent* Agent) const
{
	// Beliefs are conservative by default; subclasses must explicitly opt in to reporting a satisfied condition.
	return false;
}

FVector UKMGoapAgentBelief::Native_ObservedLocation(const UKMGoapAgentComponent* Agent) const
{
	// Location queries are separated from truth evaluation so goals/actions can use belief positions without
	// forcing the belief to encode movement or targeting logic.
	return ObservedLocation(Agent);
}

FVector UKMGoapAgentBelief::ObservedLocation_Implementation(const UKMGoapAgentComponent* Agent) const
{
	// Zero vector represents "no meaningful observed location" for base beliefs.
	return FVector::ZeroVector;
}
