#pragma once

#include "CoreMinimal.h"
#include "BlueprintAnalyzer.h"
#include "AssetReferenceExtractor.h"
#include "DataExportManager.generated.h"

/**
 * Combined data structure for complete project analysis
 * Contains all Blueprint and dependency data for the entire project
 */
USTRUCT(BlueprintType)
struct BLUEPRINTSERIALIZER_API FBS_CompleteProjectData
{
	GENERATED_BODY()

	FBS_CompleteProjectData()
		: TotalBlueprintCount(0)
		, TotalAssetCount(0)
	{
	}

	UPROPERTY()
	TArray<FBS_BlueprintData> BlueprintAnalyses;
	
	UPROPERTY()
	FBS_ProjectReferenceGraph ReferenceGraph;
	
	UPROPERTY()
	FString ProjectName;
	
	UPROPERTY()
	FString ProjectPath;
	
	UPROPERTY()
	int32 TotalBlueprintCount;
	
	UPROPERTY()
	int32 TotalAssetCount;
	
	UPROPERTY()
	FString ExtractionTimestamp;
	
	UPROPERTY()
	FString VersionInfo;
};

/**
 * Manager class for coordinating all data extraction and export operations
 * Handles file I/O and export path management using BlueprintSerializer settings
 */
class BLUEPRINTSERIALIZER_API UDataExportManager
{

public:
	/**
	 * Extract and export complete project data (Blueprints + References)
	 */
	static FBS_CompleteProjectData ExtractCompleteProjectData();
	
	/**
	 * Export complete project data to comprehensive JSON file
	 */
	static FString ExportCompleteProjectDataToJSON(const FBS_CompleteProjectData& Data);
	
	/**
	 * Save complete project data to default export directory
	 */
	static bool SaveCompleteProjectData(const FBS_CompleteProjectData& Data);
	
	/**
	 * Export only Blueprint data for current project
	 */
	static bool ExportProjectBlueprintData();
	
	/**
	 * Export only reference graph for current project  
	 */
	static bool ExportProjectReferenceGraph();
	
	/**
	 * Export everything for current project
	 */
	static bool ExportCompleteProjectData();

	/**
	 * Get default export directory path (uses BlueprintSerializer settings)
	 */
	static FString GetDefaultExportPath();
	
	/**
	 * Get timestamp string for file naming
	 */
	static FString GetTimestamp();
	
	/**
	 * Create export directory if it doesn't exist
	 */
	static bool EnsureExportDirectoryExists();

private:
	/**
	 * Convert complete project data to JSON object
	 */
	static TSharedPtr<FJsonObject> CompleteProjectDataToJsonObject(const FBS_CompleteProjectData& Data);
	
	/**
	 * Generate file name for export
	 */
	static FString GenerateExportFileName(const FString& DataType);
	
	/**
	 * Log extraction progress and results
	 */
	static void LogExtractionResults(const FBS_CompleteProjectData& Data);
	
	/**
	 * Validate extracted data for completeness
	 */
	static bool ValidateExtractedData(const FBS_CompleteProjectData& Data);
};
