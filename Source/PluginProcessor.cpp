#include "PluginProcessor.h"
#include "PluginEditor.h"
MIRRORAProcessor::MIRRORAProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),parameters(*this,nullptr,"PARAMS",createParameterLayout()){}
juce::AudioProcessorValueTreeState::ParameterLayout MIRRORAProcessor::createParameterLayout(){
 using namespace juce; std::vector<std::unique_ptr<RangedAudioParameter>> p;
 p.push_back(std::make_unique<AudioParameterChoice>("mode","Mode",StringArray{"Simple","Rhythmic"},0));
 p.push_back(std::make_unique<AudioParameterFloat>("mix","Mix",NormalisableRange<float>(0.f,1.f,.001f),1.f));
 p.push_back(std::make_unique<AudioParameterChoice>("length","Length",StringArray{"1/4","1/2","1 Bar","2 Bars"},1));
 p.push_back(std::make_unique<AudioParameterBool>("sync","BPM Sync",true));
 p.push_back(std::make_unique<AudioParameterFloat>("bpm","BPM",NormalisableRange<float>(40.f,240.f,.1f),120.f));
 p.push_back(std::make_unique<AudioParameterFloat>("output","Output",NormalisableRange<float>(-24.f,12.f,.1f),0.f));
 return {p.begin(),p.end()};
}
void MIRRORAProcessor::prepareToPlay(double sampleRate,int){sr=sampleRate; for(auto& b:buffers)b.setSize(2,juce::roundToInt(sr*8.0),false,true,false); windowSamples=juce::roundToInt(sr*.5);writePos=0;readPos=-1;active=0;}
void MIRRORAProcessor::releaseResources(){}
bool MIRRORAProcessor::isBusesLayoutSupported(const BusesLayout& l)const{auto i=l.getMainInputChannelSet(),o=l.getMainOutputChannelSet();return i==o&&(o==juce::AudioChannelSet::mono()||o==juce::AudioChannelSet::stereo());}
void MIRRORAProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&){
 juce::ScopedNoDenormals guard; auto* mode=parameters.getRawParameterValue("mode");auto* mixP=parameters.getRawParameterValue("mix");auto* lenP=parameters.getRawParameterValue("length");auto* syncP=parameters.getRawParameterValue("sync");auto* bpmP=parameters.getRawParameterValue("bpm");auto* outP=parameters.getRawParameterValue("output");
 double bpm=bpmP->load();if(syncP->load()>.5f)if(auto* h=getPlayHead())if(auto pos=h->getPosition())if(auto hb=pos->getBpm())bpm=*hb;
 int li=juce::jlimit(0,3,(int)lenP->load());double sec=.5;
 if(mode->load()>.5f){const double beats[]={1.,2.,4.,8.};sec=60.0/juce::jmax(1.0,bpm)*beats[li];}
 windowSamples=juce::jlimit(1,buffers[0].getNumSamples(),juce::roundToInt(sec*sr));
 const float mix=juce::jlimit(0.f,1.f,mixP->load()),gain=juce::Decibels::decibelsToGain(outP->load());
 for(int n=0;n<b.getNumSamples();++n){
  for(int ch=0;ch<b.getNumChannels();++ch){float dry=b.getSample(ch,n),wet=0.f;auto& cap=buffers[(size_t)active];auto& play=buffers[(size_t)(1-active)];cap.setSample(ch,writePos,dry);if(readPos>=0&&readPos<windowSamples)wet=play.getSample(ch,windowSamples-1-readPos);b.setSample(ch,n,((1.f-mix)*dry+mix*wet)*gain);}
  ++writePos;if(readPos>=0)++readPos;
  if(writePos>=windowSamples){writePos=0;active=1-active;readPos=0;}
  if(readPos>=windowSamples)readPos=-1;
 }
}
juce::AudioProcessorEditor* MIRRORAProcessor::createEditor(){return new MIRRORAEditor(*this);}
void MIRRORAProcessor::getStateInformation(juce::MemoryBlock& d){auto s=parameters.copyState();std::unique_ptr<juce::XmlElement>x(s.createXml());copyXmlToBinary(*x,d);}
void MIRRORAProcessor::setStateInformation(const void*d,int n){std::unique_ptr<juce::XmlElement>x(getXmlFromBinary(d,n));if(x&&x->hasTagName(parameters.state.getType()))parameters.replaceState(juce::ValueTree::fromXml(*x));}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MIRRORAProcessor();}
