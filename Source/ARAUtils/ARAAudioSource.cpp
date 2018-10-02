#include "ARAAudioSource.h"

class ARAAudioSource::Reader : public AudioFormatReader
{
public:
    Reader (ARAAudioSource*);
    ~Reader();

    bool readSamples (
        int** destSamples,
        int numDestChannels,
        int startOffsetInDestBuffer,
        int64 startSampleInFile,
        int numSamples) override;

    void didUpdateProperties();

    std::unique_ptr<ARA::PlugIn::HostAudioReader> araHostReader;

private:
    Ref::Ptr ref_;
    std::vector<void*> tmpPtrs_;

    // When readSamples is not reading all channels,
    // we still need to provide pointers to all channels to the ARA read call.
    // So we'll read the other channels into this dummy buffer.
    std::vector<float> dummyBuffer_;
};

ARAAudioSource::ARAAudioSource (ARA::PlugIn::Document* document, ARA::ARAAudioSourceHostRef hostRef)
    : ARA::PlugIn::AudioSource (document, hostRef)
    , ref_ (new Ref (this))
{
}

ARAAudioSource::~ARAAudioSource()
{
    ScopedWriteLock l (ref_->lock);
    for (auto& reader : readers_)
        reader->araHostReader.reset();
    ref_->reset();
}

AudioFormatReader* ARAAudioSource::newReader()
{
    return new Reader (this);
}

void ARAAudioSource::didUpdateProperties()
{
    ScopedWriteLock l (ref_->lock);
    for (auto& reader : readers_)
        reader->didUpdateProperties();
}

void ARAAudioSource::willEnableSamplesAccess (bool enable)
{
    ref_->lock.enterWrite();
    if (!enable)
        for (auto& reader : readers_)
            reader->araHostReader.reset();
}

void ARAAudioSource::didEnableSamplesAccess (bool enable)
{
    if (enable)
        for (auto& reader : readers_)
            reader->araHostReader.reset (new ARA::PlugIn::HostAudioReader (this));
    ref_->lock.exitWrite();
}

ARAAudioSource::Reader::Reader (ARAAudioSource* source)
    : AudioFormatReader (nullptr, "ARAAudioSourceReader")
    , ref_ (source->ref_)
{
    if (source->isSampleAccessEnabled())
        araHostReader.reset (new ARA::PlugIn::HostAudioReader (source));
    bitsPerSample = 32;
    usesFloatingPointData = true;
    didUpdateProperties();
    ScopedWriteLock l (ref_->lock);
    source->readers_.push_back (this);
}

ARAAudioSource::Reader::~Reader()
{
    if (Ref::ScopedAccess source { ref_ })
    {
        ScopedWriteLock l (ref_->lock);
        source->readers_.erase (std::find (source->readers_.begin(), source->readers_.end(), this));
    }
}

void ARAAudioSource::Reader::didUpdateProperties()
{
    Ref::ScopedAccess source (ref_);
    if (! source)
        return;
    sampleRate = source->getSampleRate();
    numChannels = source->getChannelCount();
    lengthInSamples = source->getSampleCount();
    tmpPtrs_.resize (numChannels);
}

bool ARAAudioSource::Reader::readSamples (
    int** destSamples,
    int numDestChannels,
    int startOffsetInDestBuffer,
    int64 startSampleInFile,
    int numSamples)
{
    Ref::ScopedAccess source (ref_, true);
    if (!araHostReader)
        return false;
    for (int chan_i = 0; chan_i < tmpPtrs_.size(); ++chan_i)
        if (chan_i < numDestChannels && destSamples[chan_i] != nullptr)
            tmpPtrs_[chan_i] = (void*) (destSamples[chan_i] + startOffsetInDestBuffer);
        else
        {
            if (numSamples > dummyBuffer_.size())
                dummyBuffer_.resize (numSamples);
            tmpPtrs_[chan_i] = (void*) dummyBuffer_.data();
        }
    return araHostReader->readAudioSamples (startSampleInFile, numSamples, tmpPtrs_.data());
}
