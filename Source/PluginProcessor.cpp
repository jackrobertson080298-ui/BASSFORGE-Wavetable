#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>
const std::array<Patch,30> factoryPatches = {{
 {"01 Reactor",.65f,.28f,.17f,.70f,.25f,.27f,.72f,.65f,0},
 {"02 Iron Foghorn",.80f,.38f,.06f,.43f,.18f,.58f,.34f,.23f,1},
 {"03 Voltage Reese",.56f,.14f,.46f,.56f,.37f,.60f,.52f,.94f,0},
 {"04 FM Punisher",.88f,.83f,.10f,.78f,.20f,.13f,.83f,.35f,2},
 {"05 Skyline",.43f,.17f,.29f,.84f,.11f,.47f,.92f,.83f,0},
 {"06 Neuro Pulse",.78f,.64f,.16f,.62f,.91f,.30f,.73f,.52f,2},
 {"07 Sub Destroyer",.91f,.18f,.08f,.35f,.17f,.44f,.22f,.16f,1},
 {"08 Razor Edge",.85f,.53f,.11f,.91f,.48f,.18f,.99f,.41f,2},
 {"09 Dark Matter",.73f,.48f,.34f,.39f,.62f,.82f,.42f,.80f,1},
 {"10 Drop Hammer",.95f,.39f,.04f,.65f,.13f,.09f,.68f,.20f,2},
 {"11 Titan Horn",.72f,.32f,.12f,.52f,.23f,.64f,.47f,.38f,1},
 {"12 Saw Riot",.79f,.12f,.31f,.93f,.34f,.31f,.96f,.79f,0},
 {"13 Metallic Bite",.84f,.92f,.05f,.82f,.44f,.16f,.89f,.29f,2},
 {"14 Dirty Donk",.90f,.73f,.03f,.68f,.10f,.12f,.65f,.14f,1},
 {"15 Wide Collision",.61f,.27f,.49f,.77f,.39f,.53f,.78f,1.f,0},
 {"16 Ghost Reese",.48f,.22f,.38f,.36f,.58f,.73f,.29f,.89f,0},
 {"17 Shredder",.98f,.71f,.15f,.96f,.69f,.22f,.95f,.59f,2},
 {"18 Rumble",.74f,.11f,.22f,.25f,.32f,.69f,.16f,.45f,1},
 {"19 Ascension",.55f,.33f,.25f,.88f,.75f,.46f,.86f,.88f,0},
 {"20 Final Impact",.99f,.59f,.08f,.73f,.26f,.11f,.77f,.30f,2},
 {"21 Punisher XL",.95f,.88f,.18f,.87f,.32f,.24f,.91f,.68f,2,.54f,.60f},
 {"22 Punisher Stab",.93f,.91f,.05f,.91f,.12f,.06f,.95f,.22f,2,.96f,.48f},
 {"23 Punisher Reese",.81f,.79f,.39f,.59f,.59f,.53f,.68f,.90f,2,.27f,.61f},
 {"24 Punisher Scream",.98f,.99f,.08f,.99f,.77f,.20f,1.f,.55f,2,.64f,.98f},
 {"25 Punisher Foghorn",.92f,.70f,.14f,.53f,.28f,.79f,.52f,.50f,1,.37f,.58f},
 {"26 Punisher Laser",.91f,.95f,.02f,.97f,.85f,.08f,.98f,.28f,2,.90f,.83f},
 {"27 Punisher Wobble",.86f,.84f,.28f,.66f,.98f,.36f,.76f,.71f,2,.36f,.73f},
 {"28 Punisher Crusher",1.f,.90f,.06f,.72f,.38f,.14f,.87f,.34f,2,.87f,.92f},
 {"29 Punisher Shimmer",.76f,.71f,.31f,.93f,.48f,.55f,.99f,.97f,0,.29f,.65f},
 {"30 Punisher Final Boss",1.f,1.f,.15f,.84f,.69f,.31f,.94f,.80f,2,1.f,1.f}
}};
const char* BassforgeProcessor::fxNames[BassforgeProcessor::fxCount]={"DISTORTION","FILTER","DYNAMICS","EQ","CHORUS","DELAY","REVERB"};
juce::AudioProcessorValueTreeState::ParameterLayout BassforgeProcessor::layout(){
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 for(auto id:{"drive","fm","detune","cutoff","movement","release","tone","width","sub","output","punch","rasp"})
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},juce::String(id).toUpperCase(),0.f,1.f,(juce::String(id)=="sub"?.65f:juce::String(id)=="output"?.65f:.5f)));
 for(int i=0;i<fxCount;++i){
  auto prefix="fx"+juce::String(i);
  p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{prefix+"on",1},prefix+" ENABLED",i<4));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{prefix+"mix",1},prefix+" MIX",0.f,1.f,i<4?0.65f:0.20f));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{prefix+"amt",1},prefix+" AMOUNT",0.f,1.f,0.5f));
 }
 // Three independent tempo-synced LFOs; depth defaults to zero to preserve older projects.
 for(int i=0;i<3;++i){
  auto id="lfo"+juce::String(i+1);
  p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"rate",1},id+" RATE",juce::StringArray{"1/16","1/8","1/4","1/2","1 BAR","2 BARS"},2));
  p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"shape",1},id+" SHAPE",juce::StringArray{"SINE","TRIANGLE","SAW","SQUARE","RAMP","CUSTOM"},0));
  p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"mode",1},id+" MODE",juce::StringArray{"RETRIGGER","FREE RUN","ONE SHOT"},0));
  p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"target",1},id+" TARGET",juce::StringArray{"FM","CUTOFF","DRIVE","RASP","MOVEMENT","PITCH"},0));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id+"depth",1},id+" DEPTH",0.f,1.f,0.f));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id+"keytrack",1},id+" KEY TRACK",-1.f,1.f,0.f));
 }
 // v2.4: 16-point editable custom LFO curves. Stored as normal APVTS parameters,
 // so patches and DAW sessions persist every point without external files.
 for(int k=1;k<=3;++k)for(int j=0;j<16;++j){
  auto id="lfo"+juce::String(k)+"point"+juce::String(j);
  const float t=(float)j/16.f;
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},id.toUpperCase(),0.f,1.f,0.5f+0.5f*std::sin(juce::MathConstants<float>::twoPi*t)));
 }
 // v2: editable additive partials, a second oscillator and a selectable character mix.
 // Eight harmonics are synthesised directly; harmonics above Nyquist are excluded.
 for(int i=1;i<=8;++i){auto id="partial"+juce::String(i);p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},"PARTIAL "+juce::String(i),0.f,1.f,i==1?1.f:i==2?.35f:i==3?.18f:0.f));}
 // v3.1 advanced oscillator and routing parameters.
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wtcustom",1},"CUSTOM WAVETABLE MIX",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"univoices",1},"UNISON VOICES",juce::StringArray{"1","2","3","4","5","6","7","8"},0));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"unidetune",1},"UNISON DETUNE",0.f,1.f,.15f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"unispread",1},"UNISON STEREO",0.f,1.f,.7f));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"filtertype",1},"FILTER TYPE",juce::StringArray{"LOW PASS","HIGH PASS","BAND PASS"},0));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"filterres",1},"FILTER RESONANCE",0.f,1.f,.1f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"filtermix",1},"FILTER MIX",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"fmquality",1},"FM ANTI-ALIAS",0.f,1.f,0.f));
 // Eight routable slots, each with source, destination and bipolar depth.
 for(int i=0;i<8;++i){auto id="matrix"+juce::String(i+1);
  p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"src",1},id+" SOURCE",juce::StringArray{"OFF","LFO 1","LFO 2","LFO 3","VELOCITY","KEYTRACK"},0));
  p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{id+"dst",1},id+" DEST",juce::StringArray{"FM","CUTOFF","DRIVE","RASP","WT A","WT B","PITCH","FILTER RES"},0));
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id+"amt",1},id+" AMOUNT",-1.f,1.f,0.f));
 }
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wtmix",1},"WAVETABLE MIX",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wtposition",1},"WAVETABLE POSITION",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wt2mix",1},"OSC B WAVETABLE MIX",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"wt2position",1},"OSC B WAVETABLE POSITION",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osc2level",1},"OSC 2 LEVEL",0.f,1.f,.0f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osc2tune",1},"OSC 2 TUNE",0.f,1.f,.5f));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"osc2wave",1},"OSC 2 WAVE",juce::StringArray{"SINE","TRIANGLE","SAW","SQUARE"},0));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"additivemix",1},"WAVE EDITOR MIX",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oscpwm",1},"OSC A PULSE WIDTH",0.05f,0.95f,0.5f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"osc2pwm",1},"OSC B PULSE WIDTH",0.05f,0.95f,0.5f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oscsync",1},"OSC B HARD SYNC",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"portamento",1},"PORTAMENTO",juce::NormalisableRange<float>(0.f,2.f,0.001f,0.4f),0.f));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"portamode",1},"GLIDE MODE",juce::StringArray{"ALWAYS","LEGATO ONLY"},1));
 // v2.3: polyphonic envelope and voice allocation controls.
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"voicemode",1},"VOICE MODE",juce::StringArray{"POLY","MONO","LEGATO"},0));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"voicecount",1},"POLY VOICES",juce::StringArray{"2","4","6","8","12"},4));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ampattack",1},"AMP ATTACK",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ampdecay",1},"AMP DECAY",0.f,1.f,.35f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"ampsustain",1},"AMP SUSTAIN",0.f,1.f,1.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"amprelease",1},"AMP RELEASE",0.f,1.f,.2f));
 // v1.1 oscillator controls are opt-in through engine10; defaults match v1.0.
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oscratio",1},"MOD RATIO",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oscwarp",1},"PHASE WARP",0.f,1.f,0.f));
 p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{"oscmix",1},"OSC BLEND",0.f,1.f,1.f));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"oscwave",1},"OSC WAVE",juce::StringArray{"SINE","SAW","METAL","TRIANGLE","SQUARE","PULSE"},2));
 // v1.0 is opt-in: legacy factory patches and projects retain their sound.
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"engine10",1},"DUAL LAYER ENGINE",false));
 for(auto id:{"bodylevel","characterlevel","split","harmonics","bodyattack"})
  p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},juce::String(id).toUpperCase(),0.f,1.f,
   juce::String(id)=="bodylevel"?.75f:juce::String(id)=="characterlevel"?.55f:juce::String(id)=="split"?.4f:juce::String(id)=="harmonics"?.55f:.65f));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"precision",1},"PRECISION ENGINE",false));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"fxbypass",1},"FX BYPASS",false));
 p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"shape",1},"SHAPE",juce::StringArray{"Saw","Horn","Metal"},0));
 return p;
}
BassforgeProcessor::BassforgeProcessor():AudioProcessor(BusesProperties().withOutput("Output",juce::AudioChannelSet::stereo(),true)),params(*this,nullptr,"PARAMS",layout()){for(int i=0;i<fxCount;++i)fxOrder[(size_t)i].store(i);setCurrentProgram(30);
 WavetableEngine::Frame initial{};
 for(int i=0;i<WavetableEngine::size;++i)initial[(size_t)i]=std::sin(juce::MathConstants<double>::twoPi*i/WavetableEngine::size);
 wavetable.setCustomFrame(initial);
}
void BassforgeProcessor::prepareToPlay(double sr,int){
 sampleRateHz=sr;for(auto& v:voices)v=Voice{};
 for(int ch=0;ch<2;++ch){delayBuffer[ch].assign((size_t)(sr*2.1),0.f);reverbBuffer[ch].assign((size_t)(sr*.65),0.f);chorusBuffer[ch].assign((size_t)(sr*.09),0.f);}
 delayPos=reverbPos=chorusPos=0;fxClock=0.0;lpFx.fill(0);hpFx.fill(0);eqLow.fill(0);eqHigh.fill(0);compEnv.fill(0);revLp.fill(0);
}
void BassforgeProcessor::moveFx(int slot,int delta){
 const int other=slot+delta;if(slot<0||slot>=fxCount||other<0||other>=fxCount)return;
 int a=fxOrder[(size_t)slot].load(),b=fxOrder[(size_t)other].load();fxOrder[(size_t)slot].store(b);fxOrder[(size_t)other].store(a);
}
float BassforgeProcessor::processFx(int effect,float x,int ch,float amount,float time,float rate){
 const float pi=juce::MathConstants<float>::pi;
 auto smooth=[rate](float hz){return 1.f-std::exp(-2.f*juce::MathConstants<float>::pi*hz/rate);};
 switch(effect){
 case 0: return std::tanh(x*(1.f+amount*20.f)) / std::tanh(1.f+amount*20.f);
 case 1: {float hz=130.f+amount*14000.f;lpFx[ch]+=smooth(hz)*(x-lpFx[ch]);return lpFx[ch];}
 case 2: { // two-band upward saturation / gentle dynamic control
   float e=compEnv[ch];e+=(std::abs(x)-e)*(std::abs(x)>e?0.015f:0.0008f);compEnv[ch]=e;
   float gain=1.f/(1.f+amount*3.5f*e);return std::tanh(x*gain*(1.f+amount*2.f));
 }
 case 3: {eqLow[ch]+=smooth(210.f)*(x-eqLow[ch]);eqHigh[ch]+=smooth(3000.f)*(x-eqHigh[ch]);return x+amount*(0.55f*eqLow[ch]+0.65f*(x-eqHigh[ch])-.25f*(eqHigh[ch]-eqLow[ch]));}
 case 4: {auto& b=chorusBuffer[ch];if(b.empty())return x;int len=(int)b.size();int d=(int)((0.011f+0.006f*std::sin(2*pi*(0.18f+amount*.9f)*time+ch*1.5f))*rate);int ix=(chorusPos-d+len*2)%len;float y=b[(size_t)ix];b[(size_t)chorusPos]=x;return y;}
 case 5: {auto& b=delayBuffer[ch];if(b.empty())return x;int len=(int)b.size();int d=juce::jlimit(1,len-1,(int)((.12f+amount*.55f)*rate));int ix=(delayPos-d+len*2)%len;float y=b[(size_t)ix];b[(size_t)delayPos]=x+0.38f*y;return y;}
 case 6: {auto& b=reverbBuffer[ch];if(b.empty())return x;int len=(int)b.size();int d=juce::jlimit(1,len-1,(int)((.071f+ch*.017f+amount*.19f)*rate));int ix=(reverbPos-d+len*2)%len;float y=b[(size_t)ix];revLp[ch]+=.22f*(y-revLp[ch]);b[(size_t)reverbPos]=x+revLp[ch]*(0.45f+amount*.35f);return y;}
 }
 return x;
}
void BassforgeProcessor::noteOn(int note,float vel){
 const int mode=(int)params.getRawParameterValue("voicemode")->load();
 constexpr int counts[]={2,4,6,8,12};
 const int limit=mode==0?counts[juce::jlimit(0,4,(int)params.getRawParameterValue("voicecount")->load())]:1;
 if(mode!=0){
  for(int i=0;i<heldCount;++i)if(heldNotes[(size_t)i]==note){for(int j=i;j<heldCount-1;++j)heldNotes[(size_t)j]=heldNotes[(size_t)j+1];--heldCount;break;}
  if(heldCount<(int)heldNotes.size())heldNotes[(size_t)heldCount++]=note;
  auto& v=voices[0];const bool connected=v.active&&!v.releasing&&heldCount>1;
  const bool glide=params.getRawParameterValue("portamento")->load()>0.f&&((int)params.getRawParameterValue("portamode")->load()==0||connected);
  const double from=v.active?v.currentPitch:(double)note;
  if(mode==2&&connected){v.note=note;v.velocity=vel;v.targetPitch=note;v.currentPitch=glide?from:note;return;}
  v=Voice{};v.active=true;v.note=note;v.velocity=vel;v.targetPitch=note;v.currentPitch=glide?from:note;
  for(size_t i=1;i<voices.size();++i)voices[i].active=false;return;
 }
 Voice* v=nullptr;for(int i=0;i<limit;++i)if(!voices[(size_t)i].active){v=&voices[(size_t)i];break;}
 if(!v){v=&voices[0];for(int i=1;i<limit;++i){auto& c=voices[(size_t)i];if((c.releasing&&!v->releasing)||(c.releasing==v->releasing&&c.env<v->env))v=&c;}}
 *v=Voice{};v->active=true;v->note=note;v->velocity=vel;v->currentPitch=note;v->targetPitch=note;
}
void BassforgeProcessor::noteOff(int note){
 const int mode=(int)params.getRawParameterValue("voicemode")->load();
 if(mode!=0){
  for(int i=0;i<heldCount;++i)if(heldNotes[(size_t)i]==note){for(int j=i;j<heldCount-1;++j)heldNotes[(size_t)j]=heldNotes[(size_t)j+1];--heldCount;break;}
  auto& v=voices[0];if(v.active&&v.note==note){if(heldCount>0){v.note=heldNotes[(size_t)heldCount-1];v.targetPitch=v.note;if(params.getRawParameterValue("portamento")->load()<=0.f)v.currentPitch=v.targetPitch;v.releasing=false;}else v.releasing=true;}return;
 }
 for(auto& v:voices)if(v.active&&v.note==note)v.releasing=true;
}
void BassforgeProcessor::processBlock(juce::AudioBuffer<float>& out,juce::MidiBuffer& midi){
 juce::ScopedNoDenormals noDenormals;
 keyboardState.processNextMidiBuffer(midi, 0, out.getNumSamples(), true);
 out.clear();
 auto get=[this](const char* id){return params.getRawParameterValue(id)->load();};
 const float drive=get("drive"),fm=get("fm"),detune=get("detune"),cutoff=get("cutoff"),movement=get("movement"),release=get("release"),tone=get("tone"),width=get("width"),sub=get("sub"),output=get("output");
 const float punch=get("punch"), rasp=get("rasp");
 const float ampAttack=get("ampattack"),ampDecay=get("ampdecay"),ampSustain=get("ampsustain"),ampRelease=get("amprelease");
 const double attackSec=.001+ampAttack*ampAttack*3.0;
 const double decaySec=.008+ampDecay*ampDecay*4.0;
 const double releaseSec=.008+ampRelease*ampRelease*5.0;
 const double sustainLevel=ampSustain;

 const bool precision=get("precision")>0.5f;
 const bool engine10=get("engine10")>0.5f;
 const float bodyLevel=get("bodylevel"), characterLevel=get("characterlevel"), split=get("split"), harmonics=get("harmonics"), bodyAttack=get("bodyattack");
 const int shape=(int)get("shape");
 // Host BPM fallback is 174 for standalone operation. Read transport once per block.
 double bpm=174.0;
 if(auto* ph=getPlayHead())if(auto position=ph->getPosition())if(auto tempo=position->getBpm())if(*tempo>20.0&&*tempo<400.0)bpm=*tempo;
 struct ModConfig {int rate,shape,mode,target;float depth,keytrack;double hz;};
 std::array<ModConfig,3> mods{};
 std::array<std::array<float,16>,3> customLfo{};
 for(int k=0;k<3;++k)for(int j=0;j<16;++j)
  customLfo[(size_t)k][(size_t)j]=params.getRawParameterValue("lfo"+juce::String(k+1)+"point"+juce::String(j))->load();
 const double beatHz=bpm/60.0;
 const double cyclesPerBeat[]={4.0,2.0,1.0,0.5,0.25,0.125};
 for(int k=0;k<3;++k){auto id="lfo"+juce::String(k+1);auto& m=mods[(size_t)k];
  m.rate=(int)params.getRawParameterValue(id+"rate")->load();m.shape=(int)params.getRawParameterValue(id+"shape")->load();
  m.mode=(int)params.getRawParameterValue(id+"mode")->load();m.target=(int)params.getRawParameterValue(id+"target")->load();
  m.depth=params.getRawParameterValue(id+"depth")->load();m.keytrack=params.getRawParameterValue(id+"keytrack")->load();m.hz=beatHz*cyclesPerBeat[juce::jlimit(0,5,m.rate)];
 }

 auto wave=[](double phase,int type){const double s=std::sin(phase*juce::MathConstants<double>::twoPi);if(type==0)return 2.0*(phase-std::floor(phase))-1.0;if(type==1)return std::sin(phase*juce::MathConstants<double>::twoPi)+.35*std::sin(phase*juce::MathConstants<double>::twoPi*2.0);return std::sin(phase*juce::MathConstants<double>::twoPi)+.6*std::sin(phase*juce::MathConstants<double>::twoPi*3.0);};
 juce::MidiBuffer::Iterator it(midi);juce::MidiMessage msg;int eventPos=0;bool has=it.getNextEvent(msg,eventPos);
 for(int i=0;i<out.getNumSamples();++i){while(has&&eventPos<=i){if(msg.isNoteOn())noteOn(msg.getNoteNumber(),msg.getFloatVelocity());else if(msg.isNoteOff())noteOff(msg.getNoteNumber());else if(msg.isAllNotesOff()||msg.isAllSoundOff()){heldCount=0;for(auto& v:voices)v.releasing=true;}has=it.getNextEvent(msg,eventPos);}
 double l=0,r=0;
 for(auto& v:voices){if(!v.active)continue;
 const double glideTime=params.getRawParameterValue("portamento")->load();
 if(glideTime<=0.0001)v.currentPitch=v.targetPitch;else v.currentPitch+=(v.targetPitch-v.currentPitch)*(1.0-std::exp(-1.0/(sampleRateHz*glideTime)));
 const double freq=440.0*std::pow(2.0,(v.currentPitch-69.0)/12.0);
 // Modulation is per-voice; one-shot holds its final value, retrigger resets on note-on.
 double offsets[8]{};
 double matrixLfo[3]{};
 for(int k=0;k<3;++k){const auto& m=mods[(size_t)k];
  // C4 (MIDI 60) is the reference pitch. Positive tracking doubles rate per octave at 100%.
  const double noteHz=m.hz*std::pow(2.0,juce::jlimit(-4.0,4.0,(v.note-60)*m.keytrack/12.0));
  double t=v.modPhase[(size_t)k];
  if(m.mode==1)t=std::fmod(fxClock*noteHz,1.0); // free-running, common phase
  const double pi=juce::MathConstants<double>::twoPi;
  double value=0;
  switch(m.shape){case 0:value=std::sin(pi*t);break;
   case 1:value=1.0-4.0*std::abs(t-0.5);break;
   case 2:value=1.0-2.0*t;break;
   case 3:value=t<0.5?1.0:-1.0;break;
   case 4:value=2.0*t-1.0;break;
   default:{
    const double pos=juce::jlimit(0.0,15.999999,t*16.0);
    const int a=(int)pos,b=(a+1)%16;
    const double frac=pos-a;
    value=2.0*(customLfo[(size_t)k][(size_t)a]*(1.0-frac)+customLfo[(size_t)k][(size_t)b]*frac)-1.0;
    break;
   }}
  matrixLfo[k]=value;
  offsets[juce::jlimit(0,5,m.target)]+=value*m.depth;
  if(m.mode!=1){v.modPhase[(size_t)k]+=noteHz/sampleRateHz;
   if(m.mode==2)v.modPhase[(size_t)k]=juce::jmin(1.0-1e-8,v.modPhase[(size_t)k]);
   else v.modPhase[(size_t)k]-=std::floor(v.modPhase[(size_t)k]);}
 }
 // Eight-slot bipolar matrix, per-voice modulation.
 for(int k=1;k<=8;++k){
  const auto id="matrix"+juce::String(k);
  const int source=(int)params.getRawParameterValue(id+"src")->load();
  if(source==0)continue;
  const int dest=(int)params.getRawParameterValue(id+"dst")->load();
  const double amount=params.getRawParameterValue(id+"amt")->load();
  const double value=source<=3?matrixLfo[source-1]:source==4?2.0*v.velocity-1.0:juce::jlimit(-1.0,1.0,(v.note-60)/48.0);
  const int mapping[8]={0,1,2,3,6,7,5,4};
  offsets[mapping[juce::jlimit(0,7,dest)]]+=value*amount;
 }
 const double modFm=juce::jlimit(0.0,1.0,(double)fm+offsets[0]*.45);
 const double modCutoff=juce::jlimit(0.0,1.0,(double)cutoff+offsets[1]*.5);
 const double modDrive=juce::jlimit(0.0,1.0,(double)drive+offsets[2]*.45);
 const double modRasp=juce::jlimit(0.0,1.0,(double)rasp+offsets[3]*.45);
 const double modMovement=juce::jlimit(0.0,1.0,(double)movement+offsets[4]*.5);
 const double delta=freq*std::pow(2.0,offsets[5]*2.0/12.0)/sampleRateHz;
 static const std::array<juce::String,8> partialIds={"partial1","partial2","partial3","partial4","partial5","partial6","partial7","partial8"};
 // v1.0 dual-layer path: separate body oscillator and 2x band-limited character.
 // All voice memory is fixed-size. No heap allocation or wavetable duplication in audio callback.
 if(engine10){
  constexpr double tau=juce::MathConstants<double>::twoPi;
  const double baseFreq=juce::jmin(freq*std::pow(2.0,offsets[5]*2.0/12.0),sampleRateHz*.42);
  const double dt=baseFreq/(sampleRateHz*2.0);
  const double crossover=85.0+split*split*1250.0;
  const double bodyHz=juce::jmin(sampleRateHz*.4,crossover*1.6);
  const double aBody=1.0-std::exp(-tau*bodyHz/sampleRateHz);
  const double aHP=1.0-std::exp(-tau*crossover/(sampleRateHz*2.0));
  if(!v.releasing){
   if(v.age<attackSec)v.env=juce::jmin(1.f,v.env+(float)(1.0/(sampleRateHz*attackSec)));
   else v.env+=(float)((sustainLevel-v.env)*(1.0-std::exp(-1.0/(sampleRateHz*decaySec))));
  } else v.env*=std::exp(-1.0/(sampleRateHz*releaseSec));
  if(v.env<.0001f&&v.releasing){v.active=false;continue;}
  // PolyBLEP discontinuity correction on saw oscillator, with Nyquist-aware harmonic control.
  auto polyblep=[](double t,double d){if(t<d){t/=d;return t+t-t*t-1.0;}if(t>1.0-d){t=(t-1.0)/d;return t*t+t+t+1.0;}return 0.0;};
  double charOut=0.0;
  for(int os=0;os<2;++os){
   const double t=v.phase;
   const double ratio=1.0+params.getRawParameterValue("oscratio")->load()*7.0;
   const double modulator=std::sin(tau*v.phase2*ratio);
   const double quality=params.getRawParameterValue("fmquality")->load();
   const double index=modFm*(.10+.24*harmonics)+params.getRawParameterValue("oscwarp")->load()*.18;
   const double nyquistGuard=juce::jlimit(0.0,1.0,(sampleRateHz*.42/baseFreq-1.0)/(1.0+ratio*index*8.0));
   const double warped=t+index*(1.0-quality+quality*nyquistGuard)*modulator;
   const double wp=warped-std::floor(warped);
   const double saw=2.0*wp-1.0-polyblep(wp,juce::jmin(.49,dt*(1.0+modFm*4.0)));
   const double sine=std::sin(tau*warped);
   const double metal=std::sin(tau*(warped+(.1+.32*harmonics)*std::sin(tau*v.phase2*2.0)));
   const float mix=params.getRawParameterValue("oscmix")->load();
   const int waveSelect=(int)params.getRawParameterValue("oscwave")->load();
   const double pulseWidth=params.getRawParameterValue("oscpwm")->load();
   const double pwEdge=std::fmod(wp+1.0-pulseWidth,1.0);
   const double pulse=(wp<pulseWidth?1.0:-1.0)+polyblep(wp,juce::jmin(.49,dt))-polyblep(pwEdge,juce::jmin(.49,dt));
   double tri=0.0;
   for(int h=1;h<=15;h+=2){
    if(baseFreq*h>=sampleRateHz*.47)break;
    const double sign=(h%4==1)?1.0:-1.0;
    tri+=sign*std::sin(tau*wp*h)/(h*h);
   }
   tri*=8.0/(juce::MathConstants<double>::pi*juce::MathConstants<double>::pi);
   const double character=waveSelect==0?sine:waveSelect==1?saw:waveSelect==2?(.58*saw+.42*metal):waveSelect==3?tri:waveSelect==4?((wp<.5?1.0:-1.0)+polyblep(wp,juce::jmin(.49,dt))-polyblep(std::fmod(wp+.5,1.0),juce::jmin(.49,dt))):pulse;
   // User-editable 8-partial oscillator. Explicit Nyquist culling prevents
   // additive partials from aliasing; no per-voice wavetable allocation.
   double custom=0.0, total=0.0;
   for(int h=1;h<=8;++h){
    if(baseFreq*h < sampleRateHz*.47){
     const double gain=params.getRawParameterValue(partialIds[(size_t)(h-1)])->load();
     custom+=gain*std::sin(tau*wp*h);total+=gain;
    }
   }
   if(total>1.e-6)custom/=std::sqrt(total);
   const double editorMix=params.getRawParameterValue("additivemix")->load();
   const double oscillatorLegacy=(1.0-editorMix)*((1.0-harmonics)*sine+harmonics*((1.0-mix)*saw+mix*character))+editorMix*custom;
   const float wtMix=params.getRawParameterValue("wtmix")->load();
   const double wtA=wavetable.sample(wp,baseFreq,sampleRateHz,juce::jlimit(0.f,1.f,params.getRawParameterValue("wtposition")->load()+(float)offsets[6]*.4f));
   const double baseOsc=oscillatorLegacy*(1.0-wtMix)+wtA*wtMix;
   const float customMix=params.getRawParameterValue("wtcustom")->load();
   const double oscillator=baseOsc*(1.0-customMix)+wavetable.customSample(wp,baseFreq,sampleRateHz)*customMix;
   // Second oscillator: fixed waveforms, coarse tune +/- 12 semitones.
   const double osc2level=params.getRawParameterValue("osc2level")->load();
   const double semis=(params.getRawParameterValue("osc2tune")->load()-.5)*24.0;
   const double osc2freq=baseFreq*std::pow(2.0,semis/12.0);
   const int osc2wave=(int)params.getRawParameterValue("osc2wave")->load();
   const double p2=v.osc2phase;
   const double dt2=osc2freq/(sampleRateHz*2.0);
   double second=std::sin(tau*p2);
   if(osc2wave==1){
    second=0.0;
    for(int h=1;h<=15;h+=2){
     if(osc2freq*h>=sampleRateHz*.47)break;
     second+=((h%4==1)?1.0:-1.0)*std::sin(tau*p2*h)/(h*h);
    }
    second*=8.0/(juce::MathConstants<double>::pi*juce::MathConstants<double>::pi);
   }
   if(osc2wave==2)second=2.0*p2-1.0-polyblep(p2,juce::jmin(.49,dt2));
   if(osc2wave==3){const double pulseB=params.getRawParameterValue("osc2pwm")->load();const double shifted=std::fmod(p2+1.0-pulseB,1.0);second=(p2<pulseB?1.0:-1.0)+polyblep(p2,juce::jmin(.49,dt2))-polyblep(shifted,juce::jmin(.49,dt2));}
   const float wt2Mix=params.getRawParameterValue("wt2mix")->load();
   const double wtB=wavetable.sample(p2,osc2freq,sampleRateHz,juce::jlimit(0.f,1.f,params.getRawParameterValue("wt2position")->load()+(float)offsets[7]*.4f));
   second=second*(1.0-wt2Mix)+wtB*wt2Mix;
   v.osc2phase+=dt2;v.osc2phase-=std::floor(v.osc2phase);
   const double source=(oscillator+osc2level*second)/(1.0+0.6*osc2level);
   const double preGain=0.85+modDrive*4.0+modRasp*1.3;
   const double sat=std::tanh(source*preGain)/(0.8+0.2*preGain);
   // High-pass only the character: the independent body/sub stays clean and centred.
   v.charHP+=aHP*(sat-v.charHP);
   const float high=(float)(sat-v.charHP);
   v.decim[(size_t)v.decimIndex]=high;
   v.decimIndex=(v.decimIndex+1)%9;
   v.phase+=dt;
   if(v.phase>=1.0){v.phase-=std::floor(v.phase);if(params.getRawParameterValue("oscsync")->load()>.5f)v.osc2phase=0.0;}
   v.phase2+=dt*(1.0+.018*detune);v.phase2-=std::floor(v.phase2);
  }
  // 9-tap lowpass FIR downsampler; fixed coefficients, 2x nonlinear synthesis.
  constexpr float fir[9]={0.f,-.045f,0.f,.295f,.5f,.295f,0.f,-.045f,0.f};
  for(int k=0;k<9;++k)charOut+=fir[k]*v.decim[(size_t)((v.decimIndex+8-k+9)%9)];
  const double bodyRaw=std::sin(tau*v.subPhase);
  v.bodyLP+=aBody*(bodyRaw-v.bodyLP);
  const double body=(bodyRaw*.70+v.bodyLP*.30)*bodyLevel;
  const double transient=1.0+(bodyAttack*.38)*std::exp(-v.age*95.0);
  const double amp=v.env*v.velocity*transient;
  // Character low-pass: tone is now an actual spectral control, not just volume.
  // Cascaded one-pole stages reduce harshness without touching the clean sub.
  const double toneHz=juce::jmin(sampleRateHz*.42,700.0+tone*tone*13500.0);
  const double toneAlpha=1.0-std::exp(-tau*toneHz/sampleRateHz);
  v.toneLP1+=toneAlpha*(charOut-v.toneLP1);
  v.toneLP2+=toneAlpha*(v.toneLP1-v.toneLP2);
  const double ch=v.toneLP2*characterLevel;
  const double low=body*amp;
  const double high=ch*amp;
  // Topology-preserving state-variable filter (TPT SVF), per channel.
  const double fc=std::min(sampleRateHz*.45,35.0*std::pow(2.0,juce::jlimit(0.0,1.0,modCutoff)*9.0));
  const double g=std::tan(juce::MathConstants<double>::pi*fc/sampleRateHz);
  const double q=0.5+juce::jlimit(0.0,1.0,(double)params.getRawParameterValue("filterres")->load()+offsets[4]*.35)*13.0;
  const double k=1.0/q;
  const double a1=1.0/(1.0+g*(g+k)),a2=g*a1,a3=g*a2;
  const int filterType=(int)params.getRawParameterValue("filtertype")->load();
  const double filterMix=params.getRawParameterValue("filtermix")->load();
  auto filter=[&](double x,double& ic1,double& ic2){
   const double v3=x-ic2,v1=a1*ic1+a2*v3,v2=ic2+a2*ic1+a3*v3;
   ic1=2.0*v1-ic1;ic2=2.0*v2-ic2;
   const double wet=filterType==0?v2:filterType==1?(x-k*v1-v2):v1;
   return x+(wet-x)*filterMix;
  };
  const int unison=1+(int)params.getRawParameterValue("univoices")->load();
  const double uniDetune=params.getRawParameterValue("unidetune")->load();
  const double spread=params.getRawParameterValue("unispread")->load();
  double uL=0.0,uR=0.0;
  for(int u=0;u<unison;++u){
   const double pan=unison==1?0.0:(2.0*u/(unison-1)-1.0)*spread;
   const double cents=unison==1?0.0:(2.0*u/(unison-1)-1.0)*uniDetune*35.0;
   const double ph=v.uniPhase[(size_t)u];
   const double uni=unison==1?high:(wavetable.sample(ph,baseFreq*std::pow(2.0,cents/1200.0),sampleRateHz,
      params.getRawParameterValue("wtposition")->load())*amp*characterLevel);
   uL+=uni*std::sqrt(.5*(1.0-pan));uR+=uni*std::sqrt(.5*(1.0+pan));
   v.uniPhase[(size_t)u]+=baseFreq*std::pow(2.0,cents/1200.0)/sampleRateHz;
   v.uniPhase[(size_t)u]-=std::floor(v.uniPhase[(size_t)u]);
  }
  const double norm=unison==1?1.41421356237:1.0/std::sqrt((double)unison);
  l+=low+filter(uL*norm,v.filterL1,v.filterL2);
  r+=low+filter(uR*norm,v.filterR1,v.filterR2);
  v.subPhase+=baseFreq/sampleRateHz;v.subPhase-=std::floor(v.subPhase);
  v.age+=1.0/sampleRateHz;
  continue;
 }
 const double mod=std::sin(juce::MathConstants<double>::twoPi*v.lfo);
 const double fmWave=std::sin(juce::MathConstants<double>::twoPi*v.phase2);
 const double ph=v.phase+modFm*.7*fmWave + modRasp*.22*std::sin(juce::MathConstants<double>::twoPi*(v.phase2*3.0));
 const double det=1.0+.018*detune;
 const double a=wave(ph,shape),b=wave(v.phase2,shape);
 const double raw=.62*a+.38*b;
 const double folded=raw + modRasp*.5*std::sin(raw*12.0);
 // Precision path uses a gentler nonlinear transfer with controlled harmonic density.
 // The legacy path is unchanged, so old projects/presets retain their timbre.
 const double dirty=precision
  ? (std::tanh(folded*(1.0+modDrive*5.5)) + .12*std::tanh(folded*2.2)) / 1.12
  : std::tanh(folded*(1.+modDrive*16.));
 const double cleanSub=std::sin(juce::MathConstants<double>::twoPi*v.subPhase)*sub*.75;
 const double bright=precision
  ? dirty*(.38+tone*.53) + modRasp*.055*(dirty-v.lpL)
  : dirty*(.28+tone*.72) + modRasp*.20*(dirty-v.lpL);
 const double targetHz=precision
  ? juce::jlimit(100.0,7800.0,130.+modCutoff*7000.*(1.+.35*modMovement*mod))
  : juce::jlimit(80.0,16000.0,100.+modCutoff*10500.*(1.+.65*modMovement*mod));
 const double alpha=1.-std::exp(-juce::MathConstants<double>::twoPi*targetHz/sampleRateHz);
 const double stereo=width*.013;
 const double leftIn=bright*(1.-stereo*mod)+cleanSub;
 const double rightIn=bright*(1.+stereo*mod)+cleanSub;
 v.lpL+=(float)(alpha*(leftIn-v.lpL));v.lpR+=(float)(alpha*(rightIn-v.lpR));
 if(!v.releasing){
  if(v.age<attackSec)v.env=juce::jmin(1.f,v.env+(float)(1.0/(sampleRateHz*attackSec)));
  else v.env+=(float)((sustainLevel-v.env)*(1.0-std::exp(-1.0/(sampleRateHz*decaySec))));
 } else v.env*=std::exp(-1.0/(sampleRateHz*releaseSec));
 if(v.env<.0001f&&v.releasing){v.active=false;continue;}
 const double transient=precision ? 1.0+punch*.72*std::exp(-v.age*80.0) : 1.0+punch*1.8*std::exp(-v.age*55.0);
 const double gain=v.env*v.velocity*transient;
 l+=v.lpL*gain;r+=v.lpR*gain;
 v.age+=1.0/sampleRateHz;
 v.phase+=delta;v.phase-=std::floor(v.phase);
 v.phase2+=delta*det;v.phase2-=std::floor(v.phase2);
 v.subPhase+=delta*.5;v.subPhase-=std::floor(v.subPhase);
 v.lfo+=(.15+movement*12.)/sampleRateHz;v.lfo-=std::floor(v.lfo);
 }
 const float g=engine10 ? (.18f+output*.34f) : precision ? (.11f+output*.31f) : (.12f+output*.45f);
 float stereoOut[2]={(float)std::tanh(l*g),(float)std::tanh(r*g)};
 const bool bypass=params.getRawParameterValue("fxbypass")->load()>0.5f;
 if(!bypass){
  for(int slot=0;slot<fxCount;++slot){
   int fx=fxOrder[(size_t)slot].load();fx=juce::jlimit(0,fxCount-1,fx);
   auto prefix="fx"+juce::String(fx);
   if(params.getRawParameterValue(prefix+"on")->load()<0.5f)continue;
   float mix=params.getRawParameterValue(prefix+"mix")->load();
   float amt=params.getRawParameterValue(prefix+"amt")->load();
   for(int ch=0;ch<2;++ch){float dry=stereoOut[ch];float wet=processFx(fx,dry,ch,amt,(float)fxClock, (float)sampleRateHz);stereoOut[ch]=dry+(wet-dry)*mix;}
  }
 }
 // Keep peaks controlled while retaining more low-level detail than hard limiting.
 for(int ch=0;ch<2;++ch)out.setSample(ch,i,precision ? std::tanh(stereoOut[ch]*.90f) : std::tanh(stereoOut[ch]));
 if(!delayBuffer[0].empty())delayPos=(delayPos+1)%(int)delayBuffer[0].size();
 if(!reverbBuffer[0].empty())reverbPos=(reverbPos+1)%(int)reverbBuffer[0].size();
 if(!chorusBuffer[0].empty())chorusPos=(chorusPos+1)%(int)chorusBuffer[0].size();
 fxClock+=1.0/sampleRateHz;
 }
}
void BassforgeProcessor::setCurrentProgram(int i){
 if(i==30){
  currentProgram=30;
  patchDisplayName="Init";
  bankArchivePath.clear();
  WavetableEngine::Frame initial{};
  for(int j=0;j<WavetableEngine::size;++j)initial[(size_t)j]=std::sin(juce::MathConstants<double>::twoPi*j/WavetableEngine::size);
  wavetable.setCustomFrame(initial);
  // Restore registered defaults first so the init patch cannot inherit any
  // modulation, FX, unison or envelope settings from the previous patch.
  for(auto* param: getParameters())
   if(auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(param))
    ranged->setValueNotifyingHost(ranged->getDefaultValue());
  const std::pair<const char*,float> initValues[]={
   {"engine10",1.f},{"oscwave",0.f},{"osc2level",0.f},
   {"additivemix",0.f},{"fm",0.f},{"drive",0.f},{"rasp",0.f},
   {"movement",0.f},{"sub",0.f},{"width",0.f},
   {"cutoff",0.8f},{"tone",0.5f},{"punch",0.f},
   {"ampattack",0.f},{"ampdecay",0.35f},{"ampsustain",1.f},
   {"amprelease",0.15f},{"voicemode",0.f},{"portamento",0.f},
   {"fxbypass",1.f}
  };
  for(const auto& [id,value]:initValues)
   if(auto* param=params.getParameter(id))param->setValueNotifyingHost(value);
  for(int iFx=0;iFx<fxCount;++iFx){
   if(auto* param=params.getParameter("fx"+juce::String(iFx)+"on"))
    param->setValueNotifyingHost(0.f);
   fxOrder[(size_t)iFx].store(iFx);
  }
  return;
 }
 if(!juce::isPositiveAndBelow(i,30))return;currentProgram=i;patchDisplayName=factoryPatches[(size_t)i].name;const auto& p=factoryPatches[(size_t)i];const std::pair<const char*,float> values[]={{"drive",p.drive},{"fm",p.fm},{"detune",p.detune},{"cutoff",p.cutoff},{"movement",p.movement},{"release",p.release},{"tone",p.tone},{"width",p.width},{"punch",p.punch},{"rasp",p.rasp}};for(auto& v:values)if(auto* param=params.getParameter(v.first))param->setValueNotifyingHost(v.second);if(auto* param=params.getParameter("shape"))param->setValueNotifyingHost(p.shape/2.f);}
const juce::String BassforgeProcessor::getProgramName(int i){return i==30?"Init":juce::isPositiveAndBelow(i,30)?factoryPatches[(size_t)i].name:"";}
void BassforgeProcessor::getStateInformation(juce::MemoryBlock& b){auto tree=params.copyState();tree.setProperty("program",currentProgram,nullptr);tree.setProperty("patchName",patchDisplayName,nullptr);tree.setProperty("bankArchivePath",bankArchivePath,nullptr);
 auto custom=wavetable.getCustomFrame();
 juce::MemoryBlock waveBytes(custom.data(),sizeof(float)*custom.size());
 tree.setProperty("customWave",waveBytes.toBase64Encoding(),nullptr);for(int i=0;i<fxCount;++i)tree.setProperty("order"+juce::String(i),fxOrder[(size_t)i].load(),nullptr);std::unique_ptr<juce::XmlElement> xml(tree.createXml());copyXmlToBinary(*xml,b);}
void BassforgeProcessor::setStateInformation(const void* data,int bytes){std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data,bytes));if(xml&&xml->hasTagName(params.state.getType())){auto tree=juce::ValueTree::fromXml(*xml);currentProgram=(int)tree.getProperty("program",0);patchDisplayName=tree.getProperty("patchName",getProgramName(currentProgram)).toString();bankArchivePath=tree.getProperty("bankArchivePath","").toString();for(int i=0;i<fxCount;++i)fxOrder[(size_t)i].store(juce::jlimit(0,fxCount-1,(int)tree.getProperty("order"+juce::String(i),i)));params.replaceState(tree);
 const auto encoded=tree.getProperty("customWave").toString();
 juce::MemoryBlock waveBytes;
 if(waveBytes.fromBase64Encoding(encoded)&&waveBytes.getSize()==sizeof(float)*WavetableEngine::size){
  WavetableEngine::Frame frame{};
  std::memcpy(frame.data(),waveBytes.getData(),waveBytes.getSize());
  wavetable.setCustomFrame(frame);
 }
}}

juce::var BassforgeProcessor::exportPreset(const juce::String& name) const {
 auto* obj = new juce::DynamicObject();
 obj->setProperty("format", "bassforge-preset");
 obj->setProperty("version", 1);
 obj->setProperty("name", name);
 auto* values = new juce::DynamicObject();
 for (auto* param : getParameters()) {
  if (auto* id = dynamic_cast<juce::AudioProcessorParameterWithID*>(param))
   values->setProperty(id->paramID, param->getValue());
 }
 obj->setProperty("parameters", juce::var(values));
 auto custom=wavetable.getCustomFrame();
 juce::MemoryBlock waveBytes(custom.data(),sizeof(float)*custom.size());
 obj->setProperty("customWave",waveBytes.toBase64Encoding());
 juce::Array<juce::var> order;
 for (int i=0;i<fxCount;++i) order.add(fxOrder[(size_t)i].load());
 obj->setProperty("fxOrder", juce::var(order));
 return juce::var(obj);
}
bool BassforgeProcessor::validatePreset(const juce::var& preset, juce::String& error) const {
 auto* obj=preset.getDynamicObject();
 if (!obj || obj->getProperty("format").toString()!="bassforge-preset" || (int)obj->getProperty("version")!=1) {error="Not a supported BASSFORGE preset (format/version).";return false;}
 auto* values=obj->getProperty("parameters").getDynamicObject();
 if (!values) {error="Preset has no parameters.";return false;}
 // Validate all fields before modifying any live parameters.
 for (auto& property : values->getProperties()) {
  auto* param=params.getParameter(property.name.toString());
  if (!param || !(property.value.isDouble() || property.value.isInt() || property.value.isInt64())) {error="Invalid parameter: "+property.name.toString();return false;}
  double v=(double)property.value;
  if (!std::isfinite(v) || v<0.0 || v>1.0) {error="Parameter out of range: "+property.name.toString();return false;}
 }
 auto order=obj->getProperty("fxOrder");
 if (!order.isArray() || order.size()!=fxCount) {error="Invalid FX order.";return false;}
 std::array<bool,fxCount> seen{};
 for(int i=0;i<fxCount;++i){int v=(int)order[i];if(v<0||v>=fxCount||seen[(size_t)v]){error="FX order must be a permutation.";return false;}seen[(size_t)v]=true;}
 return true;
}
bool BassforgeProcessor::importPreset(const juce::var& preset, juce::String& error) {
 if(!validatePreset(preset,error))return false;
 auto* obj=preset.getDynamicObject();
 auto* values=obj->getProperty("parameters").getDynamicObject();
 auto order=obj->getProperty("fxOrder");
 for (auto& property : values->getProperties())
  if (auto* param=params.getParameter(property.name.toString())) param->setValueNotifyingHost((float)(double)property.value);
 for(int i=0;i<fxCount;++i)fxOrder[(size_t)i].store((int)order[i]);
 juce::MemoryBlock waveBytes;
 if(waveBytes.fromBase64Encoding(obj->getProperty("customWave").toString())&&waveBytes.getSize()==sizeof(float)*WavetableEngine::size){
  WavetableEngine::Frame frame{};std::memcpy(frame.data(),waveBytes.getData(),waveBytes.getSize());wavetable.setCustomFrame(frame);
 }
 currentProgram=-1;
 patchDisplayName=obj->getProperty("name").toString();
 return true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new BassforgeProcessor();}

void BassforgeProcessor::setCustomWave(const WavetableEngine::Frame& frame){wavetable.setCustomFrame(frame);}
