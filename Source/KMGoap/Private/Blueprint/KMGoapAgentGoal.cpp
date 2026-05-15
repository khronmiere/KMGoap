// All rights reserved by Khrönmière Entertainment.
#include "Blueprint/KMGoapAgentGoal.h"

float UKMGoapAgentGoal::Native_GetPriority(UKMGoapAgentComponent* Agent) const
{
	// Route through the Blueprint-native event so C++ callers and Blueprint overrides share the same priority path.
	return Priority(Agent);
}

float UKMGoapAgentGoal::Priority_Implementation(UKMGoapAgentComponent* Agent) const
{
	// Base priority is the fallback for static goals. Dynamic goals can override Priority to account for
	// agent state, sensed targets, urgency, or gameplay context.
	return BasePriority;
}
