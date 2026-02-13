#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BSDataTypes.h"
#include "BSSettings.generated.h"

/**
 * Blueprint Serializer settings for Project Settings panel
 */
UCLASS(config = EditorPerProjectUserSettings, defaultconfig, meta = (DisplayName = "Blueprint Serializer"))
class BLUEPRINTSERIALIZER_API UBlueprintSerializerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UBlueprintSerializerSettings();

	// ==================== Export Settings ====================
	
	/** Mode for determining export directory */
	UPROPERTY(config, EditAnywhere, Category = "Export Settings", meta = (DisplayName = "Export Mode"))
	EBSExportMode ExportMode = EBSExportMode::ProjectSaved;
	
	/** Custom export path when ExportMode is set to Custom */
	UPROPERTY(config, EditAnywhere, Category = "Export Settings", 
		meta = (EditCondition = "ExportMode == EBSExportMode::Custom", DisplayName = "Custom Export Path"))
	FDirectoryPath CustomExportPath;
	
	/** Create timestamped subdirectories for each export */
	UPROPERTY(config, EditAnywhere, Category = "Export Settings", 
		meta = (DisplayName = "Create Timestamped Folders"))
	bool bCreateTimestampedFolders = false;
	
	/** Organize exports by Blueprint name */
	UPROPERTY(config, EditAnywhere, Category = "Export Settings",
		meta = (DisplayName = "Organize By Blueprint Name"))
	bool bOrganizeByBlueprintName = true;
	
	// ==================== Serialization Settings ====================
	
	/** Level of detail for serialization */
	UPROPERTY(config, EditAnywhere, Category = "Serialization", 
		meta = (DisplayName = "Serialization Depth"))
	EBSSerializationDepth SerializationDepth = EBSSerializationDepth::Extended;
	
	/** Format JSON with indentation for readability */
	UPROPERTY(config, EditAnywhere, Category = "Serialization", 
		meta = (DisplayName = "Pretty Print JSON"))
	bool bPrettyPrintJson = true;
	
	/** Include editor-only data in serialization */
	UPROPERTY(config, EditAnywhere, Category = "Serialization",
		meta = (DisplayName = "Include Editor-Only Data"))
	bool bIncludeEditorOnlyData = false;
	
	/** Include detailed metadata for nodes */
	UPROPERTY(config, EditAnywhere, Category = "Serialization",
		meta = (DisplayName = "Include Node Metadata"))
	bool bIncludeNodeMetadata = true;
	
	/** Include node position data for graph reconstruction */
	UPROPERTY(config, EditAnywhere, Category = "Serialization",
		meta = (DisplayName = "Include Node Positions"))
	bool bIncludeNodePositions = true;
	
	/** Include structured graph data with execution flows */
	UPROPERTY(config, EditAnywhere, Category = "Serialization",
		meta = (DisplayName = "Include Structured Graphs"))
	bool bIncludeStructuredGraphs = true;
	
	// ==================== AI/LLM Integration ====================
	
	/** Generate context files for LLM processing */
	UPROPERTY(config, EditAnywhere, Category = "AI Integration",
		meta = (DisplayName = "Generate LLM Context Files"))
	bool bGenerateLLMContext = false;
	
	/** Include reference C++ files in context */
	UPROPERTY(config, EditAnywhere, Category = "AI Integration",
		meta = (EditCondition = "bGenerateLLMContext", DisplayName = "Include Reference Files"))
	bool bIncludeReferenceFiles = true;
	
	/** Maximum number of reference files to include */
	UPROPERTY(config, EditAnywhere, Category = "AI Integration",
		meta = (EditCondition = "bGenerateLLMContext && bIncludeReferenceFiles", 
			   DisplayName = "Max Reference Files", ClampMin = 1, ClampMax = 10))
	int32 MaxReferenceFiles = 3;
	
	/** Template to use for LLM context generation */
	UPROPERTY(config, EditAnywhere, Category = "AI Integration",
		meta = (EditCondition = "bGenerateLLMContext", DisplayName = "Context Template"))
	FString ContextTemplateName = TEXT("CodeGen_CPP");
	
	/** Include implementation notes for complex nodes */
	UPROPERTY(config, EditAnywhere, Category = "AI Integration",
		meta = (EditCondition = "bGenerateLLMContext", DisplayName = "Include Implementation Notes"))
	bool bIncludeImplementationNotes = true;
	
	// ==================== Workflow Settings ====================
	
	/** Automatically open export folder after serialization */
	UPROPERTY(config, EditAnywhere, Category = "Workflow",
		meta = (DisplayName = "Auto-Open Export Folder"))
	bool bAutoOpenExportFolder = false;
	
	/** Show toast notifications for export operations */
	UPROPERTY(config, EditAnywhere, Category = "Workflow",
		meta = (DisplayName = "Show Export Notifications"))
	bool bShowExportNotifications = true;
	
	/** Copy export path to clipboard after serialization */
	UPROPERTY(config, EditAnywhere, Category = "Workflow",
		meta = (DisplayName = "Copy Path to Clipboard"))
	bool bCopyPathToClipboard = false;
	
	/** Log detailed serialization information */
	UPROPERTY(config, EditAnywhere, Category = "Workflow",
		meta = (DisplayName = "Verbose Logging"))
	bool bVerboseLogging = false;
	
	/** Automatically validate JSON after export */
	UPROPERTY(config, EditAnywhere, Category = "Workflow",
		meta = (DisplayName = "Auto-Validate JSON"))
	bool bAutoValidateJson = true;
	
	// ==================== Performance Settings ====================
	
	/** Maximum nodes to process in a single Blueprint */
	UPROPERTY(config, EditAnywhere, Category = "Performance",
		meta = (DisplayName = "Max Nodes Per Blueprint", ClampMin = 100, ClampMax = 10000))
	int32 MaxNodesPerBlueprint = 5000;
	
	/** Enable parallel processing for large Blueprints */
	UPROPERTY(config, EditAnywhere, Category = "Performance",
		meta = (DisplayName = "Enable Parallel Processing"))
	bool bEnableParallelProcessing = true;
	
	/** Memory limit for serialization in MB */
	UPROPERTY(config, EditAnywhere, Category = "Performance",
		meta = (DisplayName = "Memory Limit (MB)", ClampMin = 100, ClampMax = 4096))
	int32 MemoryLimitMB = 1024;
	
	// ==================== Advanced Settings ====================
	
	/** Include deprecated nodes in serialization */
	UPROPERTY(config, EditAnywhere, Category = "Advanced",
		meta = (DisplayName = "Include Deprecated Nodes"))
	bool bIncludeDeprecatedNodes = false;
	
	/** Export format version for backward compatibility */
	UPROPERTY(config, EditAnywhere, Category = "Advanced",
		meta = (DisplayName = "Export Format Version"))
	FString ExportFormatVersion = TEXT("1.0.0");
	
	/** Custom metadata to include in all exports */
	UPROPERTY(config, EditAnywhere, Category = "Advanced",
		meta = (DisplayName = "Custom Metadata"))
	TMap<FString, FString> CustomMetadata;
	
	// ==================== Helper Functions ====================
	
	/** Get the configured export directory */
	FString GetExportDirectory() const;
	
	/** Get the full export path for a Blueprint */
	FString GetExportPath(const FString& BlueprintName) const;
	
	/** Get the context template path */
	FString GetContextTemplatePath() const;
	
	/** Check if a feature is enabled based on serialization depth */
	bool IsFeatureEnabled(EBSSerializationDepth RequiredDepth) const;
	
	/** Validate current settings */
	bool ValidateSettings(FString& OutError) const;
	
	// ==================== UDeveloperSettings Interface ====================
	
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
	virtual FText GetSectionText() const override;
	virtual FText GetSectionDescription() const override;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;
#endif
};