////////////////////////////////////////////////////////////////////////
// Class:       OverlayAna
// Plugin Type: analyzer (Unknown Unknown)
// File:        OverlayAna_module.cc
//
// Generated at Fri Aug 22 20:10:51 2025 by Marco Del Tutto using cetskelgen
// from cetlib version 3.18.02.
////////////////////////////////////////////////////////////////////////

#include "art/Framework/Core/EDAnalyzer.h"
#include "art/Framework/Core/ModuleMacros.h"
#include "art/Framework/Principal/Event.h"
#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Principal/Run.h"
#include "art/Framework/Principal/SubRun.h"
#include "canvas/Utilities/InputTag.h"
#include "fhiclcpp/ParameterSet.h"
#include "messagefacility/MessageLogger/MessageLogger.h"
#include "art_root_io/TFileService.h"

#include "larcore/Geometry/WireReadout.h"
#include "larcorealg/Geometry/PlaneGeo.h"
#include "larcorealg/Geometry/WireGeo.h"
#include "lardataobj/RecoBase/Hit.h"
#include "larcoreobj/SimpleTypesAndConstants/geo_types.h"
#include "larcorealg/Geometry/GeometryCore.h"
#include "lardataobj/Simulation/AuxDetSimChannel.h"
#include "larcore/Geometry/AuxDetGeometry.h"
#include "lardataobj/RawData/RawDigit.h"
#include "lardataobj/RawData/OpDetWaveform.h"
#include "lardataobj/RawData/raw.h"
#include "lardata/Utilities/AssociationUtil.h"
#include "sbnobj/SBND/CRT/FEBData.hh"

#include "TTree.h"

class OverlayAna;


class OverlayAna : public art::EDAnalyzer {
public:
  explicit OverlayAna(fhicl::ParameterSet const& p);
  // The compiler-generated destructor is fine for non-base
  // classes without bare pointers or other resource use.

  // Plugins should not be copied or assigned.
  OverlayAna(OverlayAna const&) = delete;
  OverlayAna(OverlayAna&&) = delete;
  OverlayAna& operator=(OverlayAna const&) = delete;
  OverlayAna& operator=(OverlayAna&&) = delete;

  // Required functions.
  void analyze(art::Event const& e) override;

private:

  std::vector<std::string> _rawdigit_producers;
  std::vector<std::string> _opdetwaveform_producers;
  std::vector<std::string> _crtfeb_producers;

  TTree* _tree;

  int _run, _subrun, _event;
  std::vector<std::vector<float>> _tpc_waveforms_data;
  std::vector<std::vector<float>> _tpc_waveforms_mc;
  std::vector<std::vector<float>> _tpc_waveforms_overlay;
  
  std::vector<std::vector<float>> _pmt_waveforms_data;
  std::vector<std::vector<float>> _pmt_waveforms_mc;
  std::vector<std::vector<float>> _pmt_waveforms_overlay;

  std::vector<std::vector<uint16_t>> _crt_febadc_data;
  std::vector<std::vector<uint16_t>> _crt_febadc_mc;
  std::vector<std::vector<uint16_t>> _crt_febadc_overlay;
};


OverlayAna::OverlayAna(fhicl::ParameterSet const& p)
  : EDAnalyzer{p}  // ,
  // More initializers here.
{
  _rawdigit_producers = p.get<std::vector<std::string>>("RawDigits");
  _opdetwaveform_producers = p.get<std::vector<std::string>>("OpDetWaveforms");

  if (_rawdigit_producers.size() != 3) {
    std::cout << "Need to provide three RawDigit producer labels (Data, MC, Overlay)!" << std::endl;
    throw std::exception();
  }

  if (_opdetwaveform_producers.size() != 3) {
    std::cout << "Need to provide three OpDetWaveform producer labels (Data, MC, Overlay)!" << std::endl;
    throw std::exception();
  }

  art::ServiceHandle<art::TFileService> fs;
  _tree = fs->make<TTree>("tree","");
  _tree->Branch("waveforms_data", "std::vector<std::vector<float>>", &_tpc_waveforms_data);
  _tree->Branch("waveforms_mc", "std::vector<std::vector<float>>", &_tpc_waveforms_mc);
  _tree->Branch("waveforms_overlay", "std::vector<std::vector<float>>", &_tpc_waveforms_overlay);
  
  _tree->Branch("pmt_waveforms_data", "std::vector<std::vector<float>>", &_pmt_waveforms_data);
  _tree->Branch("pmt_waveforms_mc", "std::vector<std::vector<float>>", &_pmt_waveforms_mc);
  _tree->Branch("pmt_waveforms_overlay", "std::vector<std::vector<float>>", &_pmt_waveforms_overlay);

  _crtfeb_producers = p.get<std::vector<std::string>>("CRTFEBData", std::vector<std::string>());

  if (_crtfeb_producers.size() > 0 && _crtfeb_producers.size() != 3) {
    std::cout << "Need to provide three CRT FEBData producer labels (Data, MC, Overlay) or none!" << std::endl;
    throw std::exception();
  }

  if (_crtfeb_producers.size() == 3) {
    _tree->Branch("crt_febadc_data", "std::vector<std::vector<uint16_t>>", &_crt_febadc_data);
    _tree->Branch("crt_febadc_mc", "std::vector<std::vector<uint16_t>>", &_crt_febadc_mc);
    _tree->Branch("crt_febadc_overlay", "std::vector<std::vector<uint16_t>>", &_crt_febadc_overlay);
  }
}

void OverlayAna::analyze(art::Event const& e)
{
  
  // art::ServiceHandle<geo::Geometry> geom;

  geo::WireReadoutGeom const& wr_geom = art::ServiceHandle<geo::WireReadout>()->Get();

  // ====================================================================
  // TPC Waveforms Processing
  // ====================================================================
  std::vector<std::vector<art::Ptr<raw::RawDigit>>> rawdigits;

  for(auto producer : _rawdigit_producers) {
    art::Handle<std::vector<raw::RawDigit>> rawdigit_h;
    e.getByLabel(producer, rawdigit_h);
    if(!rawdigit_h.isValid()){
      std::cout << "RawDigit product " << producer << " not found..." << std::endl;
      throw std::exception();
    }
    std::cout << "Found RawDigit product " << producer << std::endl;
    std::vector<art::Ptr<raw::RawDigit>> rawdigit_v;
    art::fill_ptr_vector(rawdigit_v, rawdigit_h);
    rawdigits.push_back(rawdigit_v);
  }


  // std::cout << "Number of available RawDigits " << rawdigit_v.size() << std::endl;
  
  _tpc_waveforms_data.clear();
  _tpc_waveforms_data.resize(wr_geom.Nchannels());
  _tpc_waveforms_mc.clear();
  _tpc_waveforms_mc.resize(wr_geom.Nchannels());
  _tpc_waveforms_overlay.clear();
  _tpc_waveforms_overlay.resize(wr_geom.Nchannels());

  for (int i = 0; i < 3; i++)
  {
    auto rawdigit_v = rawdigits[i];

    for (auto const &rawdigit : rawdigit_v)
    {

      unsigned int ch  = rawdigit->Channel();
      float        ped = rawdigit->GetPedestal();

      geo::WireID w_id = wr_geom.ChannelToWire(ch)[0];
      unsigned int wire = w_id.Wire;
      unsigned int plane = w_id.Plane;
      unsigned int tpc = w_id.TPC;
      unsigned int cryo = w_id.Cryostat;
      std::cout << "RawDigit ch " << ch << ", wire " << wire << ", plane " << plane << ", tpc " << tpc << ", cryo " << cryo << std::endl;

      auto adcs = rawdigit->ADCs();
      
      // Add pedestal subtraction for MC data (i == 1)
      if (i == 1) { 
        for (auto &a : adcs) {
          a -= ped;
        }
      }
      
      if(i == 0) _tpc_waveforms_data[ch].assign(adcs.begin(), adcs.end());
      if(i == 1) _tpc_waveforms_mc[ch].assign(adcs.begin(), adcs.end());
      if(i == 2) _tpc_waveforms_overlay[ch].assign(adcs.begin(), adcs.end());
    }
  }

  // ====================================================================
  // PMT Waveforms Processing
  // ====================================================================
  std::vector<std::vector<art::Ptr<raw::OpDetWaveform>>> opdetwaveforms;

  for(auto producer : _opdetwaveform_producers) {
    art::Handle<std::vector<raw::OpDetWaveform>> opdetwaveform_h;
    e.getByLabel(producer, opdetwaveform_h);
    if(!opdetwaveform_h.isValid()){
      std::cout << "OpDetWaveform product " << producer << " not found..." << std::endl;
      throw std::exception();
    }
    std::cout << "Found OpDetWaveform product " << producer << std::endl;
    std::vector<art::Ptr<raw::OpDetWaveform>> opdetwaveform_v;
    art::fill_ptr_vector(opdetwaveform_v, opdetwaveform_h);
    opdetwaveforms.push_back(opdetwaveform_v);
  }

  // Determine the number of PMT channels
  // You may need to adjust this based on your detector geometry
  // For SBND, there are typically ~120 PMT channels
  size_t n_pmt_channels = 0;
  for (auto const& opdet_v : opdetwaveforms) {
    for (auto const& opdet : opdet_v) {
      if (opdet->ChannelNumber() >= n_pmt_channels) {
        n_pmt_channels = opdet->ChannelNumber() + 1;
      }
    }
  }

  _pmt_waveforms_data.clear();
  _pmt_waveforms_data.resize(n_pmt_channels);
  _pmt_waveforms_mc.clear();
  _pmt_waveforms_mc.resize(n_pmt_channels);
  _pmt_waveforms_overlay.clear();
  _pmt_waveforms_overlay.resize(n_pmt_channels);

  for (int i = 0; i < 3; i++)
  {
    auto opdetwaveform_v = opdetwaveforms[i];

    for (auto const &opdetwaveform : opdetwaveform_v)
    {
      unsigned int ch = opdetwaveform->ChannelNumber();
      
      std::cout << "OpDetWaveform ch " << ch << ", size " << opdetwaveform->size() << std::endl;

      // Convert the waveform to float vector
      std::vector<float> waveform;
      waveform.reserve(opdetwaveform->size());
      for (size_t j = 0; j < opdetwaveform->size(); j++) {
        waveform.push_back(static_cast<float>((*opdetwaveform)[j]));
      }
      
      if(i == 0) _pmt_waveforms_data[ch].assign(waveform.begin(), waveform.end());
      if(i == 1) _pmt_waveforms_mc[ch].assign(waveform.begin(), waveform.end());
      if(i == 2) _pmt_waveforms_overlay[ch].assign(waveform.begin(), waveform.end());
    }
  }

  // ====================================================================
  // CRT FEBData Processing 
  // ====================================================================
  if (_crtfeb_producers.size() == 3) {
    // Clear the output vectors
    _crt_febadc_data.clear();
    _crt_febadc_mc.clear();
    _crt_febadc_overlay.clear();

    // Process each producer independently
    for (int i = 0; i < 3; ++i) {
      const auto& producer = _crtfeb_producers[i];
      std::string producer_name = (i == 0) ? "Data" : ((i == 1) ? "MC" : "Overlay");
      
      art::Handle<std::vector<sbnd::crt::FEBData>> crtfeb_h;
      e.getByLabel(producer, crtfeb_h);
      
      if (!crtfeb_h.isValid()) {
        std::cout << "WARNING: FEBData product " << producer << " (" << producer_name 
                  << ") not found in event — will leave empty." << std::endl;
        continue;
      }
      
      size_t n_febs = crtfeb_h->size();
      std::cout << "Found FEBData product " << producer << " (" << producer_name 
                << ") with " << n_febs << " entries" << std::endl;
      
      if (n_febs == 0) {
        std::cout << "NOTE: FEBData product " << producer << " (" << producer_name 
                  << ") is empty (0 hits) — this is OK for MC if no CRT hits were generated." << std::endl;
        continue;
      }
      
      // Fill the appropriate vector
      for (size_t j = 0; j < n_febs; ++j) {
        const auto& febdata = (*crtfeb_h)[j];
        std::vector<uint16_t> adc_values;
        adc_values.reserve(32);
        for (size_t ch = 0; ch < 32; ++ch) {
          adc_values.push_back(febdata.ADC(ch));
        }
        
        if (i == 0) _crt_febadc_data.push_back(adc_values);
        else if (i == 1) _crt_febadc_mc.push_back(adc_values);
        else if (i == 2) _crt_febadc_overlay.push_back(adc_values);
      }
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "CRT Summary:" << std::endl;
    std::cout << "  Data FEBs:    " << _crt_febadc_data.size() << std::endl;
    std::cout << "  MC FEBs:      " << _crt_febadc_mc.size() << std::endl;
    std::cout << "  Overlay FEBs: " << _crt_febadc_overlay.size() << std::endl;
    std::cout << "========================================" << std::endl;
  }
  
  _tree->Fill();
}

DEFINE_ART_MODULE(OverlayAna)