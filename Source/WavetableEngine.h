#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>
#include <complex>

// Shared immutable, band-limited frame tables. Build once, outside processBlock.
// 4 frames x 11 octave mip levels x 2048 samples; no audio-thread allocation.
class WavetableEngine {
public:
 static constexpr int size=2048, frames=4, levels=11;
 WavetableEngine(){
  constexpr double tau=6.2831853071795864769;
  for(int f=0;f<frames;++f)for(int l=0;l<levels;++l){
   const int maxH=std::max(1,255>>l);
   double peak=0.0;
   for(int i=0;i<size;++i){
    const double ph=tau*i/size;
    double x=0;
    for(int h=1;h<=maxH;++h){
     if(f==0){if(h==1)x=std::sin(ph);}
     else if(f==1)x+=std::sin(h*ph)/h; // saw
     else if(f==2){if(h&1)x+=std::sin(h*ph)/h;} // square
     else if(h&1)x+=((h%4==1)?1.0:-1.0)*std::sin(h*ph)/(h*h); // triangle
    }
    const float scale=f==0?1.f:f==1?float(2.0/3.141592653589793):f==2?float(4.0/3.141592653589793):float(8.0/9.869604401089358);
    data[f][l][i]=float(x)*scale;
    peak=std::max(peak,std::abs(x*scale));
   }
   // Fixed per-frame scaling preserves loudness through mip transitions.
  }
 }
 // Custom waveform is transformed into an octave mip pyramid OFF the audio thread.
 // Frequency bins are zeroed above each level's harmonic limit, then inverse FFT'd.
 using Frame = std::array<float,size>;
 struct CustomTables { Frame original{}; std::array<Frame,levels> mip{}; };
 static void fft(std::array<std::complex<double>,size>& a,bool inverse){
  for(int i=1,j=0;i<size;++i){int bit=size>>1;for(;j&bit;bit>>=1)j^=bit;j^=bit;if(i<j)std::swap(a[(size_t)i],a[(size_t)j]);}
  for(int len=2;len<=size;len<<=1){
   const double angle=(inverse?2.0:-2.0)*3.14159265358979323846/len;
   const std::complex<double> step(std::cos(angle),std::sin(angle));
   for(int i=0;i<size;i+=len){std::complex<double> w(1,0);
    for(int j=0;j<len/2;++j){const auto u=a[(size_t)(i+j)],v=a[(size_t)(i+j+len/2)]*w;
     a[(size_t)(i+j)]=u+v;a[(size_t)(i+j+len/2)]=u-v;w*=step;
    }
   }
  }
  if(inverse)for(auto& x:a)x/=size;
 }
 void setCustomFrame(const Frame& frame){
  auto next=std::make_shared<CustomTables>();next->original=frame;
  std::array<std::complex<double>,size> spectrum{};
  double dc=0;for(float x:frame)dc+=x;dc/=size;
  for(int i=0;i<size;++i)spectrum[(size_t)i]=std::complex<double>(frame[(size_t)i]-dc,0);
  fft(spectrum,false);
  for(int l=0;l<levels;++l){
   auto filtered=spectrum;
   const int harmonics=std::max(1,255>>l);
   for(int k=harmonics+1;k<size-harmonics;++k)filtered[(size_t)k]=0.0;
   fft(filtered,true);
   for(int i=0;i<size;++i)next->mip[(size_t)l][(size_t)i]=float(filtered[(size_t)i].real());
  }
  std::shared_ptr<const CustomTables> published=next;
  std::atomic_store_explicit(&custom,published,std::memory_order_release);
 }
 Frame getCustomFrame() const {
  auto frame=std::atomic_load_explicit(&custom,std::memory_order_acquire);
  return frame?frame->original:Frame{};
 }
 float customSample(double phase,double hz,double sr) const noexcept {
  const auto table=std::atomic_load_explicit(&custom,std::memory_order_acquire);
  if(!table||sr<=0.0)return 0.f;
  const double harmonicBudget=std::max(1.0,.46*sr/std::max(1.0,hz));
  const double lod=std::clamp(std::log2(255.0/harmonicBudget),0.0,double(levels-1));
  const int l0=int(lod),l1=std::min(levels-1,l0+1);
  const float blend=float(lod-l0);
  const double p=(phase-std::floor(phase))*size;
  const int i0=int(p)&(size-1),i1=(i0+1)&(size-1);
  const float t=float(p-int(p));
  auto read=[&](int l){const auto& f=table->mip[(size_t)l];return f[(size_t)i0]+t*(f[(size_t)i1]-f[(size_t)i0]);};
  return read(l0)*(1-blend)+read(l1)*blend;
 }
 float sample(double phase,double hz,double sr,float position) const noexcept {
  if(sr<=0)return 0.f;
  const double harmonicBudget=std::max(1.0,0.46*sr/std::max(1.0,hz));
  const double lod=std::clamp(std::log2(255.0/harmonicBudget),0.0,double(levels-1));
  const int l0=int(lod),l1=std::min(levels-1,l0+1);
  const float lf=float(lod-l0);
  const float fp=std::clamp(position,0.f,1.f)*float(frames-1);
  const int f0=int(fp),f1=std::min(frames-1,f0+1);
  const float ff=fp-f0;
  const double p=(phase-std::floor(phase))*size;
  const int i0=int(p)&(size-1),i1=(i0+1)&(size-1);
  const float t=float(p-int(p));
  auto read=[&](int f,int l){return data[f][l][i0]+t*(data[f][l][i1]-data[f][l][i0]);};
  const float a=read(f0,l0)*(1-lf)+read(f0,l1)*lf;
  const float b=read(f1,l0)*(1-lf)+read(f1,l1)*lf;
  return a+(b-a)*ff;
 }
private:
 std::shared_ptr<const CustomTables> custom;
 std::array<std::array<std::array<float,size>,levels>,frames> data{};
};
