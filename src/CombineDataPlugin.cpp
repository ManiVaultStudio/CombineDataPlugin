#include "CombineDataPlugin.h"

#include <PointData/PointData.h>

#include <CoreInterface.h>
#include <Dataset.h>
#include <DataType.h>
#include <LinkedData.h>
#include <Set.h>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <map>
#include <utility>      // move
#include <vector>

#include <QDebug>
#include <QList>
#include <QString>
#include <QVector>

Q_PLUGIN_METADATA(IID "studio.manivault.CombineDataPlugin")

// =============================================================================
// Plugin 
// =============================================================================

CombineDataPlugin::CombineDataPlugin(const PluginFactory* factory) :
    TransformationPlugin(factory)
{
}

void CombineDataPlugin::transform()
{
    // Get input
    mv::Datasets inputDatasets = getInputDatasets();

    if (inputDatasets.size() < 2) {
        qDebug() << "CombineDataPlugin: select at least two data sets, doing nothing...";
        return;
    }

    const auto firstInputDataset = inputDatasets[0].get<Points>();
    const auto numDimensions = firstInputDataset->getNumDimensions();

    // Check if all datasets have the same number of dimensions
    if (!std::all_of(
        inputDatasets.begin(),
        inputDatasets.end(),
        [numDimensions](const auto& dataset) {
            const auto currentPoints = dataset.get<Points>();
            return currentPoints->getNumDimensions() == numDimensions;
        })) {

        qDebug() << "CombineDataPlugin: all data sets need to have the same number of dimensions...";
        return;
    }

    // TODO: check if all datasets have same data type, current float is assumed

    // New, combined dataset
    const QString combinedDataName = "Combined data";      // TODO: create dialog to as for name

    mv::Dataset<Points> combinedDataset = mv::data().createDataset<Points>("Points", combinedDataName);
    mv::Dataset<mv::DatasetImpl> combinedDatasetImpl = mv::Dataset<mv::DatasetImpl>(combinedDataset.getDataset());

    std::vector<uint32_t> indicesOffsets;
    indicesOffsets.reserve(inputDatasets.size());
    indicesOffsets.push_back(0);

    QVariantList combinedDatasetIDs;
    indicesOffsets.reserve(inputDatasets.size());

    std::vector<float> combinedData;

    for (size_t datasetID = 0; datasetID < inputDatasets.size(); datasetID++) {
        mv::Dataset<mv::DatasetImpl>& currentInputDataset = inputDatasets[datasetID];
        const mv::Dataset<Points> currentInputPointsData = currentInputDataset.get<Points>();
        combinedDatasetIDs.push_back(currentInputDataset->getId());
        const uint32_t numPointsCurrentInput = currentInputPointsData->getNumPoints();
        const uint32_t currentOffset = indicesOffsets.back();

        if (datasetID < inputDatasets.size() - 1) {
            indicesOffsets.push_back(currentOffset + numPointsCurrentInput);
        }

        // Combine data from all data sets
        combinedData.reserve(combinedData.size() + static_cast<size_t>(numDimensions) * numPointsCurrentInput);

        currentInputPointsData->visitFromBeginToEnd([&combinedData](auto begin, auto end) {
                combinedData.insert(combinedData.end(), begin, end);
            });

        // Add selection maps
        {
            mv::SelectionMap selectionMapCombinedToInput;
            auto& mapCombinedToInput = selectionMapCombinedToInput.getMap();

            for (uint32_t pointID = 0; pointID < numPointsCurrentInput; pointID++) {
                mapCombinedToInput[pointID + currentOffset] = { pointID };
            }

            combinedDataset->addLinkedData(currentInputDataset, std::move(selectionMapCombinedToInput));
        }

        // Add reverse selection map
        {
            mv::SelectionMap selectionMapInputToCombined;
            auto& mapInputToCombined = selectionMapInputToCombined.getMap();

            for (uint32_t pointID = 0; pointID < numPointsCurrentInput; pointID++) {
                mapInputToCombined[pointID] = {  pointID + currentOffset };
            }

            currentInputDataset->addLinkedData(combinedDatasetImpl, std::move(selectionMapInputToCombined));
        }


        // Connect deleted datasets and remove selection maps
        connect(&currentInputDataset, &mv::Dataset<mv::DatasetImpl>::aboutToBeRemoved, this, [this, &combinedDataset, currentInputDataset]() {
            combinedDataset->removeLinkedDataset(currentInputDataset);
            });
    }

    // Add properties: offsets of starting indices
    combinedDataset->setProperty("Combined Dataset IDs", combinedDatasetIDs);

    QVariantList indicesOffsetsVariant(indicesOffsets.size());
    for (std::size_t i = 0; i < indicesOffsets.size(); ++i)
        indicesOffsetsVariant[i] = indicesOffsets[i];

    combinedDataset->setProperty("Combined Dataset Offsets", indicesOffsetsVariant);

    // Publish data
    combinedDataset->setData(std::move(combinedData), numDimensions);
    combinedDataset->setDimensionNames(firstInputDataset->getDimensionNames());
    mv::events().notifyDatasetDataChanged(combinedDataset);

}


// =============================================================================
// Plugin Factory 
// =============================================================================

TransformationPlugin* CombineDataPluginFactory::produce()
{
    return new CombineDataPlugin(this);
}

mv::DataTypes CombineDataPluginFactory::supportedDataTypes() const
{
    return { PointType };
}

mv::gui::PluginTriggerActions CombineDataPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    mv::gui::PluginTriggerActions pluginTriggerActions;

    const auto numberOfDatasets = datasets.count();

    if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        if (numberOfDatasets >= 2) {

            auto pluginTriggerAction = new mv::gui::PluginTriggerAction(const_cast<CombineDataPluginFactory*>(this), this, QString("Combine..."), QString("Combine..."), icon(), [this, datasets](mv::gui::PluginTriggerAction& pluginTriggerAction) -> void {
                auto pluginInstance = dynamic_cast<CombineDataPlugin*>(mv::plugins().requestPlugin(getKind()));

                pluginInstance->setInputDatasets(datasets);
                pluginInstance->transform();

                });

            pluginTriggerActions << pluginTriggerAction;

        }
    }

    return pluginTriggerActions;
}
