#include "Editor.h"
#include <BinaryData.h>
using namespace juce;
namespace {
const Colour amber{0xffffc21a},cyan{0xff20d7fa},green{0xff00d99a},red{0xffff515b},white{0xffeeeef2},muted{0xffb4b5bf};
String reading(float v,int digits=1){if(std::isnan(v))return CharPointer_UTF8("—");if(std::isinf(v))return CharPointer_UTF8("−∞");if(std::abs(v)<0.05f)v=0;return String(v,digits);}
void text(Graphics& g,String s,Rectangle<float> r,float size,Colour c=white,Justification j=Justification::centredLeft){g.setColour(c);g.setFont(Font(FontOptions(size,Font::bold)));g.drawText(s,r,j,false);}
void panel(Graphics& g,Rectangle<float> r){g.setColour(Colour(0xff050608));g.fillRoundedRectangle(r.translated(0,3),22);g.setGradientFill(ColourGradient(Colour(0xff1e2027),r.getX(),r.getY(),Colour(0xff090a0d),r.getX(),r.getBottom(),false));g.fillRoundedRectangle(r,22);g.setColour(Colour(0xff393c44));g.drawRoundedRectangle(r.reduced(1),22,1.6f);}
void grid(Graphics& g,Rectangle<float> r){g.setColour(Colour(0xff051218));g.fillRoundedRectangle(r,12);g.setColour(Colour(0xff173441));for(int i=1;i<8;++i)g.drawVerticalLine(int(r.getX()+r.getWidth()*i/8),r.getY(),r.getBottom());for(int i=1;i<4;++i)g.drawHorizontalLine(int(r.getY()+r.getHeight()*i/4),r.getX(),r.getRight());}
}
void HardwareLook::drawRotarySlider(Graphics& g,int x,int y,int w,int h,float pos,float start,float end,Slider&){
 auto r=Rectangle<float>(float(x),float(y),float(w),float(h)).reduced(5);float radius=jmin(r.getWidth(),r.getHeight())/2;auto centre=r.getCentre();
 g.setColour(Colour(0xff08090b));g.fillEllipse(centre.x-radius,centre.y-radius+3,2*radius,2*radius);
 g.setGradientFill(ColourGradient(Colour(0xff555963),centre.x-radius,centre.y-radius,Colour(0xff121419),centre.x+radius,centre.y+radius,false));g.fillEllipse(centre.x-radius,centre.y-radius,2*radius,2*radius);
 g.setColour(Colour(0xff777981));g.drawEllipse(centre.x-radius,centre.y-radius,2*radius,2*radius,1);
 Path arc;arc.addCentredArc(centre.x,centre.y,radius-3,radius-3,0,start,start+pos*(end-start),true);g.setColour(amber);g.strokePath(arc,PathStrokeType(3));
 float angle=start+pos*(end-start);g.drawLine(centre.x+std::sin(angle)*radius*0.4f,centre.y-std::cos(angle)*radius*0.4f,centre.x+std::sin(angle)*radius*0.8f,centre.y-std::cos(angle)*radius*0.8f,4);
}
void HardwareLook::drawToggleButton(Graphics& g,ToggleButton& b,bool,bool){auto r=b.getLocalBounds().toFloat().reduced(2);bool on=b.getToggleState();panel(g,r);g.setColour(on?green:Colour(0xff53565e));g.drawRoundedRectangle(r,18,1.3f);text(g,b.getButtonText(),r,14,on?green:muted,Justification::centred);}
void HardwareLook::drawButtonBackground(Graphics& g,Button& b,const Colour&,bool over,bool){g.setColour(over?Colour(0xff393d45):Colour(0xff23262d));g.fillRoundedRectangle(b.getLocalBounds().toFloat().reduced(1),6);g.setColour(Colour(0xff777b87));g.drawRoundedRectangle(b.getLocalBounds().toFloat().reduced(1),6,1);}
MasterEditor::MasterEditor(MasterProcessor& p):AudioProcessorEditor(p),processor(p){
 data=processor.analysis.snapshot();setLookAndFeel(&look);setTooltip("All meters analyse after Bus Trim and before Mono/Dim. LUFS: K-weighted loudness; short-term waits 3 seconds. dBFS: sample amplitude; dBTP: reconstructed peak. Scope and history show L/R amplitude. Width spans 0% mono to 200% anti-phase. Reset for a new programme.");logo=ImageCache::getFromMemory(BinaryData::logo_png,BinaryData::logo_pngSize);
 const char* ids[]{"trim","target","limit","calibration","scopeGain","timebase","zoom"};
 const char* names[]{"BUS TRIM","LOUDNESS TARGET","TRUE-PEAK LIMIT","VU CALIBRATION","SCOPE GAIN","TIMEBASE","DISPLAY ZOOM"};
 const char* tips[]{"Output-affecting Bus Trim, before all measurements. Double-click: 0.0 dB.","Loudness Target in LUFS. Difference = measured short-term minus target. Not a processor.","True-Peak Limit in dBTP. Status and latch only; this does not limit audio.","0 VU equals this dBFS sine peak amplitude. Ballistics are approximate, not certified VU.","Graphics only: scope amplitude multiplier. Does not alter audio.","Milliseconds across scope. Rising left-channel zero-crossing trigger; Freeze holds scope and field.","Graphics only: vertical scope and stereo-field zoom. Does not alter audio."};
 const double defaults[]{0,-14,-1,-18,1,20,1};
 for(size_t i=0;i<7;++i){auto& s=knobs[i];s.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);s.setTextBoxStyle(Slider::TextBoxBelow,false,92,22);s.setColour(Slider::textBoxTextColourId,amber);s.setColour(Slider::textBoxOutlineColourId,Colours::transparentBlack);s.setDoubleClickReturnValue(true,defaults[i]);s.setName(names[i]);s.setTooltip(tips[i]);s.setNumDecimalPlacesToDisplay(1);s.setTextValueSuffix(i==0?" dB":i==1?" LUFS":i==2?" dBTP":i==3?" dBFS":i==5?" ms":" x");addAndMakeVisible(s);labels[i].setText(names[i],dontSendNotification);labels[i].setJustificationType(Justification::centred);labels[i].setColour(Label::textColourId,muted);labels[i].setFont(Font(FontOptions(12,Font::bold)));addAndMakeVisible(labels[i]);knobLinks[i]=std::make_unique<AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s);}
 const char* bids[]{"dim","mono","active","freeze"};const char* bn[]{"DIM -20 dB","MONO SUM","ACTIVE","FREEZE SCOPE"};
 for(size_t i=0;i<4;++i){auto& b=toggles[i];b.setButtonText(CharPointer_UTF8(bn[i]));b.setName(b.getButtonText());b.setTooltip(i==0?"Output monitor: attenuates listening and exported audio by 20 dB. Measurements are before Dim.":i==1?"Output monitor: M = 0.5 x (L + R) on both outputs. Affects exported audio; measurements remain stereo.":i==2?"Off: unity dry pass-through; analysis pauses. Re-enabling begins a fresh measurement.":"Hold the scope and goniometer graphics. Audio and other measurements continue.");addAndMakeVisible(b);toggleLinks[i]=std::make_unique<AudioProcessorValueTreeState::ButtonAttachment>(p.state,bids[i],b);}
 reset.onClick=[this]{processor.analysis.reset();};peaks.onClick=[this]{processor.analysis.resetPeaks();};reset.setTooltip("Start a new programme: reset loudness, elapsed time, peaks, clipping and visual history.");peaks.setTooltip("Clear sample peak holds, clipping, true-peak maximum/exceedance and spectrum holds. Loudness continues.");addAndMakeVisible(reset);addAndMakeVisible(peaks);
 setResizable(true,true);setResizeLimits(1024,768,1920,1440);getConstrainer()->setFixedAspectRatio(4.0/3.0);setSize(p.uiWidth,p.uiHeight);startTimerHz(30);
}
MasterEditor::~MasterEditor(){setLookAndFeel(nullptr);}
void MasterEditor::timerCallback(){data=processor.analysis.snapshot();repaint(juce::Rectangle<float>(0,120,1280,696).transformedBy(juce::AffineTransform::scale(getWidth()/1280.0f,getHeight()/960.0f)).getSmallestIntegerContainer());repaint(juce::Rectangle<float>(950,850,295,90).transformedBy(juce::AffineTransform::scale(getWidth()/1280.0f,getHeight()/960.0f)).getSmallestIntegerContainer());}
void MasterEditor::resized(){
 float sx=getWidth()/1280.0f,sy=getHeight()/960.0f;
 auto place=[&](Component& c,int x,int y,int w,int h){c.setBounds(roundToInt(x*sx),roundToInt(y*sy),roundToInt(w*sx),roundToInt(h*sy));};
 for(int i=0;i<3;++i)place(toggles[size_t(i)],755+i*164,44,155,46);
 place(toggles[3],468,550,142,32);place(reset,29,780,192,28);place(peaks,231,780,172,28);
 for(int i=0;i<7;++i){place(labels[size_t(i)],27+i*130,832,128,20);place(knobs[size_t(i)],40+i*130,854,102,87);}
 processor.uiWidth=getWidth();processor.uiHeight=getHeight();drawStatic();
}
void MasterEditor::drawStatic(){
 background=Image(Image::RGB,getWidth(),getHeight(),true);Graphics g(background);g.addTransform(AffineTransform::scale(getWidth()/1280.0f,getHeight()/960.0f));
 g.setGradientFill(ColourGradient(Colour(0xffe5e6e7),0,0,Colour(0xff70737c),1280,960,false));g.fillRoundedRectangle(0,0,1280,960,35);g.setColour(Colour(0xff080a0d));g.fillRoundedRectangle(5,5,1270,950,32);
 for(auto r:{Rectangle<float>{20,24,1240,94},{20,126,298,139},{328,126,298,139},{636,126,298,139},{944,126,316,139},{20,275,606,249},{636,275,624,249},{20,538,606,231},{636,538,624,231},{20,818,1240,129}})panel(g,r);
 g.drawImageWithin(logo,31,28,300,82,RectanglePlacement::centred);
 text(g,"STEREO BUS METERING & MONITORING",{342,46,410,22},16);text(g,"POST-TRIM MEASUREMENT | OUTPUT MONO / DIM",{342,72,410,20},11,muted);
 const char* titles[]{"SHORT-TERM LOUDNESS","INTEGRATED LOUDNESS","SAMPLE PEAKS  |  dBFS","TRUE PEAK"};for(int i=0;i<4;++i)text(g,titles[i],{float(38+308*i),140,280,24},16,i==1?amber:muted);
 text(g,"TWIN VU METERS",{40,289,340,28},18);text(g,"16-BAND SPECTRUM",{654,289,360,28},18);text(g,"OSCILLOSCOPE  |  L / R",{40,550,340,26},18);text(g,"STEREO FIELD",{654,550,340,26},18);
 grid(g,{40,589,566,160});grid(g,{654,590,270,160});text(g,"OUTPUT MONITOR CONTROLS AFFECT RENDERS",{425,784,550,20},12,muted);
 grid(g,{957,854,281,72});text(g,"10-SECOND AMPLITUDE HISTORY",{954,830,290,21},12,cyan);
}
void MasterEditor::paint(Graphics& original){
 original.drawImageAt(background,0,0);Graphics::ScopedSaveState saved(original);original.addTransform(AffineTransform::scale(getWidth()/1280.0f,getHeight()/960.0f));auto& g=original;
 text(g,reading(data.shortLU)+" LUFS",{38,174,275,40},29);text(g,reading(data.integrated)+" LUFS",{346,174,275,40},29,amber);
 auto delta=std::isfinite(data.shortLU)?String(data.shortLU-processor.value("target")>=0?"+":"")+reading(data.shortLU-processor.value("target"))+" LU":String(CharPointer_UTF8("—"));
 text(g,"TARGET "+reading(processor.value("target"))+"   DIFF "+delta,{38,235,280,18},13,cyan);
 text(g,String(data.seconds,1)+" s  |  gated programme",{346,235,275,18},13,muted);
 for(int c=0;c<2;++c){float y=177+32.0f*c;float d=data.peak[size_t(c)];text(g,String(c==0?"L  ":"R  ")+reading(d),{654,y,130,25},18);text(g,"HOLD "+reading(data.hold[size_t(c)]),{787,y,133,25},12,muted);g.setColour(Colour(0xff030405));g.fillRoundedRectangle(654,y+25,263,5,2);g.setColour(d>=0?red:cyan);g.fillRoundedRectangle(654,y+25,263*jlimit(0.0f,1.0f,(d+60)/60),5,2);}
 if(data.clip)text(g,"CLIP LATCHED",{654,245,263,16},11,red);
 text(g,reading(data.truePeak)+" dBTP",{962,174,280,40},29);
 text(g,!data.signal?"NO SIGNAL":data.tpExceeded?"TP LIMIT EXCEEDED":"WITHIN TP LIMIT",{962,235,280,18},13,data.signal&&data.tpExceeded?red:green);
 text(g,"MAX | LIMIT "+reading(processor.value("limit"))+" dBTP"+(data.tpExceeded?" | LATCHED":""),{962,215,280,16},11,data.tpExceeded?red:muted);
 for(int c=0;c<2;++c){Rectangle<float> r{40+284.0f*c,327,266,178};g.setGradientFill(ColourGradient(Colour(0xff332b18),r.getX(),r.getY(),Colour(0xff07080a),r.getX(),r.getBottom(),false));g.fillRoundedRectangle(r,18);g.setColour(Colour(0xff6c5528));g.drawRoundedRectangle(r,18,1);
  Point<float> pivot{r.getCentreX(),r.getBottom()-20};float radius=126;
  for(int v=-20;v<=3;++v){float angle=jmap(float(v),-20.0f,3.0f,-1.02f,1.02f);g.setColour(v>0?red:muted);auto a=pivot.getPointOnCircumference(radius,angle),b=pivot.getPointOnCircumference(radius-7,angle);g.drawLine({a,b},2);if(v==-20||v==-10||v==-5||v==0||v==3){auto t=pivot.getPointOnCircumference(radius-21,angle);text(g,String(v),{t.x-16,t.y-8,32,18},12,v>=0?amber:muted,Justification::centred);}}
  float vu=data.vu[size_t(c)]-processor.value("calibration");float a=jmap(jlimit(-20.0f,3.0f,vu),-20.0f,3.0f,-1.02f,1.02f);g.setColour(amber);g.drawLine({pivot,pivot.getPointOnCircumference(radius-10,a)},2.2f);g.setColour(Colour(0xff666974));g.fillEllipse(pivot.x-6,pivot.y-6,12,12);text(g,c==0?"LEFT":"RIGHT",{r.getX()+13,r.getBottom()-26,75,18},12);text(g,reading(vu)+" VU",{r.getRight()-101,r.getBottom()-26,91,18},14,amber,Justification::centredRight);
 }
 text(g,"0 VU = "+reading(processor.value("calibration"))+" dBFS",{410,296,196,18},12,amber,Justification::centredRight);
 Rectangle<float> spec{688,332,552,145};for(int d=-60;d<=0;d+=20){float y=spec.getBottom()-spec.getHeight()*(d+60)/60;g.setColour(Colour(0xff343741));g.drawHorizontalLine(int(y),spec.getX(),spec.getRight());text(g,String(d),{650,y-7,33,14},11,muted);}
 const char* hz[]{"31","63","125","250","400","630","1k","1.6k","2.5k","4k","6.3k","8k","10k","12k","16k","20k"};
 for(size_t i=0;i<16;++i){float x=spec.getX()+i*34.5f;float h=spec.getHeight()*jlimit(0.0f,1.0f,(data.spectrum[i]+60)/60);g.setColour(Colour(0xff040507));g.fillRoundedRectangle(x,332,24,145,4);if(data.bandAvailable[i]){g.setColour(cyan.withAlpha(0.8f));g.fillRoundedRectangle(x,spec.getBottom()-h,24,h,3);g.setColour(amber);float y=spec.getBottom()-spec.getHeight()*jlimit(0.0f,1.0f,(data.spectrumHold[i]+60)/60);g.fillRect(x,y,24.0f,2.0f);}else text(g,CharPointer_UTF8("—"),{x,446,24,20},12,muted,Justification::centred);text(g,hz[i],{x-5,485,34,18},11,muted,Justification::centred);}
 const float gain=processor.value("scopeGain")*processor.value("zoom");
 for(int c=0;c<2;++c){g.setColour(c==0?cyan:amber.withAlpha(0.7f));const auto& s=c==0?data.scopeL:data.scopeR;for(size_t i=0;i<512;++i){float x=40+566*i/512.0f;g.drawVerticalLine(int(x),669-78*jlimit(-1.0f,1.0f,s[2*i+1]*gain),669-78*jlimit(-1.0f,1.0f,s[2*i]*gain));}}
 g.setColour(amber.withAlpha(0.65f));for(size_t i=0;i<256;++i){float side=(data.fieldL[i]-data.fieldR[i])*0.5f,mid=(data.fieldL[i]+data.fieldR[i])*0.5f;g.fillEllipse(789+jlimit(-130.0f,130.0f,side*120*processor.value("zoom")),670-jlimit(-73.0f,73.0f,mid*70*processor.value("zoom")),2,2);}
 text(g,"SIDE <   MID ^   > SIDE",{667,727,246,20},11,muted,Justification::centred);
 text(g,"PHASE CORRELATION",{945,595,287,22},15,muted);text(g,reading(data.correlation,2),{945,622,287,28},24,amber,Justification::centredRight);g.setColour(Colour(0xff40444c));g.fillRect(945,656,287,3);if(std::isfinite(data.correlation)){g.setColour(data.correlation<0?red:green);g.fillEllipse(945+(data.correlation+1)*0.5f*280,651,9,12);}text(g,"-1                         0                         +1",{945,668,287,18},11,muted);
 text(g,"STEREO WIDTH",{945,695,180,20},15,muted);text(g,reading(data.width)+" %",{1100,692,132,25},21,amber,Justification::centredRight);text(g,"0% mono | 100% equal M/S | 200% anti-phase",{945,730,295,18},10.8f,muted);
 g.setColour(cyan);for(size_t i=0;i<500;++i)g.drawVerticalLine(int(957+281*i/500.0f),890-34*jlimit(-1.0f,1.0f,data.historyMax[i]),890-34*jlimit(-1.0f,1.0f,data.historyMin[i]));
 if(data.dropped||!data.healthy)text(g,"ANALYSIS GAP / ERROR | "+String(data.dropped)+" DROPPED FRAMES",{960,932,280,13},10,red);
 if(processor.measurementPaused.load())text(g,"DRY PASS-THROUGH | MEASUREMENT PAUSED",{710,255,530,16},11,amber,Justification::centredRight);
}
