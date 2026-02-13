#include "BlueprintAnalyzer.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimMontage.h"
#include "Animation/BlendSpace.h"
#include "Animation/BlendSpace1D.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimCurveTypes.h"
#include "Animation/AnimData/AnimDataModel.h"
// #include "Animation/AnimData/AnimDataModelInterface.h"  // Not available in UE 5.6
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "UObject/Script.h"

#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "AnimGraphNode_Inertialization.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_LinkedInputPose.h"
#include "AnimGraphNode_StateMachine.h"
#include "AnimationStateMachineGraph.h"
#include "AnimStateNode.h"
#include "AnimStateNodeBase.h"
#include "AnimStateEntryNode.h"
#include "AnimStateTransitionNode.h"
#include "AnimStateConduitNode.h"

#if __has_include("AnimGraphNode_LinkedAnimLayer.h")
#include "AnimGraphNode_LinkedAnimLayer.h"
#define UEARATAME_HAS_LINKED_ANIM_LAYER 1
#else
#define UEARATAME_HAS_LINKED_ANIM_LAYER 0
#endif

#if __has_include("AnimStateAliasNode.h")
#include "AnimStateAliasNode.h"
#define UEARATAME_HAS_ANIM_STATE_ALIAS 1
#else
#define UEARATAME_HAS_ANIM_STATE_ALIAS 0
#endif

#if __has_include("ControlRigBlueprint.h")
#include "ControlRigBlueprint.h"
#include "RigVMModel/RigVMGraph.h"
#include "RigVMModel/RigVMNode.h"
#include "RigVMModel/RigVMPin.h"
#include "RigVMModel/RigVMLink.h"
#include "Engine/SkeletalMesh.h"
#define UEARATAME_HAS_CONTROL_RIG 1
#else
#define UEARATAME_HAS_CONTROL_RIG 0
#endif

#if __has_include("Misc/EngineVersionComparison.h")
#include "Misc/EngineVersionComparison.h"
#endif

// Blueprint Graph includes for detailed node analysis
#include "K2Node.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_VariableSetRef.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_Timeline.h"
#include "K2Node_DynamicCast.h"
#include "EdGraphSchema_K2.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/PropertyPortFlags.h"
#include "UObject/SoftObjectPtr.h"
#include "KismetCompiler.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "UObject/UnrealType.h"
#include "Engine/Blueprint.h"
class UK2Node_CallFunction;
class UK2Node_VariableGet;
class UK2Node_VariableSet;

#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Misc/FileHelper.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

namespace
{
    FString GetPropertyValueAsString(UObject* Object, FProperty* Property)
    {
        if (!Object || !Property)
        {
            return FString();
        }

        FString Value;
        const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(Object);
        Property->ExportTextItem_Direct(Value, ValuePtr, nullptr, Object, PPF_None);
        return Value;
    }

    void ExtractObjectProperties(UObject* Object, TMap<FString, FString>& OutProperties)
    {
        if (!Object)
        {
            return;
        }

        for (TFieldIterator<FProperty> PropIt(Object->GetClass()); PropIt; ++PropIt)
        {
            FProperty* Prop = *PropIt;
            if (!Prop || Prop->HasAnyPropertyFlags(CPF_Deprecated))
            {
                continue;
            }

            const FString Value = GetPropertyValueAsString(Object, Prop);
            OutProperties.Add(Prop->GetName(), Value);
        }
    }

    void AddSortedStringMap(TSharedPtr<FJsonObject> Target, const FString& FieldName, const TMap<FString, FString>& Map)
    {
        if (!Target.IsValid())
        {
            return;
        }

        TArray<FString> Keys;
        Map.GetKeys(Keys);
        Keys.Sort();

        TSharedPtr<FJsonObject> MapObj = MakeShareable(new FJsonObject);
        for (const FString& Key : Keys)
        {
            const FString* Value = Map.Find(Key);
            MapObj->SetStringField(Key, Value ? *Value : FString());
        }

        Target->SetObjectField(FieldName, MapObj);
    }

    void AddSortedIntMap(TSharedPtr<FJsonObject> Target, const FString& FieldName, const TMap<FString, int32>& Map)
    {
        if (!Target.IsValid())
        {
            return;
        }

        TArray<FString> Keys;
        Map.GetKeys(Keys);
        Keys.Sort();

        TSharedPtr<FJsonObject> MapObj = MakeShareable(new FJsonObject);
        for (const FString& Key : Keys)
        {
            const int32* Value = Map.Find(Key);
            MapObj->SetNumberField(Key, Value ? *Value : 0);
        }

        Target->SetObjectField(FieldName, MapObj);
    }

    void AddSortedBoolMap(TSharedPtr<FJsonObject> Target, const FString& FieldName, const TMap<FString, bool>& Map)
    {
        if (!Target.IsValid())
        {
            return;
        }

        TArray<FString> Keys;
        Map.GetKeys(Keys);
        Keys.Sort();

        TSharedPtr<FJsonObject> MapObj = MakeShareable(new FJsonObject);
        for (const FString& Key : Keys)
        {
            const bool* Value = Map.Find(Key);
            MapObj->SetBoolField(Key, Value ? *Value : false);
        }

        Target->SetObjectField(FieldName, MapObj);
    }

    TArray<TSharedPtr<FJsonValue>> BuildStringArray(const TArray<FString>& Values)
    {
        TArray<TSharedPtr<FJsonValue>> Result;
        Result.Reserve(Values.Num());
        for (const FString& Value : Values)
        {
            Result.Add(MakeShareable(new FJsonValueString(Value)));
        }
        return Result;
    }

    TArray<TSharedPtr<FJsonValue>> BuildFloatArray(const TArray<float>& Values)
    {
        TArray<TSharedPtr<FJsonValue>> Result;
        Result.Reserve(Values.Num());
        for (float Value : Values)
        {
            Result.Add(MakeShareable(new FJsonValueNumber(Value)));
        }
        return Result;
    }
}

FBS_BlueprintData UBlueprintAnalyzer::AnalyzeBlueprint(UBlueprint* Blueprint)
{
	FBS_BlueprintData Data;
	
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("BlueprintAnalyzer: Invalid Blueprint provided"));
		return Data;
	}
	
	// Basic Blueprint information
	Data.SchemaVersion = TEXT("1.5");
	Data.BlueprintPath = Blueprint->GetPathName();
	Data.BlueprintName = Blueprint->GetName();
	Data.ParentClassName = Blueprint->ParentClass ? Blueprint->ParentClass->GetName() : TEXT("None");
	Data.GeneratedClassName = Blueprint->GeneratedClass ? Blueprint->GeneratedClass->GetName() : TEXT("None");
	Data.ExtractionTimestamp = FDateTime::Now().ToString();
	
	UE_LOG(LogTemp, Warning, TEXT("Analyzing Blueprint: %s"), *Data.BlueprintName);
	
	// Extract Blueprint metadata and settings
	ExtractBlueprintMetadata(Blueprint, Data);
	
	// Extract detailed Blueprint data
	Data.ImplementedInterfaces = ExtractImplementedInterfaces(Blueprint);
	
	// Legacy simple extraction (for backward compatibility)
	Data.Variables = ExtractVariables(Blueprint);
	Data.Functions = ExtractFunctions(Blueprint);
	Data.Components = ExtractComponents(Blueprint);
	
	// Enhanced detailed extraction
	Data.DetailedVariables = ExtractDetailedVariables(Blueprint);
	Data.DetailedFunctions = ExtractDetailedFunctions(Blueprint);
	Data.DetailedComponents = ExtractDetailedComponents(Blueprint);
	Data.AssetReferences = ExtractAssetReferences(Blueprint);
	
	Data.EventNodes = ExtractEventNodes(Blueprint);
	Data.GraphNodes = ExtractGraphNodes(Blueprint, Data.TotalNodeCount);
	
	// Extract structured graphs (new approach)
	Data.StructuredGraphs = ExtractStructuredGraphs(Blueprint, Data.TotalNodeCount);
	
	// Extract extended structured graphs with proper JSON objects and flows
	Data.StructuredGraphsExt = ExtractStructuredGraphsExt(Blueprint, Data.TotalNodeCount);

	// AnimBlueprint-specific extraction
	ExtractAnimBlueprintData(Blueprint, Data);
	
	UE_LOG(LogTemp, Warning, TEXT("Blueprint %s analysis complete - Variables: %d, Functions: %d, Components: %d, Nodes: %d, Graphs: %d"), 
		*Data.BlueprintName, Data.Variables.Num(), Data.Functions.Num(), Data.Components.Num(), Data.TotalNodeCount, Data.StructuredGraphs.Num());
	
	return Data;
}

TArray<FBS_BlueprintData> UBlueprintAnalyzer::AnalyzeAllProjectBlueprints()
{
	TArray<FBS_BlueprintData> AllBlueprintData;
	
	// Get Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	
	// Find all Blueprint assets (UE5.5 API)
	TArray<FAssetData> BlueprintAssets;
	FTopLevelAssetPath BlueprintPath(UBlueprint::StaticClass());
	AssetRegistry.GetAssetsByClass(BlueprintPath, BlueprintAssets);

	TArray<FAssetData> AnimBlueprintAssets;
	FTopLevelAssetPath AnimBlueprintPath(UAnimBlueprint::StaticClass());
	AssetRegistry.GetAssetsByClass(AnimBlueprintPath, AnimBlueprintAssets);

	TSet<FName> SeenPackages;
	TArray<FAssetData> AllAssets;
	AllAssets.Reserve(BlueprintAssets.Num() + AnimBlueprintAssets.Num());
	for (const FAssetData& AssetData : BlueprintAssets)
	{
		if (!SeenPackages.Contains(AssetData.PackageName))
		{
		    SeenPackages.Add(AssetData.PackageName);
			AllAssets.Add(AssetData);
		}
	}
	for (const FAssetData& AssetData : AnimBlueprintAssets)
	{
		if (!SeenPackages.Contains(AssetData.PackageName))
		{
		    SeenPackages.Add(AssetData.PackageName);
			AllAssets.Add(AssetData);
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Found %d Blueprint assets (%d AnimBlueprints) to analyze"), AllAssets.Num(), AnimBlueprintAssets.Num());
	
	// Analyze each Blueprint
	for (const FAssetData& AssetData : AllAssets)
	{
		// Load the Blueprint
		UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset());
		if (Blueprint)
		{
			FBS_BlueprintData Data = AnalyzeBlueprint(Blueprint);
			AllBlueprintData.Add(Data);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to load Blueprint: %s"), *AssetData.AssetName.ToString());
		}
	}
	
	return AllBlueprintData;
}

TArray<FString> UBlueprintAnalyzer::ExtractVariables(UBlueprint* Blueprint)
{
	TArray<FString> Variables;
	
	if (!Blueprint || !Blueprint->GeneratedClass)
	{
		return Variables;
	}
	
	// Extract Blueprint variables from NewVariables array
	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		FString VarInfo = FString::Printf(TEXT("%s (%s)"), 
			*Variable.VarName.ToString(), 
			*Variable.VarType.PinCategory.ToString());
		Variables.Add(VarInfo);
	}
	
	// Also extract inherited properties
	for (TFieldIterator<FProperty> PropertyIt(Blueprint->GeneratedClass); PropertyIt; ++PropertyIt)
	{
		FProperty* Property = *PropertyIt;
		if (Property && Property->GetOwnerClass() == Blueprint->GeneratedClass)
		{
			FString PropInfo = FString::Printf(TEXT("%s (%s) [Inherited]"), 
				*Property->GetName(), 
				*Property->GetClass()->GetName());
			Variables.Add(PropInfo);
		}
	}
	
	return Variables;
}

TArray<FString> UBlueprintAnalyzer::ExtractFunctions(UBlueprint* Blueprint)
{
	TArray<FString> Functions;
	
	if (!Blueprint)
	{
		return Functions;
	}
	
	// Extract Blueprint functions
	for (UEdGraph* FunctionGraph : Blueprint->FunctionGraphs)
	{
		if (FunctionGraph)
		{
			FString FuncInfo = FString::Printf(TEXT("%s"), *FunctionGraph->GetName());
			Functions.Add(FuncInfo);
		}
	}
	
	// Extract functions from generated class
	if (Blueprint->GeneratedClass)
	{
		for (TFieldIterator<UFunction> FunctionIt(Blueprint->GeneratedClass); FunctionIt; ++FunctionIt)
		{
			UFunction* Function = *FunctionIt;
			if (Function && Function->GetOwnerClass() == Blueprint->GeneratedClass)
			{
				FString FuncInfo = FString::Printf(TEXT("%s [Generated]"), *Function->GetName());
				Functions.Add(FuncInfo);
			}
		}
	}
	
	return Functions;
}

TArray<FString> UBlueprintAnalyzer::ExtractComponents(UBlueprint* Blueprint)
{
	TArray<FString> Components;
	
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return Components;
	}
	
	// Extract components from Simple Construction Script
	TArray<USCS_Node*> AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
	
	for (USCS_Node* Node : AllNodes)
	{
		if (Node && Node->ComponentClass)
		{
			FString CompInfo = FString::Printf(TEXT("%s (%s)"), 
				*Node->GetVariableName().ToString(), 
				*Node->ComponentClass->GetName());
			Components.Add(CompInfo);
		}
	}
	
	return Components;
}

TArray<FString> UBlueprintAnalyzer::ExtractGraphNodes(UBlueprint* Blueprint, int32& OutTotalNodeCount)
{
	TArray<FString> GraphNodes;
	OutTotalNodeCount = 0;
	
	if (!Blueprint)
	{
		return GraphNodes;
	}
	
	// Process all graphs in the Blueprint (UE5.5 compatible approach)
	TArray<UEdGraph*> AllGraphs;
	
	// Add ubergraph pages (main event graph)
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph)
		{
			AllGraphs.Add(Graph);
		}
	}
	
	// Add function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph)
		{
			AllGraphs.Add(Graph);
		}
	}
	
	// Add macro graphs if any
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph)
		{
			AllGraphs.Add(Graph);
		}
	}
	
	for (UEdGraph* Graph : AllGraphs)
	{
		if (Graph)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
					// Extract detailed node information including pins and connections
					FString NodeInfo = AnalyzeNodeWithConnections(Node);
					if (!NodeInfo.IsEmpty())
					{
						GraphNodes.Add(NodeInfo);
					}
					OutTotalNodeCount++;
				}
			}
		}
	}
	
	return GraphNodes;
}

TArray<FBS_GraphData> UBlueprintAnalyzer::ExtractStructuredGraphs(UBlueprint* Blueprint, int32& OutTotalNodeCount)
{
	TArray<FBS_GraphData> StructuredGraphs;
	OutTotalNodeCount = 0;
	
	if (!Blueprint)
	{
		return StructuredGraphs;
	}
	
	// Process Ubergraph pages (main event graph)
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph)
		{
			FBS_GraphData GraphData;
			GraphData.GraphName = Graph->GetName();
			GraphData.GraphType = TEXT("Ubergraph");
			
            // Process nodes in this graph
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
                    // Preferred: structured node
                    const FBS_NodeData NodeData = AnalyzeNodeToStruct(Node);
                    // Also provide legacy stringified JSON for backward compatibility
                    TSharedPtr<FJsonObject> NodeJson = AnalyzeNodeToJsonObject(Node);
                    if (NodeJson.IsValid())
                    {
                        FString NodeString;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeString);
                        FJsonSerializer::Serialize(NodeJson.ToSharedRef(), Writer);
                        GraphData.Nodes.Add(NodeString);
                    }
                    
                    // Exec edges (with pin names for better tracking)
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
                        {
                            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                            {
                                if (LinkedPin && LinkedPin->GetOwningNode())
                                {
                                    // Store with pin names for structured output
                                    const FString Flow = FString::Printf(TEXT("%s:%s->%s:%s"),
                                        *Node->NodeGuid.ToString(),
                                        *Pin->PinName.ToString(),
                                        *LinkedPin->GetOwningNode()->NodeGuid.ToString(),
                                        *LinkedPin->PinName.ToString());
                                    GraphData.ExecutionFlows.Add(Flow);
                                }
                            }
                        }
                    }
                    
                    // Bump count
					OutTotalNodeCount++;
				}
			}
			
			StructuredGraphs.Add(GraphData);
		}
	}
	
	// Process function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph)
		{
			FBS_GraphData GraphData;
			GraphData.GraphName = Graph->GetName();
			GraphData.GraphType = TEXT("Function");
			
            // Process nodes in this graph
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
                    const FBS_NodeData NodeData = AnalyzeNodeToStruct(Node);
                    TSharedPtr<FJsonObject> NodeJson = AnalyzeNodeToJsonObject(Node);
                    if (NodeJson.IsValid())
                    {
                        FString NodeString;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeString);
                        FJsonSerializer::Serialize(NodeJson.ToSharedRef(), Writer);
                        GraphData.Nodes.Add(NodeString);
                    }
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
                        {
                            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                            {
                                if (LinkedPin && LinkedPin->GetOwningNode())
                                {
                                    const FString Flow = FString::Printf(TEXT("%s:%s->%s:%s"),
                                        *Node->NodeGuid.ToString(),
                                        *Pin->PinName.ToString(),
                                        *LinkedPin->GetOwningNode()->NodeGuid.ToString(),
                                        *LinkedPin->PinName.ToString());
                                    GraphData.ExecutionFlows.Add(Flow);
                                }
                            }
                        }
                    }
					OutTotalNodeCount++;
				}
			}
			
			StructuredGraphs.Add(GraphData);
		}
	}
	
	// Process macro graphs
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph)
		{
			FBS_GraphData GraphData;
			GraphData.GraphName = Graph->GetName();
			GraphData.GraphType = TEXT("Macro");
			
            // Process nodes in this graph
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
                    const FBS_NodeData NodeData = AnalyzeNodeToStruct(Node);
                    TSharedPtr<FJsonObject> NodeJson = AnalyzeNodeToJsonObject(Node);
                    if (NodeJson.IsValid())
                    {
                        FString NodeString;
                        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&NodeString);
                        FJsonSerializer::Serialize(NodeJson.ToSharedRef(), Writer);
                        GraphData.Nodes.Add(NodeString);
                    }
                    for (UEdGraphPin* Pin : Node->Pins)
                    {
                        if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
                        {
                            for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
                            {
                                if (LinkedPin && LinkedPin->GetOwningNode())
                                {
                                    const FString Flow = FString::Printf(TEXT("%s:%s->%s:%s"),
                                        *Node->NodeGuid.ToString(),
                                        *Pin->PinName.ToString(),
                                        *LinkedPin->GetOwningNode()->NodeGuid.ToString(),
                                        *LinkedPin->PinName.ToString());
                                    GraphData.ExecutionFlows.Add(Flow);
                                }
                            }
                        }
                    }
					OutTotalNodeCount++;
				}
			}
			
			StructuredGraphs.Add(GraphData);
		}
	}
	
	return StructuredGraphs;
}

TArray<FBS_GraphData_Ext> UBlueprintAnalyzer::ExtractStructuredGraphsExt(UBlueprint* Blueprint, int32& OutTotalNodeCount)
{
	TArray<FBS_GraphData_Ext> StructuredGraphs;
	OutTotalNodeCount = 0;
	
	if (!Blueprint)
	{
		return StructuredGraphs;
	}
	
	auto AddDataLinks = [](UEdGraphNode* Node, FBS_GraphData_Ext& GraphData, TSet<FString>& Seen)
	{
		if (!Node)
		{
			return;
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				continue;
			}

			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!LinkedPin || !LinkedPin->GetOwningNode())
				{
					continue;
				}

				// Resolve source/target based on direction to keep data flow consistent
				FGuid SourceGuid;
				FName SourcePin;
				FGuid SourcePinId;
				FGuid TargetGuid;
				FName TargetPin;
				FGuid TargetPinId;

				if (Pin->Direction == EGPD_Output)
				{
					SourceGuid = Node->NodeGuid;
					SourcePin = Pin->PinName;
					SourcePinId = Pin->PinId;
					TargetGuid = LinkedPin->GetOwningNode()->NodeGuid;
					TargetPin = LinkedPin->PinName;
					TargetPinId = LinkedPin->PinId;
				}
				else if (Pin->Direction == EGPD_Input)
				{
					SourceGuid = LinkedPin->GetOwningNode()->NodeGuid;
					SourcePin = LinkedPin->PinName;
					SourcePinId = LinkedPin->PinId;
					TargetGuid = Node->NodeGuid;
					TargetPin = Pin->PinName;
					TargetPinId = Pin->PinId;
				}
				else
				{
					SourceGuid = Node->NodeGuid;
					SourcePin = Pin->PinName;
					SourcePinId = Pin->PinId;
					TargetGuid = LinkedPin->GetOwningNode()->NodeGuid;
					TargetPin = LinkedPin->PinName;
					TargetPinId = LinkedPin->PinId;
				}

				const FString Key = FString::Printf(TEXT("%s|%s|%s|%s|%s|%s"),
					*SourceGuid.ToString(),
					*SourcePin.ToString(),
					*TargetGuid.ToString(),
					*TargetPin.ToString(),
					*Pin->PinType.PinCategory.ToString(),
					*Pin->PinType.PinSubCategory.ToString());

				if (Seen.Contains(Key))
				{
					continue;
				}
				Seen.Add(Key);

				FBS_PinLinkData Link;
				Link.SourceNodeGuid = SourceGuid;
				Link.SourcePinName = SourcePin;
				Link.SourcePinId = SourcePinId;
				Link.TargetNodeGuid = TargetGuid;
				Link.TargetPinName = TargetPin;
				Link.TargetPinId = TargetPinId;
				Link.PinCategory = Pin->PinType.PinCategory.ToString();
				Link.PinSubCategory = Pin->PinType.PinSubCategory.ToString();
				Link.bIsExec = false;
				GraphData.DataLinks.Add(Link);
			}
		}
	};

	// Process Ubergraph pages (main event graph)
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph)
		{
			FBS_GraphData_Ext GraphData;
			GraphData.GraphName = Graph->GetName();
			GraphData.GraphType = TEXT("Ubergraph");
			TSet<FString> SeenDataLinks;
			
			// Process nodes in this graph
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
					// Use structured node data
					FBS_NodeData NodeData = AnalyzeNodeToStruct(Node);
					GraphData.Nodes.Add(NodeData);
					
					// Exec edges as structured data
					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
						{
							for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
							{
								if (LinkedPin && LinkedPin->GetOwningNode())
								{
									FBS_FlowEdge Edge;
									Edge.SourceNodeGuid = Node->NodeGuid;
									Edge.SourcePinName = Pin->PinName;
									Edge.SourcePinId = Pin->PinId;
									Edge.TargetNodeGuid = LinkedPin->GetOwningNode()->NodeGuid;
									Edge.TargetPinName = LinkedPin->PinName;
									Edge.TargetPinId = LinkedPin->PinId;
									GraphData.Execution.Add(Edge);
								}
							}
						}
					}

					AddDataLinks(Node, GraphData, SeenDataLinks);
					
					OutTotalNodeCount++;
				}
			}
			
			StructuredGraphs.Add(GraphData);
		}
	}
	
	// Process function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph)
		{
			FBS_GraphData_Ext GraphData;
			GraphData.GraphName = Graph->GetName();
			GraphData.GraphType = TEXT("Function");
			TSet<FString> SeenDataLinks;
			
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
					FBS_NodeData NodeData = AnalyzeNodeToStruct(Node);
					GraphData.Nodes.Add(NodeData);
					
					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
						{
							for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
							{
								if (LinkedPin && LinkedPin->GetOwningNode())
								{
									FBS_FlowEdge Edge;
									Edge.SourceNodeGuid = Node->NodeGuid;
									Edge.SourcePinName = Pin->PinName;
									Edge.SourcePinId = Pin->PinId;
									Edge.TargetNodeGuid = LinkedPin->GetOwningNode()->NodeGuid;
									Edge.TargetPinName = LinkedPin->PinName;
									Edge.TargetPinId = LinkedPin->PinId;
									GraphData.Execution.Add(Edge);
								}
							}
						}
					}

					AddDataLinks(Node, GraphData, SeenDataLinks);
					
					OutTotalNodeCount++;
				}
			}
			
			StructuredGraphs.Add(GraphData);
		}
	}
	
	// Process macro graphs
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph)
		{
			FBS_GraphData_Ext GraphData;
			GraphData.GraphName = Graph->GetName();
			GraphData.GraphType = TEXT("Macro");
			TSet<FString> SeenDataLinks;
			
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (Node)
				{
					FBS_NodeData NodeData = AnalyzeNodeToStruct(Node);
					GraphData.Nodes.Add(NodeData);
					
					for (UEdGraphPin* Pin : Node->Pins)
					{
						if (Pin && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec && Pin->Direction == EGPD_Output)
						{
							for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
							{
								if (LinkedPin && LinkedPin->GetOwningNode())
								{
									FBS_FlowEdge Edge;
									Edge.SourceNodeGuid = Node->NodeGuid;
									Edge.SourcePinName = Pin->PinName;
									Edge.SourcePinId = Pin->PinId;
									Edge.TargetNodeGuid = LinkedPin->GetOwningNode()->NodeGuid;
									Edge.TargetPinName = LinkedPin->PinName;
									Edge.TargetPinId = LinkedPin->PinId;
									GraphData.Execution.Add(Edge);
								}
							}
						}
					}

					AddDataLinks(Node, GraphData, SeenDataLinks);
					
					OutTotalNodeCount++;
				}
			}
			
			StructuredGraphs.Add(GraphData);
		}
	}
	
	return StructuredGraphs;
}

void UBlueprintAnalyzer::ExtractAnimBlueprintData(UBlueprint* Blueprint, FBS_BlueprintData& OutData)
{
	UAnimBlueprint* AnimBP = Cast<UAnimBlueprint>(Blueprint);
	if (!AnimBP)
	{
		return;
	}

	OutData.bIsAnimBlueprint = true;
	if (AnimBP->TargetSkeleton)
	{
		OutData.TargetSkeletonPath = AnimBP->TargetSkeleton->GetPathName();
	}

	ExtractAnimationVariables(Blueprint, OutData);
	ExtractAnimGraphs(AnimBP, OutData.AnimGraph);
	ExtractLinkedLayers(AnimBP, OutData.AnimGraph);

	ExtractAnimAssets(OutData.AssetReferences, OutData);
	ExtractControlRigData(OutData.AssetReferences, OutData);

	UpdateExtractionCoverage(OutData.AnimGraph, OutData);
}

void UBlueprintAnalyzer::ExtractAnimGraphs(UAnimBlueprint* AnimBP, FBS_AnimGraphData& OutAnimGraphData)
{
	if (!AnimBP)
	{
		return;
	}

	OutAnimGraphData.GraphName = TEXT("AnimGraph");

	TArray<UEdGraph*> AllGraphs;
	AnimBP->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		const bool bIsAnimGraphName = Graph->GetFName().ToString().Contains(TEXT("AnimGraph"));
		if (bIsAnimGraphName)
		{
			ExtractObjectProperties(Graph, OutAnimGraphData.GraphProperties);
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (!Node)
				{
					continue;
				}

				if (UAnimGraphNode_Base* AnimNode = Cast<UAnimGraphNode_Base>(Node))
				{
					FBS_AnimNodeData AnimNodeData = AnalyzeAnimNode(AnimNode);
					OutAnimGraphData.RootAnimNodes.Add(AnimNodeData);
				}

				for (UEdGraphPin* Pin : Node->Pins)
				{
					if (!Pin || Pin->Direction != EGPD_Output)
					{
						continue;
					}

					for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
					{
						if (!LinkedPin || !LinkedPin->GetOwningNode())
						{
							continue;
						}

						FBS_FlowEdge Edge;
						Edge.SourceNodeGuid = Node->NodeGuid;
						Edge.SourcePinName = Pin->PinName;
						Edge.SourcePinId = Pin->PinId;
						Edge.TargetNodeGuid = LinkedPin->GetOwningNode()->NodeGuid;
						Edge.TargetPinName = LinkedPin->PinName;
						Edge.TargetPinId = LinkedPin->PinId;
						OutAnimGraphData.RootPoseConnections.Add(Edge);
					}
				}
			}
		}

		if (UAnimationStateMachineGraph* SMGraph = Cast<UAnimationStateMachineGraph>(Graph))
		{
			ExtractStateMachine(SMGraph, OutAnimGraphData);
		}
	}
}

void UBlueprintAnalyzer::ExtractStateMachine(UAnimationStateMachineGraph* SMGraph, FBS_AnimGraphData& OutAnimGraphData)
{
	if (!SMGraph)
	{
		return;
	}

	FBS_StateMachineData SMData;
	SMData.MachineName = SMGraph->GetName();
	SMData.MachineGuid = SMGraph->GraphGuid;
	ExtractObjectProperties(SMGraph, SMData.StateMachineProperties);

	TMap<FGuid, FString> StateNamesByGuid;

	for (UEdGraphNode* NodeObject : SMGraph->Nodes)
	{
		if (UAnimStateNode* StateNode = Cast<UAnimStateNode>(NodeObject))
		{
			FBS_AnimStateData StateData;
			StateData.StateName = StateNode->GetStateName();
			StateData.StateGuid = StateNode->NodeGuid;
			StateData.StateType = TEXT("State");
			if (UEdGraphPin* OutputPin = StateNode->GetOutputPin())
			{
				StateData.bIsEndState = (OutputPin->LinkedTo.Num() == 0);
			}
			StateData.PosX = StateNode->NodePosX;
			StateData.PosY = StateNode->NodePosY;
			ExtractObjectProperties(StateNode, StateData.StateMetadata);

			if (UEdGraph* StateGraph = StateNode->GetBoundGraph())
			{
				for (UEdGraphNode* GraphNode : StateGraph->Nodes)
				{
					if (UAnimGraphNode_Base* AnimNode = Cast<UAnimGraphNode_Base>(GraphNode))
					{
						StateData.AnimGraphNodes.Add(AnalyzeAnimNode(AnimNode));
					}

					if (!GraphNode)
					{
						continue;
					}

					for (UEdGraphPin* Pin : GraphNode->Pins)
					{
						if (!Pin || Pin->Direction != EGPD_Output)
						{
							continue;
						}

						for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
						{
							if (!LinkedPin || !LinkedPin->GetOwningNode())
							{
								continue;
							}

							FBS_FlowEdge Edge;
							Edge.SourceNodeGuid = GraphNode->NodeGuid;
							Edge.SourcePinName = Pin->PinName;
							Edge.SourcePinId = Pin->PinId;
							Edge.TargetNodeGuid = LinkedPin->GetOwningNode()->NodeGuid;
							Edge.TargetPinName = LinkedPin->PinName;
							Edge.TargetPinId = LinkedPin->PinId;
							StateData.PoseConnections.Add(Edge);
						}
					}
				}
			}

			SMData.States.Add(StateData);
			StateNamesByGuid.Add(StateData.StateGuid, StateData.StateName);
		}
		else if (UAnimStateConduitNode* ConduitNode = Cast<UAnimStateConduitNode>(NodeObject))
		{
			FBS_AnimStateData StateData;
			StateData.StateName = ConduitNode->GetStateName();
			StateData.StateGuid = ConduitNode->NodeGuid;
			StateData.StateType = TEXT("Conduit");
			StateData.PosX = ConduitNode->NodePosX;
			StateData.PosY = ConduitNode->NodePosY;
			ExtractObjectProperties(ConduitNode, StateData.StateMetadata);

			SMData.States.Add(StateData);
			StateNamesByGuid.Add(StateData.StateGuid, StateData.StateName);
		}
#if UEARATAME_HAS_ANIM_STATE_ALIAS
		else if (UAnimStateAliasNode* AliasNode = Cast<UAnimStateAliasNode>(NodeObject))
		{
			FBS_AnimStateData StateData;
			StateData.StateName = AliasNode->GetStateName();
			StateData.StateGuid = AliasNode->NodeGuid;
			StateData.StateType = TEXT("Aliased");
			StateData.PosX = AliasNode->NodePosX;
			StateData.PosY = AliasNode->NodePosY;
			ExtractObjectProperties(AliasNode, StateData.StateMetadata);

			SMData.States.Add(StateData);
			StateNamesByGuid.Add(StateData.StateGuid, StateData.StateName);
		}
#endif
	}

	for (UEdGraphNode* NodeObject : SMGraph->Nodes)
	{
		if (UAnimStateEntryNode* EntryNode = Cast<UAnimStateEntryNode>(NodeObject))
		{
			UEdGraphPin* OutputPin = nullptr;
			for (UEdGraphPin* Pin : EntryNode->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output)
				{
					OutputPin = Pin;
					break;
				}
			}

			if (OutputPin && OutputPin->LinkedTo.Num() > 0)
			{
				UEdGraphNode* LinkedNode = OutputPin->LinkedTo[0]->GetOwningNode();
				if (LinkedNode)
				{
					SMData.EntryStateGuid = LinkedNode->NodeGuid;
				}
			}
			break;
		}
	}

	for (UEdGraphNode* NodeObject : SMGraph->Nodes)
	{
		if (UAnimStateTransitionNode* TransitionNode = Cast<UAnimStateTransitionNode>(NodeObject))
		{
			FBS_AnimTransitionData TransData;
			UAnimStateNodeBase* PrevState = TransitionNode->GetPreviousState();
			UAnimStateNodeBase* NextState = TransitionNode->GetNextState();
			TransData.SourceStateGuid = PrevState ? PrevState->NodeGuid : FGuid();
			TransData.TargetStateGuid = NextState ? NextState->NodeGuid : FGuid();
			TransData.TransitionGuid = TransitionNode->NodeGuid;
			TransData.CrossfadeDuration = TransitionNode->CrossfadeDuration;
			TransData.bAutomaticRuleBasedOnSequencePlayer = TransitionNode->bAutomaticRuleBasedOnSequencePlayerInState;

			if (const FString* SourceName = StateNamesByGuid.Find(TransData.SourceStateGuid))
			{
				TransData.SourceStateName = *SourceName;
			}
			if (const FString* TargetName = StateNamesByGuid.Find(TransData.TargetStateGuid))
			{
				TransData.TargetStateName = *TargetName;
			}

			ExtractObjectProperties(TransitionNode, TransData.TransitionProperties);
			if (TransitionNode->BoundGraph)
			{
				ExtractTransitionRuleGraph(TransitionNode, TransData);
			}

			SMData.Transitions.Add(TransData);
		}
	}

	OutAnimGraphData.StateMachines.Add(SMData);
}

void UBlueprintAnalyzer::ExtractTransitionRuleGraph(UAnimStateTransitionNode* TransitionNode, FBS_AnimTransitionData& OutTransData)
{
	if (!TransitionNode || !TransitionNode->BoundGraph)
	{
		return;
	}

	UEdGraph* RuleGraph = TransitionNode->BoundGraph;
	OutTransData.TransitionRuleName = RuleGraph->GetName();

	for (UEdGraphNode* Node : RuleGraph->Nodes)
	{
		FBS_NodeData NodeData = AnalyzeNodeToStruct(Node);
		OutTransData.TransitionRuleNodes.Add(NodeData);
	}

	for (UEdGraphNode* Node : RuleGraph->Nodes)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->Direction != EGPD_Output)
			{
				continue;
			}

			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!LinkedPin || !LinkedPin->GetOwningNode())
				{
					continue;
				}

				FBS_PinLinkData Link;
				Link.SourceNodeGuid = Node->NodeGuid;
				Link.SourcePinName = Pin->PinName;
				Link.SourcePinId = Pin->PinId;
				Link.TargetNodeGuid = LinkedPin->GetOwningNode()->NodeGuid;
				Link.TargetPinName = LinkedPin->PinName;
				Link.TargetPinId = LinkedPin->PinId;
				Link.PinCategory = Pin->PinType.PinCategory.ToString();
				if (UObject* SubCategoryObj = Pin->PinType.PinSubCategoryObject.Get())
				{
					Link.PinSubCategory = SubCategoryObj->GetName();
				}
				else
				{
					Link.PinSubCategory = TEXT("");
				}
				Link.bIsExec = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
				OutTransData.RulePinLinks.Add(Link);

				if (Link.bIsExec)
				{
					FBS_FlowEdge Edge;
					Edge.SourceNodeGuid = Link.SourceNodeGuid;
					Edge.SourcePinName = Link.SourcePinName;
					Edge.SourcePinId = Link.SourcePinId;
					Edge.TargetNodeGuid = Link.TargetNodeGuid;
					Edge.TargetPinName = Link.TargetPinName;
					Edge.TargetPinId = Link.TargetPinId;
					OutTransData.RuleExecutionFlow.Add(Edge);
				}
			}
		}
	}
}

FBS_AnimNodeData UBlueprintAnalyzer::AnalyzeAnimNode(UAnimGraphNode_Base* AnimNode)
{
	FBS_AnimNodeData Data;
	if (!AnimNode)
	{
		return Data;
	}

	const FBS_NodeData NodeData = AnalyzeNodeToStruct(AnimNode);
	Data.NodeGuid = NodeData.NodeGuid;
	Data.NodeType = NodeData.NodeType;
	Data.NodeTitle = NodeData.Title;
	Data.PosX = NodeData.PosX;
	Data.PosY = NodeData.PosY;
	Data.Pins = NodeData.Pins;

	if (UAnimGraphNode_BlendSpacePlayer* BlendNode = Cast<UAnimGraphNode_BlendSpacePlayer>(AnimNode))
	{
		if (UBlendSpace* BlendSpace = BlendNode->Node.GetBlendSpace())
		{
			Data.BlendSpaceAssetPath = BlendSpace->GetPathName();
			const int32 NumParams = BlendSpace->IsA<UBlendSpace1D>() ? 1 : 2;
			for (int32 Index = 0; Index < NumParams; ++Index)
			{
				const FBlendParameter& Param = BlendSpace->GetBlendParameter(Index);
				Data.BlendSpaceAxes.Add(Param.DisplayName);
			}
		}
	}
	else if (UAnimGraphNode_LayeredBoneBlend* LayeredNode = Cast<UAnimGraphNode_LayeredBoneBlend>(AnimNode))
	{
		for (const FInputBlendPose& Layer : LayeredNode->Node.LayerSetup)
		{
			for (const FBranchFilter& Filter : Layer.BranchFilters)
			{
				Data.BoneLayers.Add(FString::Printf(TEXT("%s:%d"), *Filter.BoneName.ToString(), Filter.BlendDepth));
			}
		}
	}
	else if (UAnimGraphNode_Inertialization* InertNode = Cast<UAnimGraphNode_Inertialization>(AnimNode))
	{
		// Durations are captured via property serialization on the editor node.
		(void)InertNode;
	}
	else if (UAnimGraphNode_Slot* SlotNode = Cast<UAnimGraphNode_Slot>(AnimNode))
	{
		if (FProperty* SlotNameProp = FindFProperty<FProperty>(SlotNode->GetClass(), TEXT("SlotName")))
		{
			Data.SlotName = GetPropertyValueAsString(SlotNode, SlotNameProp);
		}
		if (FProperty* GroupProp = FindFProperty<FProperty>(SlotNode->GetClass(), TEXT("GroupName")))
		{
			Data.SlotGroupName = GetPropertyValueAsString(SlotNode, GroupProp);
		}
	}
	else if (UAnimGraphNode_LinkedInputPose* LinkedNode = Cast<UAnimGraphNode_LinkedInputPose>(AnimNode))
	{
		if (FProperty* NameProp = FindFProperty<FProperty>(LinkedNode->GetClass(), TEXT("Name")))
		{
			Data.LinkedLayerName = GetPropertyValueAsString(LinkedNode, NameProp);
		}
		else if (FProperty* PoseNameProp = FindFProperty<FProperty>(LinkedNode->GetClass(), TEXT("InputPoseName")))
		{
			Data.LinkedLayerName = GetPropertyValueAsString(LinkedNode, PoseNameProp);
		}
	}
#if UEARATAME_HAS_LINKED_ANIM_LAYER
	else if (UAnimGraphNode_LinkedAnimLayer* LinkedLayerNode = Cast<UAnimGraphNode_LinkedAnimLayer>(AnimNode))
	{
		if (FProperty* LayerProp = FindFProperty<FProperty>(LinkedLayerNode->GetClass(), TEXT("Layer")))
		{
			Data.LinkedLayerName = GetPropertyValueAsString(LinkedLayerNode, LayerProp);
		}
	}
#endif

	ExtractObjectProperties(AnimNode, Data.AllProperties);
	return Data;
}

void UBlueprintAnalyzer::ExtractLinkedLayers(UAnimBlueprint* AnimBP, FBS_AnimGraphData& OutAnimGraphData)
{
	if (!AnimBP)
	{
		return;
	}

	auto GetObjectPathFromProperty = [](UObject* Object, const TCHAR* PropertyName) -> FString
	{
		if (!Object)
		{
			return FString();
		}

		if (FObjectProperty* ObjProp = FindFProperty<FObjectProperty>(Object->GetClass(), PropertyName))
		{
			if (UObject* Value = ObjProp->GetObjectPropertyValue_InContainer(Object))
			{
				return Value->GetPathName();
			}
		}
		if (FSoftObjectProperty* SoftProp = FindFProperty<FSoftObjectProperty>(Object->GetClass(), PropertyName))
		{
			const FSoftObjectPtr SoftPtr = SoftProp->GetPropertyValue_InContainer(Object);
			return SoftPtr.ToSoftObjectPath().ToString();
		}

		return FString();
	};

	TArray<UEdGraph*> AllGraphs;
	AnimBP->GetAllGraphs(AllGraphs);

	for (UEdGraph* Graph : AllGraphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			FBS_LinkedLayerData LayerData;
			bool bIsLinkedLayerNode = false;

			if (UAnimGraphNode_LinkedInputPose* LinkedNode = Cast<UAnimGraphNode_LinkedInputPose>(Node))
			{
				LayerData.InterfaceName = LinkedNode->Node.Name.ToString();
				LayerData.InterfaceAssetPath = GetObjectPathFromProperty(LinkedNode, TEXT("LinkedAnimGraphInterface"));
				bIsLinkedLayerNode = true;
			}
#if UEARATAME_HAS_LINKED_ANIM_LAYER
			else if (UAnimGraphNode_LinkedAnimLayer* LinkedLayerNode = Cast<UAnimGraphNode_LinkedAnimLayer>(Node))
			{
				LayerData.InterfaceName = LinkedLayerNode->Node.Layer.ToString();
				LayerData.InterfaceAssetPath = GetObjectPathFromProperty(LinkedLayerNode, TEXT("Interface"));
				bIsLinkedLayerNode = true;
			}
#endif

			if (bIsLinkedLayerNode)
			{
				LayerData.ImplementationClassPath = GetObjectPathFromProperty(Node, TEXT("LinkedAnimClass"));
				OutAnimGraphData.LinkedLayers.Add(LayerData);
			}
		}
	}
}

void UBlueprintAnalyzer::ExtractAnimationVariables(UBlueprint* Blueprint, FBS_BlueprintData& OutData)
{
	if (!Blueprint)
	{
		return;
	}

	for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
	{
		FBS_AnimationVariableData VarData;
		VarData.VariableName = VarDesc.VarName.ToString();
		VarData.VariableType = VarDesc.VarType.PinCategory.ToString();
		if (VarDesc.VarType.PinSubCategoryObject.IsValid())
		{
			VarData.VariableType += FString::Printf(TEXT(" (%s)"), *VarDesc.VarType.PinSubCategoryObject->GetName());
		}
		VarData.DefaultValue = VarDesc.DefaultValue;
		VarData.Category = VarDesc.Category.ToString();
		OutData.AnimationVariables.Add(VarData);
	}
}

void UBlueprintAnalyzer::UpdateExtractionCoverage(const FBS_AnimGraphData& AnimGraphData, FBS_BlueprintData& OutData)
{
	FBS_ExtractionCoverageData Coverage;
	int32 StateCount = 0;
	int32 TransitionCount = 0;

	for (const FBS_StateMachineData& Machine : AnimGraphData.StateMachines)
	{
		StateCount += Machine.States.Num();
		TransitionCount += Machine.Transitions.Num();

		for (const FBS_AnimStateData& State : Machine.States)
		{
			for (const FBS_AnimNodeData& Node : State.AnimGraphNodes)
			{
				Coverage.NodeTypeCounts.FindOrAdd(Node.NodeType)++;
			}
		}
	}

	for (const FBS_AnimNodeData& Node : AnimGraphData.RootAnimNodes)
	{
		Coverage.NodeTypeCounts.FindOrAdd(Node.NodeType)++;
	}

	Coverage.TotalStateNodes = StateCount;
	Coverage.TotalTransitions = TransitionCount;
	Coverage.TotalAnimAssets = OutData.AnimationAssets.Num();
	Coverage.TotalControlRigs = OutData.ControlRigs.Num();

	int32 NotifyCount = 0;
	int32 CurveCount = 0;
	for (const FBS_AnimAssetData& AssetData : OutData.AnimationAssets)
	{
		NotifyCount += AssetData.Notifies.Num();
		CurveCount += AssetData.Curves.Num();
	}
	Coverage.TotalNotifies = NotifyCount;
	Coverage.TotalCurves = CurveCount;

	OutData.Coverage = Coverage;
}

void UBlueprintAnalyzer::ExtractAnimAssets(const TArray<FString>& AssetPaths, FBS_BlueprintData& OutData)
{
	TSet<FString> SeenAssets;

	for (const FString& AssetPath : AssetPaths)
	{
		if (AssetPath.IsEmpty() || SeenAssets.Contains(AssetPath))
		{
			continue;
		}

		SeenAssets.Add(AssetPath);
		FSoftObjectPath SoftPath(AssetPath);
		UObject* Asset = SoftPath.TryLoad();
		if (!Asset)
		{
			continue;
		}

		FBS_AnimAssetData AssetData;
		AssetData.AssetPath = AssetPath;
		AssetData.AssetName = Asset->GetName();
		ExtractObjectProperties(Asset, AssetData.AssetProperties);

		if (UAnimMontage* Montage = Cast<UAnimMontage>(Asset))
		{
			AssetData.AssetType = TEXT("Montage");
			AssetData.Length = Montage->GetPlayLength();
			ExtractObjectProperties(Montage, AssetData.MontageProperties);
			ExtractAnimSequenceNotifies(Montage, AssetData);
			ExtractMontageSections(Montage, AssetData);
			if (USkeleton* Skeleton = Montage->GetSkeleton())
			{
				AssetData.SkeletonPath = Skeleton->GetPathName();
			}
		}
		else if (UBlendSpace* BlendSpace = Cast<UBlendSpace>(Asset))
		{
			AssetData.AssetType = TEXT("BlendSpace");
			ExtractBlendSpaceData(BlendSpace, AssetData);
			if (USkeleton* Skeleton = BlendSpace->GetSkeleton())
			{
				AssetData.SkeletonPath = Skeleton->GetPathName();
			}
		}
		else if (UAnimSequenceBase* SequenceBase = Cast<UAnimSequenceBase>(Asset))
		{
			AssetData.AssetType = TEXT("Sequence");
			AssetData.Length = SequenceBase->GetPlayLength();
			if (UAnimSequence* Sequence = Cast<UAnimSequence>(SequenceBase))
			{
				AssetData.FrameRate = Sequence->GetSamplingFrameRate().AsDecimal();
				AssetData.TotalFrames = FMath::RoundToInt(AssetData.Length * AssetData.FrameRate);
				ExtractAnimationCurves(Sequence, AssetData);
			}
			ExtractAnimSequenceNotifies(SequenceBase, AssetData);
			if (USkeleton* Skeleton = SequenceBase->GetSkeleton())
			{
				AssetData.SkeletonPath = Skeleton->GetPathName();
			}
		}
		else
		{
			continue;
		}

		OutData.AnimationAssets.Add(AssetData);
	}
}

void UBlueprintAnalyzer::ExtractAnimSequenceNotifies(UAnimSequenceBase* Sequence, FBS_AnimAssetData& OutData)
{
	if (!Sequence)
	{
		return;
	}

	float FrameRate = 30.0f;
	if (UAnimSequence* AnimSequence = Cast<UAnimSequence>(Sequence))
	{
		FrameRate = AnimSequence->GetSamplingFrameRate().AsDecimal();
	}

	for (const FAnimNotifyEvent& NotifyEvent : Sequence->Notifies)
	{
		FBS_AnimNotifyData NotifyData;
		NotifyData.NotifyName = NotifyEvent.NotifyName.ToString();
		NotifyData.TriggerTime = NotifyEvent.GetTime();
		NotifyData.TriggerFrame = FrameRate > 0.0f ? FMath::RoundToInt(NotifyData.TriggerTime * FrameRate) : 0;

		if (NotifyEvent.Notify)
		{
			NotifyData.NotifyClass = NotifyEvent.Notify->GetClass()->GetName();
			NotifyData.bIsNotifyState = NotifyEvent.Notify->IsA<UAnimNotifyState>();
			NotifyData.Duration = NotifyEvent.Duration;
			ExtractObjectProperties(NotifyEvent.Notify, NotifyData.NotifyProperties);
		}

		OutData.Notifies.Add(NotifyData);
	}
}

void UBlueprintAnalyzer::ExtractAnimationCurves(UAnimSequence* Sequence, FBS_AnimAssetData& OutData)
{
	if (!Sequence)
	{
		return;
	}

	const IAnimationDataModel* DataModel = Sequence->GetDataModel();
	if (!DataModel)
	{
		return;
	}

	for (const FFloatCurve& FloatCurve : DataModel->GetFloatCurves())
	{
		FBS_AnimCurveData CurveData;
		CurveData.CurveName = FloatCurve.GetName().ToString();
		CurveData.CurveType = TEXT("FloatCurve");

		for (const FRichCurveKey& Key : FloatCurve.FloatCurve.Keys)
		{
			CurveData.KeyTimes.Add(Key.Time);
			CurveData.KeyValues.Add(Key.Value);
		}

		OutData.Curves.Add(CurveData);
	}
}

void UBlueprintAnalyzer::ExtractMontageSections(UAnimMontage* Montage, FBS_AnimAssetData& OutData)
{
	if (!Montage)
	{
		return;
	}

	for (const FCompositeSection& Section : Montage->CompositeSections)
	{
		FBS_MontageSectionData SectionData;
		SectionData.SectionName = Section.SectionName.ToString();
		float SectionStartTime = 0.0f;
		float SectionEndTime = 0.0f;
		const int32 SectionIndex = Montage->GetSectionIndex(Section.SectionName);
		if (SectionIndex != INDEX_NONE)
		{
			Montage->GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);
		}
		SectionData.StartTime = SectionStartTime;
		SectionData.NextSectionName = Section.NextSectionName.ToString();
		if (Section.NextSectionName != NAME_None)
		{
			float NextStartTime = 0.0f;
			float NextEndTime = 0.0f;
			const int32 NextIndex = Montage->GetSectionIndex(Section.NextSectionName);
			if (NextIndex != INDEX_NONE)
			{
				Montage->GetSectionStartAndEndTime(NextIndex, NextStartTime, NextEndTime);
				SectionData.NextSectionTime = NextStartTime;
			}
		}
		OutData.Sections.Add(SectionData);
	}

	for (const FSlotAnimationTrack& SlotTrack : Montage->SlotAnimTracks)
	{
		OutData.SlotNames.Add(SlotTrack.SlotName.ToString());
	}
}

void UBlueprintAnalyzer::ExtractBlendSpaceData(UBlendSpace* BlendSpace, FBS_AnimAssetData& OutData)
{
	if (!BlendSpace)
	{
		return;
	}

	FBS_BlendSpaceData BlendData;
	BlendData.BlendSpaceName = BlendSpace->GetName();
	BlendData.BlendSpaceType = BlendSpace->IsA<UBlendSpace1D>() ? TEXT("1D") : TEXT("2D");

	const int32 NumParams = BlendSpace->IsA<UBlendSpace1D>() ? 1 : 2;
	for (int32 Index = 0; Index < NumParams; ++Index)
	{
		const FBlendParameter& Param = BlendSpace->GetBlendParameter(Index);
		BlendData.AxisNames.Add(Param.DisplayName);
		BlendData.AxisMinValues.Add(Param.Min);
		BlendData.AxisMaxValues.Add(Param.Max);
	}

	for (const FBlendSample& Sample : BlendSpace->GetBlendSamples())
	{
		FBS_BlendSpaceSamplePoint Point;
		if (Sample.Animation)
		{
			Point.AnimationName = Sample.Animation->GetName();
		}
		Point.AxisValues.Add(Sample.SampleValue.X);
		if (NumParams > 1)
		{
			Point.AxisValues.Add(Sample.SampleValue.Y);
		}
		BlendData.SamplePoints.Add(Point);
	}

	ExtractObjectProperties(BlendSpace, BlendData.BlendSpaceProperties);
	OutData.BlendSpaceData = BlendData;
}

void UBlueprintAnalyzer::ExtractControlRigData(const TArray<FString>& AssetPaths, FBS_BlueprintData& OutData)
{
#if UEARATAME_HAS_CONTROL_RIG
	TSet<FString> SeenAssets;
	for (const FString& AssetPath : AssetPaths)
	{
		if (AssetPath.IsEmpty() || SeenAssets.Contains(AssetPath))
		{
			continue;
		}

		SeenAssets.Add(AssetPath);
		FSoftObjectPath SoftPath(AssetPath);
		UObject* Asset = SoftPath.TryLoad();
		if (UControlRigBlueprint* RigBlueprint = Cast<UControlRigBlueprint>(Asset))
		{
			FBS_ControlRigData RigData;
			ExtractControlRigGraph(RigBlueprint, RigData);
			OutData.ControlRigs.Add(RigData);
		}
	}
#endif
}

void UBlueprintAnalyzer::ExtractControlRigGraph(UControlRigBlueprint* RigBlueprint, FBS_ControlRigData& OutRigData)
{
#if UEARATAME_HAS_CONTROL_RIG
	if (!RigBlueprint)
	{
		return;
	}

	OutRigData.RigName = RigBlueprint->GetName();
	if (FObjectProperty* PreviewMeshProp = FindFProperty<FObjectProperty>(RigBlueprint->GetClass(), TEXT("PreviewMesh")))
	{
		if (USkeletalMesh* PreviewMesh = Cast<USkeletalMesh>(PreviewMeshProp->GetObjectPropertyValue_InContainer(RigBlueprint)))
		{
			if (USkeleton* Skeleton = PreviewMesh->GetSkeleton())
			{
				OutRigData.SkeletonPath = Skeleton->GetPathName();
			}
		}
	}

	auto GetRigVMGraph = [RigBlueprint](const FName& GraphName) -> URigVMGraph*
	{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 4
		return RigBlueprint->GetModel(GraphName);
#else
		return RigBlueprint->GetRigVMClient().GetModel(GraphName);
#endif
	};

	URigVMGraph* SetupGraph = GetRigVMGraph(TEXT("Setup"));
	URigVMGraph* ForwardGraph = GetRigVMGraph(TEXT("ForwardSolve"));
	URigVMGraph* BackwardGraph = GetRigVMGraph(TEXT("BackwardSolve"));

	auto ExtractRigVMGraph = [&](URigVMGraph* Graph, TArray<FBS_RigVMNodeData>& OutNodes)
	{
		if (!Graph)
		{
			return;
		}

		for (URigVMNode* VMNode : Graph->GetNodes())
		{
			FBS_RigVMNodeData NodeData;
			NodeData.NodeGuid = VMNode->GetNodeGuid();
			NodeData.NodeType = VMNode->GetNodeClassName().ToString();
			NodeData.NodeTitle = VMNode->GetNodeTitle().ToString();
			NodeData.PosX = VMNode->GetPosition().X;
			NodeData.PosY = VMNode->GetPosition().Y;

			for (URigVMPin* Pin : VMNode->GetPins())
			{
				FBS_PinData PinData;
				PinData.Name = Pin->GetFName();
				PinData.Category = Pin->GetCPPType();
				switch (Pin->GetDirection())
				{
				case ERigVMPinDirection::Input: PinData.Direction = TEXT("Input"); break;
				case ERigVMPinDirection::Output: PinData.Direction = TEXT("Output"); break;
				case ERigVMPinDirection::IO: PinData.Direction = TEXT("IO"); break;
				default: PinData.Direction = TEXT("Unknown"); break;
				}
				PinData.bIsOut = Pin->GetDirection() == ERigVMPinDirection::Output;
				NodeData.Pins.Add(PinData);
			}

			ExtractObjectProperties(VMNode, NodeData.NodeProperties);
			OutNodes.Add(NodeData);
		}

		for (URigVMLink* Link : Graph->GetLinks())
		{
			if (!Link || !Link->GetSourcePin() || !Link->GetTargetPin())
			{
				continue;
			}

			FBS_PinLinkData LinkData;
			LinkData.SourceNodeGuid = Link->GetSourcePin()->GetNode()->GetNodeGuid();
			LinkData.SourcePinName = Link->GetSourcePin()->GetFName();
			LinkData.TargetNodeGuid = Link->GetTargetPin()->GetNode()->GetNodeGuid();
			LinkData.TargetPinName = Link->GetTargetPin()->GetFName();
			LinkData.PinCategory = Link->GetSourcePin()->GetCPPType();
			LinkData.PinSubCategory = TEXT("");
			LinkData.bIsExec = false;
			OutRigData.RigVMConnections.Add(LinkData);
		}
	};

	ExtractRigVMGraph(SetupGraph, OutRigData.SetupEventNodes);
	ExtractRigVMGraph(ForwardGraph, OutRigData.ForwardSolveNodes);
	ExtractRigVMGraph(BackwardGraph, OutRigData.BackwardSolveNodes);
#endif
}

// Preferred structured node analysis
FBS_NodeData UBlueprintAnalyzer::AnalyzeNodeToStruct(UEdGraphNode* Node)
{
    FBS_NodeData Out;
    if (!Node)
    {
        return Out;
    }

    Out.NodeGuid = Node->NodeGuid;
    Out.NodeType = Node->GetClass()->GetName();
    Out.Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
    Out.PosX = Node->NodePosX;
    Out.PosY = Node->NodePosY;
    Out.NodeWidth = Node->NodeWidth;
    Out.NodeHeight = Node->NodeHeight;
    Out.NodeComment = Node->NodeComment;
    Out.EnabledState = LexToString(Node->GetDesiredEnabledState());
    switch (Node->AdvancedPinDisplay)
    {
    case ENodeAdvancedPins::NoPins: Out.AdvancedPinDisplay = TEXT("NoPins"); break;
    case ENodeAdvancedPins::Shown:  Out.AdvancedPinDisplay = TEXT("Shown"); break;
    case ENodeAdvancedPins::Hidden: Out.AdvancedPinDisplay = TEXT("Hidden"); break;
    default: Out.AdvancedPinDisplay = TEXT("Unknown"); break;
    }
    Out.bHasCompilerMessage = Node->bHasCompilerMessage;
    Out.ErrorType = Node->ErrorType;
    Out.ErrorMsg = Node->ErrorMsg;

#if WITH_EDITORONLY_DATA
    Out.bCommentBubblePinned = Node->bCommentBubblePinned;
    Out.bCommentBubbleVisible = Node->bCommentBubbleVisible;
    Out.bCommentBubbleMakeVisible = Node->bCommentBubbleMakeVisible;
    Out.bCanResizeNode = Node->bCanResizeNode;
    Out.bCanRenameNode = Node->GetCanRenameNode();
#endif

    // Base property bag for node-specific metadata
    ExtractObjectProperties(Node, Out.NodeProperties);

    auto AddMeta = [&Out](const FString& Key, const FString& Value)
    {
        if (!Value.IsEmpty())
        {
            Out.NodeProperties.Add(Key, Value);
        }
    };
    auto AddMetaBool = [&Out](const FString& Key, bool Value)
    {
        Out.NodeProperties.Add(Key, Value ? TEXT("true") : TEXT("false"));
    };

    if (UK2Node* K2 = Cast<UK2Node>(Node))
    {
        if (UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(K2))
        {
            Out.MemberName = Call->FunctionReference.GetMemberName().ToString();
            if (UClass* Parent = Call->FunctionReference.GetMemberParentClass())
            {
                FString ParentName = Parent->GetName();
                ParentName.RemoveFromStart(TEXT("SKEL_"));
                ParentName.RemoveFromEnd(TEXT("_C"));
                Out.MemberParent = ParentName;
            }

            if (UFunction* TargetFunction = Call->GetTargetFunction())
            {
                AddMeta(TEXT("meta.functionName"), TargetFunction->GetName());
                if (UClass* Owner = TargetFunction->GetOwnerClass())
                {
                    AddMeta(TEXT("meta.functionOwner"), Owner->GetPathName());
                }
                AddMetaBool(TEXT("meta.isPure"), TargetFunction->HasAllFunctionFlags(FUNC_BlueprintPure));
                AddMetaBool(TEXT("meta.isConst"), TargetFunction->HasAllFunctionFlags(FUNC_Const));
                AddMetaBool(TEXT("meta.isStatic"), TargetFunction->HasAllFunctionFlags(FUNC_Static));
#if defined(FUNC_Latent)
                AddMetaBool(TEXT("meta.isLatent"), TargetFunction->HasAllFunctionFlags(FUNC_Latent));
#else
                AddMetaBool(TEXT("meta.isLatent"), TargetFunction->HasMetaData(TEXT("Latent")));
#endif
            }
        }

        if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(K2))
        {
            const FMemberReference& Ref = VarGet->VariableReference;
            AddMeta(TEXT("meta.variableName"), Ref.GetMemberName().ToString());
            if (UClass* Parent = Ref.GetMemberParentClass())
            {
                AddMeta(TEXT("meta.variableOwner"), Parent->GetPathName());
            }
            AddMetaBool(TEXT("meta.isSelfContext"), Ref.IsSelfContext());
        }

        if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(K2))
        {
            const FMemberReference& Ref = VarSet->VariableReference;
            AddMeta(TEXT("meta.variableName"), Ref.GetMemberName().ToString());
            if (UClass* Parent = Ref.GetMemberParentClass())
            {
                AddMeta(TEXT("meta.variableOwner"), Parent->GetPathName());
            }
            AddMetaBool(TEXT("meta.isSelfContext"), Ref.IsSelfContext());
        }

        if (UK2Node_VariableSetRef* VarSetRef = Cast<UK2Node_VariableSetRef>(K2))
        {
            AddMetaBool(TEXT("meta.isByRefSet"), true);
            if (UEdGraphPin* TargetPin = VarSetRef->GetTargetPin())
            {
                AddMeta(TEXT("meta.byRefPinCategory"), TargetPin->PinType.PinCategory.ToString());
                AddMeta(TEXT("meta.byRefPinSubCategory"), TargetPin->PinType.PinSubCategory.ToString());
                if (UObject* SubObj = TargetPin->PinType.PinSubCategoryObject.Get())
                {
                    AddMeta(TEXT("meta.byRefPinSubCategoryObject"), SubObj->GetPathName());
                }
            }
        }

        if (UK2Node_MacroInstance* Macro = Cast<UK2Node_MacroInstance>(K2))
        {
            if (UEdGraph* MacroGraph = Macro->GetMacroGraph())
            {
                AddMeta(TEXT("meta.macroGraphPath"), MacroGraph->GetPathName());
                AddMeta(TEXT("meta.macroGraphName"), MacroGraph->GetName());
            }
        }

        if (UK2Node_DynamicCast* DynCast = Cast<UK2Node_DynamicCast>(K2))
        {
            if (UClass* TargetClass = DynCast->TargetType)
            {
                AddMeta(TEXT("meta.castTargetClass"), TargetClass->GetPathName());
            }
        }

        if (UK2Node_Timeline* Timeline = Cast<UK2Node_Timeline>(K2))
        {
            AddMeta(TEXT("meta.timelineName"), Timeline->TimelineName.ToString());
        }

        if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(K2))
        {
            AddMeta(TEXT("meta.eventName"), EventNode->EventReference.GetMemberName().ToString());
            if (UClass* Parent = EventNode->EventReference.GetMemberParentClass())
            {
                AddMeta(TEXT("meta.eventOwner"), Parent->GetPathName());
            }
        }

        if (UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(K2))
        {
            AddMeta(TEXT("meta.customEventName"), CustomEvent->CustomFunctionName.ToString());
        }
    }

    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (!Pin) continue;
        FBS_PinData P;
        P.PinId = Pin->PinId;
        P.Name = Pin->PinName;
        P.Direction = (Pin->Direction == EGPD_Input) ? TEXT("Input") : TEXT("Output");
        P.Category = Pin->PinType.PinCategory.ToString();
        P.SubCategory = Pin->PinType.PinSubCategory.ToString();
        if (Pin->PinType.PinSubCategoryObject.IsValid())
        {
            P.ObjectType = Pin->PinType.PinSubCategoryObject->GetName();
            P.ObjectPath = Pin->PinType.PinSubCategoryObject->GetPathName();
        }
        const FSimpleMemberReference& SubMemberRef = Pin->PinType.PinSubCategoryMemberReference;
        if (SubMemberRef.MemberParent)
        {
            P.SubCategoryMemberParent = SubMemberRef.MemberParent->GetPathName();
        }
        if (SubMemberRef.MemberName != NAME_None)
        {
            P.SubCategoryMemberName = SubMemberRef.MemberName.ToString();
        }
        if (SubMemberRef.MemberGuid.IsValid())
        {
            P.SubCategoryMemberGuid = SubMemberRef.MemberGuid.ToString();
        }
        P.bIsReference = Pin->PinType.bIsReference;
#if ENGINE_MAJOR_VERSION >= 5
        P.bIsConst = Pin->PinType.bIsConst;
#endif
        P.bIsWeakPointer = Pin->PinType.bIsWeakPointer;
        P.bIsUObjectWrapper = Pin->PinType.bIsUObjectWrapper;
        switch (Pin->PinType.ContainerType)
        {
        case EPinContainerType::Array: P.bIsArray = true; break;
        case EPinContainerType::Map:   P.bIsMap = true;   break;
        case EPinContainerType::Set:   P.bIsSet = true;   break;
        default: break;
        }
        P.bIsExec = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
        // is_out flag: only for non-exec output pins (output parameters)
        P.bIsOut = (Pin->Direction == EGPD_Output) && 
                   (Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec);
        P.bConnected = (Pin->LinkedTo.Num() > 0);
        P.SourceIndex = Pin->SourceIndex;
        if (Pin->ParentPin && Pin->ParentPin->PinId.IsValid())
        {
            P.ParentPinId = Pin->ParentPin->PinId;
        }
#if WITH_EDITORONLY_DATA
        for (UEdGraphPin* SubPin : Pin->SubPins)
        {
            if (SubPin && SubPin->PinId.IsValid())
            {
                P.SubPinIds.Add(SubPin->PinId);
            }
        }
#endif

#if WITH_EDITORONLY_DATA
        P.bHidden = Pin->bHidden;
        P.bNotConnectable = Pin->bNotConnectable;
        P.bDefaultValueIsReadOnly = Pin->bDefaultValueIsReadOnly;
        P.bDefaultValueIsIgnored = Pin->bDefaultValueIsIgnored;
        P.bAdvancedView = Pin->bAdvancedView;
        P.bDisplayAsMutableRef = Pin->bDisplayAsMutableRef;
        P.bAllowFriendlyName = Pin->bAllowFriendlyName;
        P.bOrphanedPin = Pin->bOrphanedPin;
        if (Pin->PersistentGuid.IsValid())
        {
            P.PersistentGuid = Pin->PersistentGuid.ToString();
        }
        if (!Pin->PinFriendlyName.IsEmpty())
        {
            P.FriendlyName = Pin->PinFriendlyName.ToString();
        }
#endif
        P.ToolTip = Pin->PinToolTip;
        
        // Default value
        if (!Pin->DefaultValue.IsEmpty())
        {
            P.DefaultValue = Pin->DefaultValue;
            P.bIsOptional = true; // Has default value
        }
        else if (!Pin->DefaultTextValue.IsEmpty())
        {
            P.DefaultValue = Pin->DefaultTextValue.ToString();
            P.bIsOptional = true; // Has default value
        }
        else if (Pin->DefaultObject)
        {
            P.DefaultValue = Pin->DefaultObject->GetName();
            P.DefaultObjectPath = Pin->DefaultObject->GetPathName();
            P.bIsOptional = true; // Has default value
        }

        if (!Pin->AutogeneratedDefaultValue.IsEmpty())
        {
            P.AutogeneratedDefaultValue = Pin->AutogeneratedDefaultValue;
        }
        
        // Check AdvancedDisplay
#if WITH_EDITORONLY_DATA
        if (Pin->bAdvancedView)
        {
            P.bIsOptional = true;
        }
#endif

        // Map value type (for containers)
        const FEdGraphTerminalType& ValueType = Pin->PinType.PinValueType;
        if (ValueType.TerminalCategory != NAME_None)
        {
            P.ValueCategory = ValueType.TerminalCategory.ToString();
        }
        if (ValueType.TerminalSubCategory != NAME_None)
        {
            P.ValueSubCategory = ValueType.TerminalSubCategory.ToString();
        }
        if (ValueType.TerminalSubCategoryObject.IsValid())
        {
            P.ValueObjectType = ValueType.TerminalSubCategoryObject->GetName();
            P.ValueObjectPath = ValueType.TerminalSubCategoryObject->GetPathName();
        }
        P.bValueIsConst = ValueType.bTerminalIsConst;
        P.bValueIsWeakPointer = ValueType.bTerminalIsWeakPointer;
        P.bValueIsUObjectWrapper = ValueType.bTerminalIsUObjectWrapper;
        
        Out.Pins.Add(MoveTemp(P));
    }

    return Out;
}

// JSON object helper (built from structured node)
TSharedPtr<FJsonObject> UBlueprintAnalyzer::AnalyzeNodeToJsonObject(UEdGraphNode* Node)
{
    FBS_NodeData S = AnalyzeNodeToStruct(Node);
    TSharedPtr<FJsonObject> Obj = MakeShareable(new FJsonObject);
    Obj->SetStringField(TEXT("nodeGuid"), S.NodeGuid.ToString());
    Obj->SetStringField(TEXT("nodeType"), S.NodeType);
    Obj->SetStringField(TEXT("nodeTitle"), S.Title);
    Obj->SetNumberField(TEXT("posX"), S.PosX);
    Obj->SetNumberField(TEXT("posY"), S.PosY);
    Obj->SetNumberField(TEXT("width"), S.NodeWidth);
    Obj->SetNumberField(TEXT("height"), S.NodeHeight);
    if (!S.NodeComment.IsEmpty()) Obj->SetStringField(TEXT("comment"), S.NodeComment);
    if (!S.EnabledState.IsEmpty()) Obj->SetStringField(TEXT("enabledState"), S.EnabledState);
    if (!S.AdvancedPinDisplay.IsEmpty()) Obj->SetStringField(TEXT("advancedPinDisplay"), S.AdvancedPinDisplay);
    if (S.bCanRenameNode) Obj->SetBoolField(TEXT("canRename"), true);
    if (S.bCanResizeNode) Obj->SetBoolField(TEXT("canResize"), true);
    if (S.bCommentBubblePinned) Obj->SetBoolField(TEXT("commentBubblePinned"), true);
    if (S.bCommentBubbleVisible) Obj->SetBoolField(TEXT("commentBubbleVisible"), true);
    if (S.bCommentBubbleMakeVisible) Obj->SetBoolField(TEXT("commentBubbleMakeVisible"), true);
    if (S.bHasCompilerMessage) Obj->SetBoolField(TEXT("hasCompilerMessage"), true);
    if (S.ErrorType != 0) Obj->SetNumberField(TEXT("errorType"), S.ErrorType);
    if (!S.ErrorMsg.IsEmpty()) Obj->SetStringField(TEXT("errorMsg"), S.ErrorMsg);
    if (!S.MemberParent.IsEmpty()) Obj->SetStringField(TEXT("memberParent"), S.MemberParent);
    if (!S.MemberName.IsEmpty()) Obj->SetStringField(TEXT("memberName"), S.MemberName);
    if (S.NodeProperties.Num() > 0)
    {
        AddSortedStringMap(Obj, TEXT("nodeProperties"), S.NodeProperties);
    }

    TArray<TSharedPtr<FJsonValue>> PinsJson;
    for (const FBS_PinData& P : S.Pins)
    {
        TSharedPtr<FJsonObject> PJ = MakeShareable(new FJsonObject);
        PJ->SetStringField(TEXT("pinName"), P.Name.ToString());
        if (P.PinId.IsValid()) PJ->SetStringField(TEXT("pinId"), P.PinId.ToString());
        if (!P.Direction.IsEmpty()) PJ->SetStringField(TEXT("direction"), P.Direction);
        PJ->SetStringField(TEXT("pinType"), P.Category);
        PJ->SetStringField(TEXT("pinSubType"), P.SubCategory);
        if (!P.ObjectType.IsEmpty()) PJ->SetStringField(TEXT("objectType"), P.ObjectType);
        if (!P.ObjectPath.IsEmpty()) PJ->SetStringField(TEXT("objectPath"), P.ObjectPath);
        if (!P.SubCategoryMemberParent.IsEmpty()) PJ->SetStringField(TEXT("subCategoryMemberParent"), P.SubCategoryMemberParent);
        if (!P.SubCategoryMemberName.IsEmpty()) PJ->SetStringField(TEXT("subCategoryMemberName"), P.SubCategoryMemberName);
        if (!P.SubCategoryMemberGuid.IsEmpty()) PJ->SetStringField(TEXT("subCategoryMemberGuid"), P.SubCategoryMemberGuid);
        PJ->SetBoolField(TEXT("is_reference"), P.bIsReference);
        PJ->SetBoolField(TEXT("is_const"), P.bIsConst);
        if (P.bIsWeakPointer) PJ->SetBoolField(TEXT("is_weak_pointer"), true);
        if (P.bIsUObjectWrapper) PJ->SetBoolField(TEXT("is_uobject_wrapper"), true);
        PJ->SetBoolField(TEXT("is_array"), P.bIsArray);
        PJ->SetBoolField(TEXT("is_map"), P.bIsMap);
        PJ->SetBoolField(TEXT("is_set"), P.bIsSet);
        if (P.bIsExec) PJ->SetBoolField(TEXT("is_exec"), true);
        PJ->SetBoolField(TEXT("is_out"), P.bIsOut);
        PJ->SetBoolField(TEXT("is_optional"), P.bIsOptional);
        PJ->SetBoolField(TEXT("bConnected"), P.bConnected);
        if (P.SourceIndex != INDEX_NONE) PJ->SetNumberField(TEXT("sourceIndex"), P.SourceIndex);
        if (P.ParentPinId.IsValid()) PJ->SetStringField(TEXT("parentPinId"), P.ParentPinId.ToString());
        if (P.SubPinIds.Num() > 0)
        {
            TArray<TSharedPtr<FJsonValue>> SubPinArray;
            for (const FGuid& SubId : P.SubPinIds)
            {
                SubPinArray.Add(MakeShareable(new FJsonValueString(SubId.ToString())));
            }
            PJ->SetArrayField(TEXT("subPinIds"), SubPinArray);
        }
        if (P.bHidden) PJ->SetBoolField(TEXT("hidden"), true);
        if (P.bNotConnectable) PJ->SetBoolField(TEXT("not_connectable"), true);
        if (P.bDefaultValueIsReadOnly) PJ->SetBoolField(TEXT("default_value_readonly"), true);
        if (P.bDefaultValueIsIgnored) PJ->SetBoolField(TEXT("default_value_ignored"), true);
        if (P.bAdvancedView) PJ->SetBoolField(TEXT("advanced"), true);
        if (P.bDisplayAsMutableRef) PJ->SetBoolField(TEXT("display_as_mutable_ref"), true);
        if (P.bAllowFriendlyName) PJ->SetBoolField(TEXT("allow_friendly_name"), true);
        if (P.bOrphanedPin) PJ->SetBoolField(TEXT("orphaned"), true);
        if (!P.FriendlyName.IsEmpty()) PJ->SetStringField(TEXT("friendlyName"), P.FriendlyName);
        if (!P.ToolTip.IsEmpty()) PJ->SetStringField(TEXT("toolTip"), P.ToolTip);
        if (!P.DefaultValue.IsEmpty()) PJ->SetStringField(TEXT("defaultValue"), P.DefaultValue);
        if (!P.AutogeneratedDefaultValue.IsEmpty()) PJ->SetStringField(TEXT("autogeneratedDefaultValue"), P.AutogeneratedDefaultValue);
        if (!P.DefaultObjectPath.IsEmpty()) PJ->SetStringField(TEXT("defaultObjectPath"), P.DefaultObjectPath);
        if (!P.PersistentGuid.IsEmpty()) PJ->SetStringField(TEXT("persistentGuid"), P.PersistentGuid);
        if (!P.ValueCategory.IsEmpty()) PJ->SetStringField(TEXT("valueCategory"), P.ValueCategory);
        if (!P.ValueSubCategory.IsEmpty()) PJ->SetStringField(TEXT("valueSubCategory"), P.ValueSubCategory);
        if (!P.ValueObjectType.IsEmpty()) PJ->SetStringField(TEXT("valueObjectType"), P.ValueObjectType);
        if (!P.ValueObjectPath.IsEmpty()) PJ->SetStringField(TEXT("valueObjectPath"), P.ValueObjectPath);
        if (P.bValueIsConst) PJ->SetBoolField(TEXT("valueIsConst"), true);
        if (P.bValueIsWeakPointer) PJ->SetBoolField(TEXT("valueIsWeakPointer"), true);
        if (P.bValueIsUObjectWrapper) PJ->SetBoolField(TEXT("valueIsUObjectWrapper"), true);
        PinsJson.Add(MakeShareable(new FJsonValueObject(PJ)));
    }
    Obj->SetArrayField(TEXT("pins"), PinsJson);

    return Obj;
}

TArray<FString> UBlueprintAnalyzer::ExtractEventNodes(UBlueprint* Blueprint)
{
	TArray<FString> EventNodes;
	
	if (!Blueprint)
	{
		return EventNodes;
	}
	
	// Find event nodes specifically (UE5.5 compatible approach)
	TArray<UEdGraph*> AllGraphs;
	
	// Add ubergraph pages (main event graph)
	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (Graph)
		{
			AllGraphs.Add(Graph);
		}
	}
	
	// Add function graphs
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph)
		{
			AllGraphs.Add(Graph);
		}
	}
	
    for (UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (!Node) continue;
            if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
            {
                const FName EvName = EventNode->EventReference.GetMemberName();
                const FString Info = FString::Printf(TEXT("Event: %s"), *EvName.ToString());
                EventNodes.Add(Info);
            }
            else if (UK2Node_CustomEvent* CustomEventNode = Cast<UK2Node_CustomEvent>(Node))
            {
                const FString Info = FString::Printf(TEXT("CustomEvent: %s"), *CustomEventNode->CustomFunctionName.ToString());
                EventNodes.Add(Info);
            }
        }
    }
	
	return EventNodes;
}

TArray<FString> UBlueprintAnalyzer::ExtractImplementedInterfaces(UBlueprint* Blueprint)
{
	TArray<FString> Interfaces;
	
	if (!Blueprint)
	{
		return Interfaces;
	}
	
	// Extract implemented interfaces
	for (const FBPInterfaceDescription& InterfaceDesc : Blueprint->ImplementedInterfaces)
	{
		if (InterfaceDesc.Interface)
		{
			Interfaces.Add(InterfaceDesc.Interface->GetName());
		}
	}
	
	return Interfaces;
}

FString UBlueprintAnalyzer::AnalyzeNodeDetails(UK2Node* Node)
{
#if 0  // Temporarily disabled due to BlueprintGraph include issues
	if (!Node)
	{
		return FString();
	}
	
	FString NodeType = Node->GetClass()->GetName();
	FString NodeTitle = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
	
	// Analyze specific node types
	if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
	{
		if (FuncNode->FunctionReference.GetMemberName() != NAME_None)
		{
			return FString::Printf(TEXT("CallFunction: %s"), *FuncNode->FunctionReference.GetMemberName().ToString());
		}
	}
	else if (UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(Node))
	{
		return FString::Printf(TEXT("VariableGet: %s"), *VarGetNode->VariableReference.GetMemberName().ToString());
	}
	else if (UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(Node))
	{
		return FString::Printf(TEXT("VariableSet: %s"), *VarSetNode->VariableReference.GetMemberName().ToString());
	}
	
	return FString::Printf(TEXT("%s: %s"), *NodeType, *NodeTitle);
#endif
	
	// Temporary stub implementation
	return FString(TEXT("Node analysis temporarily disabled"));
}

FString UBlueprintAnalyzer::ExportBlueprintDataToJSON(const FBS_BlueprintData& Data)
{
	TSharedPtr<FJsonObject> JsonObject = BlueprintDataToJsonObject(Data);
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);
	
	return OutputString;
}

FString UBlueprintAnalyzer::ExportMultipleBlueprintsToJSON(const TArray<FBS_BlueprintData>& DataArray)
{
	TSharedPtr<FJsonObject> RootObject = MakeShareable(new FJsonObject);
	TArray<TSharedPtr<FJsonValue>> BlueprintArray;
	
	for (const FBS_BlueprintData& Data : DataArray)
	{
		TSharedPtr<FJsonObject> BlueprintObj = BlueprintDataToJsonObject(Data);
		BlueprintArray.Add(MakeShareable(new FJsonValueObject(BlueprintObj)));
	}
	
	RootObject->SetArrayField(TEXT("blueprints"), BlueprintArray);
	RootObject->SetNumberField(TEXT("totalCount"), DataArray.Num());
	RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::Now().ToString());
	
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(RootObject.ToSharedRef(), Writer);
	
	return OutputString;
}

TSharedPtr<FJsonObject> UBlueprintAnalyzer::BlueprintDataToJsonObject(const FBS_BlueprintData& Data)
{
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	
	// Version info at the top
	const FString SchemaVersion = Data.SchemaVersion.IsEmpty() ? TEXT("1.5") : Data.SchemaVersion;
	JsonObject->SetStringField(TEXT("schemaVersion"), SchemaVersion);
	JsonObject->SetStringField(TEXT("version"), SchemaVersion);
	JsonObject->SetStringField(TEXT("engine_version"), FString::Printf(TEXT("%d.%d.%d"), 
		ENGINE_MAJOR_VERSION, ENGINE_MINOR_VERSION, ENGINE_PATCH_VERSION));
	JsonObject->SetStringField(TEXT("plugin_version"), TEXT("1.0.0"));
	
	// Metadata object (inspired by NodeToCode)
	TSharedPtr<FJsonObject> Metadata = MakeShareable(new FJsonObject);
	Metadata->SetStringField(TEXT("name"), Data.BlueprintName);
	Metadata->SetStringField(TEXT("blueprint_type"), Data.bIsInterface ? TEXT("Interface") : 
		(Data.bIsMacroLibrary ? TEXT("MacroLibrary") : 
		(Data.bIsFunctionLibrary ? TEXT("FunctionLibrary") : TEXT("Blueprint"))));
	Metadata->SetStringField(TEXT("blueprint_class"), Data.ParentClassName);
	JsonObject->SetObjectField(TEXT("metadata"), Metadata);
	
	// Blueprint basic info
	JsonObject->SetStringField(TEXT("blueprintPath"), Data.BlueprintPath);
	JsonObject->SetStringField(TEXT("blueprintName"), Data.BlueprintName);
	JsonObject->SetStringField(TEXT("parentClassName"), Data.ParentClassName);
	JsonObject->SetStringField(TEXT("generatedClassName"), Data.GeneratedClassName);
	JsonObject->SetNumberField(TEXT("totalNodeCount"), Data.TotalNodeCount);
	JsonObject->SetStringField(TEXT("extractionTimestamp"), Data.ExtractionTimestamp);

	// Canonical blueprint info for schema-based consumers
	{
		TSharedPtr<FJsonObject> BlueprintInfo = MakeShareable(new FJsonObject);
		BlueprintInfo->SetStringField(TEXT("blueprintName"), Data.BlueprintName);
		BlueprintInfo->SetStringField(TEXT("blueprintPath"), Data.BlueprintPath);
		BlueprintInfo->SetBoolField(TEXT("isAnimBlueprint"), Data.bIsAnimBlueprint);
		BlueprintInfo->SetStringField(TEXT("targetSkeleton"), Data.TargetSkeletonPath);
		JsonObject->SetObjectField(TEXT("blueprintInfo"), BlueprintInfo);
	}
	
	// Convert arrays to JSON arrays
	TArray<TSharedPtr<FJsonValue>> InterfaceArray;
	for (const FString& Interface : Data.ImplementedInterfaces)
	{
		InterfaceArray.Add(MakeShareable(new FJsonValueString(Interface)));
	}
	JsonObject->SetArrayField(TEXT("implementedInterfaces"), InterfaceArray);
	
	TArray<TSharedPtr<FJsonValue>> VariableArray;
	for (const FString& Variable : Data.Variables)
	{
		VariableArray.Add(MakeShareable(new FJsonValueString(Variable)));
	}
	JsonObject->SetArrayField(TEXT("variables"), VariableArray);
	
	TArray<TSharedPtr<FJsonValue>> FunctionArray;
	for (const FString& Function : Data.Functions)
	{
		FunctionArray.Add(MakeShareable(new FJsonValueString(Function)));
	}
	JsonObject->SetArrayField(TEXT("functions"), FunctionArray);
	
	TArray<TSharedPtr<FJsonValue>> ComponentArray;
	for (const FString& Component : Data.Components)
	{
		ComponentArray.Add(MakeShareable(new FJsonValueString(Component)));
	}
	JsonObject->SetArrayField(TEXT("components"), ComponentArray);
	
	TArray<TSharedPtr<FJsonValue>> EventArray;
	for (const FString& Event : Data.EventNodes)
	{
		EventArray.Add(MakeShareable(new FJsonValueString(Event)));
	}
	JsonObject->SetArrayField(TEXT("eventNodes"), EventArray);
	
	TArray<TSharedPtr<FJsonValue>> NodeArray;
	for (const FString& Node : Data.GraphNodes)
	{
		NodeArray.Add(MakeShareable(new FJsonValueString(Node)));
	}
	JsonObject->SetArrayField(TEXT("graphNodes"), NodeArray);
	
    // Enhanced detailed information
	JsonObject->SetStringField(TEXT("blueprintDescription"), Data.BlueprintDescription);
	JsonObject->SetStringField(TEXT("blueprintCategory"), Data.BlueprintCategory);
	JsonObject->SetBoolField(TEXT("isInterface"), Data.bIsInterface);
	JsonObject->SetBoolField(TEXT("isMacroLibrary"), Data.bIsMacroLibrary);
	JsonObject->SetBoolField(TEXT("isFunctionLibrary"), Data.bIsFunctionLibrary);
	
	// Blueprint tags
	TArray<TSharedPtr<FJsonValue>> TagArray;
	for (const FString& Tag : Data.BlueprintTags)
	{
		TagArray.Add(MakeShareable(new FJsonValueString(Tag)));
	}
	JsonObject->SetArrayField(TEXT("blueprintTags"), TagArray);
	
	// Asset references
	TArray<TSharedPtr<FJsonValue>> AssetRefArray;
	for (const FString& AssetRef : Data.AssetReferences)
	{
		AssetRefArray.Add(MakeShareable(new FJsonValueString(AssetRef)));
	}
	JsonObject->SetArrayField(TEXT("assetReferences"), AssetRefArray);
	
	// Detailed variables
	TArray<TSharedPtr<FJsonValue>> DetailedVarArray;
	for (const FBS_VariableInfo& VarInfo : Data.DetailedVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShareable(new FJsonObject);
		VarObj->SetStringField(TEXT("name"), VarInfo.VariableName);
		VarObj->SetStringField(TEXT("type"), VarInfo.VariableType);
		VarObj->SetStringField(TEXT("defaultValue"), VarInfo.DefaultValue);
		VarObj->SetStringField(TEXT("category"), VarInfo.Category);
		VarObj->SetStringField(TEXT("tooltip"), VarInfo.Tooltip);
		VarObj->SetBoolField(TEXT("isPublic"), VarInfo.bIsPublic);
		VarObj->SetBoolField(TEXT("isReplicated"), VarInfo.bIsReplicated);
		VarObj->SetBoolField(TEXT("isExposedOnSpawn"), VarInfo.bIsExposedOnSpawn);
		VarObj->SetBoolField(TEXT("isEditable"), VarInfo.bIsEditable);
		VarObj->SetStringField(TEXT("replicationCondition"), VarInfo.RepCondition);
		DetailedVarArray.Add(MakeShareable(new FJsonValueObject(VarObj)));
	}
	JsonObject->SetArrayField(TEXT("detailedVariables"), DetailedVarArray);
	
	// Detailed functions
	TArray<TSharedPtr<FJsonValue>> DetailedFuncArray;
	for (const FBS_FunctionInfo& FuncInfo : Data.DetailedFunctions)
	{
		TSharedPtr<FJsonObject> FuncObj = MakeShareable(new FJsonObject);
		FuncObj->SetStringField(TEXT("name"), FuncInfo.FunctionName);
		FuncObj->SetStringField(TEXT("category"), FuncInfo.Category);
		FuncObj->SetStringField(TEXT("tooltip"), FuncInfo.Tooltip);
		FuncObj->SetBoolField(TEXT("isPure"), FuncInfo.bIsPure);
		FuncObj->SetBoolField(TEXT("isPublic"), FuncInfo.bIsPublic);
		FuncObj->SetBoolField(TEXT("isProtected"), FuncInfo.bIsProtected);
		FuncObj->SetBoolField(TEXT("isPrivate"), FuncInfo.bIsPrivate);
		FuncObj->SetBoolField(TEXT("isStatic"), FuncInfo.bIsStatic);
		FuncObj->SetBoolField(TEXT("isLatent"), FuncInfo.bIsLatent);
		FuncObj->SetBoolField(TEXT("callInEditor"), FuncInfo.bCallInEditor);
		FuncObj->SetStringField(TEXT("accessSpecifier"), FuncInfo.AccessSpecifier);
		FuncObj->SetStringField(TEXT("returnType"), FuncInfo.ReturnType);
		
		TArray<TSharedPtr<FJsonValue>> InputParamArray;
		for (const FString& Param : FuncInfo.InputParameters)
		{
			InputParamArray.Add(MakeShareable(new FJsonValueString(Param)));
		}
		FuncObj->SetArrayField(TEXT("inputParameters"), InputParamArray);
		
		TArray<TSharedPtr<FJsonValue>> OutputParamArray;
		for (const FString& Param : FuncInfo.OutputParameters)
		{
			OutputParamArray.Add(MakeShareable(new FJsonValueString(Param)));
		}
		FuncObj->SetArrayField(TEXT("outputParameters"), OutputParamArray);
		
		DetailedFuncArray.Add(MakeShareable(new FJsonValueObject(FuncObj)));
	}
	JsonObject->SetArrayField(TEXT("detailedFunctions"), DetailedFuncArray);
	
    // Detailed components
	TArray<TSharedPtr<FJsonValue>> DetailedCompArray;
	for (const FBS_ComponentInfo& CompInfo : Data.DetailedComponents)
	{
		TSharedPtr<FJsonObject> CompObj = MakeShareable(new FJsonObject);
		CompObj->SetStringField(TEXT("name"), CompInfo.ComponentName);
		CompObj->SetStringField(TEXT("class"), CompInfo.ComponentClass);
		CompObj->SetStringField(TEXT("parentComponent"), CompInfo.ParentComponent);
		CompObj->SetStringField(TEXT("transform"), CompInfo.Transform);
		
		TArray<TSharedPtr<FJsonValue>> PropArray;
		for (const FString& Prop : CompInfo.ComponentProperties)
		{
			PropArray.Add(MakeShareable(new FJsonValueString(Prop)));
		}
		CompObj->SetArrayField(TEXT("properties"), PropArray);
		
		TArray<TSharedPtr<FJsonValue>> CompAssetRefArray;
		for (const FString& AssetRef : CompInfo.AssetReferences)
		{
			CompAssetRefArray.Add(MakeShareable(new FJsonValueString(AssetRef)));
		}
		CompObj->SetArrayField(TEXT("assetReferences"), CompAssetRefArray);
		
		DetailedCompArray.Add(MakeShareable(new FJsonValueObject(CompObj)));
	}
    JsonObject->SetArrayField(TEXT("detailedComponents"), DetailedCompArray);

    // StructuredGraphs serialization (preferred new format)
    if (Data.StructuredGraphs.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> GraphsArray;
        // Coverage map
        TMap<FString, int32> NodeTypeCounts;

        for (const FBS_GraphData& GraphData : Data.StructuredGraphs)
        {
            // Build graph object from legacy + structured analyzer
            TSharedPtr<FJsonObject> G = MakeShareable(new FJsonObject);
            G->SetStringField(TEXT("name"), GraphData.GraphName);
            G->SetStringField(TEXT("graphType"), GraphData.GraphType);

            // Nodes: reparse legacy strings to objects for output consistency (fallback)
            TArray<TSharedPtr<FJsonValue>> NodesJson;
            for (const FString& NodeStr : GraphData.Nodes)
            {
                TSharedPtr<FJsonObject> NodeObj;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(NodeStr);
                if (FJsonSerializer::Deserialize(Reader, NodeObj) && NodeObj.IsValid())
                {
                    // Count node types for coverage
                    FString NodeType;
                    if (NodeObj->TryGetStringField(TEXT("nodeType"), NodeType))
                    {
                        NodeTypeCounts.FindOrAdd(NodeType)++;
                    }
                    NodesJson.Add(MakeShareable(new FJsonValueObject(NodeObj)));
                }
            }
            G->SetArrayField(TEXT("nodes"), NodesJson);

            // Execution edges: parse "NodeGuid:PinName->NodeGuid:PinName" format
            TArray<TSharedPtr<FJsonValue>> ExecJson;
            for (const FString& Flow : GraphData.ExecutionFlows)
            {
                FString Left, Right;
                if (Flow.Split(TEXT("->"), &Left, &Right))
                {
                    FString SourceGuid, SourcePin;
                    FString TargetGuid, TargetPin;
                    
                    // Parse source node:pin
                    if (Left.Split(TEXT(":"), &SourceGuid, &SourcePin))
                    {
                        // New format with pin names
                    }
                    else
                    {
                        // Legacy format without pin names
                        SourceGuid = Left;
                        SourcePin = TEXT("execute");
                    }
                    
                    // Parse target node:pin
                    if (Right.Split(TEXT(":"), &TargetGuid, &TargetPin))
                    {
                        // New format with pin names
                    }
                    else
                    {
                        // Legacy format without pin names
                        TargetGuid = Right;
                        TargetPin = TEXT("execute");
                    }
                    
                    TSharedPtr<FJsonObject> E = MakeShareable(new FJsonObject);
                    E->SetStringField(TEXT("sourceNodeGuid"), SourceGuid);
                    E->SetStringField(TEXT("sourcePinName"), SourcePin);
                    E->SetStringField(TEXT("targetNodeGuid"), TargetGuid);
                    E->SetStringField(TEXT("targetPinName"), TargetPin);
                    ExecJson.Add(MakeShareable(new FJsonValueObject(E)));
                }
            }
            G->SetArrayField(TEXT("execution"), ExecJson);

            GraphsArray.Add(MakeShareable(new FJsonValueObject(G)));
        }

        JsonObject->SetArrayField(TEXT("structuredGraphs"), GraphsArray);

        // Coverage summary
        TSharedPtr<FJsonObject> Coverage = MakeShareable(new FJsonObject);
        TSharedPtr<FJsonObject> NodeTypes = MakeShareable(new FJsonObject);
        for (const TPair<FString, int32>& P : NodeTypeCounts)
        {
            NodeTypes->SetNumberField(P.Key, P.Value);
        }
        Coverage->SetObjectField(TEXT("nodeTypes"), NodeTypes);
        Coverage->SetArrayField(TEXT("unknownNodeTypes"), TArray<TSharedPtr<FJsonValue>>{});
        JsonObject->SetObjectField(TEXT("coverage"), Coverage);
    }
    
    // NEW: Serialize StructuredGraphsExt with proper JSON objects
    if (Data.StructuredGraphsExt.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> GraphsArray;
        TMap<FString, int32> NodeTypeCounts;
        
        for (const FBS_GraphData_Ext& GraphData : Data.StructuredGraphsExt)
        {
            TSharedPtr<FJsonObject> GraphObj = MakeShareable(new FJsonObject);
            GraphObj->SetStringField(TEXT("name"), GraphData.GraphName);
            GraphObj->SetStringField(TEXT("graphType"), GraphData.GraphType);
            
            // Serialize nodes as proper JSON objects
            TArray<TSharedPtr<FJsonValue>> NodesArray;
            for (const FBS_NodeData& NodeData : GraphData.Nodes)
            {
                TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
                NodeObj->SetStringField(TEXT("nodeGuid"), NodeData.NodeGuid.ToString());
                NodeObj->SetStringField(TEXT("nodeType"), NodeData.NodeType);
                NodeObj->SetStringField(TEXT("title"), NodeData.Title);
                NodeObj->SetNumberField(TEXT("posX"), NodeData.PosX);
                NodeObj->SetNumberField(TEXT("posY"), NodeData.PosY);
                NodeObj->SetNumberField(TEXT("width"), NodeData.NodeWidth);
                NodeObj->SetNumberField(TEXT("height"), NodeData.NodeHeight);
                if (!NodeData.NodeComment.IsEmpty())
                    NodeObj->SetStringField(TEXT("comment"), NodeData.NodeComment);
                if (!NodeData.EnabledState.IsEmpty())
                    NodeObj->SetStringField(TEXT("enabledState"), NodeData.EnabledState);
                if (!NodeData.AdvancedPinDisplay.IsEmpty())
                    NodeObj->SetStringField(TEXT("advancedPinDisplay"), NodeData.AdvancedPinDisplay);
                if (NodeData.bCanRenameNode)
                    NodeObj->SetBoolField(TEXT("canRename"), true);
                if (NodeData.bCanResizeNode)
                    NodeObj->SetBoolField(TEXT("canResize"), true);
                if (NodeData.bCommentBubblePinned)
                    NodeObj->SetBoolField(TEXT("commentBubblePinned"), true);
                if (NodeData.bCommentBubbleVisible)
                    NodeObj->SetBoolField(TEXT("commentBubbleVisible"), true);
                if (NodeData.bCommentBubbleMakeVisible)
                    NodeObj->SetBoolField(TEXT("commentBubbleMakeVisible"), true);
                if (NodeData.bHasCompilerMessage)
                    NodeObj->SetBoolField(TEXT("hasCompilerMessage"), true);
                if (NodeData.ErrorType != 0)
                    NodeObj->SetNumberField(TEXT("errorType"), NodeData.ErrorType);
                if (!NodeData.ErrorMsg.IsEmpty())
                    NodeObj->SetStringField(TEXT("errorMsg"), NodeData.ErrorMsg);
                
                // Add optional fields only if present
                if (!NodeData.MemberParent.IsEmpty())
                    NodeObj->SetStringField(TEXT("memberParent"), NodeData.MemberParent);
                if (!NodeData.MemberName.IsEmpty())
                    NodeObj->SetStringField(TEXT("memberName"), NodeData.MemberName);
                
                // Serialize pins
                TArray<TSharedPtr<FJsonValue>> PinsArray;
                for (const FBS_PinData& Pin : NodeData.Pins)
                {
                    TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
                    PinObj->SetStringField(TEXT("name"), Pin.Name.ToString());
                    if (Pin.PinId.IsValid())
                        PinObj->SetStringField(TEXT("pinId"), Pin.PinId.ToString());
                    if (!Pin.Direction.IsEmpty())
                        PinObj->SetStringField(TEXT("direction"), Pin.Direction);
                    
                    // Only include type for non-exec pins (like NodeToCode)
                    if (Pin.Category != TEXT("exec"))
                    {
                        PinObj->SetStringField(TEXT("category"), Pin.Category);
                        if (!Pin.SubCategory.IsEmpty())
                            PinObj->SetStringField(TEXT("subCategory"), Pin.SubCategory);
                        if (!Pin.ObjectType.IsEmpty())
                            PinObj->SetStringField(TEXT("objectType"), Pin.ObjectType);
                        if (!Pin.ObjectPath.IsEmpty())
                            PinObj->SetStringField(TEXT("objectPath"), Pin.ObjectPath);
                    }
                    else
                    {
                        PinObj->SetStringField(TEXT("category"), TEXT("exec"));
                    }

                    if (!Pin.SubCategoryMemberParent.IsEmpty())
                        PinObj->SetStringField(TEXT("subCategoryMemberParent"), Pin.SubCategoryMemberParent);
                    if (!Pin.SubCategoryMemberName.IsEmpty())
                        PinObj->SetStringField(TEXT("subCategoryMemberName"), Pin.SubCategoryMemberName);
                    if (!Pin.SubCategoryMemberGuid.IsEmpty())
                        PinObj->SetStringField(TEXT("subCategoryMemberGuid"), Pin.SubCategoryMemberGuid);
                    
                    // Only include flags if true
                    if (Pin.bIsReference)
                        PinObj->SetBoolField(TEXT("is_reference"), true);
                    if (Pin.bIsConst)
                        PinObj->SetBoolField(TEXT("is_const"), true);
                    if (Pin.bIsWeakPointer)
                        PinObj->SetBoolField(TEXT("is_weak_pointer"), true);
                    if (Pin.bIsUObjectWrapper)
                        PinObj->SetBoolField(TEXT("is_uobject_wrapper"), true);
                    if (Pin.bIsArray)
                        PinObj->SetBoolField(TEXT("is_array"), true);
                    if (Pin.bIsMap)
                        PinObj->SetBoolField(TEXT("is_map"), true);
                    if (Pin.bIsSet)
                        PinObj->SetBoolField(TEXT("is_set"), true);
                    if (Pin.bIsExec)
                        PinObj->SetBoolField(TEXT("is_exec"), true);
                    if (Pin.bIsOut)
                        PinObj->SetBoolField(TEXT("is_out"), true);
                    if (Pin.bIsOptional)
                        PinObj->SetBoolField(TEXT("is_optional"), true);
                    if (Pin.bConnected)
                        PinObj->SetBoolField(TEXT("connected"), true);
                    if (Pin.SourceIndex != INDEX_NONE)
                        PinObj->SetNumberField(TEXT("sourceIndex"), Pin.SourceIndex);
                    if (Pin.ParentPinId.IsValid())
                        PinObj->SetStringField(TEXT("parentPinId"), Pin.ParentPinId.ToString());
                    if (Pin.SubPinIds.Num() > 0)
                    {
                        TArray<TSharedPtr<FJsonValue>> SubPinArray;
                        for (const FGuid& SubId : Pin.SubPinIds)
                        {
                            SubPinArray.Add(MakeShareable(new FJsonValueString(SubId.ToString())));
                        }
                        PinObj->SetArrayField(TEXT("subPinIds"), SubPinArray);
                    }
                    if (Pin.bHidden)
                        PinObj->SetBoolField(TEXT("hidden"), true);
                    if (Pin.bNotConnectable)
                        PinObj->SetBoolField(TEXT("not_connectable"), true);
                    if (Pin.bDefaultValueIsReadOnly)
                        PinObj->SetBoolField(TEXT("default_value_readonly"), true);
                    if (Pin.bDefaultValueIsIgnored)
                        PinObj->SetBoolField(TEXT("default_value_ignored"), true);
                    if (Pin.bAdvancedView)
                        PinObj->SetBoolField(TEXT("advanced"), true);
                    if (Pin.bDisplayAsMutableRef)
                        PinObj->SetBoolField(TEXT("display_as_mutable_ref"), true);
                    if (Pin.bAllowFriendlyName)
                        PinObj->SetBoolField(TEXT("allow_friendly_name"), true);
                    if (Pin.bOrphanedPin)
                        PinObj->SetBoolField(TEXT("orphaned"), true);
                    if (!Pin.FriendlyName.IsEmpty())
                        PinObj->SetStringField(TEXT("friendlyName"), Pin.FriendlyName);
                    if (!Pin.ToolTip.IsEmpty())
                        PinObj->SetStringField(TEXT("toolTip"), Pin.ToolTip);
                    
                    // Default value only if present
                    if (!Pin.DefaultValue.IsEmpty())
                        PinObj->SetStringField(TEXT("defaultValue"), Pin.DefaultValue);
                    if (!Pin.AutogeneratedDefaultValue.IsEmpty())
                        PinObj->SetStringField(TEXT("autogeneratedDefaultValue"), Pin.AutogeneratedDefaultValue);
                    if (!Pin.DefaultObjectPath.IsEmpty())
                        PinObj->SetStringField(TEXT("defaultObjectPath"), Pin.DefaultObjectPath);
                    if (!Pin.PersistentGuid.IsEmpty())
                        PinObj->SetStringField(TEXT("persistentGuid"), Pin.PersistentGuid);
                    if (!Pin.ValueCategory.IsEmpty())
                        PinObj->SetStringField(TEXT("valueCategory"), Pin.ValueCategory);
                    if (!Pin.ValueSubCategory.IsEmpty())
                        PinObj->SetStringField(TEXT("valueSubCategory"), Pin.ValueSubCategory);
                    if (!Pin.ValueObjectType.IsEmpty())
                        PinObj->SetStringField(TEXT("valueObjectType"), Pin.ValueObjectType);
                    if (!Pin.ValueObjectPath.IsEmpty())
                        PinObj->SetStringField(TEXT("valueObjectPath"), Pin.ValueObjectPath);
                    if (Pin.bValueIsConst)
                        PinObj->SetBoolField(TEXT("valueIsConst"), true);
                    if (Pin.bValueIsWeakPointer)
                        PinObj->SetBoolField(TEXT("valueIsWeakPointer"), true);
                    if (Pin.bValueIsUObjectWrapper)
                        PinObj->SetBoolField(TEXT("valueIsUObjectWrapper"), true);
                    
                    PinsArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
                }
                NodeObj->SetArrayField(TEXT("pins"), PinsArray);

                if (NodeData.NodeProperties.Num() > 0)
                {
                    AddSortedStringMap(NodeObj, TEXT("nodeProperties"), NodeData.NodeProperties);
                }
                
                // Track node type for coverage
                NodeTypeCounts.FindOrAdd(NodeData.NodeType)++;
                
                NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
            }
            GraphObj->SetArrayField(TEXT("nodes"), NodesArray);
            
            // Serialize execution flows
            TSharedPtr<FJsonObject> FlowsObj = MakeShareable(new FJsonObject);
            TArray<TSharedPtr<FJsonValue>> ExecutionArray;
            for (const FBS_FlowEdge& Edge : GraphData.Execution)
            {
                TSharedPtr<FJsonObject> EdgeObj = MakeShareable(new FJsonObject);
                EdgeObj->SetStringField(TEXT("sourceNodeGuid"), Edge.SourceNodeGuid.ToString());
                EdgeObj->SetStringField(TEXT("sourcePinName"), Edge.SourcePinName.ToString());
                if (Edge.SourcePinId.IsValid())
                    EdgeObj->SetStringField(TEXT("sourcePinId"), Edge.SourcePinId.ToString());
                EdgeObj->SetStringField(TEXT("targetNodeGuid"), Edge.TargetNodeGuid.ToString());
                EdgeObj->SetStringField(TEXT("targetPinName"), Edge.TargetPinName.ToString());
                if (Edge.TargetPinId.IsValid())
                    EdgeObj->SetStringField(TEXT("targetPinId"), Edge.TargetPinId.ToString());
                ExecutionArray.Add(MakeShareable(new FJsonValueObject(EdgeObj)));
            }
            FlowsObj->SetArrayField(TEXT("execution"), ExecutionArray);

            // Serialize data flows
            TArray<TSharedPtr<FJsonValue>> DataArray;
            for (const FBS_PinLinkData& Link : GraphData.DataLinks)
            {
                TSharedPtr<FJsonObject> LinkObj = MakeShareable(new FJsonObject);
                LinkObj->SetStringField(TEXT("sourceNodeGuid"), Link.SourceNodeGuid.ToString());
                LinkObj->SetStringField(TEXT("sourcePinName"), Link.SourcePinName.ToString());
                if (Link.SourcePinId.IsValid())
                    LinkObj->SetStringField(TEXT("sourcePinId"), Link.SourcePinId.ToString());
                LinkObj->SetStringField(TEXT("targetNodeGuid"), Link.TargetNodeGuid.ToString());
                LinkObj->SetStringField(TEXT("targetPinName"), Link.TargetPinName.ToString());
                if (Link.TargetPinId.IsValid())
                    LinkObj->SetStringField(TEXT("targetPinId"), Link.TargetPinId.ToString());
                LinkObj->SetStringField(TEXT("pinCategory"), Link.PinCategory);
                LinkObj->SetStringField(TEXT("pinSubCategory"), Link.PinSubCategory);
                LinkObj->SetBoolField(TEXT("isExec"), Link.bIsExec);
                DataArray.Add(MakeShareable(new FJsonValueObject(LinkObj)));
            }
            FlowsObj->SetArrayField(TEXT("data"), DataArray);
            
            GraphObj->SetObjectField(TEXT("flows"), FlowsObj);
            
            GraphsArray.Add(MakeShareable(new FJsonValueObject(GraphObj)));
        }
        
        // Override structuredGraphs with the properly formatted version
        JsonObject->SetArrayField(TEXT("structuredGraphs"), GraphsArray);
        
        // Update coverage with new counts
        TSharedPtr<FJsonObject> Coverage = MakeShareable(new FJsonObject);
        Coverage->SetNumberField(TEXT("total_nodes"), Data.TotalNodeCount);
        
        TSharedPtr<FJsonObject> NodeTypes = MakeShareable(new FJsonObject);
        for (const TPair<FString, int32>& P : NodeTypeCounts)
        {
            NodeTypes->SetNumberField(P.Key, P.Value);
        }
        Coverage->SetObjectField(TEXT("by_type"), NodeTypes);
        Coverage->SetArrayField(TEXT("unknown_types"), TArray<TSharedPtr<FJsonValue>>{});
        
        JsonObject->SetObjectField(TEXT("coverage"), Coverage);

        // Graphs summary (counts per graph)
        {
            TArray<TSharedPtr<FJsonValue>> Summaries;
            for (const FBS_GraphData_Ext& GraphData : Data.StructuredGraphsExt)
            {
                TSharedPtr<FJsonObject> Summ = MakeShareable(new FJsonObject);
                Summ->SetStringField(TEXT("name"), GraphData.GraphName);
                Summ->SetStringField(TEXT("graphType"), GraphData.GraphType);
                Summ->SetNumberField(TEXT("nodes"), GraphData.Nodes.Num());
                Summ->SetNumberField(TEXT("execEdges"), GraphData.Execution.Num());

                // Optional top node types (up to 5)
                TMap<FString, int32> TypeCounts;
                for (const FBS_NodeData& N : GraphData.Nodes)
                {
                    TypeCounts.FindOrAdd(N.NodeType)++;
                }
                TArray<TPair<FString,int32>> Sorted;
                for (const TPair<FString,int32>& P : TypeCounts)
                {
                    Sorted.Add({P.Key, P.Value});
                }
                Sorted.Sort([](const TPair<FString,int32>& A, const TPair<FString,int32>& B){ return A.Value > B.Value; });
                TArray<TSharedPtr<FJsonValue>> TopTypes;
                const int32 MaxTop = FMath::Min(5, Sorted.Num());
                for (int32 i=0; i<MaxTop; ++i)
                {
                    const TPair<FString,int32>& P = Sorted[i];
                    TSharedPtr<FJsonObject> Entry = MakeShareable(new FJsonObject);
                    Entry->SetStringField(TEXT("type"), P.Key);
                    Entry->SetNumberField(TEXT("count"), P.Value);
                    TopTypes.Add(MakeShareable(new FJsonValueObject(Entry)));
                }
                if (TopTypes.Num() > 0)
                {
                    Summ->SetArrayField(TEXT("topNodeTypes"), TopTypes);
                }

                Summaries.Add(MakeShareable(new FJsonValueObject(Summ)));
            }
            if (Summaries.Num() > 0)
            {
                JsonObject->SetArrayField(TEXT("graphsSummary"), Summaries);
            }
        }
    }

	// AnimBlueprint-specific output (schema-aligned)
	if (Data.bIsAnimBlueprint)
	{
		auto BuildPinArray = [](const TArray<FBS_PinData>& Pins)
		{
			TArray<TSharedPtr<FJsonValue>> PinArray;
			for (const FBS_PinData& Pin : Pins)
			{
				TSharedPtr<FJsonObject> PinObj = MakeShareable(new FJsonObject);
				PinObj->SetStringField(TEXT("name"), Pin.Name.ToString());
				if (Pin.PinId.IsValid())
					PinObj->SetStringField(TEXT("pinId"), Pin.PinId.ToString());
				if (!Pin.Direction.IsEmpty())
					PinObj->SetStringField(TEXT("direction"), Pin.Direction);
				PinObj->SetStringField(TEXT("category"), Pin.Category);
				PinObj->SetStringField(TEXT("subCategory"), Pin.SubCategory);
				PinObj->SetStringField(TEXT("objectType"), Pin.ObjectType);
				if (!Pin.ObjectPath.IsEmpty())
					PinObj->SetStringField(TEXT("objectPath"), Pin.ObjectPath);
				if (!Pin.SubCategoryMemberParent.IsEmpty())
					PinObj->SetStringField(TEXT("subCategoryMemberParent"), Pin.SubCategoryMemberParent);
				if (!Pin.SubCategoryMemberName.IsEmpty())
					PinObj->SetStringField(TEXT("subCategoryMemberName"), Pin.SubCategoryMemberName);
				if (!Pin.SubCategoryMemberGuid.IsEmpty())
					PinObj->SetStringField(TEXT("subCategoryMemberGuid"), Pin.SubCategoryMemberGuid);
				PinObj->SetBoolField(TEXT("isReference"), Pin.bIsReference);
				PinObj->SetBoolField(TEXT("isConst"), Pin.bIsConst);
				if (Pin.bIsWeakPointer)
					PinObj->SetBoolField(TEXT("isWeakPointer"), true);
				if (Pin.bIsUObjectWrapper)
					PinObj->SetBoolField(TEXT("isUObjectWrapper"), true);
				PinObj->SetBoolField(TEXT("isArray"), Pin.bIsArray);
				PinObj->SetBoolField(TEXT("isMap"), Pin.bIsMap);
				PinObj->SetBoolField(TEXT("isSet"), Pin.bIsSet);
				if (Pin.bIsExec)
					PinObj->SetBoolField(TEXT("isExec"), true);
				PinObj->SetBoolField(TEXT("connected"), Pin.bConnected);
				if (!Pin.DefaultValue.IsEmpty())
					PinObj->SetStringField(TEXT("defaultValue"), Pin.DefaultValue);
				if (!Pin.AutogeneratedDefaultValue.IsEmpty())
					PinObj->SetStringField(TEXT("autogeneratedDefaultValue"), Pin.AutogeneratedDefaultValue);
				if (!Pin.DefaultObjectPath.IsEmpty())
					PinObj->SetStringField(TEXT("defaultObjectPath"), Pin.DefaultObjectPath);
				if (!Pin.PersistentGuid.IsEmpty())
					PinObj->SetStringField(TEXT("persistentGuid"), Pin.PersistentGuid);
				PinObj->SetBoolField(TEXT("isOut"), Pin.bIsOut);
				PinObj->SetBoolField(TEXT("isOptional"), Pin.bIsOptional);
				if (Pin.SourceIndex != INDEX_NONE)
					PinObj->SetNumberField(TEXT("sourceIndex"), Pin.SourceIndex);
				if (Pin.ParentPinId.IsValid())
					PinObj->SetStringField(TEXT("parentPinId"), Pin.ParentPinId.ToString());
				if (Pin.SubPinIds.Num() > 0)
				{
					TArray<TSharedPtr<FJsonValue>> SubPinArray;
					for (const FGuid& SubId : Pin.SubPinIds)
					{
						SubPinArray.Add(MakeShareable(new FJsonValueString(SubId.ToString())));
					}
					PinObj->SetArrayField(TEXT("subPinIds"), SubPinArray);
				}
				if (Pin.bHidden)
					PinObj->SetBoolField(TEXT("hidden"), true);
				if (Pin.bNotConnectable)
					PinObj->SetBoolField(TEXT("notConnectable"), true);
				if (Pin.bDefaultValueIsReadOnly)
					PinObj->SetBoolField(TEXT("defaultValueReadOnly"), true);
				if (Pin.bDefaultValueIsIgnored)
					PinObj->SetBoolField(TEXT("defaultValueIgnored"), true);
				if (Pin.bAdvancedView)
					PinObj->SetBoolField(TEXT("advanced"), true);
				if (Pin.bDisplayAsMutableRef)
					PinObj->SetBoolField(TEXT("displayAsMutableRef"), true);
				if (Pin.bAllowFriendlyName)
					PinObj->SetBoolField(TEXT("allowFriendlyName"), true);
				if (Pin.bOrphanedPin)
					PinObj->SetBoolField(TEXT("orphaned"), true);
				if (!Pin.FriendlyName.IsEmpty())
					PinObj->SetStringField(TEXT("friendlyName"), Pin.FriendlyName);
				if (!Pin.ToolTip.IsEmpty())
					PinObj->SetStringField(TEXT("toolTip"), Pin.ToolTip);
				if (!Pin.ValueCategory.IsEmpty())
					PinObj->SetStringField(TEXT("valueCategory"), Pin.ValueCategory);
				if (!Pin.ValueSubCategory.IsEmpty())
					PinObj->SetStringField(TEXT("valueSubCategory"), Pin.ValueSubCategory);
				if (!Pin.ValueObjectType.IsEmpty())
					PinObj->SetStringField(TEXT("valueObjectType"), Pin.ValueObjectType);
				if (!Pin.ValueObjectPath.IsEmpty())
					PinObj->SetStringField(TEXT("valueObjectPath"), Pin.ValueObjectPath);
				if (Pin.bValueIsConst)
					PinObj->SetBoolField(TEXT("valueIsConst"), true);
				if (Pin.bValueIsWeakPointer)
					PinObj->SetBoolField(TEXT("valueIsWeakPointer"), true);
				if (Pin.bValueIsUObjectWrapper)
					PinObj->SetBoolField(TEXT("valueIsUObjectWrapper"), true);
				PinArray.Add(MakeShareable(new FJsonValueObject(PinObj)));
			}
			return PinArray;
		};

		auto BuildFlowArray = [](const TArray<FBS_FlowEdge>& Edges)
		{
			TArray<TSharedPtr<FJsonValue>> FlowArray;
			for (const FBS_FlowEdge& Edge : Edges)
			{
				TSharedPtr<FJsonObject> EdgeObj = MakeShareable(new FJsonObject);
				EdgeObj->SetStringField(TEXT("sourceNodeGuid"), Edge.SourceNodeGuid.ToString());
				EdgeObj->SetStringField(TEXT("sourcePinName"), Edge.SourcePinName.ToString());
				if (Edge.SourcePinId.IsValid())
					EdgeObj->SetStringField(TEXT("sourcePinId"), Edge.SourcePinId.ToString());
				EdgeObj->SetStringField(TEXT("targetNodeGuid"), Edge.TargetNodeGuid.ToString());
				EdgeObj->SetStringField(TEXT("targetPinName"), Edge.TargetPinName.ToString());
				if (Edge.TargetPinId.IsValid())
					EdgeObj->SetStringField(TEXT("targetPinId"), Edge.TargetPinId.ToString());
				FlowArray.Add(MakeShareable(new FJsonValueObject(EdgeObj)));
			}
			return FlowArray;
		};

		auto BuildPinLinksArray = [](const TArray<FBS_PinLinkData>& Links)
		{
			TArray<TSharedPtr<FJsonValue>> LinkArray;
			for (const FBS_PinLinkData& Link : Links)
			{
				TSharedPtr<FJsonObject> LinkObj = MakeShareable(new FJsonObject);
				LinkObj->SetStringField(TEXT("sourceNodeGuid"), Link.SourceNodeGuid.ToString());
				LinkObj->SetStringField(TEXT("sourcePinName"), Link.SourcePinName.ToString());
				if (Link.SourcePinId.IsValid())
					LinkObj->SetStringField(TEXT("sourcePinId"), Link.SourcePinId.ToString());
				LinkObj->SetStringField(TEXT("targetNodeGuid"), Link.TargetNodeGuid.ToString());
				LinkObj->SetStringField(TEXT("targetPinName"), Link.TargetPinName.ToString());
				if (Link.TargetPinId.IsValid())
					LinkObj->SetStringField(TEXT("targetPinId"), Link.TargetPinId.ToString());
				LinkObj->SetStringField(TEXT("pinCategory"), Link.PinCategory);
				LinkObj->SetStringField(TEXT("pinSubCategory"), Link.PinSubCategory);
				LinkObj->SetBoolField(TEXT("isExec"), Link.bIsExec);
				LinkArray.Add(MakeShareable(new FJsonValueObject(LinkObj)));
			}
			return LinkArray;
		};

		// Animation variables
		TArray<TSharedPtr<FJsonValue>> AnimVars;
		for (const FBS_AnimationVariableData& Var : Data.AnimationVariables)
		{
			TSharedPtr<FJsonObject> VarObj = MakeShareable(new FJsonObject);
			VarObj->SetStringField(TEXT("variableName"), Var.VariableName);
			VarObj->SetStringField(TEXT("variableType"), Var.VariableType);
			VarObj->SetStringField(TEXT("defaultValue"), Var.DefaultValue);
			VarObj->SetStringField(TEXT("category"), Var.Category);
			AnimVars.Add(MakeShareable(new FJsonValueObject(VarObj)));
		}
		JsonObject->SetArrayField(TEXT("animationVariables"), AnimVars);

		// AnimGraph
		TSharedPtr<FJsonObject> AnimGraphObj = MakeShareable(new FJsonObject);
		AnimGraphObj->SetStringField(TEXT("graphName"), Data.AnimGraph.GraphName);
		AnimGraphObj->SetBoolField(TEXT("isFastPathCompliant"), Data.AnimGraph.bIsFastPathCompliant);

		TArray<TSharedPtr<FJsonValue>> RootNodes;
		for (const FBS_AnimNodeData& Node : Data.AnimGraph.RootAnimNodes)
		{
			TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
			NodeObj->SetStringField(TEXT("nodeGuid"), Node.NodeGuid.ToString());
			NodeObj->SetStringField(TEXT("nodeType"), Node.NodeType);
			NodeObj->SetStringField(TEXT("nodeTitle"), Node.NodeTitle);
			NodeObj->SetNumberField(TEXT("posX"), Node.PosX);
			NodeObj->SetNumberField(TEXT("posY"), Node.PosY);
			NodeObj->SetStringField(TEXT("blendSpaceAssetPath"), Node.BlendSpaceAssetPath);
			NodeObj->SetArrayField(TEXT("blendSpaceAxes"), BuildStringArray(Node.BlendSpaceAxes));
			NodeObj->SetArrayField(TEXT("boneLayers"), BuildStringArray(Node.BoneLayers));
			NodeObj->SetStringField(TEXT("blendMaskAssetPath"), Node.BlendMaskAssetPath);
			NodeObj->SetStringField(TEXT("slotName"), Node.SlotName);
			NodeObj->SetStringField(TEXT("slotGroupName"), Node.SlotGroupName);
			NodeObj->SetStringField(TEXT("linkedLayerName"), Node.LinkedLayerName);
			NodeObj->SetStringField(TEXT("linkedLayerAssetPath"), Node.LinkedLayerAssetPath);
			NodeObj->SetNumberField(TEXT("inertializationDuration"), Node.InertializationDuration);
			NodeObj->SetNumberField(TEXT("blendOutDuration"), Node.BlendOutDuration);
			NodeObj->SetBoolField(TEXT("isStateMachine"), Node.bIsStateMachine);
			NodeObj->SetStringField(TEXT("stateMachineGroupName"), Node.StateMachineGroupName);
			NodeObj->SetArrayField(TEXT("pins"), BuildPinArray(Node.Pins));
			AddSortedStringMap(NodeObj, TEXT("allProperties"), Node.AllProperties);
			RootNodes.Add(MakeShareable(new FJsonValueObject(NodeObj)));
		}
		AnimGraphObj->SetArrayField(TEXT("rootNodes"), RootNodes);

		if (Data.AnimGraph.RootPoseConnections.Num() > 0)
		{
			AnimGraphObj->SetArrayField(TEXT("rootPoseConnections"), BuildFlowArray(Data.AnimGraph.RootPoseConnections));
		}

		TArray<TSharedPtr<FJsonValue>> StateMachines;
		for (const FBS_StateMachineData& Machine : Data.AnimGraph.StateMachines)
		{
			TSharedPtr<FJsonObject> MachineObj = MakeShareable(new FJsonObject);
			MachineObj->SetStringField(TEXT("machineName"), Machine.MachineName);
			MachineObj->SetStringField(TEXT("machineGuid"), Machine.MachineGuid.ToString());
			MachineObj->SetStringField(TEXT("entryStateGuid"), Machine.EntryStateGuid.ToString());
			MachineObj->SetBoolField(TEXT("isSubStateMachine"), Machine.bIsSubStateMachine);
			MachineObj->SetStringField(TEXT("parentStateName"), Machine.ParentStateName);
			AddSortedStringMap(MachineObj, TEXT("stateMachineProperties"), Machine.StateMachineProperties);

			TArray<TSharedPtr<FJsonValue>> StatesArray;
			for (const FBS_AnimStateData& State : Machine.States)
			{
				TSharedPtr<FJsonObject> StateObj = MakeShareable(new FJsonObject);
				StateObj->SetStringField(TEXT("stateName"), State.StateName);
				StateObj->SetStringField(TEXT("stateGuid"), State.StateGuid.ToString());
				StateObj->SetStringField(TEXT("stateType"), State.StateType);
				StateObj->SetBoolField(TEXT("isEndState"), State.bIsEndState);
				StateObj->SetNumberField(TEXT("posX"), State.PosX);
				StateObj->SetNumberField(TEXT("posY"), State.PosY);
				AddSortedStringMap(StateObj, TEXT("stateMetadata"), State.StateMetadata);

				TArray<TSharedPtr<FJsonValue>> StateNodesArray;
				for (const FBS_AnimNodeData& Node : State.AnimGraphNodes)
				{
					TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
					NodeObj->SetStringField(TEXT("nodeGuid"), Node.NodeGuid.ToString());
					NodeObj->SetStringField(TEXT("nodeType"), Node.NodeType);
					NodeObj->SetStringField(TEXT("nodeTitle"), Node.NodeTitle);
					NodeObj->SetNumberField(TEXT("posX"), Node.PosX);
					NodeObj->SetNumberField(TEXT("posY"), Node.PosY);
					NodeObj->SetStringField(TEXT("blendSpaceAssetPath"), Node.BlendSpaceAssetPath);
					NodeObj->SetArrayField(TEXT("blendSpaceAxes"), BuildStringArray(Node.BlendSpaceAxes));
					NodeObj->SetArrayField(TEXT("boneLayers"), BuildStringArray(Node.BoneLayers));
					NodeObj->SetStringField(TEXT("blendMaskAssetPath"), Node.BlendMaskAssetPath);
					NodeObj->SetStringField(TEXT("slotName"), Node.SlotName);
					NodeObj->SetStringField(TEXT("slotGroupName"), Node.SlotGroupName);
					NodeObj->SetStringField(TEXT("linkedLayerName"), Node.LinkedLayerName);
					NodeObj->SetStringField(TEXT("linkedLayerAssetPath"), Node.LinkedLayerAssetPath);
					NodeObj->SetNumberField(TEXT("inertializationDuration"), Node.InertializationDuration);
					NodeObj->SetNumberField(TEXT("blendOutDuration"), Node.BlendOutDuration);
					NodeObj->SetBoolField(TEXT("isStateMachine"), Node.bIsStateMachine);
					NodeObj->SetStringField(TEXT("stateMachineGroupName"), Node.StateMachineGroupName);
					NodeObj->SetArrayField(TEXT("pins"), BuildPinArray(Node.Pins));
					AddSortedStringMap(NodeObj, TEXT("allProperties"), Node.AllProperties);
					StateNodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
				}
				StateObj->SetArrayField(TEXT("animNodes"), StateNodesArray);

				if (State.PoseConnections.Num() > 0)
				{
					StateObj->SetArrayField(TEXT("poseConnections"), BuildFlowArray(State.PoseConnections));
				}

				StatesArray.Add(MakeShareable(new FJsonValueObject(StateObj)));
			}
			MachineObj->SetArrayField(TEXT("states"), StatesArray);

			TArray<TSharedPtr<FJsonValue>> TransitionsArray;
			for (const FBS_AnimTransitionData& Transition : Machine.Transitions)
			{
				TSharedPtr<FJsonObject> TransitionObj = MakeShareable(new FJsonObject);
				TransitionObj->SetStringField(TEXT("transitionGuid"), Transition.TransitionGuid.ToString());
				TransitionObj->SetStringField(TEXT("sourceStateGuid"), Transition.SourceStateGuid.ToString());
				TransitionObj->SetStringField(TEXT("targetStateGuid"), Transition.TargetStateGuid.ToString());
				TransitionObj->SetStringField(TEXT("sourceStateName"), Transition.SourceStateName);
				TransitionObj->SetStringField(TEXT("targetStateName"), Transition.TargetStateName);
				TransitionObj->SetStringField(TEXT("transitionRuleName"), Transition.TransitionRuleName);
				TransitionObj->SetNumberField(TEXT("crossfadeDuration"), Transition.CrossfadeDuration);
				TransitionObj->SetStringField(TEXT("blendMode"), Transition.BlendMode);
				TransitionObj->SetBoolField(TEXT("interruptible"), Transition.bInterruptible);
				TransitionObj->SetNumberField(TEXT("blendOutTriggerTime"), Transition.BlendOutTriggerTime);
				TransitionObj->SetBoolField(TEXT("automaticRuleBasedOnSequencePlayer"), Transition.bAutomaticRuleBasedOnSequencePlayer);
				TransitionObj->SetStringField(TEXT("notifyStateIndex"), Transition.NotifyStateIndex);
				TransitionObj->SetNumberField(TEXT("priority"), Transition.Priority);
				AddSortedStringMap(TransitionObj, TEXT("transitionProperties"), Transition.TransitionProperties);

				TArray<TSharedPtr<FJsonValue>> RuleNodesArray;
				for (const FBS_NodeData& Node : Transition.TransitionRuleNodes)
				{
					TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
					NodeObj->SetStringField(TEXT("nodeGuid"), Node.NodeGuid.ToString());
					NodeObj->SetStringField(TEXT("nodeType"), Node.NodeType);
					NodeObj->SetStringField(TEXT("title"), Node.Title);
					NodeObj->SetNumberField(TEXT("posX"), Node.PosX);
					NodeObj->SetNumberField(TEXT("posY"), Node.PosY);
					NodeObj->SetArrayField(TEXT("pins"), BuildPinArray(Node.Pins));
					if (!Node.MemberName.IsEmpty())
					{
						TSharedPtr<FJsonObject> MemberObj = MakeShareable(new FJsonObject);
						MemberObj->SetStringField(TEXT("functionName"), Node.MemberName);
						MemberObj->SetStringField(TEXT("parentClass"), Node.MemberParent);
						NodeObj->SetObjectField(TEXT("memberInfo"), MemberObj);
					}
					RuleNodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
				}
				TransitionObj->SetArrayField(TEXT("transitionRuleNodes"), RuleNodesArray);
				TransitionObj->SetArrayField(TEXT("ruleExecutionFlow"), BuildFlowArray(Transition.RuleExecutionFlow));
				TransitionObj->SetArrayField(TEXT("rulePinLinks"), BuildPinLinksArray(Transition.RulePinLinks));

				TransitionsArray.Add(MakeShareable(new FJsonValueObject(TransitionObj)));
			}
			MachineObj->SetArrayField(TEXT("transitions"), TransitionsArray);

			StateMachines.Add(MakeShareable(new FJsonValueObject(MachineObj)));
		}
		AnimGraphObj->SetArrayField(TEXT("stateMachines"), StateMachines);

		TArray<TSharedPtr<FJsonValue>> LinkedLayersArray;
		for (const FBS_LinkedLayerData& Layer : Data.AnimGraph.LinkedLayers)
		{
			TSharedPtr<FJsonObject> LayerObj = MakeShareable(new FJsonObject);
			LayerObj->SetStringField(TEXT("interfaceName"), Layer.InterfaceName);
			LayerObj->SetStringField(TEXT("interfaceAssetPath"), Layer.InterfaceAssetPath);
			LayerObj->SetStringField(TEXT("implementationClassPath"), Layer.ImplementationClassPath);
			LinkedLayersArray.Add(MakeShareable(new FJsonValueObject(LayerObj)));
		}
		AnimGraphObj->SetArrayField(TEXT("linkedLayers"), LinkedLayersArray);
		AddSortedStringMap(AnimGraphObj, TEXT("graphProperties"), Data.AnimGraph.GraphProperties);

		JsonObject->SetObjectField(TEXT("animGraph"), AnimGraphObj);

		// Animation assets
		TArray<TSharedPtr<FJsonValue>> AnimAssets;
		for (const FBS_AnimAssetData& AssetData : Data.AnimationAssets)
		{
			TSharedPtr<FJsonObject> AssetObj = MakeShareable(new FJsonObject);
			AssetObj->SetStringField(TEXT("assetName"), AssetData.AssetName);
			AssetObj->SetStringField(TEXT("assetPath"), AssetData.AssetPath);
			AssetObj->SetStringField(TEXT("assetType"), AssetData.AssetType);
			AssetObj->SetStringField(TEXT("skeletonPath"), AssetData.SkeletonPath);
			AssetObj->SetNumberField(TEXT("length"), AssetData.Length);
			AssetObj->SetNumberField(TEXT("frameRate"), AssetData.FrameRate);
			AssetObj->SetNumberField(TEXT("totalFrames"), AssetData.TotalFrames);

			TArray<TSharedPtr<FJsonValue>> NotifiesArray;
			for (const FBS_AnimNotifyData& Notify : AssetData.Notifies)
			{
				TSharedPtr<FJsonObject> NotifyObj = MakeShareable(new FJsonObject);
				NotifyObj->SetStringField(TEXT("notifyName"), Notify.NotifyName);
				NotifyObj->SetStringField(TEXT("notifyClass"), Notify.NotifyClass);
				NotifyObj->SetBoolField(TEXT("isNotifyState"), Notify.bIsNotifyState);
				NotifyObj->SetNumberField(TEXT("triggerTime"), Notify.TriggerTime);
				NotifyObj->SetNumberField(TEXT("triggerFrame"), Notify.TriggerFrame);
				NotifyObj->SetNumberField(TEXT("duration"), Notify.Duration);
				NotifyObj->SetStringField(TEXT("trackName"), Notify.TrackName);
				NotifyObj->SetNumberField(TEXT("trackIndex"), Notify.TrackIndex);
				AddSortedStringMap(NotifyObj, TEXT("notifyProperties"), Notify.NotifyProperties);
				NotifyObj->SetStringField(TEXT("triggeredEventName"), Notify.TriggeredEventName);
				NotifyObj->SetArrayField(TEXT("eventParameters"), BuildStringArray(Notify.EventParameters));
				NotifiesArray.Add(MakeShareable(new FJsonValueObject(NotifyObj)));
			}
			AssetObj->SetArrayField(TEXT("notifies"), NotifiesArray);

			TArray<TSharedPtr<FJsonValue>> CurvesArray;
			for (const FBS_AnimCurveData& Curve : AssetData.Curves)
			{
				TSharedPtr<FJsonObject> CurveObj = MakeShareable(new FJsonObject);
				CurveObj->SetStringField(TEXT("curveName"), Curve.CurveName);
				CurveObj->SetStringField(TEXT("curveType"), Curve.CurveType);
				CurveObj->SetStringField(TEXT("curveAssetPath"), Curve.CurveAssetPath);
				CurveObj->SetArrayField(TEXT("keyTimes"), BuildFloatArray(Curve.KeyTimes));
				CurveObj->SetArrayField(TEXT("keyValues"), BuildFloatArray(Curve.KeyValues));
				CurveObj->SetStringField(TEXT("interpolationMode"), Curve.InterpolationMode);
				CurveObj->SetStringField(TEXT("axisType"), Curve.AxisType);
				CurvesArray.Add(MakeShareable(new FJsonValueObject(CurveObj)));
			}
			AssetObj->SetArrayField(TEXT("curves"), CurvesArray);

			TArray<TSharedPtr<FJsonValue>> SectionsArray;
			for (const FBS_MontageSectionData& Section : AssetData.Sections)
			{
				TSharedPtr<FJsonObject> SectionObj = MakeShareable(new FJsonObject);
				SectionObj->SetStringField(TEXT("sectionName"), Section.SectionName);
				SectionObj->SetNumberField(TEXT("startTime"), Section.StartTime);
				SectionObj->SetNumberField(TEXT("startFrame"), Section.StartFrame);
				SectionObj->SetStringField(TEXT("nextSectionName"), Section.NextSectionName);
				SectionObj->SetNumberField(TEXT("nextSectionTime"), Section.NextSectionTime);
				SectionsArray.Add(MakeShareable(new FJsonValueObject(SectionObj)));
			}
			AssetObj->SetArrayField(TEXT("sections"), SectionsArray);
			AssetObj->SetArrayField(TEXT("slotNames"), BuildStringArray(AssetData.SlotNames));

			TSharedPtr<FJsonObject> BlendObj = MakeShareable(new FJsonObject);
			BlendObj->SetStringField(TEXT("blendSpaceName"), AssetData.BlendSpaceData.BlendSpaceName);
			BlendObj->SetStringField(TEXT("blendSpaceType"), AssetData.BlendSpaceData.BlendSpaceType);
			BlendObj->SetArrayField(TEXT("axisNames"), BuildStringArray(AssetData.BlendSpaceData.AxisNames));
			BlendObj->SetArrayField(TEXT("axisMinValues"), BuildFloatArray(AssetData.BlendSpaceData.AxisMinValues));
			BlendObj->SetArrayField(TEXT("axisMaxValues"), BuildFloatArray(AssetData.BlendSpaceData.AxisMaxValues));

			TArray<TSharedPtr<FJsonValue>> SamplePointsArray;
			for (const FBS_BlendSpaceSamplePoint& Point : AssetData.BlendSpaceData.SamplePoints)
			{
				TSharedPtr<FJsonObject> PointObj = MakeShareable(new FJsonObject);
				PointObj->SetStringField(TEXT("animationName"), Point.AnimationName);
				PointObj->SetArrayField(TEXT("axisValues"), BuildFloatArray(Point.AxisValues));
				SamplePointsArray.Add(MakeShareable(new FJsonValueObject(PointObj)));
			}
			BlendObj->SetArrayField(TEXT("samplePoints"), SamplePointsArray);
			AddSortedStringMap(BlendObj, TEXT("blendSpaceProperties"), AssetData.BlendSpaceData.BlendSpaceProperties);
			AssetObj->SetObjectField(TEXT("blendSpaceData"), BlendObj);

			AddSortedStringMap(AssetObj, TEXT("assetProperties"), AssetData.AssetProperties);
			AddSortedStringMap(AssetObj, TEXT("montageProperties"), AssetData.MontageProperties);

			AnimAssets.Add(MakeShareable(new FJsonValueObject(AssetObj)));
		}
		JsonObject->SetArrayField(TEXT("animationAssets"), AnimAssets);

		// Control rigs
		TArray<TSharedPtr<FJsonValue>> RigArray;
		for (const FBS_ControlRigData& Rig : Data.ControlRigs)
		{
			TSharedPtr<FJsonObject> RigObj = MakeShareable(new FJsonObject);
			RigObj->SetStringField(TEXT("rigName"), Rig.RigName);
			RigObj->SetStringField(TEXT("skeletonPath"), Rig.SkeletonPath);
			RigObj->SetArrayField(TEXT("controls"), BuildStringArray(Rig.ControlNames));
			RigObj->SetArrayField(TEXT("bones"), BuildStringArray(Rig.BoneNames));
			AddSortedStringMap(RigObj, TEXT("controlToBoneMap"), Rig.ControlToBoneMap);

			auto BuildRigNodes = [&](const TArray<FBS_RigVMNodeData>& Nodes)
			{
				TArray<TSharedPtr<FJsonValue>> NodesArray;
				for (const FBS_RigVMNodeData& Node : Nodes)
				{
					TSharedPtr<FJsonObject> NodeObj = MakeShareable(new FJsonObject);
					NodeObj->SetStringField(TEXT("nodeGuid"), Node.NodeGuid.ToString());
					NodeObj->SetStringField(TEXT("nodeType"), Node.NodeType);
					NodeObj->SetStringField(TEXT("nodeTitle"), Node.NodeTitle);
					NodeObj->SetNumberField(TEXT("posX"), Node.PosX);
					NodeObj->SetNumberField(TEXT("posY"), Node.PosY);
					NodeObj->SetArrayField(TEXT("pins"), BuildPinArray(Node.Pins));
					AddSortedStringMap(NodeObj, TEXT("nodeProperties"), Node.NodeProperties);
					NodesArray.Add(MakeShareable(new FJsonValueObject(NodeObj)));
				}
				return NodesArray;
			};

			RigObj->SetArrayField(TEXT("setupEventNodes"), BuildRigNodes(Rig.SetupEventNodes));
			RigObj->SetArrayField(TEXT("forwardSolveNodes"), BuildRigNodes(Rig.ForwardSolveNodes));
			RigObj->SetArrayField(TEXT("backwardSolveNodes"), BuildRigNodes(Rig.BackwardSolveNodes));
			RigObj->SetArrayField(TEXT("rigVmConnections"), BuildPinLinksArray(Rig.RigVMConnections));
			RigObj->SetArrayField(TEXT("enabledFeatures"), BuildStringArray(Rig.EnabledFeatures));
			AddSortedStringMap(RigObj, TEXT("featureSettings"), Rig.FeatureSettings);
			AddSortedStringMap(RigObj, TEXT("rigProperties"), Rig.RigProperties);
			RigArray.Add(MakeShareable(new FJsonValueObject(RigObj)));
		}
		JsonObject->SetArrayField(TEXT("controlRigs"), RigArray);

		// Gameplay tags
		TArray<TSharedPtr<FJsonValue>> GameplayTagArray;
		for (const FBS_GameplayTagStateData& Tag : Data.GameplayTags)
		{
			TSharedPtr<FJsonObject> TagObj = MakeShareable(new FJsonObject);
			TagObj->SetStringField(TEXT("tagName"), Tag.TagName);
			TagObj->SetStringField(TEXT("tagCategory"), Tag.TagCategory);
			TagObj->SetArrayField(TEXT("parentTags"), BuildStringArray(Tag.ParentTags));
			TagObj->SetArrayField(TEXT("associatedStates"), BuildStringArray(Tag.AssociatedAnimStates));
			AddSortedStringMap(TagObj, TEXT("tagMetadata"), Tag.TagMetadata);
			GameplayTagArray.Add(MakeShareable(new FJsonValueObject(TagObj)));
		}
		JsonObject->SetArrayField(TEXT("gameplayTags"), GameplayTagArray);

		// Fast path analysis
		{
			TSharedPtr<FJsonObject> FastPathObj = MakeShareable(new FJsonObject);
			FastPathObj->SetBoolField(TEXT("isFastPathCompliant"), Data.FastPathAnalysis.bIsFastPathCompliant);
			FastPathObj->SetArrayField(TEXT("fastPathViolations"), BuildStringArray(Data.FastPathAnalysis.FastPathViolations));
			FastPathObj->SetArrayField(TEXT("nonFastPathNodes"), BuildStringArray(Data.FastPathAnalysis.NonFastPathNodes));
			AddSortedBoolMap(FastPathObj, TEXT("nodeFastPathStatus"), Data.FastPathAnalysis.NodeFastPathStatus);
			JsonObject->SetObjectField(TEXT("fastPathAnalysis"), FastPathObj);
		}

		// Coverage
		{
			TSharedPtr<FJsonObject> CoverageObj = MakeShareable(new FJsonObject);
			CoverageObj->SetNumberField(TEXT("totalStateNodes"), Data.Coverage.TotalStateNodes);
			CoverageObj->SetNumberField(TEXT("totalTransitions"), Data.Coverage.TotalTransitions);
			CoverageObj->SetNumberField(TEXT("totalNotifies"), Data.Coverage.TotalNotifies);
			CoverageObj->SetNumberField(TEXT("totalCurves"), Data.Coverage.TotalCurves);
			CoverageObj->SetNumberField(TEXT("totalAnimAssets"), Data.Coverage.TotalAnimAssets);
			CoverageObj->SetNumberField(TEXT("totalControlRigs"), Data.Coverage.TotalControlRigs);
			AddSortedIntMap(CoverageObj, TEXT("nodeTypeCounts"), Data.Coverage.NodeTypeCounts);
			JsonObject->SetObjectField(TEXT("coverage"), CoverageObj);
		}
	}
 	
	return JsonObject;
}

bool UBlueprintAnalyzer::SaveBlueprintDataToFile(const FString& JsonData, const FString& FilePath)
{
	return FFileHelper::SaveStringToFile(JsonData, *FilePath, FFileHelper::EEncodingOptions::AutoDetect);
}

bool UBlueprintAnalyzer::ExportSingleBlueprintToJSON(const FString& BlueprintPath, const FString& OutputDirectory)
{
	UE_LOG(LogTemp, Warning, TEXT("🚀 Starting single Blueprint export for: %s"), *BlueprintPath);
	
	// Load the Blueprint
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintPath);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Could not load Blueprint: %s"), *BlueprintPath);
		return false;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("✅ Blueprint loaded successfully"));
	
	// Analyze the Blueprint
	FBS_BlueprintData BlueprintData = AnalyzeBlueprint(Blueprint);
	if (BlueprintData.BlueprintName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Blueprint analysis failed"));
		return false;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("📊 Blueprint analyzed: %d nodes, %d variables, %d functions"), 
		BlueprintData.TotalNodeCount, BlueprintData.Variables.Num(), BlueprintData.Functions.Num());
	
	// Convert to JSON
	FString JsonData = ExportBlueprintDataToJSON(BlueprintData);
	if (JsonData.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("❌ JSON conversion failed"));
		return false;
	}
	
	// Determine output directory
	FString ExportDirectory = OutputDirectory;
	if (ExportDirectory.IsEmpty())
	{
		ExportDirectory = FPaths::ProjectSavedDir() / TEXT("BlueprintExports");
	}
	
	// Create directory if it doesn't exist
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*ExportDirectory))
	{
		PlatformFile.CreateDirectoryTree(*ExportDirectory);
	}
	
	// Generate filename with timestamp
	FString FileName = FString::Printf(TEXT("BP_SLZR_Blueprint_%s_%s.json"), 
		*BlueprintData.BlueprintName, 
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	
	FString FullFilePath = ExportDirectory / FileName;
	
	// Save to file
	if (SaveBlueprintDataToFile(JsonData, FullFilePath))
	{
		UE_LOG(LogTemp, Warning, TEXT("🎯 Single Blueprint export successful! File: %s"), *FullFilePath);
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to save file: %s"), *FullFilePath);
		return false;
	}
}

FString UBlueprintAnalyzer::AnalyzeNodeWithConnections(UEdGraphNode* Node)
{
	if (!Node)
	{
		return FString();
	}

	TSharedPtr<FJsonObject> NodeJson = MakeShareable(new FJsonObject);
	
	// Basic node information
	NodeJson->SetStringField(TEXT("nodeType"), Node->GetClass()->GetName());
	NodeJson->SetStringField(TEXT("nodeTitle"), Node->GetNodeTitle(ENodeTitleType::ListView).ToString());
	NodeJson->SetStringField(TEXT("nodeGuid"), Node->NodeGuid.ToString());
	
	// Node position
	NodeJson->SetNumberField(TEXT("posX"), Node->NodePosX);
	NodeJson->SetNumberField(TEXT("posY"), Node->NodePosY);
	NodeJson->SetNumberField(TEXT("width"), Node->NodeWidth);
	NodeJson->SetNumberField(TEXT("height"), Node->NodeHeight);
	if (!Node->NodeComment.IsEmpty())
	{
		NodeJson->SetStringField(TEXT("comment"), Node->NodeComment);
	}
	NodeJson->SetStringField(TEXT("enabledState"), LexToString(Node->GetDesiredEnabledState()));
	switch (Node->AdvancedPinDisplay)
	{
	case ENodeAdvancedPins::NoPins: NodeJson->SetStringField(TEXT("advancedPinDisplay"), TEXT("NoPins")); break;
	case ENodeAdvancedPins::Shown:  NodeJson->SetStringField(TEXT("advancedPinDisplay"), TEXT("Shown")); break;
	case ENodeAdvancedPins::Hidden: NodeJson->SetStringField(TEXT("advancedPinDisplay"), TEXT("Hidden")); break;
	default: NodeJson->SetStringField(TEXT("advancedPinDisplay"), TEXT("Unknown")); break;
	}
	if (Node->bHasCompilerMessage)
	{
		NodeJson->SetBoolField(TEXT("hasCompilerMessage"), true);
	}
	if (Node->ErrorType != 0)
	{
		NodeJson->SetNumberField(TEXT("errorType"), Node->ErrorType);
	}
	if (!Node->ErrorMsg.IsEmpty())
	{
		NodeJson->SetStringField(TEXT("errorMsg"), Node->ErrorMsg);
	}
#if WITH_EDITORONLY_DATA
	if (Node->bCommentBubblePinned) NodeJson->SetBoolField(TEXT("commentBubblePinned"), true);
	if (Node->bCommentBubbleVisible) NodeJson->SetBoolField(TEXT("commentBubbleVisible"), true);
	if (Node->bCommentBubbleMakeVisible) NodeJson->SetBoolField(TEXT("commentBubbleMakeVisible"), true);
	if (Node->bCanResizeNode) NodeJson->SetBoolField(TEXT("canResize"), true);
	if (Node->GetCanRenameNode()) NodeJson->SetBoolField(TEXT("canRename"), true);
#endif
	
	// Specific node type analysis
	if (UK2Node* K2Node = Cast<UK2Node>(Node))
	{
		// Add K2-specific information
		NodeJson->SetStringField(TEXT("k2NodeClass"), K2Node->GetClass()->GetName());
		
		// Analyze specific K2Node types
		if (UK2Node_CallFunction* CallFuncNode = Cast<UK2Node_CallFunction>(K2Node))
		{
			NodeJson->SetStringField(TEXT("functionName"), CallFuncNode->FunctionReference.GetMemberName().ToString());
			if (CallFuncNode->FunctionReference.GetMemberParentClass())
			{
				NodeJson->SetStringField(TEXT("functionClass"), CallFuncNode->FunctionReference.GetMemberParentClass()->GetName());
			}
		}
		else if (UK2Node_VariableGet* VarGetNode = Cast<UK2Node_VariableGet>(K2Node))
		{
			NodeJson->SetStringField(TEXT("variableName"), VarGetNode->VariableReference.GetMemberName().ToString());
			NodeJson->SetStringField(TEXT("variableType"), TEXT("VariableGet"));
		}
		else if (UK2Node_VariableSet* VarSetNode = Cast<UK2Node_VariableSet>(K2Node))
		{
			NodeJson->SetStringField(TEXT("variableName"), VarSetNode->VariableReference.GetMemberName().ToString());
			NodeJson->SetStringField(TEXT("variableType"), TEXT("VariableSet"));
		}
		else if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(K2Node))
		{
			NodeJson->SetStringField(TEXT("eventName"), EventNode->EventReference.GetMemberName().ToString());
			NodeJson->SetStringField(TEXT("eventType"), TEXT("Event"));
		}
		else if (UK2Node_CustomEvent* CustomEventNode = Cast<UK2Node_CustomEvent>(K2Node))
		{
			NodeJson->SetStringField(TEXT("customEventName"), CustomEventNode->CustomFunctionName.ToString());
			NodeJson->SetStringField(TEXT("eventType"), TEXT("CustomEvent"));
		}
	}

	// Include nodeProperties (best-effort UPROPERTY export)
	{
		TMap<FString, FString> NodeProps;
		ExtractObjectProperties(Node, NodeProps);
		if (NodeProps.Num() > 0)
		{
			AddSortedStringMap(NodeJson, TEXT("nodeProperties"), NodeProps);
		}
	}
	
	// Extract all pins and their connections
	TArray<TSharedPtr<FJsonValue>> PinsArray;
	for (UEdGraphPin* Pin : Node->Pins)
	{
		if (Pin)
		{
			TSharedPtr<FJsonObject> PinJson = MakeShareable(new FJsonObject);
			
			// Pin basic info
			PinJson->SetStringField(TEXT("pinName"), Pin->PinName.ToString());
			if (Pin->PinId.IsValid())
			{
				PinJson->SetStringField(TEXT("pinId"), Pin->PinId.ToString());
			}
			PinJson->SetStringField(TEXT("pinType"), Pin->PinType.PinCategory.ToString());
			PinJson->SetStringField(TEXT("pinSubType"), Pin->PinType.PinSubCategory.ToString());
			PinJson->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
			PinJson->SetBoolField(TEXT("isExec"), Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);
			
			// Pin object type if available
			if (Pin->PinType.PinSubCategoryObject.IsValid())
			{
				PinJson->SetStringField(TEXT("objectType"), Pin->PinType.PinSubCategoryObject->GetName());
				PinJson->SetStringField(TEXT("objectPath"), Pin->PinType.PinSubCategoryObject->GetPathName());
			}

			// Pin flags
			if (Pin->PinType.bIsReference) PinJson->SetBoolField(TEXT("isReference"), true);
			if (Pin->PinType.bIsConst) PinJson->SetBoolField(TEXT("isConst"), true);
			if (Pin->PinType.bIsWeakPointer) PinJson->SetBoolField(TEXT("isWeakPointer"), true);
			if (Pin->PinType.bIsUObjectWrapper) PinJson->SetBoolField(TEXT("isUObjectWrapper"), true);
			switch (Pin->PinType.ContainerType)
			{
			case EPinContainerType::Array: PinJson->SetBoolField(TEXT("isArray"), true); break;
			case EPinContainerType::Map:   PinJson->SetBoolField(TEXT("isMap"), true); break;
			case EPinContainerType::Set:   PinJson->SetBoolField(TEXT("isSet"), true); break;
			default: break;
			}
			if (Pin->SourceIndex != INDEX_NONE) PinJson->SetNumberField(TEXT("sourceIndex"), Pin->SourceIndex);
#if WITH_EDITORONLY_DATA
			if (Pin->ParentPin && Pin->ParentPin->PinId.IsValid())
			{
				PinJson->SetStringField(TEXT("parentPinId"), Pin->ParentPin->PinId.ToString());
			}
			if (Pin->SubPins.Num() > 0)
			{
				TArray<TSharedPtr<FJsonValue>> SubPinArray;
				for (UEdGraphPin* SubPin : Pin->SubPins)
				{
					if (SubPin && SubPin->PinId.IsValid())
					{
						SubPinArray.Add(MakeShareable(new FJsonValueString(SubPin->PinId.ToString())));
					}
				}
				if (SubPinArray.Num() > 0)
				{
					PinJson->SetArrayField(TEXT("subPinIds"), SubPinArray);
				}
			}
#endif
#if WITH_EDITORONLY_DATA
			if (Pin->bHidden) PinJson->SetBoolField(TEXT("hidden"), true);
			if (Pin->bNotConnectable) PinJson->SetBoolField(TEXT("notConnectable"), true);
			if (Pin->bDefaultValueIsReadOnly) PinJson->SetBoolField(TEXT("defaultValueReadOnly"), true);
			if (Pin->bDefaultValueIsIgnored) PinJson->SetBoolField(TEXT("defaultValueIgnored"), true);
			if (Pin->bAdvancedView) PinJson->SetBoolField(TEXT("advanced"), true);
			if (Pin->bDisplayAsMutableRef) PinJson->SetBoolField(TEXT("displayAsMutableRef"), true);
			if (Pin->bAllowFriendlyName) PinJson->SetBoolField(TEXT("allowFriendlyName"), true);
			if (Pin->bOrphanedPin) PinJson->SetBoolField(TEXT("orphaned"), true);
			if (!Pin->PinFriendlyName.IsEmpty()) PinJson->SetStringField(TEXT("friendlyName"), Pin->PinFriendlyName.ToString());
			if (Pin->PersistentGuid.IsValid())
			{
				PinJson->SetStringField(TEXT("persistentGuid"), Pin->PersistentGuid.ToString());
			}
#endif
			if (!Pin->PinToolTip.IsEmpty()) PinJson->SetStringField(TEXT("toolTip"), Pin->PinToolTip);
			
			// Default value
			if (!Pin->DefaultValue.IsEmpty())
			{
				PinJson->SetStringField(TEXT("defaultValue"), Pin->DefaultValue);
			}
			if (!Pin->AutogeneratedDefaultValue.IsEmpty())
			{
				PinJson->SetStringField(TEXT("autogeneratedDefaultValue"), Pin->AutogeneratedDefaultValue);
			}
			if (Pin->DefaultObject)
			{
				PinJson->SetStringField(TEXT("defaultObjectPath"), Pin->DefaultObject->GetPathName());
			}

			// Map/Set value terminal type
			const FEdGraphTerminalType& ValueType = Pin->PinType.PinValueType;
			if (ValueType.TerminalCategory != NAME_None)
			{
				PinJson->SetStringField(TEXT("valueCategory"), ValueType.TerminalCategory.ToString());
			}
			if (ValueType.TerminalSubCategory != NAME_None)
			{
				PinJson->SetStringField(TEXT("valueSubCategory"), ValueType.TerminalSubCategory.ToString());
			}
			if (ValueType.TerminalSubCategoryObject.IsValid())
			{
				PinJson->SetStringField(TEXT("valueObjectType"), ValueType.TerminalSubCategoryObject->GetName());
				PinJson->SetStringField(TEXT("valueObjectPath"), ValueType.TerminalSubCategoryObject->GetPathName());
			}
			if (ValueType.bTerminalIsConst) PinJson->SetBoolField(TEXT("valueIsConst"), true);
			if (ValueType.bTerminalIsWeakPointer) PinJson->SetBoolField(TEXT("valueIsWeakPointer"), true);
			if (ValueType.bTerminalIsUObjectWrapper) PinJson->SetBoolField(TEXT("valueIsUObjectWrapper"), true);
			
			// Pin connections - this is the crucial part!
			TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (LinkedPin && LinkedPin->GetOwningNode())
				{
					TSharedPtr<FJsonObject> ConnectionJson = MakeShareable(new FJsonObject);
					ConnectionJson->SetStringField(TEXT("connectedNodeGuid"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
					ConnectionJson->SetStringField(TEXT("connectedNodeType"), LinkedPin->GetOwningNode()->GetClass()->GetName());
					ConnectionJson->SetStringField(TEXT("connectedPinName"), LinkedPin->PinName.ToString());
					if (LinkedPin->PinId.IsValid())
					{
						ConnectionJson->SetStringField(TEXT("connectedPinId"), LinkedPin->PinId.ToString());
					}
					ConnectionJson->SetStringField(TEXT("connectedPinType"), LinkedPin->PinType.PinCategory.ToString());
					ConnectionJson->SetStringField(TEXT("connectedDirection"), LinkedPin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
					
					ConnectionsArray.Add(MakeShareable(new FJsonValueObject(ConnectionJson)));
				}
			}
			PinJson->SetArrayField(TEXT("connections"), ConnectionsArray);
			
			PinsArray.Add(MakeShareable(new FJsonValueObject(PinJson)));
		}
	}
	NodeJson->SetArrayField(TEXT("pins"), PinsArray);
	
	// Convert to string
	FString OutputString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(NodeJson.ToSharedRef(), Writer);
	
	return OutputString;
}

void UBlueprintAnalyzer::ExtractBlueprintMetadata(UBlueprint* Blueprint, FBS_BlueprintData& OutData)
{
	if (!Blueprint)
	{
		return;
	}

	// Blueprint type classification
	OutData.bIsInterface = Blueprint->BlueprintType == BPTYPE_Interface;
	OutData.bIsMacroLibrary = Blueprint->BlueprintType == BPTYPE_MacroLibrary;
	OutData.bIsFunctionLibrary = Blueprint->BlueprintType == BPTYPE_FunctionLibrary;
	
	// Blueprint description and category
	OutData.BlueprintDescription = Blueprint->BlueprintDescription;
	OutData.BlueprintCategory = Blueprint->BlueprintCategory;
	
	// Blueprint tags
	for (const FString& Tag : Blueprint->HideCategories)
	{
		OutData.BlueprintTags.Add(FString::Printf(TEXT("HideCategory: %s"), *Tag));
	}
}

TArray<FBS_VariableInfo> UBlueprintAnalyzer::ExtractDetailedVariables(UBlueprint* Blueprint)
{
	TArray<FBS_VariableInfo> DetailedVars;
	
	if (!Blueprint)
	{
		return DetailedVars;
	}
	
	// Extract Blueprint variables with full metadata
	for (const FBPVariableDescription& VarDesc : Blueprint->NewVariables)
	{
		FBS_VariableInfo VarInfo;
		
		VarInfo.VariableName = VarDesc.VarName.ToString();
		VarInfo.VariableType = VarDesc.VarType.PinCategory.ToString();
		
		if (VarDesc.VarType.PinSubCategoryObject.IsValid())
		{
			VarInfo.VariableType += FString::Printf(TEXT(" (%s)"), *VarDesc.VarType.PinSubCategoryObject->GetName());
		}
		
		// Default value
		VarInfo.DefaultValue = VarDesc.DefaultValue;
		
		// Category and tooltip
		VarInfo.Category = VarDesc.Category.ToString();
		VarInfo.Tooltip = VarDesc.FriendlyName;
		
		// Variable properties
		VarInfo.bIsPublic = (VarDesc.PropertyFlags & CPF_DisableEditOnInstance) == 0;
		VarInfo.bIsReplicated = (VarDesc.ReplicationCondition != COND_None);
		VarInfo.bIsExposedOnSpawn = (VarDesc.PropertyFlags & CPF_ExposeOnSpawn) != 0;
		VarInfo.bIsEditable = (VarDesc.PropertyFlags & CPF_Edit) != 0;
		
		// Replication condition
		switch (VarDesc.ReplicationCondition)
		{
		case COND_InitialOnly: VarInfo.RepCondition = TEXT("InitialOnly"); break;
		case COND_OwnerOnly: VarInfo.RepCondition = TEXT("OwnerOnly"); break;
		case COND_SkipOwner: VarInfo.RepCondition = TEXT("SkipOwner"); break;
		case COND_SimulatedOnly: VarInfo.RepCondition = TEXT("SimulatedOnly"); break;
		case COND_AutonomousOnly: VarInfo.RepCondition = TEXT("AutonomousOnly"); break;
		case COND_SimulatedOrPhysics: VarInfo.RepCondition = TEXT("SimulatedOrPhysics"); break;
		case COND_InitialOrOwner: VarInfo.RepCondition = TEXT("InitialOrOwner"); break;
		default: VarInfo.RepCondition = TEXT("None"); break;
		}
		
		DetailedVars.Add(VarInfo);
	}
	
	return DetailedVars;
}

TArray<FBS_FunctionInfo> UBlueprintAnalyzer::ExtractDetailedFunctions(UBlueprint* Blueprint)
{
	TArray<FBS_FunctionInfo> DetailedFuncs;
	
	if (!Blueprint)
	{
		return DetailedFuncs;
	}
	
	// Extract Blueprint functions with full metadata
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph)
		{
			FBS_FunctionInfo FuncInfo;
			
			FuncInfo.FunctionName = Graph->GetFName().ToString();
			
			// Find function entry node to get metadata
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
				{
					// Function metadata (UE5.5 compatible)
					// Try to get the UFunction for metadata
					UFunction* Func = nullptr;
					if (Blueprint->GeneratedClass)
					{
						Func = Blueprint->GeneratedClass->FindFunctionByName(Graph->GetFName());
					}
					if (!Func && Blueprint->SkeletonGeneratedClass)
					{
						Func = Blueprint->SkeletonGeneratedClass->FindFunctionByName(Graph->GetFName());
					}
					
					if (Func)
					{
						FuncInfo.bIsPure = Func->HasAnyFunctionFlags(FUNC_BlueprintPure);
						FuncInfo.bCallInEditor = Func->HasMetaData(TEXT("CallInEditor"));
						FuncInfo.bIsStatic = Func->HasAnyFunctionFlags(FUNC_Static);
						FuncInfo.bIsLatent = Func->HasMetaData(TEXT("Latent"));
						
						// Access specifier
						if (Func->HasAnyFunctionFlags(FUNC_Protected))
						{
							FuncInfo.bIsProtected = true;
							FuncInfo.AccessSpecifier = TEXT("protected");
						}
						else if (Func->HasAnyFunctionFlags(FUNC_Private))
						{
							FuncInfo.bIsPrivate = true;
							FuncInfo.AccessSpecifier = TEXT("private");
						}
						else
						{
							FuncInfo.bIsPublic = true;
							FuncInfo.AccessSpecifier = TEXT("public");
						}
						
						// Category and tooltip
						if (Func->HasMetaData(TEXT("Category")))
						{
							FuncInfo.Category = Func->GetMetaData(TEXT("Category"));
						}
						if (Func->HasMetaData(TEXT("ToolTip")))
						{
							FuncInfo.Tooltip = Func->GetMetaData(TEXT("ToolTip"));
						}
					}
					else
					{
						// Fallback to basic extraction
						FuncInfo.bIsPure = EntryNode->IsNodePure();
						FuncInfo.Tooltip = EntryNode->GetTooltipText().ToString();
						FuncInfo.bIsPublic = true;
						FuncInfo.AccessSpecifier = TEXT("public");
					}
					
					// Extract parameters
					for (UEdGraphPin* Pin : EntryNode->Pins)
					{
						if (Pin && Pin->Direction == EGPD_Output && Pin->PinName != UEdGraphSchema_K2::PN_Then)
						{
							FString ParamInfo = FString::Printf(TEXT("%s: %s"), *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString());
							if (Pin->PinType.PinSubCategoryObject.IsValid())
							{
								ParamInfo += FString::Printf(TEXT(" (%s)"), *Pin->PinType.PinSubCategoryObject->GetName());
							}
							FuncInfo.OutputParameters.Add(ParamInfo);
						}
						else if (Pin && Pin->Direction == EGPD_Input && Pin->PinName != UEdGraphSchema_K2::PN_Execute)
						{
							FString ParamInfo = FString::Printf(TEXT("%s: %s"), *Pin->PinName.ToString(), *Pin->PinType.PinCategory.ToString());
							if (Pin->PinType.PinSubCategoryObject.IsValid())
							{
								ParamInfo += FString::Printf(TEXT(" (%s)"), *Pin->PinType.PinSubCategoryObject->GetName());
							}
							if (!Pin->DefaultValue.IsEmpty())
							{
								ParamInfo += FString::Printf(TEXT(" = %s"), *Pin->DefaultValue);
							}
							FuncInfo.InputParameters.Add(ParamInfo);
						}
					}
					break;
				}
			}
			
			DetailedFuncs.Add(FuncInfo);
		}
	}
	
	return DetailedFuncs;
}

TArray<FBS_ComponentInfo> UBlueprintAnalyzer::ExtractDetailedComponents(UBlueprint* Blueprint)
{
	TArray<FBS_ComponentInfo> DetailedComps;
	
	if (!Blueprint || !Blueprint->SimpleConstructionScript)
	{
		return DetailedComps;
	}
	
	// Extract components with full hierarchy and properties
	for (USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
	{
		if (Node && Node->ComponentTemplate)
		{
			FBS_ComponentInfo CompInfo;
			
			CompInfo.ComponentName = Node->GetVariableName().ToString();
			CompInfo.ComponentClass = Node->ComponentTemplate->GetClass()->GetName();
			
			// Parent component relationship (simplified for UE5.5)
			CompInfo.ParentComponent = TEXT("Unknown"); // TODO: Extract parent hierarchy in UE5.5
			
			// Transform information
			if (USceneComponent* SceneComp = Cast<USceneComponent>(Node->ComponentTemplate))
			{
				FTransform RelativeTransform = SceneComp->GetRelativeTransform();
				CompInfo.Transform = RelativeTransform.ToString();
			}
			
			// Component properties
			UClass* CompClass = Node->ComponentTemplate->GetClass();
			for (FProperty* Property : TFieldRange<FProperty>(CompClass))
			{
				if (Property && Property->HasAnyPropertyFlags(CPF_Edit))
				{
					// UE5.5 compatible property extraction
					FString PropertyValue = Property->GetCPPType();
					
					if (!PropertyValue.IsEmpty())
					{
						CompInfo.ComponentProperties.Add(FString::Printf(TEXT("%s: %s"), *Property->GetName(), *PropertyValue));
					}
				}
			}
			
			DetailedComps.Add(CompInfo);
		}
	}
	
	return DetailedComps;
}

TArray<FString> UBlueprintAnalyzer::ExtractAssetReferences(UBlueprint* Blueprint)
{
	TArray<FString> AssetRefs;
	
	if (!Blueprint)
	{
		return AssetRefs;
	}
	
	// Extract asset references from Blueprint
	TArray<UObject*> ReferencedObjects;
	FReferenceFinder AssetReferenceCollector(ReferencedObjects, nullptr, false, true, false, true);
	AssetReferenceCollector.FindReferences(Blueprint);
	
	for (UObject* ReferencedObject : ReferencedObjects)
	{
		if (ReferencedObject && ReferencedObject != Blueprint && !ReferencedObject->IsIn(Blueprint))
		{
			FString AssetPath = ReferencedObject->GetPathName();
			if (AssetPath.StartsWith(TEXT("/Game/")) || AssetPath.StartsWith(TEXT("/Engine/")))
			{
				AssetRefs.AddUnique(AssetPath);
			}
		}
	}
	
	return AssetRefs;
}
