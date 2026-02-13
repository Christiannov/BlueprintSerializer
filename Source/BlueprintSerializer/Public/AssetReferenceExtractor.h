#pragma once

#include "CoreMinimal.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Dom/JsonObject.h"
#include "AssetReferenceExtractor.generated.h"

/**
 * Structure to hold asset reference relationship data
 * Used for dependency mapping and reference tracking
 */
USTRUCT(BlueprintType)
struct BLUEPRINTSERIALIZER_API FBS_AssetReferenceData
{
	GENERATED_BODY()

	FBS_AssetReferenceData()
		: TotalDependencyCount(0)
		, TotalReferencerCount(0)
	{
	}

	UPROPERTY()
	FString AssetPath;
	
	UPROPERTY()
	FString AssetName;
	
	UPROPERTY()
	FString AssetClass;
	
	UPROPERTY()
	TArray<FString> HardDependencies;
	
	UPROPERTY()
	TArray<FString> SoftDependencies;
	
	UPROPERTY()
	TArray<FString> HardReferencers;
	
	UPROPERTY()
	TArray<FString> SoftReferencers;
	
	UPROPERTY()
	int32 TotalDependencyCount;
	
	UPROPERTY()
	int32 TotalReferencerCount;
	
	UPROPERTY()
	FString ExtractionTimestamp;
};

/**
 * Structure for complete project reference graph
 * Contains all asset dependencies and references in the project
 */
USTRUCT(BlueprintType)
struct BLUEPRINTSERIALIZER_API FBS_ProjectReferenceGraph
{
	GENERATED_BODY()

	FBS_ProjectReferenceGraph()
		: TotalAssetCount(0)
		, TotalReferenceCount(0)
	{
	}

	UPROPERTY()
	TArray<FBS_AssetReferenceData> AssetReferences;
	
	UPROPERTY()
	int32 TotalAssetCount;
	
	UPROPERTY()
	int32 TotalReferenceCount;
	
	UPROPERTY()
	FString ProjectPath;
	
	UPROPERTY()
	FString ExtractionTimestamp;
};

/**
 * Core asset reference extraction class
 * Analyzes and extracts asset dependencies and references
 */
class BLUEPRINTSERIALIZER_API UAssetReferenceExtractor
{

public:
	/**
	 * Extract complete reference data for a single asset
	 */
	static FBS_AssetReferenceData ExtractAssetReferences(const FString& AssetPath);
	
	/**
	 * Extract reference data for all assets in the project
	 */
	static FBS_ProjectReferenceGraph ExtractProjectReferenceGraph();
	
	/**
	 * Extract reference data for all Blueprint assets only
	 */
	static TArray<FBS_AssetReferenceData> ExtractBlueprintReferences();
	
	/**
	 * Export asset reference data to JSON format
	 */
	static FString ExportAssetReferenceToJSON(const FBS_AssetReferenceData& Data);
	
	/**
	 * Export complete project reference graph to JSON
	 */
	static FString ExportProjectGraphToJSON(const FBS_ProjectReferenceGraph& Graph);
	
	/**
	 * Save reference data to file
	 */
	static bool SaveReferenceDataToFile(const FString& JsonData, const FString& FilePath);
	
	/**
	 * Find all assets that depend on a specific asset (recursive)
	 */
	static TArray<FString> FindDependentAssets(const FString& AssetPath, int32 MaxDepth = 5);
	
	/**
	 * Find all assets that a specific asset depends on (recursive)
	 */
	static TArray<FString> FindDependencyChain(const FString& AssetPath, int32 MaxDepth = 5);

	/**
	 * Convert project reference graph to JSON object
	 */
	static TSharedPtr<FJsonObject> ProjectGraphToJsonObject(const FBS_ProjectReferenceGraph& Graph);

	/**
	 * Convert asset reference data to JSON object
	 */
	static TSharedPtr<FJsonObject> AssetReferenceToJsonObject(const FBS_AssetReferenceData& Data);

private:
	/**
	 * Get dependencies using Asset Registry
	 */
	static TArray<FString> GetAssetDependencies(const FString& AssetPath, bool bHardReferencesOnly = false);
	
	/**
	 * Get referencers using Asset Registry
	 */
	static TArray<FString> GetAssetReferencers(const FString& AssetPath, bool bHardReferencesOnly = false);
	
	/**
	 * Convert FAssetData to asset path string
	 */
	static FString AssetDataToPath(const FAssetData& AssetData);
	
	/**
	 * Get all assets in project using Asset Registry
	 */
	static TArray<FAssetData> GetAllProjectAssets();
	
	/**
	 * Filter assets by class type
	 */
	static TArray<FAssetData> FilterAssetsByClass(const TArray<FAssetData>& Assets, const FString& ClassName);
	
	/**
	 * Perform recursive dependency search
	 */
	static void RecursiveDependencySearch(const FString& AssetPath, TSet<FString>& VisitedAssets, 
									   TArray<FString>& ResultArray, int32 CurrentDepth, int32 MaxDepth, bool bSearchReferencers);
};