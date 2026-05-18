// All rights reserved by Khrönmière Entertainment.
#include "KMGoapAgentComponentDetails.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Blueprint/KMGoapAgentGoal.h"
#include "Blueprint/KMGoapAgentAction.h"

#define LOCTEXT_NAMESPACE "KMGoapAgentComponentDetails"

TSharedRef<IDetailCustomization> FKMGoapAgentComponentDetails::MakeInstance()
{
	return MakeShareable(new FKMGoapAgentComponentDetails);
}

void FKMGoapAgentComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	if (ObjectsBeingCustomized.Num() != 1)
	{
		return; // Only support single selection for the live debugger
	}

	AgentComponent = Cast<UKMGoapAgentComponent>(ObjectsBeingCustomized[0].Get());
	if (!AgentComponent.IsValid())
	{
		return;
	}

	// Create a new category at the top of the details panel
	IDetailCategoryBuilder& DebugCategory = DetailBuilder.EditCategory("GOAP Debugger (Live)", LOCTEXT("GoapDebugCategory", "GOAP Debugger (Live)"), ECategoryPriority::Important);

	TSharedRef<SWidget> DebugWidget = GenerateDebugWidget();

	DebugCategory.AddCustomRow(LOCTEXT("GoapDebugRow", "GOAP Debugger"))
	.WholeRowContent()
	[
		DebugWidget
	];

	// Register an active timer to poll the agent state while PIE is active
	DebugWidget->RegisterActiveTimer(0.1f, FWidgetActiveTimerDelegate::CreateSP(this, &FKMGoapAgentComponentDetails::UpdateDebugState));
}

EActiveTimerReturnType FKMGoapAgentComponentDetails::UpdateDebugState(double InCurrentTime, float InDeltaTime)
{
	if (AgentComponent.IsValid() && AgentComponent->HasBegunPlay())
	{
		CachedSnapshot = AgentComponent->GetDebugSnapshot();
	}
	else
	{
		CachedSnapshot = FKMGoapDebugSnapshot();
	}

	return EActiveTimerReturnType::Continue;
}

TSharedRef<SWidget> FKMGoapAgentComponentDetails::GenerateDebugWidget()
{
	return SNew(SBox)
	.Padding(5.0f)
	[
		SNew(SVerticalBox)
		
		// Current Goal
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
			[
				SNew(STextBlock).Text(LOCTEXT("CurrentGoalLabel", "Current Goal:")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(this, &FKMGoapAgentComponentDetails::GetCurrentGoalText)
			]
		]

		// Current Action
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
			[
				SNew(STextBlock).Text(LOCTEXT("CurrentActionLabel", "Current Action:")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(this, &FKMGoapAgentComponentDetails::GetCurrentActionText)
			]
		]

		// Plan Status
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 2)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 5, 0)
			[
				SNew(STextBlock).Text(LOCTEXT("PlanStatusLabel", "Plan Status:")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SNew(STextBlock).Text(this, &FKMGoapAgentComponentDetails::GetPlanStatusText)
			]
		]

		// Facts and Beliefs Split
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 10, 0, 0)
		[
			SNew(SHorizontalBox)
			
			// Facts Column
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(0, 0, 5, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
				[
					SNew(STextBlock).Text(LOCTEXT("FactsLabel", "Runtime Facts")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox)
					.MaxDesiredHeight(150.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							GenerateFactsList()
						]
					]
				]
			]

			// Beliefs Column
			+ SHorizontalBox::Slot()
			.FillWidth(0.5f)
			.Padding(5, 0, 0, 0)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
				[
					SNew(STextBlock).Text(LOCTEXT("BeliefsLabel", "Evaluated Beliefs")).Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBox)
					.MaxDesiredHeight(150.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							GenerateBeliefsList()
						]
					]
				]
			]
		]
	];
}

FText FKMGoapAgentComponentDetails::GetCurrentGoalText() const
{
	if (!AgentComponent.IsValid() || !AgentComponent->HasBegunPlay())
	{
		return LOCTEXT("NotPlaying", "Not Playing");
	}

	if (CachedSnapshot.CurrentGoal)
	{
		return FText::FromString(FString::Printf(TEXT("%s (Priority: %.1f)"), 
			*CachedSnapshot.CurrentGoal->GoalTag.ToString(), 
			CachedSnapshot.CurrentGoal->GetPriority(AgentComponent.Get())));
	}

	return LOCTEXT("NoGoal", "None");
}

FText FKMGoapAgentComponentDetails::GetCurrentActionText() const
{
	if (!AgentComponent.IsValid() || !AgentComponent->HasBegunPlay())
	{
		return LOCTEXT("NotPlaying", "Not Playing");
	}

	if (CachedSnapshot.CurrentAction)
	{
		FString StatusStr = TEXT("Unknown");
		switch (CachedSnapshot.CurrentAction->GetStatus())
		{
			case EKMGoapActionStatus::NotStarted: StatusStr = TEXT("Not Started"); break;
			case EKMGoapActionStatus::Running: StatusStr = TEXT("Running"); break;
			case EKMGoapActionStatus::Succeeded: StatusStr = TEXT("Succeeded"); break;
			case EKMGoapActionStatus::Failed: StatusStr = TEXT("Failed"); break;
		}

		return FText::FromString(FString::Printf(TEXT("%s [%s]"), 
			*CachedSnapshot.CurrentAction->ActionTag.ToString(), 
			*StatusStr));
	}

	return LOCTEXT("NoAction", "None");
}

FText FKMGoapAgentComponentDetails::GetPlanStatusText() const
{
	if (!AgentComponent.IsValid() || !AgentComponent->HasBegunPlay())
	{
		return LOCTEXT("NotPlaying", "Not Playing");
	}

	if (CachedSnapshot.bIsWaitingForPlan)
	{
		return LOCTEXT("WaitingForPlan", "Waiting for Async Plan...");
	}

	if (CachedSnapshot.QueuedActions.Num() > 0)
	{
		return FText::FromString(FString::Printf(TEXT("%d actions queued"), CachedSnapshot.QueuedActions.Num()));
	}

	return LOCTEXT("NoPlan", "Idle / No Plan");
}

TSharedRef<SWidget> FKMGoapAgentComponentDetails::GenerateFactsList() const
{
	// In a real dynamic UI, we would use a ListView, but for a simple debug panel, 
	// rebuilding a vertical box on poll is acceptable for small data sets.
	// To make it truly dynamic without rebuilding the widget tree, we use a single TextBlock 
	// that formats the map into a string.
	
	auto GetFactsString = [this]() -> FText
	{
		if (!AgentComponent.IsValid() || !AgentComponent->HasBegunPlay() || CachedSnapshot.Facts.IsEmpty())
		{
			return LOCTEXT("NoFacts", "No facts known.");
		}

		FString Result;
		for (const auto& Pair : CachedSnapshot.Facts)
		{
			Result += FString::Printf(TEXT("%s: %s\n"), *Pair.Key.ToString(), Pair.Value ? TEXT("True") : TEXT("False"));
		}
		return FText::FromString(Result);
	};

	return SNew(STextBlock).Text_Lambda(GetFactsString);
}

TSharedRef<SWidget> FKMGoapAgentComponentDetails::GenerateBeliefsList() const
{
	auto GetBeliefsString = [this]() -> FText
	{
		if (!AgentComponent.IsValid() || !AgentComponent->HasBegunPlay() || CachedSnapshot.Beliefs.IsEmpty())
		{
			return LOCTEXT("NoBeliefs", "No beliefs evaluated.");
		}

		FString Result;
		for (const auto& Pair : CachedSnapshot.Beliefs)
		{
			Result += FString::Printf(TEXT("%s: %s\n"), *Pair.Key.ToString(), Pair.Value ? TEXT("True") : TEXT("False"));
		}
		return FText::FromString(Result);
	};

	return SNew(STextBlock).Text_Lambda(GetBeliefsString);
}

#undef LOCTEXT_NAMESPACE
