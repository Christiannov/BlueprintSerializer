#include "AssetReferenceExtractor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/Blueprint.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

FBS_AssetReferenceData UAssetReferenceExtractor::ExtractAssetReferences(const FString& AssetPath)
{
	FBS_AssetReferenceData Data;
	
	if (AssetPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("AssetReferenceExtractor: Invalid asset path provided"));
		return Data;
	}
	
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	// Get asset data
	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	if (!AssetData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("AssetReferenceExtractor: Could not find asset data for %s"), *AssetPath);
		return Data;
	}
	
	// Basic asset information
	Data.AssetPath = AssetPath;
	Data.AssetName = AssetData.AssetName.ToString();
	Data.AssetClass = AssetData.AssetClassPath.ToString();
	Data.ExtractionTimestamp = FDateTime::Now().ToString();
	
	UE_LOG(LogTemp, Warning, TEXT("Extracting references for asset: %s"), *Data.AssetName);
	
	// Extract dependencies
	Data.HardDependencies = GetAssetDependencies(AssetPath, true);  // Hard references only
	Data.SoftDependencies = GetAssetDependencies(AssetPath, false); // All references, then filter
	
	// Remove hard dependencies from soft dependencies array (soft = all - hard)
	for (const FString& HardDep : Data.HardDependencies)
	{
		Data.SoftDependencies.Remove(HardDep);
	}
	
	// Extract referencers
	Data.HardReferencers = GetAssetReferencers(AssetPath, true);   // Hard referencers only
	Data.SoftReferencers = GetAssetReferencers(AssetPath, false);  // All referencers, then filter
	
	// Remove hard referencers from soft referencers array
	for (const FString& HardRef : Data.HardReferencers)
	{
		Data.SoftReferencers.Remove(HardRef);
	}
	
	// Calculate totals
	Data.TotalDependencyCount = Data.HardDependencies.Num() + Data.SoftDependencies.Num();
	Data.TotalReferencerCount = Data.HardReferencers.Num() + Data.SoftReferencers.Num();
	
	UE_LOG(LogTemp, Warning, TEXT("Asset %s: Dependencies=%d (Hard=%d, Soft=%d), Referencers=%d (Hard=%d, Soft=%d)"), 
		*Data.AssetName, Data.TotalDependencyCount, Data.HardDependencies.Num(), Data.SoftDependencies.Num(),
		Data.TotalReferencerCount, Data.HardReferencers.Num(), Data.SoftReferencers.Num());
	
	return Data;
}

FBS_ProjectReferenceGraph UAssetReferenceExtractor::ExtractProjectReferenceGraph()
{
	FBS_ProjectReferenceGraph Graph;
	Graph.ProjectPath = FPaths::GetProjectFilePath();
	Graph.ExtractionTimestamp = FDateTime::Now().ToString();
	
	UE_LOG(LogTemp, Warning, TEXT("Extracting complete project reference graph..."));
	
	// Get all project assets
	TArray<FAssetData> AllAssets = GetAllProjectAssets();
	
	Graph.TotalAssetCount = AllAssets.Num();
	UE_LOG(LogTemp, Warning, TEXT("Found %d assets in project"), Graph.TotalAssetCount);
	
	// Extract reference data for each asset
	int32 ProcessedCount = 0;
	for (const FAssetData& AssetData : AllAssets)
	{
		FString AssetPath = AssetDataToPath(AssetData);
		FBS_AssetReferenceData ReferenceData = ExtractAssetReferences(AssetPath);
		Graph.AssetReferences.Add(ReferenceData);
		
		Graph.TotalReferenceCount += ReferenceData.TotalDependencyCount;
		
		ProcessedCount++;
		if (ProcessedCount % 100 == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("Processed %d/%d assets..."), ProcessedCount, AllAssets.Num());
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Project reference graph extraction complete: %d assets, %d total references"), 
		Graph.TotalAssetCount, Graph.TotalReferenceCount);
	
	return Graph;
}

TArray<FBS_AssetReferenceData> UAssetReferenceExtractor::ExtractBlueprintReferences()
{
	TArray<FBS_AssetReferenceData> BlueprintReferences;
	
	// Get all project assets and filter for Blueprints
	TArray<FAssetData> AllAssets = GetAllProjectAssets();
	TArray<FAssetData> BlueprintAssets = FilterAssetsByClass(AllAssets, TEXT("Blueprint"));
	TArray<FAssetData> AnimBlueprintAssets = FilterAssetsByClass(AllAssets, TEXT("AnimBlueprint"));

	TSet<FName> SeenPackages;
	TArray<FAssetData> AllBlueprintAssets;
	AllBlueprintAssets.Reserve(BlueprintAssets.Num() + AnimBlueprintAssets.Num());
	for (const FAssetData& AssetData : BlueprintAssets)
	{
		if (!SeenPackages.Contains(AssetData.PackageName))
		{
		    SeenPackages.Add(AssetData.PackageName);
			AllBlueprintAssets.Add(AssetData);
		}
	}
	for (const FAssetData& AssetData : AnimBlueprintAssets)
	{
		if (!SeenPackages.Contains(AssetData.PackageName))
		{
		    SeenPackages.Add(AssetData.PackageName);
			AllBlueprintAssets.Add(AssetData);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Extracting references for %d Blueprint assets"), AllBlueprintAssets.Num());
	
	// Extract reference data for each Blueprint
	for (const FAssetData& AssetData : AllBlueprintAssets)
	{
		FString AssetPath = AssetDataToPath(AssetData);
		FBS_AssetReferenceData ReferenceData = ExtractAssetReferences(AssetPath);
		BlueprintReferences.Add(ReferenceData);
	}
	
	return BlueprintReferences;
}

TArray<FString> UAssetReferenceExtractor::GetAssetDependencies(const FString& AssetPath, bool bHardReferencesOnly)
{
	TArray<FString> Dependencies;
	
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	// Get asset data
	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	if (!AssetData.IsValid())
	{
		return Dependencies;
	}
	
	// Set dependency options
	FAssetRegistryDependencyOptions DependencyOptions;
	DependencyOptions.bIncludeSoftPackageReferences = !bHardReferencesOnly;
	DependencyOptions.bIncludeHardPackageReferences = true;
	DependencyOptions.bIncludeSoftManagementReferences = !bHardReferencesOnly;
	DependencyOptions.bIncludeHardManagementReferences = true;
	
	// Get dependencies (UE5.5 API)
	TArray<FName> DependencyPackageNames;
	AssetRegistry.K2_GetDependencies(AssetData.PackageName, DependencyOptions, DependencyPackageNames);
	
	// Convert package names to asset paths
	for (const FName& PackageName : DependencyPackageNames)
	{
		TArray<FAssetData> AssetsInPackage;
		AssetRegistry.GetAssetsByPackageName(PackageName, AssetsInPackage);
		
		for (const FAssetData& DepAssetData : AssetsInPackage)
		{
			FString DepAssetPath = AssetDataToPath(DepAssetData);
			if (!DepAssetPath.IsEmpty())
			{
				Dependencies.AddUnique(DepAssetPath);
			}
		}
	}
	
	return Dependencies;
}

TArray<FString> UAssetReferenceExtractor::GetAssetReferencers(const FString& AssetPath, bool bHardReferencesOnly)
{
	TArray<FString> Referencers;
	
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	// Get asset data
	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	if (!AssetData.IsValid())
	{
		return Referencers;
	}
	
	// Set referencer options
	FAssetRegistryDependencyOptions ReferencerOptions;
	ReferencerOptions.bIncludeSoftPackageReferences = !bHardReferencesOnly;
	ReferencerOptions.bIncludeHardPackageReferences = true;
	ReferencerOptions.bIncludeSoftManagementReferences = !bHardReferencesOnly;
	ReferencerOptions.bIncludeHardManagementReferences = true;
	
	// Get referencers (UE5.5 API)
	TArray<FName> ReferencerPackageNames;
	AssetRegistry.K2_GetReferencers(AssetData.PackageName, ReferencerOptions, ReferencerPackageNames);
	
	// Convert package names to asset paths
	for (const FName& PackageName : ReferencerPackageNames)
	{
		TArray<FAssetData> AssetsInPackage;
		AssetRegistry.GetAssetsByPackageName(PackageName, AssetsInPackage);
		
		for (const FAssetData& RefAssetData : AssetsInPackage)
		{
			FString RefAssetPath = AssetDataToPath(RefAssetData);
			if (!RefAssetPath.IsEmpty())
			{
				Referencers.AddUnique(RefAssetPath);
			}
		}
	}
	
	return Referencers;
}

TArray<FAssetData> UAssetReferenceExtractor::GetAllProjectAssets()
{
	TArray<FAssetData> AllAssets;
	
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	// Get all assets in the /Game directory (project content)
	AssetRegistry.GetAssetsByPath(TEXT("/Game"), AllAssets, true);
	
	return AllAssets;
}

TArray<FAssetData> UAssetReferenceExtractor::FilterAssetsByClass(const TArray<FAssetData>& Assets, const FString& ClassName)
{
	TArray<FAssetData> FilteredAssets;
	
	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath.ToString().Contains(ClassName))
		{
			FilteredAssets.Add(Asset);
		}
	}
	
	return FilteredAssets;
}

FString UAssetReferenceExtractor::AssetDataToPath(const FAssetData& AssetData)
{
	return AssetData.GetObjectPathString();
}

TArray<FString> UAssetReferenceExtractor::FindDependentAssets(const FString& AssetPath, int32 MaxDepth)
{
	TArray<FString> DependentAssets;
	TSet<FString> VisitedAssets;
	
	RecursiveDependencySearch(AssetPath, VisitedAssets, DependentAssets, 0, MaxDepth, true);
	
	return DependentAssets;
}

TArray<FString> UAssetReferenceExtractor::FindDependencyChain(const FString& AssetPath, int32 MaxDepth)
{
	TArray<FString> DependencyChain;
	TSet<FString> VisitedAssets;
	
	RecursiveDependencySearch(AssetPath, VisitedAssets, DependencyChain, 0, MaxDepth, false);
	
	return DependencyChain;
}

void UAssetReferenceExtractor::RecursiveDependencySearch(const FString& AssetPath, TSet<FString>& VisitedAssets, 
													   TArray<FString>& ResultArray, int32 CurrentDepth, int32 MaxDepth, bool bSearchReferencers)
{
	if (CurrentDepth >= MaxDepth || VisitedAssets.Contains(AssetPath))
	{
		return;
	}
	
	VisitedAssets.Add(AssetPath);
	
	TArray<FString> RelatedAssets;
	if (bSearchReferencers)
	{
		RelatedAssets = GetAssetReferencers(AssetPath, false); // Get all referencers
	}
	else
	{
		RelatedAssets = GetAssetDependencies(AssetPath, false); // Get all dependencies
	}
	
	for (const FString& RelatedAsset : RelatedAssets)
	{
		if (!VisitedAssets.Contains(RelatedAsset))
		{
			ResultArray.AddUnique(RelatedAsset);
			RecursiveDependencySearch(RelatedAsset, VisitedAssets, ResultArray, CurrentDepth + 1, MaxDepth, bSearchReferencers);
		}
	}
}

FString UAssetReferenceExtractor::ExportAssetReferenceToJSON(const FBS_AssetReferenceData& Data)
{
	TSharedPtr<FJsonObject> JsonObject = AssetReferenceToJsonObject(Data);
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	return OutputString;
}

FString UAssetReferenceExtractor::ExportProjectGraphToJSON(const FBS_ProjectReferenceGraph& Graph)
{
	TSharedPtr<FJsonObject> JsonObject = ProjectGraphToJsonObject(Graph);
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	return OutputString;
}

TSharedPtr<FJsonObject> UAssetReferenceExtractor::AssetReferenceToJsonObject(const FBS_AssetReferenceData& Data)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	JsonObject->SetStringField(TEXT("assetPath"), Data.AssetPath);
	JsonObject->SetStringField(TEXT("assetName"), Data.AssetName);
	JsonObject->SetStringField(TEXT("assetClass"), Data.AssetClass);
	JsonObject->SetNumberField(TEXT("totalDependencyCount"), Data.TotalDependencyCount);
	JsonObject->SetNumberField(TEXT("totalReferencerCount"), Data.TotalReferencerCount);
	JsonObject->SetStringField(TEXT("extractionTimestamp"), Data.ExtractionTimestamp);
	
	// Convert arrays to JSON arrays
	TArray<TSharedPtr<FJsonValue>> HardDepsArray;
	for (const FString& Dep : Data.HardDependencies)
	{
		HardDepsArray.Add(MakeShareable(new FJsonValueString(Dep)));
	}
	JsonObject->SetArrayField(TEXT("hardDependencies"), HardDepsArray);
	
	TArray<TSharedPtr<FJsonValue>> SoftDepsArray;
	for (const FString& Dep : Data.SoftDependencies)
	{
		SoftDepsArray.Add(MakeShareable(new FJsonValueString(Dep)));
	}
	JsonObject->SetArrayField(TEXT("softDependencies"), SoftDepsArray);
	
	TArray<TSharedPtr<FJsonValue>> HardRefsArray;
	for (const FString& Ref : Data.HardReferencers)
	{
		HardRefsArray.Add(MakeShareable(new FJsonValueString(Ref)));
	}
	JsonObject->SetArrayField(TEXT("hardReferencers"), HardRefsArray);
	
	TArray<TSharedPtr<FJsonValue>> SoftRefsArray;
	for (const FString& Ref : Data.SoftReferencers)
	{
		SoftRefsArray.Add(MakeShareable(new FJsonValueString(Ref)));
	}
	JsonObject->SetArrayField(TEXT("softReferencers"), SoftRefsArray);
	
	return JsonObject;
}

TSharedPtr<FJsonObject> UAssetReferenceExtractor::ProjectGraphToJsonObject(const FBS_ProjectReferenceGraph& Graph)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	JsonObject->SetStringField(TEXT("projectPath"), Graph.ProjectPath);
	JsonObject->SetNumberField(TEXT("totalAssetCount"), Graph.TotalAssetCount);
	JsonObject->SetNumberField(TEXT("totalReferenceCount"), Graph.TotalReferenceCount);
	JsonObject->SetStringField(TEXT("extractionTimestamp"), Graph.ExtractionTimestamp);
	
	// Convert asset references to JSON array
	TArray<TSharedPtr<FJsonValue>> AssetArray;
	for (const FBS_AssetReferenceData& AssetRef : Graph.AssetReferences)
	{
		TSharedPtr<FJsonObject> AssetObj = AssetReferenceToJsonObject(AssetRef);
		AssetArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
	}
	JsonObject->SetArrayField(TEXT("assetReferences"), AssetArray);
	
	return JsonObject;
}

bool UAssetReferenceExtractor::SaveReferenceDataToFile(const FString& JsonData, const FString& FilePath)
{
	return FFileHelper::SaveStringToFile(JsonData, *FilePath, FFileHelper::EEncodingOptions::AutoDetect);
}
