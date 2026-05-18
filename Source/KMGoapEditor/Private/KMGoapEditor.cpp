#include "KMGoapEditor.h"
#include "KMGoapAgentComponentDetails.h"
#include "PropertyEditorModule.h"
#include "Blueprint/Component/KMGoapAgentComponent.h"

#define LOCTEXT_NAMESPACE "FKMGoapEditorModule"

void FKMGoapEditorModule::StartupModule()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		UKMGoapAgentComponent::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FKMGoapAgentComponentDetails::MakeInstance)
	);
}

void FKMGoapEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(UKMGoapAgentComponent::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FKMGoapEditorModule, KMGoapEditor)