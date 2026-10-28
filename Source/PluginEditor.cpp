#include "PluginEditor.h"
MIRRORAEditor::MIRRORAEditor(MIRRORAProcessor&p):AudioProcessorEditor(&p),proc(p){
 setResizable(true,true);setResizeLimits(520,360,1400,900);setSize(760,520);
 auto btn=[](juce::TextButton&b){b.setColour(juce::TextButton::buttonColourId,juce::Colour(0xff17151d));b.setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff7d55d8));b.setColour(juce::TextButton::textColourOffId,juce::Colour(0xffe7e1f2));};btn(simple);btn(rhythmic);addAndMakeVisible(simple);addAndMakeVisible(rhythmic);
 simple.onClick=[this]{proc.parameters.getParameter("mode")->setValueNotifyingHost(0.f);};rhythmic.onClick=[this]{proc.parameters.getParameter("mode")->setValueNotifyingHost(1.f);};
 auto sl=[this](juce::Slider&s,juce::String suf){s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,75,20);s.setTextValueSuffix(suf);s.setColour(juce::Slider::rotarySliderFillColourId,juce::Colour(0xffa77aff));s.setColour(juce::Slider::thumbColourId,juce::Colour(0xffe8d8ff));s.setColour(juce::Slider::textBoxTextColourId,juce::Colour(0xffe7e1f2));s.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour(0xff17151d));addAndMakeVisible(s);};
 sl(mix,"%");sl(length,"");sl(bpm," BPM");sl(output," dB");length.setRange(0,3,1);length.textFromValueFunction=[](double v){const juce::StringArray a{"1/4","1/2","1 Bar","2 Bars"};return a[juce::jlimit(0,3,(int)std::round(v))];};bpm.setRange(40,240,.1);output.setRange(-24,12,.1);
 sync.setColour(juce::ToggleButton::textColourId,juce::Colour(0xffd6cbe8));addAndMakeVisible(sync);
 auto lab=[this](juce::Label&l,juce::String t,int sz){l.setText(t,juce::dontSendNotification);l.setFont(juce::Font((float)sz,juce::Font::bold));l.setColour(juce::Label::textColourId,juce::Colour(0xffeee8f7));l.setJustificationType(juce::Justification::centred);addAndMakeVisible(l);};
 lab(title,"MIRRORA",25);lab(brand,"Plugin Store",13);lab(mixL,"MIX",12);lab(lengthL,"LENGTH",12);lab(bpmL,"TEMPO",12);lab(outL,"OUTPUT",12);lab(about,"About: MaaNi KaaFi",11);
 ma=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.parameters,"mix",mix);
 la=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.parameters,"length",length);
 ba=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.parameters,"bpm",bpm);
 oa=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(proc.parameters,"output",output);
 sa=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(proc.parameters,"sync",sync);
}
void MIRRORAEditor::paint(juce::Graphics&g){g.fillAll(juce::Colour(0xff0c0b10));auto a=getLocalBounds().toFloat().reduced(8);g.setColour(juce::Colour(0xff201b2a));g.fillRoundedRectangle(a,16);g.setColour(juce::Colour(0xff8b65c9).withAlpha(.8f));g.drawRoundedRectangle(a,16,1);
 g.setColour(juce::Colour(0xff15121b));g.fillRoundedRectangle(18,62,(float)getWidth()-36,(float)getHeight()*.42f,12);g.setColour(juce::Colour(0xffb18aff).withAlpha(.3f));g.drawRoundedRectangle(18,62,(float)getWidth()-36,(float)getHeight()*.42f,12,1);
 float mid=getHeight()*.27f,left=getWidth()*.1f,right=getWidth()*.9f;juce::Path p;p.startNewSubPath(left,mid);for(int i=0;i<120;++i){float x=juce::jmap((float)i,0.f,119.f,left,right),env=std::sin((float)i/119.f*juce::MathConstants<float>::pi),y=mid-env*(8.f+(float)((i*37)%29));p.lineTo(x,y);p.lineTo(x,mid+(mid-y));}g.setColour(juce::Colour(0xffa982f3).withAlpha(.65f));g.strokePath(p,juce::PathStrokeType(1.f));}
void MIRRORAEditor::resized(){auto w=getWidth(),h=getHeight();title.setBounds(24,15,w-48,38);simple.setBounds(35,75,w/3-40,52);rhythmic.setBounds(w*2/3+5,75,w/3-40,52);mixL.setBounds(w/2-55,210,110,20);mix.setBounds(w/2-65,112,130,105);
 int y=(int)(h*.61f),cw=(w-60)/4;lengthL.setBounds(25,y,cw,20);length.setBounds(25,y+18,cw,90);bpmL.setBounds(25+cw,y,cw,20);bpm.setBounds(25+cw,y+18,cw,90);sync.setBounds(25+cw,y+105,cw,25);outL.setBounds(25+2*cw,y,cw,20);output.setBounds(25+2*cw,y+18,cw,90);about.setBounds(25+3*cw,y+15,cw,40);brand.setBounds(w-145,h-32,120,18);}
