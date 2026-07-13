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
	destSetFromBusStr = "Destination is set from main bus",
	minimalThemeStr = "Minimal style",
	brutalThemeStr = "Brutal style";
	
// exceptions
const juce::String
	refFileExStr = "Reference file: ",
	destFileExStr = "Destination file: ",
	refStreamExStr = "Reference audio stream: ",
	destStreamExStr = "Destination audio stream: ",
	emptyFileExStr = "File is not chosen or empty",
	corruptedChannelExStr = "One of channels is corrupted",
	numChannelsIsNullExStr = "Channels number is null",
	sampleRateIsNullExStr = "Sample rate is null",
	samplesNumberIsTooLarge = "Samples number is too large",
	samplesNumberIsTooSmall = "Samples number is too small",

	numRegionsTooBigExStr = "Samples number must exceed regions number",
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
