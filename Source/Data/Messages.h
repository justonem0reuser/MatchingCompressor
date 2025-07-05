#pragma once

// DataReceiver
const juce::String
	refAudioStr = "Reference audio",
	destAudioStr = "Destination audio",
	openFromFileStr = "Open from file",
	readFromMainBusStr = "Read from\nmain bus",
	readFromSidechainBusStr = "Read from\nsidechain bus",
	mainBusIsDisconnectedStr = "Main bus is\ndisconnected",
	sidechainBusIsDisconnectedStr = "Sidechain bus\nis disconnected",
	refSetFromFileStr = "Reference is set from file",
	destSetFromFileStr = "Destination is set from file",
	refSetFromBusStr = "Reference is set from sidechain bus",
	destSetFromBusStr = "Destination is set from main bus";
	
// exceptions
const juce::String
	refFileExStr = "Reference file: ",
	destFileExStr = "Destination file: ",
	refStreamExStr = "Reference audio stream: ",
	destStreamExStr = "Destination audio stream: ",
	emptyFileExStr = "File is not chosen or empty",
	corruptedChannelExStr = "One of channels is corrupted",
	numChannelsIsNullExStr = "Channels number is null",
	lengthIsNullExStr = "Samples number is null",
	sampleRateIsNullExStr = "Sample rate is null",

	regionsNumberTooSmall = "Regions number for source statistics data is too small",
	numRegionsTooBigExStr = "Samples number must exceed regions number",
	numRegionsTooSmallExStr = "Regions number must be more than 1",
	blockSizeWrongExStr = "Block size must be positive",
	notEnoughSourceSamplesExStr = "Not enough source file samples with sufficient gain with chosen regions number and minimum gain",
	notEnoughDestinationSamplesExStr = "Not enough destination file samples with sufficient gain with chosen regions number and minimum gain",
	cannotCalculateErrStr = "Cannot calculate compression parameters for these files and settings";

const juce::String fileChooserTitleStr = "Select a Wave file...";
const juce::String matchWindowTitleStr = "Match";
const juce::String matchBtnStr = "Match";
const juce::String resetBtnStr = "Reset to calculated parameters";

// parameters IDs
const juce::String 
	kneesNumberId = "kneesNumber",
	gainId = "gain",
	thresholdId = "threshold",
	ratioId = "ratio",
	kneeWidthId = "kneeWidth",
	attackId = "attack",
	releaseId = "release",
	balFilterTypeId = "balFilterType",
	channelAggrerationTypeId = "channelAggregationType";

// settinfs IDs
const juce::String 
	setGainRegionsNumberId = "setGainRegionsNumber",
	setQuantileRegionsNumberId = "setQuantileRegionsNumber",
	setKneesNumberId = "setKneesNumber",
	setKneeTypeId = "setKneeType",
	setAttackId = "setAttack",
	setReleaseId = "setRelease",
	setChannelAggregationTypeId = "setChannelAggregationType",
	setBalFilterTypeId = "setBalFilterType";

const juce::StringArray 
	kneesNumbers{ "1", "2", "3" },
	kneeTypes{ "hard", "soft" },
	balFilterTypes{ "peak", "RMS" },
	channelAggregationTypes{ "separate", "max", "mean" };
