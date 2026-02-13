#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBlueprintSerializer, Log, All);

/**
 * Main module class for Blueprint Serializer plugin
 */
class BLUEPRINTSERIALIZER_API FBlueprintSerializerModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	/** Singleton access */
	static FBlueprintSerializerModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FBlueprintSerializerModule>("BlueprintSerializer");
	}
	
	/** Check if module is loaded */
	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("BlueprintSerializer");
	}

private:
	/** Register UI extensions */
	void RegisterMenuExtensions();
	void UnregisterMenuExtensions();
	
	/** Register settings */
	void RegisterSettings();
	void UnregisterSettings();
	
	/** Register console commands */
	void RegisterConsoleCommands();
	void UnregisterConsoleCommands();
	
	/** Console command handles */
	TArray<IConsoleCommand*> ConsoleCommands;
};