/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for an ARA document controller implementation. 

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "ARA_Library/PlugIn/ARAPlug.h"
#include "AudioView.h"

//==============================================================================
class ArasampleProjectDocumentController  : public ARA::PlugIn::DocumentController
{
public:
    class Editor;

    ArasampleProjectDocumentController();
    ~ArasampleProjectDocumentController();
//==============================================================================
// Override document controller methods here
protected:
    // needed for ARA AudioFormatReaders to be thread-safe and work properly!
    ARA::PlugIn::AudioSource* doCreateAudioSource       (ARA::PlugIn::Document*, ARA::ARAAudioSourceHostRef) override;
    ARA::PlugIn::RegionSequence* doCreateRegionSequence (ARA::PlugIn::Document*, ARA::ARARegionSequenceHostRef) override;
    void willEnableAudioSourceSamplesAccess (ARA::PlugIn::AudioSource* audioSource, bool enable) override;
    void didEnableAudioSourceSamplesAccess  (ARA::PlugIn::AudioSource* audioSource, bool enable) override;
    void willUpdateAudioSourceProperties (
        ARA::PlugIn::AudioSource*, ARA::PlugIn::PropertiesPtr<ARA::ARAAudioSourceProperties>) override;
    void didUpdateAudioSourceProperties     (ARA::PlugIn::AudioSource *audioSource) override;
    void willUpdatePlaybackRegionProperties (ARA::PlugIn::PlaybackRegion* playbackRegion, ARA::PlugIn::PropertiesPtr<ARA::ARAPlaybackRegionProperties> newProperties) override;
    void didUpdatePlaybackRegionProperties  (ARA::PlugIn::PlaybackRegion* playbackRegion) override;

    ARA::PlugIn::EditorView* doCreateEditorView() override;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ArasampleProjectDocumentController)
};;

/** Naive Editor class that visualize current ARA Document RegionSequences state */
class ArasampleProjectDocumentController::Editor : public juce::Component, public ARA::PlugIn::EditorView
{
public:
    Editor (ARA::PlugIn::DocumentController*);
    void doNotifySelection (const ARA::PlugIn::ViewSelection*) override;
    void resized () override;

private:
    double _maxRegionSequenceLength;
    juce::CriticalSection selectionLock;
    juce::OwnedArray <AudioView> _regionSequenceViews;
};
