// All rights reserved by Khrönmière Entertainment.
#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Blueprint/Component/KMGoapAgentComponent.h"

class IDetailLayoutBuilder;
class UKMGoapAgentComponent;

/**
 * Custom details panel for UKMGoapAgentComponent.
 * Injects a live Slate widget during Play-In-Editor (PIE) to display the agent's
 * current GOAP execution state, including goals, actions, facts, and beliefs.
 */
class FKMGoapAgentComponentDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it. */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	/** Weak reference to the agent component being customized. */
	TWeakObjectPtr<UKMGoapAgentComponent> AgentComponent;

	/** Generates the Slate widget for the live debugger. */
	TSharedRef<SWidget> GenerateDebugWidget();

	/** Active timer callback that forces the Slate widget to redraw while playing. */
	EActiveTimerReturnType UpdateDebugState(double InCurrentTime, float InDeltaTime);

	/** Cached snapshot data used to populate the UI. */
	FKMGoapDebugSnapshot CachedSnapshot;

	// UI Getters
	FText GetCurrentGoalText() const;
	FText GetCurrentActionText() const;
	FText GetPlanStatusText() const;
	TSharedRef<SWidget> GenerateFactsList() const;
	TSharedRef<SWidget> GenerateBeliefsList() const;
};
