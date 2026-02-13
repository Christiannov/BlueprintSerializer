#include "BlueprintExtractorCommands.h"
#include "AssetReferenceExtractor.h"
#include "DataExportManager.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"
#endif

void FBlueprintExtractorCommands::ExtractAssetDependencies(const TArray<FString>& Args)
{
#if WITH_EDITOR
    if (Args.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ Usage: BP_SLZR.ExtractAssetDependencies <AssetPath> [MaxDepth] [bHardOnly] [bReferencersOnly]"));
        UE_LOG(LogTemp, Log, TEXT("Examples:"));
        UE_LOG(LogTemp, Log, TEXT("  BP_SLZR.ExtractAssetDependencies /Game/MyBlueprint"));
        UE_LOG(LogTemp, Log, TEXT("  BP_SLZR.ExtractAssetDependencies /Game/MyBlueprint 3"));
        UE_LOG(LogTemp, Log, TEXT("  BP_SLZR.ExtractAssetDependencies /Game/MyBlueprint 5 true false"));
        return;
    }
    
    FString AssetPath = Args[0];
    
    // Handle semicolon-separated commands
    int32 SemicolonIndex;
    if (AssetPath.FindChar(';', SemicolonIndex))
    {
        AssetPath = AssetPath.Left(SemicolonIndex);
    }
    
    // Parse optional parameters
    int32 MaxDepth = 5; // Default depth
    bool bHardOnly = false;
    bool bReferencersOnly = false;
    
    if (Args.Num() > 1)
    {
        MaxDepth = FCString::Atoi(*Args[1]);
        if (MaxDepth <= 0) MaxDepth = 5;
    }
    
    if (Args.Num() > 2)
    {
        bHardOnly = Args[2].Equals(TEXT("true"), ESearchCase::IgnoreCase) || Args[2] == TEXT("1");
    }
    
    if (Args.Num() > 3)
    {
        bReferencersOnly = Args[3].Equals(TEXT("true"), ESearchCase::IgnoreCase) || Args[3] == TEXT("1");
    }
    
    UE_LOG(LogTemp, Log, TEXT("🔍 Extracting asset dependencies:"));
    UE_LOG(LogTemp, Log, TEXT("  Asset: %s"), *AssetPath);
    UE_LOG(LogTemp, Log, TEXT("  Max Depth: %d"), MaxDepth);
    UE_LOG(LogTemp, Log, TEXT("  Hard References Only: %s"), bHardOnly ? TEXT("Yes") : TEXT("No"));
    UE_LOG(LogTemp, Log, TEXT("  Referencers Only: %s"), bReferencersOnly ? TEXT("Yes") : TEXT("No"));
    
    try
    {
        // Extract basic reference data
        FBS_AssetReferenceData ReferenceData = UAssetReferenceExtractor::ExtractAssetReferences(AssetPath);
        
        if (ReferenceData.AssetPath.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Could not find asset: %s"), *AssetPath);
            return;
        }
        
        TArray<FString> DepthResults;
        
        // Extract with depth control
        if (bReferencersOnly)
        {
            DepthResults = UAssetReferenceExtractor::FindDependentAssets(AssetPath, MaxDepth);
            UE_LOG(LogTemp, Log, TEXT("📊 Found %d assets that reference %s (depth %d):"), DepthResults.Num(), *ReferenceData.AssetName, MaxDepth);
        }
        else
        {
            DepthResults = UAssetReferenceExtractor::FindDependencyChain(AssetPath, MaxDepth);
            UE_LOG(LogTemp, Log, TEXT("📊 Found %d dependencies for %s (depth %d):"), DepthResults.Num(), *ReferenceData.AssetName, MaxDepth);
        }
        
        // Log results with categorization
        for (int32 i = 0; i < FMath::Min(DepthResults.Num(), 20); i++) // Limit to first 20 for readability
        {
            const FString& DepAsset = DepthResults[i];
            FString Category;
            
            if (DepAsset.Contains(TEXT("/Engine/"))) Category = TEXT("🏭 Engine");
            else if (DepAsset.Contains(TEXT("/Game/")) && DepAsset.Contains(TEXT("Blueprint"))) Category = TEXT("📘 Blueprint");
            else if (DepAsset.Contains(TEXT("/Game/"))) Category = TEXT("🎮 Game Asset");
            else Category = TEXT("❓ Other");
            
            UE_LOG(LogTemp, Log, TEXT("  %s: %s"), *Category, *DepAsset);
        }
        
        if (DepthResults.Num() > 20)
        {
            UE_LOG(LogTemp, Log, TEXT("  ... and %d more dependencies (check JSON export for complete list)"), DepthResults.Num() - 20);
        }
        
        // Export to JSON with enhanced metadata
        FString ExportPath = UDataExportManager::GetDefaultExportPath();
        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*ExportPath))
        {
            FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ExportPath);
        }
        
        FString FileName = FString::Printf(TEXT("BP_SLZR_AssetDependencies_%s_%s.json"), 
            *FPaths::GetCleanFilename(AssetPath), *UDataExportManager::GetTimestamp());
        FString FullPath = FPaths::Combine(ExportPath, FileName);
        
        // Create JSON with enhanced dependency data
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        JsonObject->SetStringField(TEXT("analysisType"), TEXT("AssetDependencyAnalysis"));
        JsonObject->SetStringField(TEXT("extractionTimestamp"), FDateTime::Now().ToString());
        JsonObject->SetStringField(TEXT("toolVersion"), TEXT("Blueprint Serializer v1.0"));
        JsonObject->SetNumberField(TEXT("maxDepth"), MaxDepth);
        JsonObject->SetBoolField(TEXT("hardReferencesOnly"), bHardOnly);
        JsonObject->SetBoolField(TEXT("referencersOnly"), bReferencersOnly);
        JsonObject->SetObjectField(TEXT("rootAsset"), UAssetReferenceExtractor::AssetReferenceToJsonObject(ReferenceData));
        
        TArray<TSharedPtr<FJsonValue>> DepthArray;
        for (const FString& Asset : DepthResults)
        {
            DepthArray.Add(MakeShareable(new FJsonValueString(Asset)));
        }
        JsonObject->SetArrayField(bReferencersOnly ? TEXT("referencersWithDepth") : TEXT("dependenciesWithDepth"), DepthArray);
        
        // Summary statistics
        TSharedPtr<FJsonObject> Summary = MakeShareable(new FJsonObject);
        Summary->SetNumberField(TEXT("totalDepthResults"), DepthResults.Num());
        Summary->SetNumberField(TEXT("immediateDependencies"), ReferenceData.HardDependencies.Num() + ReferenceData.SoftDependencies.Num());
        Summary->SetNumberField(TEXT("immediateReferencers"), ReferenceData.HardReferencers.Num() + ReferenceData.SoftReferencers.Num());
        JsonObject->SetObjectField(TEXT("summary"), Summary);
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        
        bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FullPath, FFileHelper::EEncodingOptions::AutoDetect);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Dependency analysis exported to: %s"), *FullPath);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Green, 
                    FString::Printf(TEXT("✅ Dependencies exported: %d found, depth %d"), DepthResults.Num(), MaxDepth));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to export dependency analysis"));
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Exception during asset dependency extraction"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("❌ Exception during dependency extraction"));
        }
    }
#endif
}

void FBlueprintExtractorCommands::ExtractProjectDependencyMap(const TArray<FString>& Args)
{
#if WITH_EDITOR
    // Parse parameters
    int32 MaxDepth = 3; // Conservative default for project-wide analysis
    FString AssetClassFilter = TEXT(""); // Empty means all asset types
    
    if (Args.Num() > 0)
    {
        MaxDepth = FCString::Atoi(*Args[0]);
        if (MaxDepth <= 0) MaxDepth = 3;
    }
    
    if (Args.Num() > 1)
    {
        AssetClassFilter = Args[1];
    }
    
    UE_LOG(LogTemp, Log, TEXT("🌐 Starting project-wide dependency mapping:"));
    UE_LOG(LogTemp, Log, TEXT("  Max Depth: %d"), MaxDepth);
    UE_LOG(LogTemp, Log, TEXT("  Asset Filter: %s"), AssetClassFilter.IsEmpty() ? TEXT("All Assets") : *AssetClassFilter);
    
    // Warning for high depth values
    if (MaxDepth > 5)
    {
        UE_LOG(LogTemp, Warning, TEXT("⚠️ High depth value (%d) may cause long processing times!"), MaxDepth);
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Orange, 
                FString::Printf(TEXT("⚠️ High depth analysis in progress...")));
        }
    }
    
    try
    {
        // Get filtered assets
        const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
        const IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
        
        TArray<FAssetData> AllAssets;
        AssetRegistry.GetAssetsByPath(TEXT("/Game"), AllAssets, true);
        
        // Apply class filter if specified
        TArray<FAssetData> FilteredAssets = AllAssets;
        if (!AssetClassFilter.IsEmpty())
        {
            FilteredAssets.Empty();
            for (const FAssetData& Asset : AllAssets)
            {
                if (Asset.AssetClassPath.ToString().Contains(AssetClassFilter))
                {
                    FilteredAssets.Add(Asset);
                }
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("📊 Processing %d assets (filtered from %d total)"), FilteredAssets.Num(), AllAssets.Num());
        
        // Create project dependency map
        TArray<FBS_AssetReferenceData> ProjectDependencyMap;
        int32 ProcessedCount = 0;
        
        for (const FAssetData& AssetData : FilteredAssets)
        {
            FString AssetPath = AssetData.GetObjectPathString();
            FBS_AssetReferenceData RefData = UAssetReferenceExtractor::ExtractAssetReferences(AssetPath);
            
            // Add depth analysis
            TArray<FString> DepthDependencies = UAssetReferenceExtractor::FindDependencyChain(AssetPath, MaxDepth);
            TArray<FString> DepthReferencers = UAssetReferenceExtractor::FindDependentAssets(AssetPath, MaxDepth);
            
            // Store depth counts as metadata
            RefData.TotalDependencyCount = DepthDependencies.Num();
            RefData.TotalReferencerCount = DepthReferencers.Num();
            
            ProjectDependencyMap.Add(RefData);
            
            ProcessedCount++;
            if (ProcessedCount % 50 == 0)
            {
                UE_LOG(LogTemp, Log, TEXT("Processed %d/%d assets..."), ProcessedCount, FilteredAssets.Num());
                if (GEngine)
                {
                    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Blue, 
                        FString::Printf(TEXT("Processing: %d/%d assets"), ProcessedCount, FilteredAssets.Num()));
                }
            }
        }
        
        // Export results
        FString ExportPath = UDataExportManager::GetDefaultExportPath();
        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*ExportPath))
        {
            FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ExportPath);
        }
        
        FString FileName = FString::Printf(TEXT("BP_SLZR_ProjectDependencyMap_%s_Depth%d_%s.json"), 
            AssetClassFilter.IsEmpty() ? TEXT("AllAssets") : *AssetClassFilter,
            MaxDepth, *UDataExportManager::GetTimestamp());
        FString FullPath = FPaths::Combine(ExportPath, FileName);
        
        // Create enhanced JSON
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        JsonObject->SetStringField(TEXT("analysisType"), TEXT("ProjectDependencyMap"));
        JsonObject->SetStringField(TEXT("extractionTimestamp"), FDateTime::Now().ToString());
        JsonObject->SetStringField(TEXT("projectPath"), FPaths::GetProjectFilePath());
        JsonObject->SetStringField(TEXT("toolVersion"), TEXT("Blueprint Serializer v1.0"));
        JsonObject->SetNumberField(TEXT("maxDepth"), MaxDepth);
        JsonObject->SetStringField(TEXT("assetClassFilter"), AssetClassFilter.IsEmpty() ? TEXT("All") : AssetClassFilter);
        
        TArray<TSharedPtr<FJsonValue>> AssetsArray;
        for (const FBS_AssetReferenceData& AssetRef : ProjectDependencyMap)
        {
            TSharedPtr<FJsonObject> AssetObj = UAssetReferenceExtractor::AssetReferenceToJsonObject(AssetRef);
            AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
        }
        JsonObject->SetArrayField(TEXT("assetDependencies"), AssetsArray);
        
        // Summary statistics
        TSharedPtr<FJsonObject> Summary = MakeShareable(new FJsonObject);
        Summary->SetNumberField(TEXT("totalAssets"), ProjectDependencyMap.Num());
        int32 TotalDeps = 0, TotalRefs = 0;
        for (const FBS_AssetReferenceData& Asset : ProjectDependencyMap)
        {
            TotalDeps += Asset.TotalDependencyCount;
            TotalRefs += Asset.TotalReferencerCount;
        }
        Summary->SetNumberField(TEXT("totalDependencies"), TotalDeps);
        Summary->SetNumberField(TEXT("totalReferences"), TotalRefs);
        JsonObject->SetObjectField(TEXT("summary"), Summary);
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        
        bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FullPath, FFileHelper::EEncodingOptions::AutoDetect);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Project dependency map exported to: %s"), *FullPath);
            UE_LOG(LogTemp, Log, TEXT("📊 Mapped %d assets with depth %d analysis"), ProjectDependencyMap.Num(), MaxDepth);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, 
                    FString::Printf(TEXT("✅ Project map: %d assets, depth %d"), ProjectDependencyMap.Num(), MaxDepth));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to export project dependency map"));
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Exception during project dependency mapping"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("❌ Exception during project mapping"));
        }
    }
#endif
}

void FBlueprintExtractorCommands::MapAssetNetwork(const TArray<FString>& Args)
{
#if WITH_EDITOR
    if (Args.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("❌ Usage: BP_SLZR.MapAssetNetwork <AssetPath> [MaxDepth] [bBidirectional]"));
        UE_LOG(LogTemp, Log, TEXT("Examples:"));
        UE_LOG(LogTemp, Log, TEXT("  BP_SLZR.MapAssetNetwork /Game/MyBlueprint"));
        UE_LOG(LogTemp, Log, TEXT("  BP_SLZR.MapAssetNetwork /Game/MyBlueprint 4 true"));
        return;
    }
    
    FString AssetPath = Args[0];
    
    // Handle semicolon-separated commands
    int32 SemicolonIndex;
    if (AssetPath.FindChar(';', SemicolonIndex))
    {
        AssetPath = AssetPath.Left(SemicolonIndex);
    }
    
    // Parse optional parameters
    int32 MaxDepth = 3; // Conservative default
    bool bBidirectional = true; // Include both dependencies and referencers
    
    if (Args.Num() > 1)
    {
        MaxDepth = FCString::Atoi(*Args[1]);
        if (MaxDepth <= 0) MaxDepth = 3;
    }
    
    if (Args.Num() > 2)
    {
        bBidirectional = Args[2].Equals(TEXT("true"), ESearchCase::IgnoreCase) || Args[2] == TEXT("1");
    }
    
    UE_LOG(LogTemp, Log, TEXT("🕸️ Mapping asset network:"));
    UE_LOG(LogTemp, Log, TEXT("  Root Asset: %s"), *AssetPath);
    UE_LOG(LogTemp, Log, TEXT("  Max Depth: %d"), MaxDepth);
    UE_LOG(LogTemp, Log, TEXT("  Bidirectional: %s"), bBidirectional ? TEXT("Yes (Dependencies + Referencers)") : TEXT("No (Dependencies Only)"));
    
    try
    {
        // Extract root asset info
        FBS_AssetReferenceData RootAsset = UAssetReferenceExtractor::ExtractAssetReferences(AssetPath);
        
        if (RootAsset.AssetPath.IsEmpty())
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Could not find root asset: %s"), *AssetPath);
            return;
        }
        
        // Build network map
        TArray<FString> Dependencies = UAssetReferenceExtractor::FindDependencyChain(AssetPath, MaxDepth);
        TArray<FString> Referencers;
        
        if (bBidirectional)
        {
            Referencers = UAssetReferenceExtractor::FindDependentAssets(AssetPath, MaxDepth);
        }
        
        // Create network nodes (get reference data for each connected asset)
        TArray<FBS_AssetReferenceData> NetworkNodes;
        TSet<FString> ProcessedAssets;
        
        // Add root
        NetworkNodes.Add(RootAsset);
        ProcessedAssets.Add(AssetPath);
        
        // Add dependency nodes
        for (const FString& DepAsset : Dependencies)
        {
            if (!ProcessedAssets.Contains(DepAsset))
            {
                FBS_AssetReferenceData NodeData = UAssetReferenceExtractor::ExtractAssetReferences(DepAsset);
                if (!NodeData.AssetPath.IsEmpty())
                {
                    NetworkNodes.Add(NodeData);
                    ProcessedAssets.Add(DepAsset);
                }
            }
        }
        
        // Add referencer nodes
        if (bBidirectional)
        {
            for (const FString& RefAsset : Referencers)
            {
                if (!ProcessedAssets.Contains(RefAsset))
                {
                    FBS_AssetReferenceData NodeData = UAssetReferenceExtractor::ExtractAssetReferences(RefAsset);
                    if (!NodeData.AssetPath.IsEmpty())
                    {
                        NetworkNodes.Add(NodeData);
                        ProcessedAssets.Add(RefAsset);
                    }
                }
            }
        }
        
        UE_LOG(LogTemp, Log, TEXT("📊 Network Analysis Results:"));
        UE_LOG(LogTemp, Log, TEXT("  Root: %s"), *RootAsset.AssetName);
        UE_LOG(LogTemp, Log, TEXT("  Dependencies: %d (depth %d)"), Dependencies.Num(), MaxDepth);
        if (bBidirectional) UE_LOG(LogTemp, Log, TEXT("  Referencers: %d (depth %d)"), Referencers.Num(), MaxDepth);
        UE_LOG(LogTemp, Log, TEXT("  Total Network Nodes: %d"), NetworkNodes.Num());
        
        // Export network to JSON
        FString ExportPath = UDataExportManager::GetDefaultExportPath();
        if (!FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*ExportPath))
        {
            FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ExportPath);
        }
        
        FString FileName = FString::Printf(TEXT("BP_SLZR_AssetNetwork_%s_Depth%d_%s_%s.json"), 
            *FPaths::GetCleanFilename(AssetPath), MaxDepth, 
            bBidirectional ? TEXT("Bidirectional") : TEXT("Dependencies"),
            *UDataExportManager::GetTimestamp());
        FString FullPath = FPaths::Combine(ExportPath, FileName);
        
        // Create comprehensive network JSON
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
        JsonObject->SetStringField(TEXT("analysisType"), TEXT("AssetNetworkMap"));
        JsonObject->SetStringField(TEXT("extractionTimestamp"), FDateTime::Now().ToString());
        JsonObject->SetStringField(TEXT("toolVersion"), TEXT("Blueprint Serializer v1.0"));
        JsonObject->SetStringField(TEXT("rootAssetPath"), RootAsset.AssetPath);
        JsonObject->SetNumberField(TEXT("maxDepth"), MaxDepth);
        JsonObject->SetBoolField(TEXT("bidirectional"), bBidirectional);
        
        // Root asset
        JsonObject->SetObjectField(TEXT("rootAsset"), UAssetReferenceExtractor::AssetReferenceToJsonObject(RootAsset));
        
        // Network nodes
        TArray<TSharedPtr<FJsonValue>> NodesArray;
        for (const FBS_AssetReferenceData& Node : NetworkNodes)
        {
            TSharedPtr<FJsonObject> NodeObj = UAssetReferenceExtractor::AssetReferenceToJsonObject(Node);
            NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
        }
        JsonObject->SetArrayField(TEXT("networkNodes"), NodesArray);
        
        // Dependency paths
        TArray<TSharedPtr<FJsonValue>> DepsArray;
        for (const FString& Dep : Dependencies)
        {
            DepsArray.Add(MakeShareable(new FJsonValueString(Dep)));
        }
        JsonObject->SetArrayField(TEXT("dependencyPaths"), DepsArray);
        
        // Referencer paths (if bidirectional)
        if (bBidirectional)
        {
            TArray<TSharedPtr<FJsonValue>> RefsArray;
            for (const FString& Ref : Referencers)
            {
                RefsArray.Add(MakeShareable(new FJsonValueString(Ref)));
            }
            JsonObject->SetArrayField(TEXT("referencerPaths"), RefsArray);
        }
        
        // Network statistics
        TSharedPtr<FJsonObject> NetworkStats = MakeShareable(new FJsonObject);
        NetworkStats->SetNumberField(TEXT("totalNodes"), NetworkNodes.Num());
        NetworkStats->SetNumberField(TEXT("dependencyConnections"), Dependencies.Num());
        NetworkStats->SetNumberField(TEXT("referencerConnections"), bBidirectional ? Referencers.Num() : 0);
        NetworkStats->SetNumberField(TEXT("maxDepthReached"), MaxDepth);
        JsonObject->SetObjectField(TEXT("networkStatistics"), NetworkStats);
        
        FString OutputString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
        
        bool bSuccess = FFileHelper::SaveStringToFile(OutputString, *FullPath, FFileHelper::EEncodingOptions::AutoDetect);
        
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("✅ Asset network map exported to: %s"), *FullPath);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Green, 
                    FString::Printf(TEXT("✅ Network: %d nodes, %d deps, %d refs"), NetworkNodes.Num(), Dependencies.Num(), Referencers.Num()));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("❌ Failed to export asset network map"));
        }
    }
    catch (...)
    {
        UE_LOG(LogTemp, Error, TEXT("❌ Exception during asset network mapping"));
        if (GEngine)
        {
            GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, TEXT("❌ Exception during network mapping"));
        }
    }
#endif
}
