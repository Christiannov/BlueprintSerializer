#include "BSSettings.h"
#include "BSDataTypes.h"
#include "BlueprintSerializerModule.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/App.h"

#define LOCTEXT_NAMESPACE "BlueprintSerializer"

UBlueprintSerializerSettings::UBlueprintSerializerSettings()
{
	// Set default values
	ExportMode = EBSExportMode::ProjectSaved;
	SerializationDepth = EBSSerializationDepth::Extended;
	bPrettyPrintJson = true;
	bOrganizeByBlueprintName = true;
	bIncludeNodeMetadata = true;
	bIncludeStructuredGraphs = true;
	bShowExportNotifications = true;
	bAutoValidateJson = true;
	MaxNodesPerBlueprint = 5000;
	bEnableParallelProcessing = true;
	MemoryLimitMB = 1024;
	ExportFormatVersion = TEXT("1.0.0");
}

FString UBlueprintSerializerSettings::GetExportDirectory() const
{
	FString BaseDir;
	
	switch (ExportMode)
	{
	case EBSExportMode::ProjectSaved:
		if (FApp::HasProjectName())
		{
			BaseDir = FPaths::ProjectSavedDir() / TEXT("BlueprintSerializer");
		}
		else
		{
			// Fallback to engine saved if no project
			BaseDir = FPaths::EngineSavedDir() / TEXT("BlueprintSerializer") / TEXT("NoProject");
		}
		break;
		
	case EBSExportMode::EngineSaved:
		BaseDir = FPaths::EngineSavedDir() / TEXT("BlueprintSerializer");
		if (FApp::HasProjectName())
		{
			BaseDir = BaseDir / FApp::GetProjectName();
		}
		break;
		
	case EBSExportMode::Documents:
		{
			FString DocumentsPath;
#if PLATFORM_WINDOWS
			DocumentsPath = FPaths::Combine(
				FPlatformMisc::GetEnvironmentVariable(TEXT("USERPROFILE")),
				TEXT("Documents")
			);
#elif PLATFORM_MAC
			DocumentsPath = FPaths::Combine(
				FPlatformMisc::GetEnvironmentVariable(TEXT("HOME")),
				TEXT("Documents")
			);
#else
			DocumentsPath = FPlatformMisc::GetEnvironmentVariable(TEXT("HOME"));
#endif
			BaseDir = FPaths::Combine(DocumentsPath, TEXT("UnrealEngine"), TEXT("BlueprintSerializer"));
			if (FApp::HasProjectName())
			{
				BaseDir = BaseDir / FApp::GetProjectName();
			}
		}
		break;
		
	case EBSExportMode::Temporary:
		BaseDir = FPaths::ProjectIntermediateDir() / TEXT("BlueprintSerializer");
		break;
		
	case EBSExportMode::Custom:
		BaseDir = CustomExportPath.Path;
		if (BaseDir.IsEmpty())
		{
			// Fallback to project saved if custom path is empty
			BaseDir = FPaths::ProjectSavedDir() / TEXT("BlueprintSerializer");
		}
		break;
		
	default:
		BaseDir = FPaths::ProjectSavedDir() / TEXT("BlueprintSerializer");
		break;
	}
	
	// Add timestamp folder if enabled
	if (bCreateTimestampedFolders)
	{
		FDateTime Now = FDateTime::Now();
		FString TimestampFolder = FString::Printf(TEXT("%04d%02d%02d_%02d%02d%02d"),
			Now.GetYear(), Now.GetMonth(), Now.GetDay(),
			Now.GetHour(), Now.GetMinute(), Now.GetSecond());
		BaseDir = BaseDir / TimestampFolder;
	}
	
	// Ensure directory exists
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*BaseDir))
	{
		PlatformFile.CreateDirectoryTree(*BaseDir);
	}
	
	return BaseDir;
}

FString UBlueprintSerializerSettings::GetExportPath(const FString& BlueprintName) const
{
	FString ExportDir = GetExportDirectory();
	
	if (bOrganizeByBlueprintName && !BlueprintName.IsEmpty())
	{
		// Clean the blueprint name for use as a folder name
		FString CleanName = BlueprintName;
		CleanName.RemoveFromStart(TEXT("/Game/"));
		CleanName.ReplaceCharInline('/', '_');
		CleanName.ReplaceCharInline('.', '_');
		
		ExportDir = ExportDir / CleanName;
		
		// Ensure directory exists
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*ExportDir))
		{
			PlatformFile.CreateDirectoryTree(*ExportDir);
		}
	}
	
	return ExportDir;
}

FString UBlueprintSerializerSettings::GetContextTemplatePath() const
{
	// Return path to context template in plugin content
	return FPaths::Combine(
		FPaths::EnginePluginsDir(),
		TEXT("Developer"),
		TEXT("BlueprintSerializer"),
		TEXT("Content"),
		TEXT("Prompts"),
		ContextTemplateName + TEXT(".txt")
	);
}

bool UBlueprintSerializerSettings::IsFeatureEnabled(EBSSerializationDepth RequiredDepth) const
{
	return (uint8)SerializationDepth >= (uint8)RequiredDepth;
}

bool UBlueprintSerializerSettings::ValidateSettings(FString& OutError) const
{
	// Validate export path
	if (ExportMode == EBSExportMode::Custom && CustomExportPath.Path.IsEmpty())
	{
		OutError = TEXT("Custom export path is empty");
		return false;
	}
	
	// Validate memory limit
	if (MemoryLimitMB < 100)
	{
		OutError = TEXT("Memory limit is too low (minimum 100 MB)");
		return false;
	}
	
	// Validate max nodes
	if (MaxNodesPerBlueprint < 100)
	{
		OutError = TEXT("Max nodes per blueprint is too low (minimum 100)");
		return false;
	}
	
	OutError.Empty();
	return true;
}

FText UBlueprintSerializerSettings::GetSectionText() const
{
	return LOCTEXT("BlueprintSerializerSettingsSection", "Blueprint Serializer");
}

FText UBlueprintSerializerSettings::GetSectionDescription() const
{
	return LOCTEXT("BlueprintSerializerSettingsDescription", 
		"Configure Blueprint serialization settings for JSON export, LLM context generation, and workflow preferences.");
}

#if WITH_EDITOR
void UBlueprintSerializerSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	
	if (PropertyChangedEvent.Property)
	{
		FName PropertyName = PropertyChangedEvent.Property->GetFName();
		
		// Validate settings when critical properties change
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UBlueprintSerializerSettings, ExportMode) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UBlueprintSerializerSettings, CustomExportPath))
		{
			FString Error;
			if (!ValidateSettings(Error))
			{
				UE_LOG(LogBlueprintSerializer, Warning, TEXT("Invalid settings: %s"), *Error);
			}
		}
		
		// Create export directory when path changes
		if (PropertyName == GET_MEMBER_NAME_CHECKED(UBlueprintSerializerSettings, ExportMode) ||
			PropertyName == GET_MEMBER_NAME_CHECKED(UBlueprintSerializerSettings, CustomExportPath))
		{
			FString ExportDir = GetExportDirectory();
			UE_LOG(LogBlueprintSerializer, Log, TEXT("Export directory set to: %s"), *ExportDir);
		}
	}
}

void UBlueprintSerializerSettings::PostInitProperties()
{
	Super::PostInitProperties();
	
	// Ensure export directory exists on initialization
	FString ExportDir = GetExportDirectory();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*ExportDir))
	{
		PlatformFile.CreateDirectoryTree(*ExportDir);
		UE_LOG(LogBlueprintSerializer, Log, TEXT("Created export directory: %s"), *ExportDir);
	}
}
#endif

#undef LOCTEXT_NAMESPACE