/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for an ARA document controller implementation. 

  ==============================================================================
*/

#include "PluginARADocumentController.h"

#include "ARAUtils/ARAAudioSource.h"
#include "ARAUtils/ARARegionSequence.h"

//==============================================================================
ArasampleProjectDocumentController::ArasampleProjectDocumentController()
{
}

ArasampleProjectDocumentController::~ArasampleProjectDocumentController()
{
}

ARA::PlugIn::AudioSource* ArasampleProjectDocumentController::doCreateAudioSource (ARA::PlugIn::Document *document, ARA::ARAAudioSourceHostRef hostRef)
{
    return new ARAAudioSource (document, hostRef);
}

ARA::PlugIn::RegionSequence* ArasampleProjectDocumentController::doCreateRegionSequence (ARA::PlugIn::Document *document, ARA::ARARegionSequenceHostRef hostRef)
{
    return new ARARegionSequence (document, hostRef);
}

void ArasampleProjectDocumentController::willEnableAudioSourceSamplesAccess (ARA::PlugIn::AudioSource* audioSource, bool enable)
{
    auto source = static_cast<ARAAudioSource*> (audioSource);
    source->willEnableSamplesAccess (enable);
}

void ArasampleProjectDocumentController::didEnableAudioSourceSamplesAccess (ARA::PlugIn::AudioSource* audioSource, bool enable)
{
    auto source = static_cast<ARAAudioSource*> (audioSource);
    source->didEnableSamplesAccess (enable);
}

void ArasampleProjectDocumentController::willUpdateAudioSourceProperties (
    ARA::PlugIn::AudioSource* audioSource,
    ARA::PlugIn::PropertiesPtr<ARA::ARAAudioSourceProperties>)
{
    static_cast<ARAAudioSource*> (audioSource)->willUpdateProperties();
}

void ArasampleProjectDocumentController::didUpdateAudioSourceProperties (ARA::PlugIn::AudioSource* audioSource)
{
    static_cast<ARAAudioSource*> (audioSource)->didUpdateProperties();
}

void ArasampleProjectDocumentController::willUpdatePlaybackRegionProperties (ARA::PlugIn::PlaybackRegion* playbackRegion, ARA::PlugIn::PropertiesPtr<ARA::ARAPlaybackRegionProperties> newProperties)
{
    ARARegionSequence::willUpdatePlaybackRegionProperties (playbackRegion, newProperties);
}

void ArasampleProjectDocumentController::didUpdatePlaybackRegionProperties (ARA::PlugIn::PlaybackRegion* playbackRegion)
{
    ARARegionSequence::didUpdatePlaybackRegionProperties (playbackRegion);
}

ARA::PlugIn::EditorView *ArasampleProjectDocumentController::doCreateEditorView()
{
    return new ArasampleProjectDocumentController::Editor (this);
}


//==============================================================================
// This creates new instances of the document controller..
ARA::PlugIn::DocumentController* ARA::PlugIn::DocumentController::doCreateDocumentController ()
{
    return new ArasampleProjectDocumentController();
};


ArasampleProjectDocumentController::Editor::Editor (ARA::PlugIn::DocumentController* ctrl)
: ARA::PlugIn::EditorView (ctrl)
{
}

void ArasampleProjectDocumentController::Editor::doNotifySelection (const ARA::PlugIn::ViewSelection* currentSelection)
{
    const ScopedLock lock (selectionLock);

    removeAllChildren();
    _maxRegionSequenceLength = 0.0;
    _regionSequenceViews.clear();
    for (auto regionSequence : getDocumentController()->getDocument()->getRegionSequences())
    {
        auto regSeqView = new AudioView (*regionSequence);
        // shows all RegionSequences, highlight ones in current selection.
        for (auto selectedRegionSequence : currentSelection->getRegionSequences())
        {
            if (regionSequence == selectedRegionSequence)
            {
                regSeqView->isSelected (true);
                break;
            }
        }
        addAndMakeVisible (regSeqView);
        _regionSequenceViews.add (regSeqView);
        _maxRegionSequenceLength = std::max (_maxRegionSequenceLength, regSeqView->getStartInSecs() + regSeqView->getLengthInSecs());
    }
    resized();
}

void ArasampleProjectDocumentController::Editor::resized()
{
    int i = 0;
    const int width = getParentWidth();
    const int height = 80;
    for (auto v : _regionSequenceViews)
    {
        double normalizedStartPos = v->getStartInSecs() / _maxRegionSequenceLength;
        double normalizedLength = v->getLengthInSecs() / _maxRegionSequenceLength;
        jassert(normalizedStartPos+normalizedLength <= 1.0);
        v->setBounds (width * normalizedStartPos, height * i, width * normalizedLength, height);
        i++;
    }
    setBounds (0, 0, width, height * i);
}
