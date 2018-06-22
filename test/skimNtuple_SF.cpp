// ntuple skimmer for analysis and synchronization

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <bitset>
#include <map>
#include "TTree.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TFile.h"
#include "TBranch.h"
#include "TString.h"
#include "TLorentzVector.h"

// bigTree is produced on an existing ntuple as follows (see at the end of the file) 
#include "bigTree.h" 
#include "smallTree.h"
#include "OfflineProducerHelper.h"
#include "PUReweight.h"
//#include "triggerReader.h" //FRA: not used anymore
#include "triggerReader_cross.h" //FRA
#include "bJetRegrVars.h"
#include "bTagSF.h"
// #include "HHReweight.h"
#include "HHReweight5D.h"
#include "../../HHKinFit2/include/HHKinFitMasterHeavyHiggs.h"
#include "BDTfunctionsUtils.h"

// for minuit-based minimization
// #include "mt2.h"
// #include "Math/Minimizer.h"
// #include "Math/Factory.h"
// #include "Math/Functor.h"
#include "lester_mt2_bisect.h"

//#include "../../HTT-utilities/LepEffInterface/interface/ScaleFactor.h"
#include "ScaleFactor.h"
#include "ConfigParser.h"
#include "EffCounter.h"
#include "tauTrigSFreader.h"
#include "exceptions/HHInvMConstraintException.h"
#include "exceptions/HHEnergyRangeException.h"
#include "exceptions/HHEnergyConstraintException.h"

#include "TMVA/MsgLogger.h"
#include "TMVA/Config.h"
#include "TMVA/Tools.h"
#include "TMVA/Factory.h"
#include "TMVA/Reader.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/format.hpp>
#include <Math/VectorUtil.h>

using namespace std ;

const double aTopRW = 0.0615;
const double bTopRW = -0.0005;
// const float DYscale_LL[3] = {8.72847e-01, 1.69905e+00, 1.63717e+00} ; // computed from fit for LL and MM b tag
// const float DYscale_MM[3] = {9.44841e-01, 1.29404e+00, 1.28542e+00} ;
const float DYscale_LL[3] = {1.13604, 0.784789, 1.06947} ; // computed from fit for LL and MM b tag
const float DYscale_MM[3] = {1.14119, 1.18722, 1.17042} ;

/* NOTE ON THE COMPUTATION OF STITCH WEIGHTS:
** - to be updated at each production, using the number of processed events N_inclusive and N_njets for each sample
** - say f_i is the fraction of inclusive events in the i bin on njets (can be 2D nb-njet as well)
** = then sigma_i = f_i * sigmal_LO
** - stitchWeight (0jet) = f_0 / (f_0 * N_inclusive)
** - stitchWeight (njet) = f_n / (f_n * N_inclusive + N_njets)
*/ 

// const float stitchWeights [5] = {1.11179e-7, 3.04659e-9, 3.28633e-9, 3.48951e-9, 2.5776e-9} ; // weights DY stitch in njets, to be updated at each production (depend on n evtsn processed)
// const float stitchWeights [5] = {11.55916, 0.316751, 0.341677, 0.362801, 0.267991} ; // weights DY stitch in njets, to be updated at each production (depend on n evts processed)

// const float stitchWeights [5] = {2.01536E-08, 2.71202E-09, 2.92616E-09, 3.0373E-09, 2.38728E-09} ; // jet binned only, 27 giu 2016
// const float stitchWeights [][5] = {
//     {2.98077961089 , 0.0 , 0.0 , 0.0 , 0.0},
//     {0.400849633946 , 0.313302746388 , 0.0 , 0.0 , 0.0},
//     {0.434801486598 , 0.334010654578 , 0.102986214642 , 0.0 , 0.0},
//     {0.449060210108 , 0.342010066467 , 0.101739957862 , 0.100837020714 , 0.0},
//     {0.354615200387 , 0.285223034235 , 0.0977183487048 , 0.098552902997 , 0.0936281612454}
// }; // jet binned and b binned, 8 jul 2016

// const float stitchWeights [][5] = {
//     {2.97927051428 , 0.0 , 0.0 , 0.0 , 0.0},
//     {0.401700471936 , 0.313567146487 , 0.0 , 0.0 , 0.0},
//     {0.43376913912 , 0.33311444536 , 0.101563317164 , 0.0 , 0.0},
//     {0.44761355606 , 0.340870252304 , 0.10029914665 , 0.0994092045617 , 0.0},
//     {0.353436942964 , 0.284254291888 , 0.0963966329522 , 0.0972197079415 , 0.092337393936}
// };  // jet binned and b binned, 28 nov 2016

//const float stitchWeights [][5] = {
//  {2.96970591027 , 0.0 , 0.0 , 0.0 , 0.0},
//  {0.40848145146 , 0.33006603191 , 0.0 , 0.0 , 0.0},
//  {0.420075226373 , 0.337888056259 , 0.096536134169 , 0.0 , 0.0},
//  {0.431426212048 , 0.344817310665 , 0.0952166256522 , 0.094350931903 , 0.0},
//  {0.339954183508 , 0.284560899763 , 0.0915028373483 , 0.0922864405088 , 0.0874799674999}
//}; // jet binned and b binned, 07 Feb 2017

const float stitchWeights [][5] = {
  {1.51432918823 , 0.0 , 0.0 , 0.0 , 0.0},
  {0.537021861953 , 0.537498571149 , 0.0 , 0.0 , 0.0},
  {0.598363343318 , 0.598946235287 , 0.10509584187 , 0.0 , 0.0},
  {0.947765704522 , 0.942420974822 , 0.109982447103 , 0.108336573578 , 0.0},
  {1.51470061251 , 1.51281269465 , 0.117228562794 , 0.118913419351 , 0.109809154254}
};// jet binned and b binned, 15 May 2018 

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -
// open input txt file and append all the files it contains to TChain
void appendFromFileList (TChain* chain, TString filename)
{
  //cout << "=== inizio parser ===" << endl;
  std::ifstream infile(filename.Data());
  std::string line;
  while (std::getline(infile, line))
    {
      line = line.substr(0, line.find("#", 0)); // remove comments introduced by #
      while (line.find(" ") != std::string::npos) line = line.erase(line.find(" "), 1); // remove white spaces
      while (line.find("\n") != std::string::npos) line = line.erase(line.find("\n"), 1); // remove new line characters
      while (line.find("\r") != std::string::npos) line = line.erase(line.find("\r"), 1); // remove carriage return characters
      if (!line.empty()) // skip empty lines
	chain->Add(line.c_str());
    }
  return;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -
// open the first file in the input list, retrieve the histogram "Counters" for the trigger names and return a copy of it
TH1F* getFirstFileHisto (TString filename, bool isForTriggers=true)
{
  std::ifstream infile(filename.Data());
  std::string line;
  while (std::getline(infile, line))
    {
      line = line.substr(0, line.find("#", 0)); // remove comments introduced by #
      while (line.find(" ") != std::string::npos) line = line.erase(line.find(" "), 1); // remove white spaces
      while (line.find("\n") != std::string::npos) line = line.erase(line.find("\n"), 1); // remove new line characters
      while (line.find("\r") != std::string::npos) line = line.erase(line.find("\r"), 1); // remove carriage return characters
      if (!line.empty()) // skip empty lines
	break;
    }
    
  TFile* fIn = TFile::Open (line.c_str());
  TH1F* dummy = (TH1F*) fIn->Get ("HTauTauTree/Counters");
  TString name = "Counters_perTrigger";
  if(!isForTriggers) {
    dummy = (TH1F*) fIn->Get ("HTauTauTree/TauIDs");
    name = "Counters_pertauID";
  }
  TH1F* histo = new TH1F (*dummy);
  histo-> SetDirectory(0);
  histo->SetName (name.Data());
  fIn->Close();
  return histo;
}

// --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- --- ---
/**
   muons = 0
   electrons = 1
   taus = 2
*/

float getIso (unsigned int iDau, float pt, bigTree & theBigTree)
{
  int type = theBigTree.particleType->at (iDau) ;
  // is tauH
  if (type == 2)
    // return theBigTree.daughters_byCombinedIsolationDeltaBetaCorrRaw3Hits->at(iDau) ;
    //return theBigTree.daughters_byIsolationMVArun2v1DBoldDMwLTraw->at(iDau) ;
    return theBigTree.daughters_byIsolationMVArun2017v2DBoldDMwLTraw2017->at(iDau) ; //FRA: update for 2017 data (94X)
  // muon
  if (type == 1 || type == 0)
    return theBigTree.combreliso->at(iDau);

  return -1 ;
}

// convert the 6 tau iso discriminators into an integer, from 0 == VLoose to 6 == VVTight
// example for MVA id (same for cut based):
// == 0 : NotIso
// >= 1 : pass VLoose
// >= 2 : pass Loose
// >= 3 : pass Medium
// >= 4 : pass Tigth
// >= 5 : pass VTight
// each number denotes the most stringent discriminator passed, so that selection candidates as:
// MVAiso >= 3 --> all candidates that are *at least* medium iso (or more isolated)
// MVAiso <= 3 --> sideband : all anti-isolated candidates, *not more* isolated than medium WP
int makeIsoDiscr (unsigned int iDau, vector<int>& nameToIdxMap, bigTree & theBigTree)
{
  int isoInt = 0;
  Long64_t tauID = theBigTree.tauID->at(iDau);
  while (isoInt < (int) nameToIdxMap.size())
    {
      int bit = nameToIdxMap.at(isoInt) ;
      bool pass = (((tauID >> bit) & 1 ) > 0);
      if (!pass) break; // will freeze isoInt to the previous value
      isoInt += 1;
    } 
  return isoInt;
}

int getTauIDIdx (TH1F* h_tauID, string tauIDName)
{
  int ibin = -1;
  for (int i = 1; i <= h_tauID->GetNbinsX(); ++i)
    {
      string binlabel = h_tauID->GetXaxis()->GetBinLabel(i);
      if (tauIDName == binlabel)
      {
        ibin = i;
        break;
      }
    }
  return ibin-1;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

bool CheckBit (int number, int bitpos)
{
  bool res = number & (1 << bitpos);
  return res;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
// check that the VBF jets are correctly matched to different TrigerObjects
bool checkVBFjetMatch(bool DEBUG, int iJet, int kJet, bigTree & theBigTree)
{
  bool goodmatch= true;

  Long64_t jet1lead = theBigTree.jets_VBFleadFilterMatch   ->at(iJet);
  Long64_t jet1subl = theBigTree.jets_VBFsubleadFilterMatch->at(iJet);
  Long64_t jet2lead = theBigTree.jets_VBFleadFilterMatch   ->at(kJet);
  Long64_t jet2subl = theBigTree.jets_VBFsubleadFilterMatch->at(kJet);

  if (DEBUG)
  {
    cout << "---   checking VBF jet legs   --- " << endl;
    cout << "jet "<< iJet << " - leading    : " << std::bitset<64>(jet1lead) << endl;
    cout << "jet "<< iJet << " - subleading : " << std::bitset<64>(jet1subl) << endl;
    cout << "jet "<< kJet << " - leading    : " << std::bitset<64>(jet2lead) << endl;
    cout << "jet "<< kJet << " - subleading : " << std::bitset<64>(jet2subl) << endl;
    cout << "XOR leading       : " << (jet1lead^jet2lead) << endl;
    cout << "XOR subleading    : " << (jet1subl^jet2subl) << endl;
    if ( (jet1subl^jet2subl) == 0 ) cout << " -- same TO for the subleading jets" << endl;
    if ( (jet1lead^jet2lead) == 0 ) cout << " -- same TO for the leading jets" << endl;
    cout << "--- end checking VBF jet legs --- " << endl;
  }

  bool firstJetZero  = (jet1lead==0 && jet1subl==0);
  bool secondJetZero = (jet2lead==0 && jet2subl==0);
  if (firstJetZero || secondJetZero) goodmatch = false; //one of the two jets is not matched to any filter
  if ( (jet1subl^jet2subl) == 0 )    goodmatch = false; //two jets are matched to the same TrigObj (subleading filter)
  if ( (jet1lead^jet2lead) == 0 )    goodmatch = false; //two jets are matched to the same TrigObj (subleading filter)

  return goodmatch;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

// implement operator < for b tag . first : CSV score ;  second : index
bool bJetSort (const pair<float, int>& i, const pair<float, int>& j) {
  if (i.first != j.first) return (i.first < j.first) ; // lowest CVS
  return i.second > j.second ; // highest index == lowest pt
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -


float deltaPhi (float phi1, float phi2)
{
  float deltaphi = fabs (phi1 - phi2) ;
  if (deltaphi > 6.283185308) deltaphi -= 6.283185308 ;
  if (deltaphi > 3.141592654) deltaphi = 6.283185308 - deltaphi ;
  return deltaphi ;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
float getZ (float eta, float eta1, float eta2)
{
  float Z  =  (eta - 0.5*(eta1 + eta2)) / fabs (eta1 - eta2) ;
  return Z;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

vector<int> findSubjetIdxs (unsigned int iFatJet, bigTree & theBigTree)
{
  vector<int> indexes;
  int idxFatJet = (int) iFatJet;
  for (unsigned int isj = 0; isj < theBigTree.subjets_ak8MotherIdx->size(); isj++)
    {
      if (theBigTree.subjets_ak8MotherIdx->at(isj) == idxFatJet)
	indexes.push_back(isj);
    }
  return indexes;
}


// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -
// get shifted MET
TVector2 getShiftedMET (float shift, TVector2 MET, bigTree & theBigTree, bool DEBUG=false)
{
  double corrMETx = MET.Px();
  double corrMETy = MET.Py();

  if (DEBUG) cout << "*********** DEBUGGING JETS *********** "<< endl;

  for (unsigned int iJet = 0 ; iJet < theBigTree.jets_px->size () ; ++iJet)
  {
    // if (theBigTree.PFjetID->at (iJet) < 1) continue;
    // build original jet
    TLorentzVector tlv_jet (theBigTree.jets_px->at(iJet), theBigTree.jets_py->at(iJet), theBigTree.jets_pz->at(iJet), theBigTree.jets_e->at(iJet));

    // get uncertainty
    double unc = theBigTree.jets_jecUnc->at(iJet);

    // build shifted jet
    TLorentzVector tlv_jet_shifted = tlv_jet;
    tlv_jet_shifted.SetPtEtaPhiE(
        (1.+shift*unc) * tlv_jet.Pt(),
        tlv_jet.Eta(),
        tlv_jet.Phi(),
        (1.+shift*unc) * tlv_jet.E()
        );

    // shift MET - first the original jet
    corrMETx += tlv_jet.Px();
    corrMETy += tlv_jet.Py();

    // shift MET - then the shifted jet
    corrMETx -= tlv_jet_shifted.Px();
    corrMETy -= tlv_jet_shifted.Py();

    // Debug printing
    if (DEBUG)
    {
      cout << " Jet "   << setw(1) << left << iJet
      << " - pt: "      << setw(7) << left << tlv_jet.Pt()
      << " - eta: "     << setw(9) << left << tlv_jet.Eta()
      << " - sf: "      << setw(8) << left << (1.+shift*unc)
      << " - unc: "     << setw(10) << left << unc
      << " - pfjetID: " << setw(2) <<left << theBigTree.PFjetID->at(iJet) << endl;
    }
  }

  TVector2 shiftedMET(corrMETx, corrMETy);

  if (DEBUG) cout << "*********** ************** *********** "<< endl;

  return shiftedMET;
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -
// turn on function for trigger reweight

float turnOnCB(float x, float m0, float sigma, float alpha, float n, float norm)
{
  float sqrtPiOver2 = TMath::Sqrt(TMath::PiOver2());
  float sqrt2 = TMath::Sqrt(2.);
  float sig = abs(sigma);
  float t = (x - m0)/sig * alpha / abs(alpha);
  float absAlpha = abs(alpha/sig);
  float a = TMath::Power(n/absAlpha, n) * TMath::Exp(-0.5 * absAlpha * absAlpha);
  float b = absAlpha - n/absAlpha;
  float arg = absAlpha / sqrt2;
  float ApproxErf = TMath::Erf(arg);
  if (arg > 5.) ApproxErf = 1.;
  else if(arg < -5.) ApproxErf = -1.;
  float leftArea = (1. + ApproxErf) * sqrtPiOver2;
  float rightArea = ( a * 1./TMath::Power(absAlpha-b, n-1) ) / (n-1);
  float area = leftArea + rightArea;
  if (t <= absAlpha){
    arg = t / sqrt2;
    if (arg > 5.) ApproxErf = 1.;
    else {
      if(arg < -5.) ApproxErf = -1.;
      else ApproxErf = TMath::Erf(arg);
    }
    return norm * (1 + ApproxErf) * sqrtPiOver2 / area;
  }
  return norm * (leftArea + a * (1/TMath::Power(t-b,n-1) - 1/TMath::Power(absAlpha - b,n-1)) / (1-n)) / area;
}

// WP is 0 : Noiso , 1 : VLoose, 2: Loose, 3: Medium , 4: Tight 5: Vtight 6: VVtight
float turnOnSF(float pt, int WP, bool realTau)
{
  //return 1.0/turnOnCB(pt,3.60274e+01,5.89434e+00,5.82870e+00,1.83737e+00,9.58000e-01)*turnOnCB(pt,3.45412e+01,5.63353e+00,2.49242e+00,3.35896e+00,1);
  //return turnOnCB(pt,3.45412e+01,5.63353e+00,2.49242e+00,3.35896e+00,1);
  // return turnOnCB(pt,3.60274e+01,5.89434e+00,5.82870e+00,1.83737e+00,9.58000e-01);

  float real_m0    [7] = {3.86506E+01 , 3.86057E+01 , 3.85953E+01 , 3.81821E+01 , 3.81919E+01 , 3.77850E+01 , 3.76157E+01} ;
  float real_sigma [7] = {5.81155E+00 , 5.77127E+00 , 5.74632E+00 , 5.33452E+00 , 5.38746E+00 , 4.93611E+00 , 4.76127E+00} ;
  float real_alpha [7] = {5.82783E+00 , 5.61388E+00 , 5.08553E+00 , 4.42570E+00 , 4.44730E+00 , 4.22634E+00 , 3.62497E+00} ;
  float real_n     [7] = {3.38903E+00 , 3.77719E+00 , 5.45593E+00 , 4.70512E+00 , 7.39646E+00 , 2.85533E+00 , 3.51839E+00} ;
  float real_norm  [7] = {9.33449E+00 , 9.30159E-01 , 9.42168E-01 , 9.45637E-01 , 9.33402E-01 , 9.92196E-01 , 9.83701E-01} ;

  float fake_m0    [7] = {4.03919E+01 , 3.99115E+01 , 3.94747E+01 , 3.92674E+01 , 3.90677E+01 , 3.92867E+01 , 3.89518E+01} ;
  float fake_sigma [7] = {7.55333E+00 , 7.32760E+00 , 7.23546E+00 , 7.17092E+00 , 7.03152E+00 , 7.22249E+00 , 6.69525E+00} ;
  float fake_alpha [7] = {1.20102E+01 , 1.17174E+01 , 1.08089E+01 , 1.10546E+01 , 1.11690E+01 , 1.14726E+01 , 9.86033E+00} ;
  float fake_n     [7] = {1.26661E+00 , 1.26857E+00 , 1.33930E+00 , 1.31852E+00 , 1.29314E+00 , 1.32792E+00 , 1.39875E+00} ;
  float fake_norm  [7] = {1.00000E+00 , 1.00000E+00 , 1.00000E+00 , 1.00000E+00 , 9.99999E-01 , 1.00000E+00 , 1.00000E+00} ;

  if (realTau) return turnOnCB (pt, real_m0[WP], real_sigma[WP], real_alpha[WP], real_n[WP], real_norm[WP] );  
  else         return turnOnCB (pt, fake_m0[WP], fake_sigma[WP], fake_alpha[WP], fake_n[WP], fake_norm[WP] );  
}

// float getTriggerWeight(int partType, float pt, TH1F* weightHisto)
// {
//     if (partType == 0) return 0.95;
//     else if (partType == 1) return 0.95;
//     else if (partType == 2)
//     {
//         int ibin = weightHisto->FindBin(pt);
//         if (ibin < 1) ibin = 1;
//         if (ibin > weightHisto->GetNbinsX()) ibin = weightHisto->GetNbinsX() ;
//         return weightHisto->GetBinContent(ibin);
//     }
//     cout << "** WARNING: trigger weight now known for particle type " << partType << endl;
//     return 1.;
// }


float getTriggerWeight(int partType, float pt, float eta, TH1F* rewHisto = 0, ScaleFactor* sfreader = 0, int tauWP = 0, bool realTau = false)
{
  float weight = 1.0;
    
  switch(partType)
    {
    case 0: // mu
      {
        weight = sfreader->get_EfficiencyData(pt, eta);        
        break;
      }
    case 1: // ele
      {
        int bin = rewHisto->FindBin (pt);
        if (bin == 0) bin = 1;
        if (bin >= rewHisto->GetNbinsX()+1) bin = rewHisto->GetNbinsX();
        weight =  rewHisto->GetBinContent (bin);
        break;
      }
    case 2: // tau
      {
        weight = turnOnSF (pt, tauWP, realTau) ;
        break;
      }
    default:
      {
        cout << "** WARNING: trigger weight now known for particle type " << partType << endl;
        weight =  1.;
        break;
      }
    }

  return weight;
}

// generic function to read content of 1D / 2D histos, taking care of x axis limit (no under/over flow)
double getContentHisto1D(TH1* histo, double x)
{
  int nbinsx = histo->GetNbinsX();
  int ibin = histo->FindBin(x);
  if (ibin == 0) ibin = 1;
  if (ibin > nbinsx) ibin = nbinsx;
  return histo->GetBinContent(ibin);
}

double getContentHisto2D(TH2* histo, double x, double y)
{
  int nbinsx = histo->GetNbinsX();
  int nbinsy = histo->GetNbinsY();
  int ibinx = histo->GetXaxis()->FindBin(x);
  int ibiny = histo->GetYaxis()->FindBin(y);

  if (ibinx == 0)     ibinx = 1;
  if (ibinx > nbinsx) ibinx = nbinsx;

  if (ibiny == 0)     ibiny = 1;
  if (ibiny > nbinsy) ibiny = nbinsy;
  
  return histo->GetBinContent(ibinx, ibiny);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -
// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- -


int main (int argc, char** argv)
{
  // read input file and cfg
  // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

  if (argc < 22)
    { 
      cerr << "missing input parameters : argc is: " << argc << endl ;
      cerr << "usage: " << argv[0]
           << " inputFileNameList outputFileName crossSection isData configFile runHHKinFit"
           << " xsecScale(stitch) HTMax(stitch) isTTBar DY_Nbs HHreweightFile TT_stitchType"
           << " runMT2 isHHsignal NjetRequired(stitch) kl_rew kt_rew c2_rew cg_rew c2g_rew susyModel" << endl ; 
      return 1;
    }

  TString inputFile = argv[1] ;
  TString outputFile = argv[2] ;
  cout << "** INFO: inputFile  : " << inputFile << endl;
  cout << "** INFO: outputFile : " << outputFile << endl;
  
  float XS = atof (argv[3]) ;  
  bool isMC = true;
  int isDatabuf = atoi (argv[4]);
  if (isDatabuf == 1)
    {
      isMC = false; 
      XS = 1.;
    }
  cout << "** INFO: x-section: " << XS << endl;
  cout << "** INFO: is it MC?  " << isMC << endl;

  if (gConfigParser) return 1 ;
  gConfigParser = new ConfigParser () ;  
  TString config ; 
  config.Form ("%s",argv[5]) ;
  cout << "** INFO: reading config : " << config << endl;

  bool runHHKinFit = false;
  string opt7 (argv[6]);
  if (opt7 == "1") runHHKinFit = true;
  cout << "** INFO: running HHKinFit: " << runHHKinFit << endl;

  float xsecScale = 1.0;
  xsecScale = atof (argv[7]);
  cout << "** INFO: xsec scaled by: " << xsecScale << endl;
  XS = XS*xsecScale;

  float HTMax = -999.0;
  HTMax = atof(argv[8]);
  cout << "** INFO: removing HT < " << HTMax << " [-999 means no removal]" << endl;

  int isTTBarI = atoi(argv[9]);
  bool isTTBar = (isTTBarI == 1) ? true : false;
  if (!isMC) isTTBar = false; // force it, you never know...
  cout << "** INFO: is this a TTbar sample? : " << isTTBar << endl;

  bool DY_Nbs = false; // run on genjets to count in DY samples the number of b jets
  bool DY_tostitch = false;
  int I_DY_Nbs = atoi(argv[10]);
  if (I_DY_Nbs == 1)
    {
      DY_Nbs = true; 
      DY_tostitch = true; // FIXME!! this is ok only if we use jet binned samples
    }
  cout << "** INFO: loop on gen jet to do a b-based DY split? " << DY_Nbs << " " << DY_tostitch << endl;

  TFile* HHreweightFile = 0;
  TString doreweight = argv[11];
  cout << "** INFO: reweightin file for non-resonant hh is: " << doreweight << " [ 0 for no reweghting done ]" << endl;
  if (doreweight != TString("0"))
    HHreweightFile = new TFile (doreweight);

  int TT_stitchType = atoi(argv[12]);
  if (!isTTBar) TT_stitchType = 0; // just force if not TT...
  cout << "** INFO: TT stitch type: " << TT_stitchType << " [0: no stitch , 1: fully had, 2: semilept t, 3: semilept tbar, 4: fully lept, 5: semilept all]" << endl;

  bool runMT2 = false;
  string opt14 (argv[13]);
  if (opt14 == "1") runMT2 = true;
  cout << "** INFO: running MT2: " << runMT2 << endl;

  bool isHHsignal = false;
  string opt15 (argv[14]);
  if (opt15 == "1") isHHsignal = true;
  cout << "** INFO: is HH signal: " << isHHsignal << endl;

  int NjetRequired = atoi(argv[15]);
  cout << "** INFO: requiring exactly " << NjetRequired << " outgoing partons [<0 for no cut on this]" << endl;

  float kl_rew = atof(argv[16]);
  float kt_rew = atof(argv[17]);
  float c2_rew = atof(argv[18]);
  float cg_rew = atof(argv[19]);
  float c2g_rew = atof(argv[20]);
  cout << "** INFO: kl, kt reweight " << kl_rew << " " << kt_rew << " [kt < -990 || kl < -990 : no HH reweight]" << endl;
  cout << "**       c2, cg, c2g reweight " << c2_rew << " " << cg_rew << " " << c2g_rew << " [if any is < -990: will do only a klambda / kt reweight if requested]" << endl;

  string susyModel = argv[21];
  cout << "** INFO: requesting SUSY model to be: -" << susyModel << "- [NOTSUSY: no request on this parameter]" << endl;

  // ------------------  decide what to do for the reweight of HH samples
  enum HHrewTypeList {
    kNone      = 0,
    kFromHisto = 1,
    kDynamic   = 2
  };
  int HHrewType = kNone; // default is no reweight
  if (HHreweightFile && kl_rew >= -990 && kt_rew >= -990) {
    cout << "** WARNING: you required both histo based and dynamic reweight, cannot do both at the same time. Will set histo" << endl;
    HHrewType = kFromHisto;
  }
  else if (HHreweightFile)
    HHrewType = kFromHisto;
  else if (kl_rew >= -990 && kt_rew >= -990)
    HHrewType = kDynamic;
  cout << "** INFO: HH reweight type is " << HHrewType << " [ 0: no reweight, 1: from histo, 2: dynamic ]" << endl;



  // prepare variables needed throughout the code
  // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----    
  if (! (gConfigParser->init (config)))
    {
      cout << ">>> parseConfigFile::Could not open configuration file " << config << endl ;
      return -1 ;
    }

  bool   beInclusive         = gConfigParser->readBoolOption   ("selections::beInclusive") ;
  float  PUjetID_minCut      = gConfigParser->readFloatOption  ("parameters::PUjetIDminCut") ;
  float  PFjetID_WP          = gConfigParser->readIntOption    ("parameters::PFjetIDWP") ;
  // int    saveOS              = gConfigParser->readIntOption    ("parameters::saveOS") ;
  float  lepCleaningCone     = gConfigParser->readFloatOption  ("parameters::lepCleaningCone") ;
  int    bChoiceFlag         = gConfigParser->readFloatOption  ("parameters::bChoiceFlag") ;
  int    PUReweight_MC       = gConfigParser->readFloatOption  ("parameters::PUReweightMC") ; 
  int    PUReweight_target   = gConfigParser->readFloatOption  ("parameters::PUReweighttarget") ; 
  string leptonSelectionFlag = gConfigParser->readStringOption ("parameters::lepSelections") ;
  int maxNjetsSaved          = gConfigParser->readIntOption    ("parameters::maxNjetsSaved") ;
  
  enum sortingStrategy {
    kLLRFramDefault = 0, // taking order from LLR framework <--> ordered by MVA ID
    kHTauTau = 1,        // using HTauTau of lowest iso on 1st candidate, including (A,B) and (B,A)
    kPtAndRawIso = 2     // order each pair leg by pt (ptA > ptB), then compare *raw* iso of first leg
  };

  int sortStrategyThTh = 0;
  if (gConfigParser->isDefined("parameters::pairStrategy"))
    {
      sortStrategyThTh = gConfigParser->readIntOption("parameters::pairStrategy");
    }
  cout << "** INFO: thth sorting strategy? [0: kLLRFramDefault, 1: kHTauTau, 2: kPtAndRawIso]" << sortStrategyThTh << endl;

  ULong64_t debugEvent = -1; // will be converted to numerical max, and never reached
  if (gConfigParser->isDefined("parameters::debugEvent"))
    debugEvent = (ULong64_t) gConfigParser->readIntOption("parameters::debugEvent");


  vector<string> trigMuTau   =  (isMC ? gConfigParser->readStringListOption ("triggersMC::MuTau")  : gConfigParser->readStringListOption ("triggersData::MuTau")) ;
  vector<string> trigTauTau  =  (isMC ? gConfigParser->readStringListOption ("triggersMC::TauTau") : gConfigParser->readStringListOption ("triggersData::TauTau")) ;
  vector<string> trigEleTau  =  (isMC ? gConfigParser->readStringListOption ("triggersMC::EleTau") : gConfigParser->readStringListOption ("triggersData::EleTau")) ;
  // vector<string> trigEleMu   =  (isMC ? gConfigParser->readStringListOption ("triggersMC::EleMu")  : gConfigParser->readStringListOption ("triggersData::EleMu")) ;
  //I didn't store MuMu and I don't care for eleele
  vector<string> trigEleEle  =  (isMC ? gConfigParser->readStringListOption ("triggersMC::EleEle")  : gConfigParser->readStringListOption ("triggersData::EleEle")) ;
  vector<string> trigMuMu    =  (isMC ? gConfigParser->readStringListOption ("triggersMC::MuMu")  : gConfigParser->readStringListOption ("triggersData::MuMu")) ;
  // cross triggers for muTau and eleTau
  vector<string> crossTrigTauTau = (isMC ? gConfigParser->readStringListOption ("triggersMC::crossTauTau") : gConfigParser->readStringListOption ("triggersData::crossTauTau")) ;
  vector<string> crossTrigMuTau  = (isMC ? gConfigParser->readStringListOption ("triggersMC::crossMuTau")  : gConfigParser->readStringListOption ("triggersData::crossMuTau") ) ;
  vector<string> crossTrigEleTau = (isMC ? gConfigParser->readStringListOption ("triggersMC::crossEleTau") : gConfigParser->readStringListOption ("triggersData::crossEleTau")) ;
  vector<string> vbfTriggers     = (isMC ? gConfigParser->readStringListOption ("triggersMC::vbfTriggers") : gConfigParser->readStringListOption ("triggersData::vbfTriggers")) ;

  // bool applyTriggers = isMC ? false : true; // true if ask triggerbit + matching, false if doing reweight
  bool applyTriggers = isMC ? gConfigParser->readBoolOption ("parameters::applyTriggersMC") : true; // true if ask triggerbit + matching, false if doing reweight

  // applyTriggers = true;
  cout << "** INFO: apply triggers? " << applyTriggers << " [ 0: reweight , 1: triggerbit+matching ]" << endl;
  if (applyTriggers)
    {
      cout << "** INFO: triggers used in the skim : " << endl;
    
      cout << "  @ MuTau" << endl; cout << "   --> ";
      for (unsigned int i = 0 ; i < trigMuTau.size(); i++) cout << "  " << trigMuTau.at(i);
      cout << endl;

      cout << "  @ EleTau" << endl; cout << "   --> ";
      for (unsigned int i = 0 ; i < trigEleTau.size(); i++) cout << "  " << trigEleTau.at(i);
      cout << endl;

      cout << "  @ TauTau" << endl; cout << "   --> ";
      for (unsigned int i = 0 ; i < trigTauTau.size(); i++) cout << "  " << trigTauTau.at(i);
      cout << endl;

      cout << "  @ crossMuTau" << endl; cout << "   --> ";
      for (unsigned int i = 0 ; i < crossTrigMuTau.size(); i++) cout << "  " << crossTrigMuTau.at(i);
      cout << endl;

      cout << "  @ crossEleTau" << endl; cout << "   --> ";
      for (unsigned int i = 0 ; i < crossTrigEleTau.size(); i++) cout << "  " << crossTrigEleTau.at(i);
      cout << endl;
      
      cout << "  @ crossTauTau" << endl; cout << "   --> ";
      for (unsigned int i = 0 ; i < crossTrigTauTau.size(); i++) cout << "  " << crossTrigTauTau.at(i);
      cout << endl;

      cout << "  @ vbfTriggers" << endl; cout << "   --> ";
      for (unsigned int i = 0 ; i < vbfTriggers.size(); i++) cout << "  " << vbfTriggers.at(i);
      cout << endl;
    }

  // // histo reweight triggers
  // TFile* trigRewFiles  [3];
  // TH1F*  trigRewHistos [3];
  
  // trigRewFile[0]   = new TFile ();  // mu
  // trigRewHistos[0] = (TH1F*) trigRewFile[0] ->Get ();

  // trigRewFile[1]   = new TFile (); // ele
  // trigRewHistos[1] = (TH1F*) trigRewFile[1] ->Get ();

  // trigRewFile[2]   = 0; // tau : WARNING: UNUSED!!
  // trigRewHistos[2] = 0;
  TFile* trigRewEle = new TFile ("weights/ele25TightEff.root"); //FIXME: move to cfg ?
  TH1F*  trigRewEleHisto = (TH1F*) trigRewEle->Get("ele25TightEff");

  string bRegrWeights("");
  bool computeBregr = gConfigParser->readBoolOption ("bRegression::computeBregr");
  if (computeBregr) bRegrWeights = gConfigParser->readStringOption("bRegression::weights");

  cout << "** INFO: computing b jet regression? " << computeBregr << " with weights " << bRegrWeights << endl;

  // input and output setup
  // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
  TChain * bigChain = new TChain ("HTauTauTree/HTauTauTree") ;
  appendFromFileList (bigChain, inputFile);
  bigTree theBigTree (bigChain) ;

  //Create a new file + a clone of old tree header. Do not copy events
  TFile * smallFile = new TFile (outputFile, "recreate") ;
  smallFile->cd () ;
  smallTree theSmallTree ("HTauTauTree") ;

  // for HHKinFit
  int hypo_mh1 = 125;
  int hypo_mh2 = 125;

  // for efficiencies
  float totalEvents = 0. ;
  float selectedEvents = 0. ;
  int totalNoWeightsEventsNum = 0 ;
  int selectedNoWeightsEventsNum = 0 ;

  // for VBF trigger matching
  bool isVBFfired = false;

  // ------------------------------
  
  TH1F* hTriggers = getFirstFileHisto (inputFile);
  TH1F* hTauIDS = getFirstFileHisto (inputFile,false);
  /*triggerReader trigReader (hTriggers);
  trigReader.addTauTauTrigs (trigTauTau);
  trigReader.addMuTauTrigs  (trigMuTau);
  trigReader.addEleTauTrigs (trigEleTau);
  // trigReader.addMuEleTrigs  (trigEleMu);
  trigReader.addMuMuTrigs   (trigMuMu);
  trigReader.addEleEleTrigs (trigEleEle);

  // add crossTriggers
  trigReader.addMuTauCrossTrigs  (crossTrigMuTau);
  trigReader.addEleTauCrossTrigs (crossTrigEleTau);*/
  
  //FRA new triggerReader_cross to take into account the usage of crossTriggers
  triggerReader_cross trigReader (hTriggers);
  trigReader.addTauTauTrigs (trigTauTau);
  trigReader.addMuTauTrigs  (trigMuTau);
  trigReader.addEleTauTrigs (trigEleTau);
  trigReader.addMuMuTrigs   (trigMuMu);
  trigReader.addEleEleTrigs (trigEleEle);

  // add crossTriggers
  trigReader.addTauTauCrossTrigs (crossTrigTauTau);
  trigReader.addMuTauCrossTrigs  (crossTrigMuTau);
  trigReader.addEleTauCrossTrigs (crossTrigEleTau);

  // add VBF triggers for jet matching
  trigReader.addVBFTrigs (vbfTriggers);

  // ------------------------------

  OfflineProducerHelper oph (hTriggers, hTauIDS) ;

  // ------------------------------

  bJetRegrVars bjrv;
  TMVA::Reader *bRreader = new TMVA::Reader();
  bjrv.setReader (bRreader);
  string bRegrMethodName = "BDTG method";
  if (computeBregr)
    bRreader->BookMVA( bRegrMethodName.c_str(), bRegrWeights.c_str() ); 
  
  // ------------------------------

  //PUReweight reweight (PUReweight::RUN2ANALYSIS); // none : no PU reweight (always returns 1) - RUN2ANALYSIS: get weights according to MC and data targets
  PUReweight reweight (PUReweight::NONE); // none : no PU reweight (always returns 1) - RUN2ANALYSIS: get weights according to MC and data targets

  // ------------------------------

  string bTag_SFFile = gConfigParser->readStringOption("bTagScaleFactors::SFFile") ;
  string bTag_effFile = gConfigParser->readStringOption("bTagScaleFactors::effFile") ;
  cout << "B Tag SF file: " << bTag_SFFile << endl;
  //bTagSF bTagSFHelper (bTag_SFFile, bTag_effFile, "", "80X_MORIOND_2017"); // third field unused, but could be needed to select efficiencies for different selection levels
  bTagSF bTagSFHelper (bTag_SFFile, bTag_effFile, "", "94X_DeepCSV_V1"); // third field unused, but could be needed to select efficiencies for different selection levels

  // ------------------------------
  
  ScaleFactor * myScaleFactor[2][2]; // [0: mu, 1: ele] [0: trigger, 1: ID]
  for (int i = 0 ; i < 2; i++)
    for (int j = 0; j < 2; j++)
      myScaleFactor[i][j]= new ScaleFactor();
 
  // FIXME: move to cfg?
  myScaleFactor[0][0] -> init_ScaleFactor("weights/HTT_SF_2016/Muon/Run2016BtoH/Muon_IsoMu24_OR_TkIsoMu24_2016BtoH_eff.root");
  myScaleFactor[0][1] -> init_ScaleFactor("weights/HTT_SF_2016/Muon/Run2016BtoH/Muon_IdIso_IsoLt0p15_2016BtoH_eff.root");
  myScaleFactor[1][0] -> init_ScaleFactor("weights/HTT_SF_2016/Electron/Run2016BtoH/Electron_Ele25_eta2p1_WPTight_eff.root");
  myScaleFactor[1][1] -> init_ScaleFactor("weights/HTT_SF_2016/Electron/Run2016BtoH/Electron_IdIso_IsoLt0p1_eff.root");

  // muon POG SFs
  // TFile* fMuPOGSF_ID = new TFile ("weights/MuPogSF/MuonID_Z_RunBCD_prompt80X_7p65.root");
  // TFile* fMuPOGSF_ISO = new TFile ("weights/MuPogSF/MuonIso_Z_RunBCD_prompt80X_7p65.root");
  // TH2F* hMuPOGSF_ID  = (TH2F*) fMuPOGSF_ID -> Get("MC_NUM_TightIDandIPCut_DEN_genTracks_PAR_pt_spliteta_bin1/pt_abseta_ratio");  // pt: x, eta: y
  // TH2F* hMuPOGSF_ISO = (TH2F*) fMuPOGSF_ISO -> Get("MC_NUM_TightRelIso_DEN_TightID_PAR_pt_spliteta_bin1/pt_abseta_ratio"); // pt: x, eta: y
  
  // muon POG SFs - summer16 MC
  TFile* fMuPOGSF_ID_BF   = new TFile ("weights/MuPogSF/Summer16MC/IDSF/EfficienciesAndSF_BCDEF.root");
  TFile* fMuPOGSF_ISO_BF  = new TFile ("weights/MuPogSF/Summer16MC/ISOSF/EfficienciesAndSF_BCDEF.root");
  TFile* fMuPOGSF_trig_p3 = new TFile ("weights/MuPogSF/Summer16MC/TrigSF/EfficienciesAndSF_RunBtoF.root");

  TFile* fMuPOGSF_ID_GH   = new TFile ("weights/MuPogSF/Summer16MC/IDSF/EfficienciesAndSF_GH.root");
  TFile* fMuPOGSF_ISO_GH  = new TFile ("weights/MuPogSF/Summer16MC/ISOSF/EfficienciesAndSF_GH.root");
  TFile* fMuPOGSF_trig_p4 = new TFile ("weights/MuPogSF/Summer16MC/TrigSF/EfficienciesAndSF_Period4.root");

  TH2F* hMuPOGSF_ID_BF    = (TH2F*) fMuPOGSF_ID_BF -> Get("MC_NUM_TightID_DEN_genTracks_PAR_pt_eta/pt_abseta_ratio");  // pt: x, eta: y
  TH2F* hMuPOGSF_ISO_BF   = (TH2F*) fMuPOGSF_ISO_BF -> Get("TightISO_TightID_pt_eta/pt_abseta_ratio"); // pt: x, eta: y
  TH2F* hMuPOGSF_trig_p3  = (TH2F*) fMuPOGSF_trig_p3 -> Get("IsoMu24_OR_IsoTkMu24_PtEtaBins/pt_abseta_ratio"); // pt: x, eta: y

  TH2F* hMuPOGSF_ID_GH    = (TH2F*) fMuPOGSF_ID_GH -> Get("MC_NUM_TightID_DEN_genTracks_PAR_pt_eta/pt_abseta_ratio");  // pt: x, eta: y
  TH2F* hMuPOGSF_ISO_GH   = (TH2F*) fMuPOGSF_ISO_GH -> Get("TightISO_TightID_pt_eta/pt_abseta_ratio"); // pt: x, eta: y
  TH2F* hMuPOGSF_trig_p4  = (TH2F*) fMuPOGSF_trig_p4 -> Get("IsoMu24_OR_IsoTkMu24_PtEtaBins/pt_abseta_ratio"); // pt: x, eta: y

  // // for loose ID:
  // MC_NUM_LooseID_DEN_genTracks_PAR_pt_spliteta_bin1
  // MC_NUM_LooseRelIso_DEN_TightID_PAR_pt_spliteta_bin1

  // tau trigger SFs from Riccardo Summer 2016 MC
  tauTrigSFreader tauTrgSF ("weights/tau_trigger_SF_2016.root");

  // --- 2017 SFs ---
  // EleGamma POG SFs
  TFile* fElePOGSF_TightID_80WP  = new TFile ("weights/SF_2017/gammaEffi.txt_EGM2D_runBCDEF_passingMVA94Xwp80iso.root");
  TFile* fElePOGSF_MediumID_90WP = new TFile ("weights/SF_2017/gammaEffi.txt_EGM2D_runBCDEF_passingMVA94Xwp90iso.root");
  
  TH2* hElePOGSF_TightID_80WP  = (TH2*) fElePOGSF_TightID_80WP  -> Get("EGamma_SF2D");
  TH2* hElePOGSF_MediumID_90WP = (TH2*) fElePOGSF_MediumID_90WP -> Get("EGamma_SF2D");
  
  //Muon POG SFs
  TFile* fMuPOGSF_ID  = new TFile ("weights/SF_2017/Muon_RunBCDEF_SF_ID.root");
  TFile* fMuPOGSF_ISO = new TFile ("weights/SF_2017/Muon_RunBCDEF_SF_ISO.root");
  
  TH2* hMuPOGSF_ID  = (TH2*) fMuPOGSF_ID  -> Get("NUM_TightID_DEN_genTracks_pt_abseta");
  TH2* hMuPOGSF_ISO = (TH2*) fMuPOGSF_ISO -> Get("NUM_TightRelIso_DEN_TightIDandIPCut_pt_abseta");

  // ------------------------------
  // smT2 mt2Class = smT2();

  // ------------------------------
  // reweighting file for HH non resonant
  
  TH1F* hreweightHH   = 0;
  TH2F* hreweightHH2D = 0;
  // if (HHreweightFile)
  if (HHrewType == kFromHisto)
    {
      cout << "** INFO: doing reweight for HH samples" << endl;
      if (HHreweightFile->GetListOfKeys()->Contains("hratio") )
	{  
	  hreweightHH = (TH1F*) HHreweightFile->Get("hratio");
	  cout << "** INFO: 1D reweight using hratio" << endl;
	}
      else if (HHreweightFile->GetListOfKeys()->Contains("hratio2D") )
	{
	  hreweightHH2D = (TH2F*) HHreweightFile->Get("hratio2D");            
	  cout << "** INFO: 2D reweight using hratio2D" << endl;
	}
      else
	{
	  cout << "** ERROR: reweight histo not found in file provided, stopping execuction" << endl;
	  return 1;
	}
    }

  // ------------------------------
  // reweight file in case of "dynamic" reweight
  // there is a unique input map, read it from the cfg file
  // HHReweight* hhreweighter = nullptr;
  HHReweight5D* hhreweighter = nullptr;
  TH2* hhreweighterInputMap = nullptr;
  if (HHrewType == kDynamic)
    {
      string inMapFile   = gConfigParser->readStringOption("HHReweight::inputFile");
      string inHistoName = gConfigParser->readStringOption("HHReweight::histoName");
      string coeffFile    = gConfigParser->readStringOption("HHReweight::coeffFile");
      cout << "** INFO: reading histo named: " << inHistoName << " from file: " << inMapFile << endl;
      cout << "** INFO: HH reweight coefficient file is: " << coeffFile << endl;
      TFile* fHHDynamicRew = new TFile(inMapFile.c_str());
      hhreweighterInputMap = (TH2*) fHHDynamicRew->Get(inHistoName.c_str());
      // hhreweighter = new HHReweight(coeffFile, hhreweighterInputMap);
      hhreweighter = new HHReweight5D(coeffFile, hhreweighterInputMap);
    }


  // ------------------------------
  // indexes of tau ID bits

  map <string, int> tauIDsMap;
  for (int ibin = 0; ibin < hTauIDS->GetNbinsX(); ++ibin)
    {
      if (string(hTauIDS->GetXaxis()->GetBinLabel(ibin+1)) == string("byLooseCombinedIsolationDeltaBetaCorr3Hits") )
	tauIDsMap ["byLooseCombinedIsolationDeltaBetaCorr3Hits"] = ibin ;

      if (string(hTauIDS->GetXaxis()->GetBinLabel(ibin+1)) == string("byMediumCombinedIsolationDeltaBetaCorr3Hits") )
	tauIDsMap ["byLooseCombinedIsolationDeltaBetaCorr3Hits"] = ibin ;

      if (string(hTauIDS->GetXaxis()->GetBinLabel(ibin+1)) == string("byTightCombinedIsolationDeltaBetaCorr3Hits") )
	tauIDsMap ["byLooseCombinedIsolationDeltaBetaCorr3Hits"] = ibin ;
    }

  // MVA tau ID
  vector<int> tauMVAIDIdx;
  tauMVAIDIdx.push_back(getTauIDIdx(hTauIDS, "byVLooseIsolationMVArun2v1DBoldDMwLT"));
  tauMVAIDIdx.push_back(getTauIDIdx(hTauIDS, "byLooseIsolationMVArun2v1DBoldDMwLT"));
  tauMVAIDIdx.push_back(getTauIDIdx(hTauIDS, "byMediumIsolationMVArun2v1DBoldDMwLT"));
  tauMVAIDIdx.push_back(getTauIDIdx(hTauIDS, "byTightIsolationMVArun2v1DBoldDMwLT"));
  tauMVAIDIdx.push_back(getTauIDIdx(hTauIDS, "byVTightIsolationMVArun2v1DBoldDMwLT"));
  if (find(tauMVAIDIdx.begin(), tauMVAIDIdx.end(), -1) != tauMVAIDIdx.end())
    {
      cout << "** WARNING!! did not found some MVA tau IDs" << endl;
      for (unsigned int i = 0; i < tauMVAIDIdx.size(); ++i)
	cout << tauMVAIDIdx.at(i) << " " ;
      cout << endl;
    }
    
  // new MVA tau ID // FRA syncFeb2018
  vector<int> tauMVAIDIdxNew;
  tauMVAIDIdxNew.push_back(getTauIDIdx(hTauIDS, "byVLooseIsolationMVArun2017v2DBoldDMwLT2017"));
  tauMVAIDIdxNew.push_back(getTauIDIdx(hTauIDS, "byLooseIsolationMVArun2017v2DBoldDMwLT2017"));
  tauMVAIDIdxNew.push_back(getTauIDIdx(hTauIDS, "byMediumIsolationMVArun2017v2DBoldDMwLT2017"));
  tauMVAIDIdxNew.push_back(getTauIDIdx(hTauIDS, "byTightIsolationMVArun2017v2DBoldDMwLT2017"));
  tauMVAIDIdxNew.push_back(getTauIDIdx(hTauIDS, "byVTightIsolationMVArun2017v2DBoldDMwLT2017"));
  if (find(tauMVAIDIdxNew.begin(), tauMVAIDIdxNew.end(), -1) != tauMVAIDIdxNew.end())
    {
      cout << "** WARNING!! did not found some MVA tau IDs New" << endl;
      for (unsigned int i = 0; i < tauMVAIDIdxNew.size(); ++i)
	cout << tauMVAIDIdxNew.at(i) << " " ;
      cout << endl;
    }

  // new MVA tau ID // FRA syncFeb2018
  vector<int> tauMVAIDIdxNewdR0p3;
  tauMVAIDIdxNewdR0p3.push_back(getTauIDIdx(hTauIDS, "byVLooseIsolationMVArun2017v2DBoldDMdR0p3wLT2017"));
  tauMVAIDIdxNewdR0p3.push_back(getTauIDIdx(hTauIDS, "byLooseIsolationMVArun2017v2DBoldDMdR0p3wLT2017"));
  tauMVAIDIdxNewdR0p3.push_back(getTauIDIdx(hTauIDS, "byMediumIsolationMVArun2017v2DBoldDMdR0p3wLT2017"));
  tauMVAIDIdxNewdR0p3.push_back(getTauIDIdx(hTauIDS, "byTightIsolationMVArun2017v2DBoldDMdR0p3wLT2017"));
  tauMVAIDIdxNewdR0p3.push_back(getTauIDIdx(hTauIDS, "byVTightIsolationMVArun2017v2DBoldDMdR0p3wLT2017"));
  if (find(tauMVAIDIdxNewdR0p3.begin(), tauMVAIDIdxNewdR0p3.end(), -1) != tauMVAIDIdxNewdR0p3.end())
    {
      cout << "** WARNING!! did not found some MVA tau IDs New" << endl;
      for (unsigned int i = 0; i < tauMVAIDIdxNewdR0p3.size(); ++i)
	cout << tauMVAIDIdxNewdR0p3.at(i) << " " ;
      cout << endl;
    }

  // cut based tau ID
  vector<int> tauCUTIDIdx;
  tauCUTIDIdx.push_back(getTauIDIdx(hTauIDS, "byLooseCombinedIsolationDeltaBetaCorr3Hits"));
  tauCUTIDIdx.push_back(getTauIDIdx(hTauIDS, "byMediumCombinedIsolationDeltaBetaCorr3Hits"));
  tauCUTIDIdx.push_back(getTauIDIdx(hTauIDS, "byTightCombinedIsolationDeltaBetaCorr3Hits"));
  if (find(tauCUTIDIdx.begin(), tauCUTIDIdx.end(), -1) != tauCUTIDIdx.end())
    {
      cout << "** WARNING!! did not found some cut-based tau IDs" << endl;
      for (unsigned int i = 0; i < tauCUTIDIdx.size(); ++i)
	cout << tauCUTIDIdx.at(i) << " " ;
      cout << endl;
    }


  // anti-ele discr
  vector<int> tauAntiEleIdx;
  tauAntiEleIdx.push_back(getTauIDIdx(hTauIDS, "againstElectronVLooseMVA6"));
  tauAntiEleIdx.push_back(getTauIDIdx(hTauIDS, "againstElectronLooseMVA6"));
  tauAntiEleIdx.push_back(getTauIDIdx(hTauIDS, "againstElectronMediumMVA6"));
  tauAntiEleIdx.push_back(getTauIDIdx(hTauIDS, "againstElectronTightMVA6"));
  tauAntiEleIdx.push_back(getTauIDIdx(hTauIDS, "againstElectronVTightMVA6"));

  // anti-mu discr
  vector<int> tauAntiMuIdx;
  tauAntiMuIdx.push_back(getTauIDIdx(hTauIDS, "againstMuonLoose3"));
  tauAntiMuIdx.push_back(getTauIDIdx(hTauIDS, "againstMuonTight3"));

  // -----------------------------------
  // event counters for efficiency study
  EffCounter ec;
  ec.AddMarker ("all");
  ec.AddMarker ("METfilter");
  ec.AddMarker ("NoBadMuons");
  ec.AddMarker ("PairExists");
  ec.AddMarker ("PairFoundBaseline");
  ec.AddMarker ("Trigger");
  ec.AddMarker ("TwoJets");

  // for hh signal only -- split by gen decay
  EffCounter* ecHHsig;
  if (isHHsignal)
    {
      ecHHsig = new EffCounter[6];
      for (int ic = 0; ic < 6; ++ic)
	{
	  ecHHsig[ic].AddMarker ("all");
	  ecHHsig[ic].AddMarker ("METfilter");
	  ecHHsig[ic].AddMarker ("NoBadMuons");
	  ecHHsig[ic].AddMarker ("PairExists");
	  ecHHsig[ic].AddMarker ("PairFoundBaseline");
	  ecHHsig[ic].AddMarker ("PairMatchesGen");
	  ecHHsig[ic].AddMarker ("Trigger");
	  ecHHsig[ic].AddMarker ("TwoJets");      
	}
    }

  // loop over events
  // ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
  for (Long64_t iEvent = 0 ; true ; ++iEvent) 
    {
      if (iEvent % 10000 == 0)  cout << "- reading event " << iEvent << endl ;
      // cout << "- reading event " << iEvent << endl ;
      theSmallTree.clearVars () ;
      int got = theBigTree.fChain->GetEntry(iEvent);
      if (got == 0) break;
      bool DEBUG = false;
      //if (theBigTree.EventNumber != debugEvent) continue; //FRA debug
      if (theBigTree.EventNumber == debugEvent )
	{
	  cout << "****** DEBUG : debugging event=" << theBigTree.EventNumber << " run=" << theBigTree.RunNumber << " lumi=" << theBigTree.lumi << " (entry number=" << iEvent << ")" << endl;
	  DEBUG = true;
	}

      // remove a lumisection that was present in 16 Giu JSON and removed in 22 and subsequent JSON
      // 25 Nov 2016 : edit : removed line because of new reprocessing and json
      // if (!isMC && theBigTree.RunNumber == 274094 && theBigTree.lumi >= 105 && theBigTree.lumi <= 107) continue;

      // directly reject events outside HT range in case of stitching of inclusive sample-- they should not count in weights
      if (HTMax > 0)
	{
	  if (theBigTree.lheHt > HTMax) continue;
	}

      // directly reject events I want to remove in W+Jets stitching on njets
      if (NjetRequired >= 0)
	{
	  if (theBigTree.lheNOutPartons != NjetRequired) continue;
	}

      // skip event if I want a specific SUSY point from the fastsim
      if (susyModel != string("NOTSUSY"))
	{
	  if (string(theBigTree.susyModel.Data()) != susyModel) continue;  
	}

      float stitchWeight = 1.0;
      if (DY_tostitch)
	{
	  int njets = theBigTree.lheNOutPartons;
	  int nb    = theBigTree.lheNOutB;
	  // these protections should be useless
	  if (njets < 0) njets = 0;
	  if (njets > 4) njets = 4;
	  if (nb < 0)    nb = 0;
	  if (nb > 4)    nb = 4;

	  stitchWeight = stitchWeights[njets][nb];
	}


      if (DEBUG && isMC)
	{
	  cout << "** DEBUG : gen particle list" << endl;
	  for (unsigned int igen = 0; igen < theBigTree.genpart_pdg->size(); igen++)
	    {
	      int pdg = theBigTree.genpart_pdg->at(igen);
	      if (abs(pdg) == 66615 || abs(pdg) == 11 || abs(pdg) == 13)
		{
		  TLorentzVector vGPDebug;
		  vGPDebug.SetPxPyPzE (theBigTree.genpart_px->at(igen), theBigTree.genpart_py->at(igen), theBigTree.genpart_pz->at(igen), theBigTree.genpart_e->at(igen) ) ;
		  cout << igen << " pdg=" << pdg << " pt=" << vGPDebug.Pt() << " eta=" << vGPDebug.Eta() << " phi=" << vGPDebug.Phi() << endl;          
		}
	      if (abs(pdg) == 25)
		{
		  cout << igen << " pdg=" << pdg << " decay=" << theBigTree.genpart_HZDecayMode->at(igen) << endl; 
		}
	    }
	}

      // gen info -- fetch tt pair and compute top PT reweight
      float topPtReweight = 1.0; // 1 for all the other samples      
      theSmallTree.m_TTtopPtreweight =  1.0 ;
      theSmallTree.m_TTtopPtreweight_up =  1.0 ;
      theSmallTree.m_TTtopPtreweight_down =  1.0 ;
      if (isTTBar)
	{
	  float ptTop1 = -1.0;
	  float ptTop2 = -1.0;
	  int decayTop1 = -999;
	  int decayTop2 = -999;
	  int pdgIdTop1 = -999;
	  int pdgIdTop2 = -999;

	  for (unsigned int igen = 0; igen < theBigTree.genpart_pdg->size(); igen++)
	    {
	      int pdg = theBigTree.genpart_pdg->at(igen);
	      int genflags = theBigTree.genpart_flags->at(igen);
	      bool isFirst = CheckBit (genflags, 12);  //if (fl.isFirstCopy()) flags |= (1 << 12);
	      //int topDM = theBigTree.genpart_TopDecayMode->at(igen);

	      if (abs(pdg) == 6 && isFirst) // top -- pt reweight wants to have ME tops
		{
		  TLorentzVector TopV;
		  TopV.SetPxPyPzE (theBigTree.genpart_px->at(igen), theBigTree.genpart_py->at(igen), theBigTree.genpart_pz->at(igen), theBigTree.genpart_e->at(igen) ) ;
		  if (ptTop1 < 0)
		    {
		      ptTop1 = TopV.Pt();
		      decayTop1 = theBigTree.genpart_TopDecayMode->at(igen);
		      pdgIdTop1 = theBigTree.genpart_pdg->at(igen);
		    }
		  else if (ptTop2 < 0)
		    { 
		      ptTop2 = TopV.Pt(); 
		      decayTop2 = theBigTree.genpart_TopDecayMode->at(igen);
		      pdgIdTop2 = theBigTree.genpart_pdg->at(igen);
		      break;
		    } // check done in paralles shows that I never have > 2 top .  break is safe .
		  // else cout << " !! skim warning: sample is declared as as ttbar, but I have > 2 gen top in the event! " << endl;
		}
	    }
	  if (ptTop1 < 0 || ptTop2 < 0)
	    {
	      cout << "** WARNING: sample is declared as TTbar but in the event I didn't find 2 tops (1,2) :" << ptTop1 << " " << ptTop2 << endl;
	    }
	  else
	    {
	      // filter by decay mode if needed for stitching
	      // [0: no stitch , 1: fully had, 2: semilept t, 3: semilept tbar, 4: fully lept, 5: semilept all]
	      // TopDecayMode: 0: Had, 1-5: leptonic, 6: other -- consider "other" as a possible hadronic decay (includes rare W->bc)
          
	      bool isT1Lept = (decayTop1 >= 1 && decayTop1 <= 5) ;
	      bool isT2Lept = (decayTop2 >= 1 && decayTop2 <= 5) ;
	      if (pdgIdTop1 / pdgIdTop2 > 0) cout << "** WARNING: I found two tops with the same sign " << pdgIdTop1 << " " << pdgIdTop2 << endl;
          
	      // cout << "DEBUG: event with " << pdgIdTop1 << "-->  " << decayTop1 << " | " << pdgIdTop2 << "--> " << decayTop2 << " SKIMTYPE=" << TT_stitchType << endl;
	      switch (TT_stitchType)
		{
		case 0:
		  break; // no stitching
            
		case 1:
		  if (isT1Lept || isT2Lept) continue; // fully had
		  break;
            
		case 2: // top leptonic, antitop hadronic
		  if (pdgIdTop1 > 0) // 1 is top
		    {
		      if (! (isT1Lept && !isT2Lept) ) continue;
		    }
		  else // 1 is antitop
		    {
		      if (! (isT2Lept && !isT1Lept) ) continue;
		    }
		  break;
            
		case 3:
		  if (pdgIdTop1 > 0) // 1 is top
		    {
		      if (! (!isT1Lept && isT2Lept) ) continue;
		    }
		  else // 1 is antitop
		    {
		      if (! (!isT2Lept && isT1Lept) ) continue;
		    }
		  break;
            
		case 4:
		  if (!isT1Lept || !isT2Lept) continue;
		  break;

		case 5:
		  if (isT1Lept == isT2Lept) continue; // must be one had and the other lep, not equal
		  break;
            
		default:
		  cout << "** WARNING: unknown TT stytch type " << TT_stitchType << endl;
		}

	      // cout << "WAS ACCEPTED" << endl;

	      if(DEBUG)
		{
		  cout << "@ TOP pt reweight: " << endl;
		  cout << "  top1 pt=" << ptTop1 << endl;
		  cout << "  top2 pt=" << ptTop2 << endl;
		}

	      float SFTop1 = TMath::Exp(aTopRW+bTopRW*ptTop1);
	      float SFTop2 = TMath::Exp(aTopRW+bTopRW*ptTop2);
	      topPtReweight = TMath::Sqrt (SFTop1*SFTop2);
          
	      // old recipe
	      // theSmallTree.m_TTtopPtreweight      = topPtReweight ;
	      // theSmallTree.m_TTtopPtreweight_up   = topPtReweight*topPtReweight ;
	      // theSmallTree.m_TTtopPtreweight_down = 1.0 ;

	      // new recipe
	      theSmallTree.m_TTtopPtreweight      = 1.0; // nominal has no weight
	      theSmallTree.m_TTtopPtreweight_up   = topPtReweight; // pt rew to be used as a syst
	      theSmallTree.m_TTtopPtreweight_down = 1.0 ; // and down unused

	      theSmallTree.m_genDecMode1 = decayTop1;
	      theSmallTree.m_genDecMode2 = decayTop2;
	    }
	}

      // For Drell-Yan only: loop on genjets and count how many are there with 0,1,2 b
      // 0: 0bjet, 1: 1 b jet, 2: >= 2 b jet
      theSmallTree.m_DYscale_LL = 1.0; // all the other MC samples + data have no weight
      theSmallTree.m_DYscale_MM = 1.0;        
      // if (isMC && (DY_Nbs || isHHsignal))
      if (isMC && DY_Nbs)
	{
	  TLorentzVector vgj;
	  int nbs = 0;
	  for (unsigned int igj = 0; igj < theBigTree.genjet_px->size(); igj++)
	    {
	      vgj.SetPxPyPzE(theBigTree.genjet_px->at(igj), theBigTree.genjet_py->at(igj), theBigTree.genjet_pz->at(igj), theBigTree.genjet_e->at(igj));
	      if (vgj.Pt() > 20 && TMath::Abs(vgj.Eta()) < 2.5)
		{

		  int theFlav = theBigTree.genjet_hadronFlavour->at(igj);
		  if (abs(theFlav) == 5) nbs++;
              
		  // about 2% of DY events print the following message :-(
		  // if (theFlav == -999) cout << "** warning: gen jet with flav = -999 of pt: " << vgj.Pt() << " eta: " << vgj.Eta() << endl;
		}

	      if(DEBUG)
		{
		  cout << " -- gen jet : " << igj << " pt=" << vgj.Pt() << " eta=" << vgj.Eta() <<  " hadFlav=" << theBigTree.genjet_hadronFlavour->at(igj) << endl;
		}

	    }
	  if (nbs > 2) nbs = 2;
	  theSmallTree.m_nBhadrons = nbs;

	  theSmallTree.m_DYscale_LL = DYscale_LL[nbs];
	  theSmallTree.m_DYscale_MM = DYscale_MM[nbs];

	  // loop through gen parts ot identify Z boson 
	  int idx1 = -1;
	  for (unsigned int igen = 0; igen < theBigTree.genpart_px->size(); igen++)
	    {
	      if (theBigTree.genpart_pdg->at(igen) == 23) // Z0
		{
		  // bool isFirst = CheckBit (theBigTree.genpart_flags->at(igen), 12) ; // 12 = isFirstCopy
		  if (idx1 >= 0)
		    {
		      cout << "** ERROR: more than 1 Z identified " << endl;
		      // continue; // no need to skip the event for errors in gen info
		    }
		  idx1 = igen;
		}
	    }

	  if (idx1 >= 0)
	    {
	      // cout << "** GOOD: could find 1 Z" << endl;
	      // store gen decay mode of the Z identified
	      theSmallTree.m_genDecMode1 = theBigTree.genpart_HZDecayMode->at(idx1);
	    }
	  // else // probably these are events mediated by a photon where I do not have Z info
	  // {
	  //   cout << "** ERROR: couldn't find 1 Z" << endl;
	  // }
	}
      // HH reweight for non resonant
      float HHweight = 1.0;
      TLorentzVector vHardScatter1;
      TLorentzVector vHardScatter2;
      int idx1hs = -1; // hard scatted product
      int idx2hs = -1;
      int pdg1hs = -999;
      int pdg2hs = -999;    
      int t1hs = -1;
      int t2hs = -1;

      // if (hreweightHH || hreweightHH2D || isHHsignal) // isHHsignal: only to do loop on genparts, but no rew
      if (isHHsignal || HHrewType == kFromHisto || HHrewType == kDynamic) // isHHsignal: only to do loop on genparts, but no rew
	{
	  // cout << "DEBUG: reweight!!!" << endl;
	  TLorentzVector vH1, vH2, vBoost, vSum;
	  float mHH = -1;
	  float ct1 = -999;
	  // loop on gen to find Higgs
	  int idx1 = -1;
	  int idx2 = -1;
	  int idx1last = -1;
	  int idx2last = -1;
	  // cout << " ------------------------ " << endl;
	  for (unsigned int igen = 0; igen < theBigTree.genpart_px->size(); igen++)
	    {
	      bool isFirst     = CheckBit (theBigTree.genpart_flags->at(igen), 12) ; // 12 = isFirstCopy
	      bool isLast      = CheckBit (theBigTree.genpart_flags->at(igen), 13) ; // 13 = isLastCopy
	      bool isHardScatt = CheckBit (theBigTree.genpart_flags->at(igen), 5) ; //   3 = isPromptTauDecayProduct
	      // bool isDirectPromptTauDecayProduct = CheckBit (theBigTree.genpart_flags->at(igen), 5) ; // 5 = isDirectPromptTauDecayProduct
	      int pdg = theBigTree.genpart_pdg->at(igen);
	      int mothIdx = theBigTree.genpart_TauMothInd->at(igen);
        
	      bool mothIsHardScatt = false;
	      if (mothIdx > -1)
		{
		  bool mothIsLast =  CheckBit(theBigTree.genpart_flags->at(mothIdx), 13);
		  // NB: I need t ask that the mother is last idx, otherwise I get a nonphysics "tauh" by the tauh builder function from the tau->tau "decay" in pythia
		  mothIsHardScatt = (mothIsLast && CheckBit (theBigTree.genpart_flags->at(mothIdx), 8)); // 0 = isPrompt(), 7: hardProcess , 8: fromHardProcess
		}

	      // if (abs(pdg) == 11 || abs(pdg) == 13 || abs(pdg) == 15 || abs(pdg) == 66615)
	      // {
	      //   bitset<32> bs (theBigTree.genpart_flags->at(igen)) ; 
	      //   cout << "/// igen = " << igen << " pdgId " << pdg << " flag=" << bs << " mothidx=" <<  theBigTree.genpart_TauMothInd->at(igen) << " px=" << theBigTree.genpart_px->at(igen) << endl;
	      //   // cout << "/// igen = " << igen << " pdgId " << pdg << " isFirst=" << isFirst << " isLast=" << isLast << " isHardScatt=" << isHardScatt << " mothIsHardScatt=" << mothIsHardScatt << " isDirectPromptTauDecayProduct=" << isDirectPromptTauDecayProduct << " mothIdx=" << theBigTree.genpart_TauMothInd->at(igen) << endl;
	      // }


	      if (abs(pdg) == 25)
		{
		  // cout << igen << " H boson: Px " << theBigTree.genpart_px->at(igen) << " first? " << isFirst << " decMode : " << theBigTree.genpart_HZDecayMode->at(igen) << endl;
		  if (isFirst)
		    {
		      if (idx1 >= 0 && idx2 >= 0)
			{
			  cout << "** ERROR: more than 2 H identified (first)" << endl;
			  continue;
			}
		      (idx1 == -1) ? (idx1 = igen) : (idx2 = igen) ;
		    }
		  if (isLast)
		    {
		      if (idx1last >= 0 && idx2last >= 0)
			{
			  cout << "** ERROR: more than 2 H identified (last)" << endl;
			  // continue; // no need to skip the event in this case -- dec mode just for studies
			}
		      (idx1last == -1) ? (idx1last = igen) : (idx2last = igen) ;
		    }
		}
        
	      if ( (abs(pdg) == 11 || abs(pdg) == 13 ) && isHardScatt && isLast && mothIsHardScatt)
		{
		  if (idx1hs == -1) idx1hs = igen;
		  else if (idx2hs == -1) idx2hs = igen;
		  else
		    {
		      cout << "** ERROR: there are more than 2 hard scatter tau dec prod: evt = " << theBigTree.EventNumber << endl;
		      // cout << "idx1: " << idx1hs << " --> pdg = " << theBigTree.genpart_pdg->at(idx1hs) << " px = " << theBigTree.genpart_px->at(idx1hs) << endl;
		      // cout << "idx2: " << idx2hs << " --> pdg = " << theBigTree.genpart_pdg->at(idx2hs) << " px = " << theBigTree.genpart_px->at(idx2hs) << endl;
		      // cout << "THIS: " << pdg << " px=" << theBigTree.genpart_px->at(igen) << endl;
		    }
		}
        
	      if ( abs(pdg) == 66615 && mothIsHardScatt)
		{
		  // cout << "  <<< preso" << endl;
		  if (idx1hs == -1) idx1hs = igen;
		  else if (idx2hs == -1) idx2hs = igen;
		  else
		    {
		      cout << "** ERROR: there are more than 2 hard scatter tau dec prod: evt = " << theBigTree.EventNumber << endl;
		      // cout << "idx1: " << idx1hs << " --> pdg = " << theBigTree.genpart_pdg->at(idx1hs) << " px = " << theBigTree.genpart_px->at(idx1hs) << endl;
		      // cout << "idx2: " << idx2hs << " --> pdg = " << theBigTree.genpart_pdg->at(idx2hs) << " px = " << theBigTree.genpart_px->at(idx2hs) << endl;
		      // cout << "THIS: " << pdg << " px=" << theBigTree.genpart_px->at(igen) << endl;
		    }
		}
	    }

	  if (idx1 == -1 || idx2 == -1)
	    {
	      cout << "** ERROR: couldn't find 2 H (first)" << endl;
	      continue;
	    }

	  if (idx1last != -1 && idx2last != -1) // this is not critical if not found
	    {
	      // store gen decay mode of the two H identified
	      theSmallTree.m_genDecMode1 = theBigTree.genpart_HZDecayMode->at(idx1last);
	      theSmallTree.m_genDecMode2 = theBigTree.genpart_HZDecayMode->at(idx2last);
        
	      // cout << "THIS H decay mode: " << theSmallTree.m_genDecMode1 << " " << theSmallTree.m_genDecMode2 << endl;

	      // // get tau decaying one
	      // int idxTauDecayed = (theBigTree.genpart_HZDecayMode->at(idx1last) != 8 ? idx1last : idx2last);

	      // // find hard scatter daughters and check if they match this decay type
	      // pair<int, int> hsProds = oph.getHardScatterSons()
	      // int hsIdx1 = hsProds.first;
	      // int hsIdx2 = hsProds.second;
	    }
	  else
	    cout << "** ERROR: couldn't find 2 H (last)" << endl;

	  if (idx1hs != -1 && idx2hs != -1)
	    {
	      pdg1hs = theBigTree.genpart_pdg->at(idx1hs);
	      pdg2hs = theBigTree.genpart_pdg->at(idx2hs);

	      if      (abs(pdg1hs) == 11) t1hs = 1;
	      else if (abs(pdg1hs) == 13) t1hs = 0;
	      else                        t1hs = 2;

	      if      (abs(pdg2hs) == 11) t2hs = 1;
	      else if (abs(pdg2hs) == 13) t2hs = 0;
	      else                        t2hs = 2;

	      if (oph.getPairType(t1hs, t2hs) != (theSmallTree.m_genDecMode1 + theSmallTree.m_genDecMode2 - 8))
		{
		  cout << "** ERROR: decay modes do not match! " << theBigTree.genpart_pdg->at(idx1hs) << " " << theBigTree.genpart_pdg->at(idx2hs) << " != "
		       << ( theSmallTree.m_genDecMode1 + theSmallTree.m_genDecMode2 - 8) << endl;
		}
	      vHardScatter1.SetPxPyPzE (theBigTree.genpart_px->at(idx1hs), theBigTree.genpart_py->at(idx1hs), theBigTree.genpart_pz->at(idx1hs), theBigTree.genpart_e->at(idx1hs));
	      vHardScatter2.SetPxPyPzE (theBigTree.genpart_px->at(idx2hs), theBigTree.genpart_py->at(idx2hs), theBigTree.genpart_pz->at(idx2hs), theBigTree.genpart_e->at(idx2hs));
	    }
	  else
	    cout << "** ERROR: couldn't find 2 H->tautau gen dec prod " << idx1hs << " " << idx2hs << endl;


	  vH1.SetPxPyPzE (theBigTree.genpart_px->at(idx1), theBigTree.genpart_py->at(idx1), theBigTree.genpart_pz->at(idx1), theBigTree.genpart_e->at(idx1) );
	  vH2.SetPxPyPzE (theBigTree.genpart_px->at(idx2), theBigTree.genpart_py->at(idx2), theBigTree.genpart_pz->at(idx2), theBigTree.genpart_e->at(idx2) );
	  vSum = vH1+vH2;
	  mHH = vSum.M();
	  vH1.Boost(-vSum.BoostVector());                     
	  ct1 = vH1.CosTheta();

	  // assign a weight depending on the reweight type 

	  if (hreweightHH && HHrewType == kFromHisto) // 1D
	    {
	      int ibin = hreweightHH->FindBin(mHH);
	      HHweight = hreweightHH->GetBinContent(ibin);
	    }
	  else if (hreweightHH2D && HHrewType == kFromHisto) // 2D
	    {
	      int ibin = hreweightHH2D->FindBin(mHH, ct1);
	      HHweight = hreweightHH2D->GetBinContent(ibin);        
	    }
	  else if (HHrewType == kDynamic)
	    {
	      // HHweight = hhreweighter->getWeight(kl_rew, kt_rew, mHH, ct1);
	      if (c2_rew < -990 || cg_rew < -990 || c2g_rew < -990) // no valid BSM coefficients -- just kl/kt reweight (for backwards compatibility)
		HHweight = hhreweighter->getWeight(kl_rew, kt_rew, mHH, ct1);
	      else // full 5D reweight
		HHweight = hhreweighter->getWeight(kl_rew, kt_rew, c2_rew, cg_rew, c2g_rew, mHH, ct1);
	    }

	  theSmallTree.m_genMHH = mHH;
	  theSmallTree.m_genCosth = ct1;

	  // cout << " ........... GEN FINISHED ........... " << " evt=" << theBigTree.EventNumber << " run=" << theBigTree.RunNumber << " lumi=" << theBigTree.lumi << endl;

	}

      ///////////////////////////////////////////////////////////
      // END of gen related stuff -- compute tot number of events

      int genHHDecMode = (isHHsignal ? theSmallTree.m_genDecMode1 + theSmallTree.m_genDecMode2 - 8 : 0);
      if (genHHDecMode < 0)
	{
	  genHHDecMode = 0; // dummy protection if couldn't find initial H
	  cout << "** ERROR: negative dec mode, for safety set it ot 0" << endl;
	}
      double EvtW = isMC ? (theBigTree.aMCatNLOweight * reweight.weight(PUReweight_MC,PUReweight_target,theBigTree.npu) * topPtReweight * HHweight) : 1.0;
      if (isMC)
	{
	  totalEvents += EvtW;
	}
      else
	{
	  totalEvents += 1 ;
	}
      ec.Increment ("all", EvtW);
      if (isHHsignal) ecHHsig[genHHDecMode].Increment ("all", EvtW);

      ++totalNoWeightsEventsNum ;
    
    
      // ----------------------------------------------------------
      //  apply MET filters -- FIXME: not applied now

      int metbit = theBigTree.metfilterbit;
      int metpass = metbit & (1 << 0);
      metpass += metbit & (1 << 2);
      metpass += metbit & (1 << 5);
      metpass += metbit & (1 << 6);
      // Update Fall17 94X
      metpass += metbit & (1 << 1);
      metpass += metbit & (1 << 3);
      metpass += metbit & (1 << 4);
      // metpass += metbit & (1 << 7); // "Flag_eeBadScFilter" not suggested on twiki
      metpass += metbit & (1 << 8);
      //if(metpass <= 0) cout << " - failed metbit(9): " << std::bitset<9>(metbit) << endl; //FRA
      
      // if(metpass > 0) continue ; // FIXME!!! disabled only temporarily
      ec.Increment ("METfilter", EvtW);
      if (isHHsignal) ecHHsig[genHHDecMode].Increment ("METfilter", EvtW);

      // ----------------------------------------------------------
      // require that the event is not affected by the Bad/Clone Muon problem -- for 2016 data
      //if (theBigTree.NBadMu > 0) continue ; //FRA: Sync Feb2018
      ec.Increment ("NoBadMuons", EvtW);
      if (isHHsignal) ecHHsig[genHHDecMode].Increment ("NoBadMuons", EvtW);

      // ----------------------------------------------------------
      // require at least 1 pair
      if (theBigTree.indexDau1->size () == 0) continue ;
      ec.Increment ("PairExists", EvtW);
      if (isHHsignal) ecHHsig[genHHDecMode].Increment ("PairExists", EvtW);

      // ----------------------------------------------------------
      // assess the pair type 
      // loop over the daughters to select pair type: mu > e > tau
      // apply tight baseline (with iso to check)
    
      int nmu = 0;
      int nmu10 = 0; // low pt muons for DY sideband, not entering in nmu
      int nele = 0;
      int nele10 = 0;
      // int ntau = 0;
    
      if(DEBUG)
	{
	  cout << "***** DEBUG: reco particles (remember: check if baseline sels are aligned to OfflineProducerHelper)" << endl;
	}

      for (unsigned int idau = 0; idau < theBigTree.daughters_px->size(); ++idau)
	{
	  int dauType = theBigTree.particleType->at(idau);
	  if (oph.isMuon(dauType))
	    {
	      //bool passMu = oph.muBaseline (&theBigTree, idau, 23., 2.1, 0.15, OfflineProducerHelper::MuTight, string("All") , (theBigTree.EventNumber == debugEvent ? true : false)) ; //FRA: syncFeb2018
	      bool passMu = oph.muBaseline (&theBigTree, idau, 10., 2.1, 0.15, OfflineProducerHelper::MuTight, string("All") , (DEBUG ? true : false)) ;   //FRA: syncFeb2018
	      bool passMu10 = oph.muBaseline (&theBigTree, idau, 10., 2.4, 0.15, OfflineProducerHelper::MuTight, string("All") , (DEBUG ? true : false)) ;
	      if (passMu) ++nmu;
	      else if (passMu10) ++nmu10;
	    }
	  else if (oph.isElectron(dauType))
	    {
	      //bool passEle   = oph.eleBaseline (&theBigTree, idau, 27., 2.1, 0.1, OfflineProducerHelper::EMVATight, string("All") , (theBigTree.EventNumber == debugEvent ? true : false)) ; //FRA: syncFeb2018
	      bool passEle   = oph.eleBaseline (&theBigTree, idau, 10., 2.1, 0.1, OfflineProducerHelper::EMVATight, string("All") , (DEBUG ? true : false)) ; //FRA: syncFeb2018
	      bool passEle10 = oph.eleBaseline (&theBigTree, idau, 10., 2.5, 0.1, OfflineProducerHelper::EMVATight, string("All") , (DEBUG ? true : false)) ;
	      if (passEle) ++nele;
	      else if (passEle10) ++nele10;
	    }

	  if(DEBUG)
	    {
	      TLorentzVector dauTlvDebug (
					  theBigTree.daughters_px->at (idau),
					  theBigTree.daughters_py->at (idau),
					  theBigTree.daughters_pz->at (idau),
					  theBigTree.daughters_e->at (idau)
					  );

	      // NB: remember to align this debug to the content of OfflineProducerHelper
	      cout << ".... reco part "
		   << " idx dau="   << setw(3) << left << idau
		   << " type="      << setw(3) << left << dauType
		   << " pt="        << setw(10) << left << dauTlvDebug.Pt()
		// << " px="        << setw(10) << left << theBigTree.daughters_px->at (idau)
		// << " py="        << setw(10) << left << theBigTree.daughters_py->at (idau)
		   << " eta="       << setw(10) << left << dauTlvDebug.Eta()
		   << " phi="       << setw(10) << left << dauTlvDebug.Phi()
		   << " iso="       << setw(10) << left << getIso (idau, dauTlvDebug.Pt (), theBigTree)
		   << " dxy="       << setw(15) << left << theBigTree.dxy->at(idau)
		   << " dz="        << setw(15) << left << theBigTree.dz->at(idau)
		   << " mutightID=" << setw(3) << left << CheckBit(theBigTree.daughters_muonID->at(idau),3)
		   //<< " mubase="    << setw(3) << left << oph.muBaseline (&theBigTree, idau, 23., 2.1, 0.15, OfflineProducerHelper::MuTight, string("All")) //FRA: syncFeb2018
		   << " mubase="    << setw(3) << left << oph.muBaseline (&theBigTree, idau, 10., 2.1, 0.15, OfflineProducerHelper::MuTight, string("All"))   //FRA: syncFeb2018
		   //<< " ebase="     << setw(3) << left << oph.eleBaseline (&theBigTree, idau, 27., 2.1, 0.1, OfflineProducerHelper::EMVATight, string("All"))
		   << " ebase="     << setw(3) << left << oph.eleBaseline (&theBigTree, idau, 10., 2.1, 0.1, OfflineProducerHelper::EMVATight, string("All")) //FRA: syncFeb2018
		//<< " taubase="   << setw(3) << left << oph.tauBaseline (&theBigTree, idau, 25., 2.3, OfflineProducerHelper::aeleVLoose, OfflineProducerHelper::amuTight, 3.0, string("All"), 1)
		// << " passaele="  << setw(3) << left << oph.tauBaseline (&theBigTree, idau, 0., 999., 0, 1, 999., string("againstEle")) 
		// << " passamu="   << setw(3) << left << oph.tauBaseline (&theBigTree, idau, 0., 999., 0, 1, 999., string("againstMu")) 
		   << endl;
	    }

	}
      int pairType = 2; // tau tau
      if (nmu > 0)
	{
	  if (nmu == 1 && nmu10 == 0)
	    pairType = 0 ; // mu tau
	  else
	    pairType = 3 ; // mu mu
	}
      else if (nele > 0)
	{
	  if (nele == 1 && nele10 == 0)
	    pairType = 1;
	  else
	    pairType = 4 ; // ele ele
	}
      // ----------------------------------------------------------
      // choose the first pair passing baseline and being of the right pair type

      int chosenTauPair = -1;

      // vector <pair<float, int>> chosenTauPairsIso;   // sum pt , index
      // vector <pair<float, int>> chosenTauPairsRlxIso;

      if (pairType == 2 && sortStrategyThTh == kHTauTau)
	{
	  chosenTauPair = oph.getBestPairHTauTau(&theBigTree, leptonSelectionFlag, (DEBUG ? true : false));
	}

      else if (pairType == 2 && sortStrategyThTh == kPtAndRawIso)
	{
	  chosenTauPair = oph.getBestPairPtAndRawIsoOrd(&theBigTree, leptonSelectionFlag, (DEBUG ? true : false));
	}

      // (mu tauh), (e tauh), (tauhtauh && kLLRFramDefault)
      else
	{
	  if(DEBUG)
	    {
	      for (unsigned int iPair = 0 ; iPair < theBigTree.indexDau1->size () ; ++iPair)
		{
		  int t_firstDaughterIndex  = theBigTree.indexDau1->at (iPair) ;
		  int t_secondDaughterIndex = theBigTree.indexDau2->at (iPair) ;
		  int t_type1 = theBigTree.particleType->at (t_firstDaughterIndex) ;
		  int t_type2 = theBigTree.particleType->at (t_secondDaughterIndex) ;
		  cout << " **## Pair: " << iPair << " indexes(" <<t_firstDaughterIndex << "," << t_secondDaughterIndex << ") pairType: "<< pairType << " getPairType: "<< oph.getPairType (t_type1, t_type2) << endl;
		}
          
	      for (unsigned int iLep = 0 ; (iLep < theBigTree.daughters_px->size ()) ; ++iLep)
		{
          
          	  TLorentzVector tlv_dummyLepton
		    (
		     theBigTree.daughters_px->at (iLep),
		     theBigTree.daughters_py->at (iLep),
		     theBigTree.daughters_pz->at (iLep),
		     theBigTree.daughters_e->at (iLep)
		     ) ;
      
		  cout << " idx="  << iLep
		       << " type=" << theBigTree.particleType->at(iLep)
		       << " pt="   << tlv_dummyLepton.Pt()
		       << " eta="  << tlv_dummyLepton.Eta()
		       << " phi="  << tlv_dummyLepton.Phi()
		       << " iso="  << getIso (iLep, tlv_dummyLepton.Pt (), theBigTree)
		       << " dxy="  << theBigTree.dxy->at(iLep)
		       << " dz="   << theBigTree.dz->at(iLep)
		       << " elePassConvVeto=" << theBigTree.daughters_passConversionVeto->at(iLep)
		       << " eleMissingHits="  << theBigTree.daughters_eleMissingHits->at(iLep)
		       << endl;
		}
          
	    }
	  for (unsigned int iPair = 0 ; iPair < theBigTree.indexDau1->size () ; ++iPair)
	    {
	      int t_firstDaughterIndex  = theBigTree.indexDau1->at (iPair) ;  
	      int t_secondDaughterIndex = theBigTree.indexDau2->at (iPair) ;
	      int t_type1 = theBigTree.particleType->at (t_firstDaughterIndex) ;
	      int t_type2 = theBigTree.particleType->at (t_secondDaughterIndex) ;
	      if ( oph.getPairType (t_type1, t_type2) != pairType ) continue ;
	      // string whatApplyForIsoLep = "Vertex-LepID-pTMin-etaMax-againstEle-againstMu-Iso" ;
	      // if ( oph.pairPassBaseline (&theBigTree, iPair, string("Vertex-LepID-pTMin-etaMax-againstEle-againstMu") ) )
          
	      // TLorentzVector t_tlv_firstLepton (
	      //   theBigTree.daughters_px->at (t_firstDaughterIndex),
	      //   theBigTree.daughters_py->at (t_firstDaughterIndex),
	      //   theBigTree.daughters_pz->at (t_firstDaughterIndex),
	      //   theBigTree.daughters_e->at (t_firstDaughterIndex)
	      // );
	      // TLorentzVector t_tlv_secondLepton (
	      //   theBigTree.daughters_px->at (t_secondDaughterIndex),
	      //   theBigTree.daughters_py->at (t_secondDaughterIndex),
	      //   theBigTree.daughters_pz->at (t_secondDaughterIndex),
	      //   theBigTree.daughters_e->at (t_secondDaughterIndex)
	      // );

	      // if ( oph.pairPassBaseline (&theBigTree, iPair, leptonSelectionFlag+string("-TauRlxIzo") ) ) // rlx izo to limit to tau iso < 7 -- good for sideband
	      string baselineSels = ( (pairType <= 2) ? leptonSelectionFlag : (leptonSelectionFlag + "-Iso")) ; // for ee, mumu, emu, ask isolation in baseline
	      if ( oph.pairPassBaseline (&theBigTree, iPair, baselineSels, (DEBUG ? true : false) ) ) // rlx izo to limit to tau iso < 7 -- good for sideband
		{
		  chosenTauPair = iPair;
		  break;          
		}
	      // if ( oph.pairPassBaseline (&theBigTree, iPair, (leptonSelectionFlag+string("-Iso")) ) )
	      // {
	      //     chosenTauPairsIso.push_back(make_pair(t_tlv_firstLepton.Pt()+t_tlv_secondLepton.Pt(), iPair));
	      //     // chosenTauPair = iPair;
	      //     // break;
	      // }
	      // if ( oph.pairPassBaseline (&theBigTree, iPair, (leptonSelectionFlag+string("-TauRlxIzo")) ) )
	      // {
	      //     chosenTauPairsIso.push_back(make_pair(t_tlv_firstLepton.Pt()+t_tlv_secondLepton.Pt(), iPair));
	      // }
	    }
	}

      if(DEBUG)
	{
	  cout << "**** DEBUG : chosen pair : " << chosenTauPair << " str=" << leptonSelectionFlag << " pairType==" << pairType << endl;
	  cout << "     ... going to list all pairs of same pairType as the one assessed with reco leptons" << endl;
	  for (unsigned int iPair = 0 ; iPair < theBigTree.indexDau1->size () ; ++iPair)
	    {
	      int t_firstDaughterIndex  = theBigTree.indexDau1->at (iPair) ;  
	      int t_secondDaughterIndex = theBigTree.indexDau2->at (iPair) ;
	      int t_type1 = theBigTree.particleType->at (t_firstDaughterIndex) ;
	      int t_type2 = theBigTree.particleType->at (t_secondDaughterIndex) ;        
	      if ( oph.getPairType (t_type1, t_type2) != pairType ) continue ;
	      TLorentzVector tttt (
				   theBigTree.daughters_px->at (t_secondDaughterIndex),
				   theBigTree.daughters_py->at (t_secondDaughterIndex),
				   theBigTree.daughters_pz->at (t_secondDaughterIndex),
				   theBigTree.daughters_e->at (t_secondDaughterIndex)
				   );
	      cout << "- " << iPair << " idx1=" << t_firstDaughterIndex << " idx2=" << t_secondDaughterIndex << " isoTau=" <<  getIso (t_secondDaughterIndex, tttt.Pt (), theBigTree) << " tauPt=" << tttt.Pt() << " type2=" << t_type2 << " eta=" << tttt.Eta() << " phi=" << tttt.Phi() << endl;
	      cout << "   >>> DM=" << theBigTree.daughters_decayModeFindingOldDMs->at(t_secondDaughterIndex) << " dxy=" << theBigTree.dxy->at(t_secondDaughterIndex) << " dz=" << theBigTree.dz->at(t_secondDaughterIndex) << endl;
	      // cout << "   >>> tauBase="
	    }      
	}
      // if (chosenTauPairsIso.size() > 0)
      // {
      //   sort(chosenTauPairsIso.begin(), chosenTauPairsIso.end()); // will get highest pt sum
      //   chosenTauPair = chosenTauPairsIso.back().second;
      // }
      // else if (chosenTauPairsRlxIso.size() > 0)
      // {
      //   sort(chosenTauPairsRlxIso.begin(), chosenTauPairsRlxIso.end()); // will get highest pt sum
      //   chosenTauPair = chosenTauPairsRlxIso.back().second;
      // }
      // else continue; // no pair found

      if (chosenTauPair < 0) continue; // no pair found over baseline
      ec.Increment ("PairFoundBaseline", EvtW);
      if (isHHsignal)
	{
	  ecHHsig[genHHDecMode].Increment ("PairFoundBaseline", EvtW);
	  if (pairType == genHHDecMode) 
	    ecHHsig[genHHDecMode].Increment ("PairMatchesGen", EvtW);
	}

      // ----------------------------------------------------------
      // pair has been assessed , check trigger information

      const int firstDaughterIndex  = theBigTree.indexDau1->at (chosenTauPair) ;  
      const int secondDaughterIndex = theBigTree.indexDau2->at (chosenTauPair) ;
      const int type1 = theBigTree.particleType->at (firstDaughterIndex) ;
      const int type2 = theBigTree.particleType->at (secondDaughterIndex) ;        
      const int pType = pairType ;
      const int isOS  = theBigTree.isOSCand->at (chosenTauPair) ;
      bool lep1HasTES = false;
      bool lep2HasTES = false;
      if (isMC)
	{
	  lep1HasTES = (theBigTree.daughters_TauUpExists->at(firstDaughterIndex) == 1 ? true : false);
	  lep2HasTES = (theBigTree.daughters_TauUpExists->at(secondDaughterIndex) == 1 ? true : false);
	}

      // // x check of MC info from genJet()
      // some differences observed, and some cases of real taus also when taking fully had tt only
      // // 
      // int genIdx1 = theBigTree.daughters_genindex->at(firstDaughterIndex);
      // int genIdx2 = theBigTree.daughters_genindex->at(secondDaughterIndex);
      // int aidg1 = (genIdx1  > 0 ? abs(theBigTree.genpart_pdg->at(genIdx1)) : 0) ;
      // int aidg2 = (genIdx2  > 0 ? abs(theBigTree.genpart_pdg->at(genIdx2)) : 0) ;
      // if (pType == 2 && ( lep1HasTES || lep2HasTES)) cout << "OoOooooooooooooooo ci sono dei real tau!!!! " << lep1HasTES << " " << lep2HasTES << " /// " << iEvent << endl;  
      // if (aidg1 == 15 || aidg2 == 15 || aidg1 == 66615 || aidg2 == 66615) cout << " BBBBBBBBBBBBB real tau!!! " << aidg1 << " " << aidg2 << " /// " << iEvent << endl;

      const TLorentzVector tlv_firstLepton (
					    theBigTree.daughters_px->at (firstDaughterIndex),
					    theBigTree.daughters_py->at (firstDaughterIndex),
					    theBigTree.daughters_pz->at (firstDaughterIndex),
					    theBigTree.daughters_e->at (firstDaughterIndex)
					    );

      const TLorentzVector tlv_secondLepton (
					     theBigTree.daughters_px->at (secondDaughterIndex),
					     theBigTree.daughters_py->at (secondDaughterIndex),
					     theBigTree.daughters_pz->at (secondDaughterIndex),
					     theBigTree.daughters_e->at (secondDaughterIndex)
					     );

      // scale up: only applies to tau
      TLorentzVector tlv_firstLepton_tauup (tlv_firstLepton);
      TLorentzVector tlv_firstLepton_taudown (tlv_firstLepton);
      if (lep1HasTES)
	{
	  tlv_firstLepton_tauup.SetPxPyPzE (
					    theBigTree.daughters_px_TauUp->at (firstDaughterIndex),
					    theBigTree.daughters_py_TauUp->at (firstDaughterIndex),
					    theBigTree.daughters_pz_TauUp->at (firstDaughterIndex),
					    theBigTree.daughters_e_TauUp->at (firstDaughterIndex)
					    );
	  tlv_firstLepton_taudown.SetPxPyPzE (
					      theBigTree.daughters_px_TauDown->at (firstDaughterIndex),
					      theBigTree.daughters_py_TauDown->at (firstDaughterIndex),
					      theBigTree.daughters_pz_TauDown->at (firstDaughterIndex),
					      theBigTree.daughters_e_TauDown->at (firstDaughterIndex)
					      );
	}

      TLorentzVector tlv_secondLepton_tauup (tlv_secondLepton);
      TLorentzVector tlv_secondLepton_taudown (tlv_secondLepton);
      if (lep2HasTES)
	{
	  tlv_secondLepton_tauup.SetPxPyPzE (
					     theBigTree.daughters_px_TauUp->at (secondDaughterIndex),
					     theBigTree.daughters_py_TauUp->at (secondDaughterIndex),
					     theBigTree.daughters_pz_TauUp->at (secondDaughterIndex),
					     theBigTree.daughters_e_TauUp->at (secondDaughterIndex)
					     );
	  tlv_secondLepton_taudown.SetPxPyPzE (
					       theBigTree.daughters_px_TauDown->at (secondDaughterIndex),
					       theBigTree.daughters_py_TauDown->at (secondDaughterIndex),
					       theBigTree.daughters_pz_TauDown->at (secondDaughterIndex),
					       theBigTree.daughters_e_TauDown->at (secondDaughterIndex)
					       );
	}

    if (DEBUG)
    {
        cout << "------- TAU TES DEBUG -------" << endl;
        cout << " tau1 centr: " << tlv_firstLepton.Pt() << " / " << tlv_firstLepton.Eta() << endl;
        cout << " tau1 up: " << tlv_firstLepton_tauup.Pt() << " / " << tlv_firstLepton_tauup.Eta() << endl;
        cout << " tau1 dw: " << tlv_firstLepton_taudown.Pt() << " / " << tlv_firstLepton_taudown.Eta() << endl;

        cout << " tau2 centr: " << tlv_secondLepton.Pt() << " / " << tlv_secondLepton.Eta() << endl;
        cout << " tau2 up: " << tlv_secondLepton_tauup.Pt() << " / " << tlv_secondLepton_tauup.Eta() << endl;
        cout << " tau2 dw: " << tlv_secondLepton_taudown.Pt() << " / " << tlv_secondLepton_taudown.Eta() << endl;
        cout << "---------------------"<< endl;
    }


      // the following code is the match of the tau to a L1 tau seed due to an error in seed removal from path
      // needed only when analyzing 2015 data
      // if (pairType == 2 && isMC)
      // {
      //   bool hasL1Match_1 = theBigTree.daughters_isL1IsoTau28Matched->at (firstDaughterIndex);
      //   bool hasL1Match_2 = theBigTree.daughters_isL1IsoTau28Matched->at (secondDaughterIndex);
      //   bool goodL1 = (hasL1Match_1 && hasL1Match_2);
      //   if (!goodL1) continue;
      // }

      // DATA strategy
      float trgEvtWeight     = 1.0;
      float trgEvtWeightUp   = 1.0;
      float trgEvtWeightDown = 1.0;
    
      if (applyTriggers)
	{
	  Long64_t triggerbit = theBigTree.triggerbit;

	  Long64_t matchFlag1 = (Long64_t) theBigTree.daughters_trgMatched->at(firstDaughterIndex);
	  Long64_t matchFlag2 = (Long64_t) theBigTree.daughters_trgMatched->at(secondDaughterIndex);
	  Long64_t goodTriggerType1 = (Long64_t) theBigTree.daughters_isGoodTriggerType->at(firstDaughterIndex);
	  Long64_t goodTriggerType2 = (Long64_t) theBigTree.daughters_isGoodTriggerType->at(secondDaughterIndex);

	  Long64_t trgNotOverlapFlag = (Long64_t) theBigTree.mothers_trgSeparateMatch->at(chosenTauPair);
	  bool passTrg = trigReader.checkOR (pairType,triggerbit, matchFlag1, matchFlag2, trgNotOverlapFlag, goodTriggerType1, goodTriggerType2, tlv_firstLepton.Pt(), tlv_secondLepton.Pt()) ;
      isVBFfired = trigReader.isVBFfired(triggerbit, matchFlag1, matchFlag2, trgNotOverlapFlag, goodTriggerType1, goodTriggerType2, tlv_firstLepton.Pt(), tlv_secondLepton.Pt());

      /* // Old version used with single triggers
      bool isCrossTrg = true;
	  bool trgNotOverlap = trigReader.checkOR (pairType, trgNotOverlapFlag) ;
	  // FIXME!! true only if single lep trigger for eTau and muTau
	  if (pairType == 0 || pairType == 1 || pairType == 3 || pairType == 4)
	  {
          passMatch1 = trigReader.checkOR (pairType, matchFlag1) ;
          passMatch2 = true;
          trgNotOverlap = true; // FIXME: true only for single lepton triggers!
	  }
      */
	  bool triggerAccept = false;
	    triggerAccept = passTrg;

	  // require trigger + legs matched
	  //bool triggerAccept = (passTrg && passMatch1 && passMatch2 && trgNotOverlap) ; //FRA: with crossTrigs match1&match2 are together
	  

	  if(DEBUG)
	    {
	      //Long64_t matchFlag1LF = (Long64_t) theBigTree.daughters_L3FilterFired->at(firstDaughterIndex);
	      //Long64_t matchFlag2LF = (Long64_t) theBigTree.daughters_L3FilterFired->at(secondDaughterIndex);
	      //Long64_t matchFlag1L3 = (Long64_t) theBigTree.daughters_L3FilterFiredLast->at(firstDaughterIndex);
	      //Long64_t matchFlag2L3 = (Long64_t) theBigTree.daughters_L3FilterFiredLast->at(secondDaughterIndex);
	      //bool isLF1 = trigReader.checkOR (pairType, matchFlag1LF);
	      //bool isL31 = trigReader.checkOR (pairType, matchFlag1L3);
	      //bool isLF2 = trigReader.checkOR (pairType, matchFlag2LF);
	      //bool isL32 = trigReader.checkOR (pairType, matchFlag2L3);
	      //cout << "** trg check: trgAccept=" << triggerAccept << " passTrg=" << passTrg << " passMatch=" << passMatch << " noOverlap=" << trgNotOverlap<<" goodTriggerType= "<<goodTriggerType<<endl;
		  //cout <<  " LF1=" << isLF1 << " L31=" << isL31 <<  " LF2=" << isLF2 << " L32=" << isL32 << endl;
		  //cout << "** trg check: trgAccept=" << triggerAccept	     <<endl;
		  if(pairType == 0)//MuTau
		  {
		    trigReader.listMuTau(triggerbit, matchFlag1, matchFlag2, trgNotOverlapFlag, goodTriggerType1, goodTriggerType2);
		  }
		  if(pairType == 1)//ETau
		  {
		     trigReader.listETau(triggerbit, matchFlag1, matchFlag2, trgNotOverlapFlag, goodTriggerType1, goodTriggerType2);
		  }
		  if(pairType == 2)//TauTau
		  {
		    trigReader.listTauTau(triggerbit, matchFlag1, matchFlag2, trgNotOverlapFlag, goodTriggerType1, goodTriggerType2);
		  }
	     
	    }

	  if (!triggerAccept) continue;
	  ec.Increment ("Trigger", EvtW); // for data, EvtW is 1.0
	  if (isHHsignal && pairType == genHHDecMode) ecHHsig[genHHDecMode].Increment ("Trigger", EvtW);
	}

      // MC strategy
      else
	{
	  float evtLeg1weight = 1.0;
	  float evtLeg2weight = 1.0;

	  float evtLeg1weightUp = 1.0;
	  float evtLeg2weightUp = 1.0;

	  float evtLeg1weightDown = 1.0;
	  float evtLeg2weightDown = 1.0;

	  if (pairType == 0 || pairType == 3)  //mutau -- mumu
	    {
	      evtLeg1weight = getTriggerWeight(type1, tlv_firstLepton.Pt(), tlv_firstLepton.Eta(), 0, myScaleFactor[type1][0], 0) ;
	      evtLeg2weight = 1.0;
          
	      evtLeg1weightUp   = (lep1HasTES ? getTriggerWeight(type1, tlv_firstLepton_tauup.Pt(), tlv_firstLepton_tauup.Eta(), 0, myScaleFactor[type1][0], 0) : evtLeg1weight);
	      evtLeg1weightDown = (lep1HasTES ? getTriggerWeight(type1, tlv_firstLepton_taudown.Pt(), tlv_firstLepton_taudown.Eta(), 0, myScaleFactor[type1][0], 0) : evtLeg1weight);

	      evtLeg2weightUp   = 1.0;
	      evtLeg2weightDown = 1.0;
	    }

	  else if (pairType == 1 || pairType == 4) //eletau -- ee
	    {
	      evtLeg1weight = getTriggerWeight(type1, tlv_firstLepton.Pt(), tlv_firstLepton.Eta(), trigRewEleHisto, 0, 0) ;
	      evtLeg2weight = 1.0;        

	      evtLeg1weightUp   = (lep1HasTES ? getTriggerWeight(type1, tlv_firstLepton_tauup.Pt(), tlv_firstLepton_tauup.Eta(), trigRewEleHisto, 0, 0)  : evtLeg1weight);
	      evtLeg1weightDown = (lep1HasTES ? getTriggerWeight(type1, tlv_firstLepton_taudown.Pt(), tlv_firstLepton_taudown.Eta(), trigRewEleHisto, 0, 0)  : evtLeg1weight);

	      evtLeg2weightUp   = 1.0;
	      evtLeg2weightDown = 1.0;
	    }

	  else if (pairType == 2) //tautau
	    {
	      evtLeg1weight = getTriggerWeight(type1, tlv_firstLepton.Pt(), tlv_firstLepton.Eta(), 0, 0, 1, lep1HasTES) ;
	      evtLeg2weight = getTriggerWeight(type2, tlv_secondLepton.Pt(), tlv_secondLepton.Eta(), 0, 0, 1, lep2HasTES) ;

	      evtLeg1weightUp     = (lep1HasTES ? getTriggerWeight(type1, tlv_firstLepton_tauup.Pt(), tlv_firstLepton_tauup.Eta(), 0, 0, 1) : evtLeg1weight , lep1HasTES) ;
	      evtLeg1weightDown   = (lep1HasTES ? getTriggerWeight(type1, tlv_firstLepton_taudown.Pt(), tlv_firstLepton_taudown.Eta(), 0, 0, 1) : evtLeg1weight , lep2HasTES) ;
          
	      evtLeg2weightUp   = (lep2HasTES ? getTriggerWeight(type2, tlv_secondLepton_tauup.Pt(), tlv_secondLepton_tauup.Eta(), 0, 0, 1) : evtLeg2weight , lep1HasTES) ;
	      evtLeg2weightDown = (lep2HasTES ? getTriggerWeight(type2, tlv_secondLepton_taudown.Pt(), tlv_secondLepton_taudown.Eta(), 0, 0, 1) : evtLeg2weight , lep2HasTES) ;

	    }
	  trgEvtWeight = evtLeg1weight*evtLeg2weight;
	  trgEvtWeightUp   = evtLeg1weightUp*evtLeg2weightUp;
	  trgEvtWeightDown = evtLeg1weightDown*evtLeg2weightDown;

	  EvtW *= trgEvtWeight;
	  ec.Increment ("Trigger", EvtW); // for MC, weight the event for the trigger acceptance
	  if (isHHsignal && pairType == genHHDecMode) ecHHsig[genHHDecMode].Increment ("Trigger", EvtW);
	}

      // ----------------------------------------------------------
      // pair selection is now complete, compute oher quantitites
    
      TLorentzVector tlv_tauH = tlv_firstLepton + tlv_secondLepton ;
      TLorentzVector tlv_tauH_SVFIT ;

      theSmallTree.m_tauH_SVFIT_mass = theBigTree.SVfitMass->at (chosenTauPair) ;
      theSmallTree.m_tauH_SVFIT_mass_up   = (isMC ? theBigTree.SVfitMassTauUp->at (chosenTauPair) : theSmallTree.m_tauH_SVFIT_mass);
      theSmallTree.m_tauH_SVFIT_mass_down = (isMC ? theBigTree.SVfitMassTauDown->at (chosenTauPair) : theSmallTree.m_tauH_SVFIT_mass);
      theSmallTree.m_tauH_SVFIT_mass_METup   = (isMC ? theBigTree.SVfitMassMETUp->at (chosenTauPair) : theSmallTree.m_tauH_SVFIT_mass);
      theSmallTree.m_tauH_SVFIT_mass_METdown = (isMC ? theBigTree.SVfitMassMETDown->at (chosenTauPair) : theSmallTree.m_tauH_SVFIT_mass);
      // in case the SVFIT mass is calculated
      if (theBigTree.SVfitMass->at (chosenTauPair) > -900.)
	{
	  theSmallTree.m_tauH_SVFIT_pt     = theBigTree.SVfit_pt->at  (chosenTauPair) ;
	  theSmallTree.m_tauH_SVFIT_eta    = theBigTree.SVfit_eta->at (chosenTauPair) ;
	  theSmallTree.m_tauH_SVFIT_phi    = theBigTree.SVfit_phi->at (chosenTauPair) ;
	  theSmallTree.m_tauH_SVFIT_METphi = theBigTree.SVfit_fitMETPhi->at (chosenTauPair) ;
	  theSmallTree.m_tauH_SVFIT_METrho = theBigTree.SVfit_fitMETRho->at (chosenTauPair) ;      

	  tlv_tauH_SVFIT.SetPtEtaPhiM (
				       theBigTree.SVfit_pt->at (chosenTauPair),
				       theBigTree.SVfit_eta->at (chosenTauPair),
				       theBigTree.SVfit_phi->at (chosenTauPair),
				       theBigTree.SVfitMass->at (chosenTauPair)
				       ) ;

	  theSmallTree.m_tauHsvfitMet_deltaPhi = deltaPhi (theBigTree.metphi, tlv_tauH_SVFIT.Phi ()) ;
	  theSmallTree.m_ditau_deltaR_per_tauHsvfitpt = tlv_firstLepton.DeltaR(tlv_secondLepton) * tlv_tauH_SVFIT.Pt();
	}

    if (DEBUG)
    {
      cout << "------- SVFIT ------" << endl;
      cout << " is calculated ? " << theBigTree.SVfitMass->at(chosenTauPair) << endl;
      cout << " pt/eta/phi: " << tlv_tauH_SVFIT.Pt() << " / " << tlv_tauH_SVFIT.Eta() << " / " << tlv_tauH_SVFIT.Phi() << endl;
      cout << " is calculated UP ? " << theSmallTree.m_tauH_SVFIT_mass_up << endl;
      cout << " is calculated DOWN ? " << theSmallTree.m_tauH_SVFIT_mass_down << endl;
      cout << "--------------------" << endl;
    }

      // check if the selected leptons A,B match the gen hard scatter products 1,2
      if (isHHsignal)
	{
	  bool type1A = (abs(t1hs) == abs(type1));
	  bool type1B = (abs(t1hs) == abs(type2));
	  bool type2A = (abs(t2hs) == abs(type1));
	  bool type2B = (abs(t2hs) == abs(type2));
    
	  bool dR1A = (vHardScatter1.DeltaR(tlv_firstLepton) < 0.5);
	  bool dR1B = (vHardScatter1.DeltaR(tlv_secondLepton) < 0.5);
	  bool dR2A = (vHardScatter2.DeltaR(tlv_firstLepton) < 0.5);
	  bool dR2B = (vHardScatter2.DeltaR(tlv_secondLepton) < 0.5);

	  // FIXME: fill gen matched info pt/eta/phi/e
	  if ( (type1A && dR1A) || (type2A && dR2A))
	    theSmallTree.m_hasgenmatch1 = true;
	  if ( (type1B && dR1B) || (type2B && dR2B))
	    theSmallTree.m_hasgenmatch2 = true;
	}


      theSmallTree.m_pairType    = pType ;
      // Need to change the channel: LLR-> 0:muTau - 1:eTau  /  PI-> 0:eTau - 1:muTau
      if      (theSmallTree.m_pairType == 0) theSmallTree.m_BDT_channel = 1.;
      else if (theSmallTree.m_pairType == 1) theSmallTree.m_BDT_channel = 0.;
      else if (theSmallTree.m_pairType == 2) theSmallTree.m_BDT_channel = 2.;

      //cout << " ------------------> CHECK CHANNEL pairType/BDT_chan: " << theSmallTree.m_pairType << "/" << theSmallTree.m_BDT_channel << endl;

      theSmallTree.m_PUReweight  = (isMC ? reweight.weight(PUReweight_MC,PUReweight_target,theBigTree.npu) : 1) ;      
      theSmallTree.m_MC_weight   = (isMC ? theBigTree.aMCatNLOweight * XS * stitchWeight * HHweight * trgEvtWeight : 1) ;
      theSmallTree.m_turnOnreweight   = (isMC ? trgEvtWeight : 1.);
      theSmallTree.m_turnOnreweight_tauup    = (isMC ? trgEvtWeightUp  : 1.);
      theSmallTree.m_turnOnreweight_taudown  = (isMC ? trgEvtWeightDown : 1.);
      theSmallTree.m_lheht       = (isMC ? theBigTree.lheHt : 0) ;
      theSmallTree.m_EventNumber = theBigTree.EventNumber ;
      theSmallTree.m_RunNumber   = theBigTree.RunNumber ;
      theSmallTree.m_npv  = theBigTree.npv ;
      theSmallTree.m_npu  = theBigTree.npu ;
      theSmallTree.m_lumi = theBigTree.lumi ;
      theSmallTree.m_triggerbit = theBigTree.triggerbit ;
      theSmallTree.m_rho  = theBigTree.rho ;
      theSmallTree.m_isMC = isMC ;
      theSmallTree.m_isOS = theBigTree.isOSCand->at (chosenTauPair) ;
      theSmallTree.m_lheNOutPartons = theBigTree.lheNOutPartons ;
      theSmallTree.m_lheNOutB = theBigTree.lheNOutB ;
      // theSmallTree.m_met_phi = theBigTree.metphi ;
      // theSmallTree.m_met_et = theBigTree.met ;
      theSmallTree.m_met_er_phi = theBigTree.met_er_phi ;
      theSmallTree.m_met_er_et = theBigTree.met_er ;
      TVector2 vMET (theBigTree.METx->at(chosenTauPair) , theBigTree.METy->at(chosenTauPair));
      theSmallTree.m_met_phi   = vMET.Phi();
      theSmallTree.m_met_et    = vMET.Mod();
      theSmallTree.m_met_cov00 = theBigTree.MET_cov00->at (chosenTauPair);
      theSmallTree.m_met_cov01 = theBigTree.MET_cov01->at (chosenTauPair);
      theSmallTree.m_met_cov10 = theBigTree.MET_cov10->at (chosenTauPair);
      theSmallTree.m_met_cov11 = theBigTree.MET_cov11->at (chosenTauPair);

      // Shifted MET for JES
      // This will be useful when splitting JECs
      //TVector2 vMET_jetup   = getShiftedMET(+1., vMET, theBigTree, DEBUG);
      //TVector2 vMET_jetdown = getShiftedMET(-1., vMET, theBigTree, DEBUG);
      // Now we use the total shift already stored in LLR ntuples
      TVector2 vMET_jetup   (theBigTree.METx_UP->at(chosenTauPair) , theBigTree.METy_UP->at(chosenTauPair));
      TVector2 vMET_jetdown (theBigTree.METx_DOWN->at(chosenTauPair) , theBigTree.METy_DOWN->at(chosenTauPair));
      theSmallTree.m_met_phi_jetup   = vMET_jetup.Phi();
      theSmallTree.m_met_et_jetup    = vMET_jetup.Mod();
      theSmallTree.m_met_phi_jetdown = vMET_jetdown.Phi();
      theSmallTree.m_met_et_jetdown  = vMET_jetdown.Mod();

      // Shifted MET for TES
      TVector2 vMET_tauup   (theBigTree.METx_UP_TES->at(chosenTauPair)  , theBigTree.METy_UP_TES->at(chosenTauPair));
      TVector2 vMET_taudown (theBigTree.METx_DOWN_TES->at(chosenTauPair), theBigTree.METy_DOWN_TES->at(chosenTauPair));
      theSmallTree.m_met_phi_tauup   = vMET_tauup.Phi();
      theSmallTree.m_met_et_tauup    = vMET_tauup.Mod();
      theSmallTree.m_met_phi_taudown = vMET_taudown.Phi();
      theSmallTree.m_met_et_taudown  = vMET_taudown.Mod();

      theSmallTree.m_mT1       = theBigTree.mT_Dau1->at (chosenTauPair) ;
      theSmallTree.m_mT2       = theBigTree.mT_Dau2->at (chosenTauPair) ;

      theSmallTree.m_tauH_pt   = tlv_tauH.Pt () ;
      theSmallTree.m_tauH_eta  = tlv_tauH.Eta () ;
      theSmallTree.m_tauH_phi  = tlv_tauH.Phi () ;
      theSmallTree.m_tauH_e    = tlv_tauH.E () ;
      theSmallTree.m_tauH_mass = tlv_tauH.M () ;

      theSmallTree.m_tauHMet_deltaPhi = deltaPhi (theBigTree.metphi, tlv_tauH.Phi ()) ;
      theSmallTree.m_ditau_deltaPhi = deltaPhi (tlv_firstLepton.Phi (), tlv_secondLepton.Phi ()) ;
      theSmallTree.m_ditau_deltaEta = fabs(tlv_firstLepton.Eta ()- tlv_secondLepton.Eta ()) ;
      theSmallTree.m_ditau_deltaR = tlv_firstLepton.DeltaR(tlv_secondLepton) ;
      theSmallTree.m_dau1MET_deltaphi = deltaPhi (theBigTree.metphi , tlv_firstLepton.Phi ()) ;
      theSmallTree.m_dau2MET_deltaphi = deltaPhi (theBigTree.metphi , tlv_secondLepton.Phi ()) ;

      // Create the MET TLorentzVector for BDT variables, since it's MET --> (px,py,0,0)
      TLorentzVector tlv_MET;
      tlv_MET.SetPxPyPzE( theBigTree.METx->at(chosenTauPair), theBigTree.METy->at(chosenTauPair), 0, std::hypot(theBigTree.METx->at(chosenTauPair), theBigTree.METy->at(chosenTauPair)) );

      theSmallTree.m_tauH_MET_pt                  = (tlv_tauH + tlv_MET).Pt();
      theSmallTree.m_dau2_MET_deltaEta            = fabs(tlv_secondLepton.Eta()); // since MET.Eta()==0 by definition, dEta(tau2,MET)=|tau2.Eta()|
      theSmallTree.m_ditau_deltaR_per_tauH_MET_pt = theSmallTree.m_ditau_deltaR * theSmallTree.m_tauH_MET_pt;

      theSmallTree.m_p_zeta         = Calculate_Pzeta(tlv_firstLepton, tlv_secondLepton, tlv_MET);
      theSmallTree.m_p_zeta_visible = Calculate_visiblePzeta(tlv_firstLepton, tlv_secondLepton);

      theSmallTree.m_mT_tauH_MET       = Calculate_MT(tlv_tauH + tlv_MET, tlv_MET);
      theSmallTree.m_mT_total          = Calculate_TotalMT(tlv_firstLepton, tlv_secondLepton, tlv_MET);
      if (theBigTree.SVfitMass->at (chosenTauPair) > -900.) // in case SVfit is calculated
      {
        theSmallTree.m_mT_tauH_SVFIT_MET             = Calculate_MT(tlv_tauH_SVFIT, tlv_MET);
        theSmallTree.m_BDT_tauHsvfitMet_abs_deltaPhi = fabs(ROOT::Math::VectorUtil::DeltaPhi(tlv_tauH_SVFIT, tlv_MET));
        theSmallTree.m_BDT_tauHsvfitMet_deltaPhi     = ROOT::Math::VectorUtil::DeltaPhi(tlv_tauH_SVFIT, tlv_MET);
      }

      theSmallTree.m_BDT_ditau_deltaPhi = ROOT::Math::VectorUtil::DeltaPhi(tlv_firstLepton, tlv_secondLepton);
      theSmallTree.m_BDT_dau1MET_deltaPhi = ROOT::Math::VectorUtil::DeltaPhi(tlv_firstLepton, tlv_MET);

      theSmallTree.m_dau1_iso = getIso (firstDaughterIndex, tlv_firstLepton.Pt (), theBigTree) ;
      theSmallTree.m_dau1_MVAiso = makeIsoDiscr (firstDaughterIndex, tauMVAIDIdx, theBigTree) ;
      theSmallTree.m_dau1_MVAisoNew = makeIsoDiscr (firstDaughterIndex, tauMVAIDIdxNew, theBigTree) ; //FRA syncFeb2018
      theSmallTree.m_dau1_MVAisoNewdR0p3 = makeIsoDiscr (firstDaughterIndex, tauMVAIDIdxNewdR0p3, theBigTree) ; // FRA syncApr2018
      theSmallTree.m_dau1_CUTiso = makeIsoDiscr (firstDaughterIndex, tauCUTIDIdx, theBigTree) ;
      theSmallTree.m_dau1_antiele = makeIsoDiscr (firstDaughterIndex, tauAntiEleIdx, theBigTree) ;
      theSmallTree.m_dau1_antimu  = makeIsoDiscr (firstDaughterIndex, tauAntiMuIdx, theBigTree) ;

      theSmallTree.m_dau1_photonPtSumOutsideSignalCone = theBigTree.photonPtSumOutsideSignalCone->at (firstDaughterIndex) ;

      int ibit = tauIDsMap["byLooseCombinedIsolationDeltaBetaCorr3Hits"] ;
      theSmallTree.m_dau1_byLooseCombinedIsolationDeltaBetaCorr3Hits = ( theBigTree.tauID->at (firstDaughterIndex)  & (1 << ibit) ) ? true : false ;
      theSmallTree.m_dau2_byLooseCombinedIsolationDeltaBetaCorr3Hits = ( theBigTree.tauID->at (secondDaughterIndex) & (1 << ibit) ) ? true : false ;

      ibit = tauIDsMap["byMediumCombinedIsolationDeltaBetaCorr3Hits"] ;
      theSmallTree.m_dau1_byMediumCombinedIsolationDeltaBetaCorr3Hits = ( theBigTree.tauID->at (firstDaughterIndex)  & (1 << ibit) ) ? true : false ;
      theSmallTree.m_dau2_byMediumCombinedIsolationDeltaBetaCorr3Hits = ( theBigTree.tauID->at (secondDaughterIndex) & (1 << ibit) ) ? true : false ;

      ibit = tauIDsMap["byTightCombinedIsolationDeltaBetaCorr3Hits"] ;
      theSmallTree.m_dau1_byTightCombinedIsolationDeltaBetaCorr3Hits = ( theBigTree.tauID->at (firstDaughterIndex)  & (1 << ibit) ) ? true : false ;
      theSmallTree.m_dau2_byTightCombinedIsolationDeltaBetaCorr3Hits = ( theBigTree.tauID->at (secondDaughterIndex) & (1 << ibit) ) ? true : false ;

      theSmallTree.m_dau1_pt = tlv_firstLepton.Pt () ;
      theSmallTree.m_dau1_pt_tauup = tlv_firstLepton_tauup.Pt () ;
      theSmallTree.m_dau1_pt_taudown = tlv_firstLepton_taudown.Pt () ;
      theSmallTree.m_dau1_eta = tlv_firstLepton.Eta () ;
      theSmallTree.m_dau1_phi = tlv_firstLepton.Phi () ;
      theSmallTree.m_dau1_e = theBigTree.daughters_e->at (firstDaughterIndex) ;
      theSmallTree.m_dau1_dxy = theBigTree.dxy->at(firstDaughterIndex) ;
      theSmallTree.m_dau1_dz  = theBigTree.dz->at(firstDaughterIndex) ;
      theSmallTree.m_dau1_flav = theBigTree.daughters_charge->at (firstDaughterIndex) * (theBigTree.particleType->at (firstDaughterIndex) + 1) ;
      // 1 = from muons collection
      // 2 = from electrons collection
      // 3 = from tauH collection
      theSmallTree.m_dau2_iso = getIso (secondDaughterIndex, tlv_secondLepton.Pt (), theBigTree) ;
      theSmallTree.m_dau2_MVAiso = makeIsoDiscr (secondDaughterIndex, tauMVAIDIdx, theBigTree) ;
      theSmallTree.m_dau2_MVAisoNew = makeIsoDiscr (secondDaughterIndex, tauMVAIDIdxNew, theBigTree) ; //FRA syncFeb2018
      theSmallTree.m_dau2_MVAisoNewdR0p3 = makeIsoDiscr (secondDaughterIndex, tauMVAIDIdxNewdR0p3, theBigTree) ; // FRA syncApr2018
      theSmallTree.m_dau2_CUTiso = makeIsoDiscr (secondDaughterIndex, tauCUTIDIdx, theBigTree) ;
      theSmallTree.m_dau2_antiele = makeIsoDiscr (secondDaughterIndex, tauAntiEleIdx, theBigTree) ;
      theSmallTree.m_dau2_antimu  = makeIsoDiscr (secondDaughterIndex, tauAntiMuIdx, theBigTree) ;
      theSmallTree.m_dau2_photonPtSumOutsideSignalCone = theBigTree.photonPtSumOutsideSignalCone->at (secondDaughterIndex) ;      
      theSmallTree.m_dau2_pt = tlv_secondLepton.Pt () ;
      theSmallTree.m_dau2_pt_tauup = tlv_secondLepton_tauup.Pt () ;
      theSmallTree.m_dau2_pt_taudown = tlv_secondLepton_taudown.Pt () ;
      theSmallTree.m_dau2_eta = tlv_secondLepton.Eta () ;
      theSmallTree.m_dau2_phi = tlv_secondLepton.Phi () ;
      theSmallTree.m_dau2_e = theBigTree.daughters_e->at (secondDaughterIndex) ;
      theSmallTree.m_dau2_dxy = theBigTree.dxy->at(secondDaughterIndex) ;
      theSmallTree.m_dau2_dz  = theBigTree.dz->at(secondDaughterIndex) ;
      theSmallTree.m_dau2_flav = theBigTree.daughters_charge->at (secondDaughterIndex) * (theBigTree.particleType->at (secondDaughterIndex) + 1) ;



      // DATA/MC ID and ISO ScaleFactors
      // Tau: https://indico.cern.ch/event/719250/contributions/2971854/attachments/1635435/2609013/tauid_recommendations2017.pdf
      //      https://twiki.cern.ch/twiki/bin/viewauth/CMS/TauIDRecommendation13TeV#Performance_in_data_and_recommen
      // Ele: https://twiki.cern.ch/twiki/bin/view/CMS/Egamma2017DataRecommendations#Efficiency_Scale_Factors
      // MU : https://twiki.cern.ch/twiki/bin/view/CMS/MuonReferenceEffs2017#Scale_Factors_with_statistical_e
      
      float idAndIsoSF = 1.0;

      // MuTau Channel
      if (pType == 0 && isMC)
      {
        float mu1pt  = tlv_firstLepton.Pt();
        float mu1eta = TMath::Abs(tlv_firstLepton.Eta());

        float idSF_leg1  = getContentHisto2D(hMuPOGSF_ID , mu1pt, mu1eta);
        float isoSF_leg1 = getContentHisto2D(hMuPOGSF_ISO, mu1pt, mu1eta);
        float idAndIsoSF_leg1 = idSF_leg1 * isoSF_leg1;
        float idAndIsoSF_leg2 = 0.89; // TauPOG recommendation for 2017 data
        
        idAndIsoSF = idAndIsoSF_leg1 * idAndIsoSF_leg2;
      }

      // EleTau Channel
      else if (pType == 1 && isMC)
      {
        float ele1pt = tlv_firstLepton.Pt();
        float ele1eta = tlv_firstLepton.Eta();
          
        float idAndIsoSF_leg1 = getContentHisto2D(hElePOGSF_TightID_80WP, ele1eta, ele1pt);  // EMVATight == 80% eff WP
        float idAndIsoSF_leg2 = 0.89; // TauPOG recommendation for 2017 data
        
        idAndIsoSF = idAndIsoSF_leg1 * idAndIsoSF_leg2;
      }

      // TauTau Channel
      else if (pType == 2 && isMC)
      {
        float idAndIsoSF_leg1 = 0.89; // TauPOG recommendation for 2017 data
        float idAndIsoSF_leg2 = 0.89; // TauPOG recommendation for 2017 data
        
        idAndIsoSF = idAndIsoSF_leg1 * idAndIsoSF_leg2;
      }

      // MuMu Channel
      else if (pType == 3 && isMC)
      {
        float mu1pt  = tlv_firstLepton.Pt();
        float mu1eta = TMath::Abs(tlv_firstLepton.Eta());
        float mu2pt  = tlv_secondLepton.Pt();
        float mu2eta = TMath::Abs(tlv_secondLepton.Eta());
        
        float idSF_leg1  = getContentHisto2D(hMuPOGSF_ID , mu1pt, mu1eta);
        float isoSF_leg1 = getContentHisto2D(hMuPOGSF_ISO, mu1pt, mu1eta);
        float idAndIsoSF_leg1 = idSF_leg1 * isoSF_leg1;
        float idSF_leg2  = getContentHisto2D(hMuPOGSF_ID , mu2pt, mu2eta);
        float isoSF_leg2 = getContentHisto2D(hMuPOGSF_ISO, mu2pt, mu2eta);
        float idAndIsoSF_leg2 = idSF_leg2 * isoSF_leg2;
        
        idAndIsoSF = idAndIsoSF_leg1 * idAndIsoSF_leg2;
      }

      // EleEle Channel
      else if (pType == 4 && isMC)
      {
        float ele1pt = tlv_firstLepton.Pt();
        float ele2pt = tlv_secondLepton.Pt();
        float ele1eta = tlv_firstLepton.Eta();
        float ele2eta = tlv_secondLepton.Eta();

        float idAndIsoSF_leg1 = getContentHisto2D(hElePOGSF_MediumID_90WP, ele1eta, ele1pt);  // EMVAMedium == 90% eff WP
        float idAndIsoSF_leg2 = getContentHisto2D(hElePOGSF_MediumID_90WP, ele2eta, ele2pt);  // EMVAMedium == 90% eff WP
        
        idAndIsoSF = idAndIsoSF_leg1 * idAndIsoSF_leg2;
      }
      // Save the IDandISO SF (event per event)
      theSmallTree.m_IdAndIsoSF = (isMC ? idAndIsoSF : 1.0);


      // DATA/MC Trigger ScaleFactors
      // https://github.com/CMS-HTT/LeptonEfficiencies
      // https://github.com/truggles/TauTriggerSFs2017

      float trigSF = 1.0;

      /*if (pType == 0 && isMC)
      {

      }

      // EleTau Channel
      else if (pType == 1 && isMC)
      {
      
      }

      // TauTau Channel
      else if (pType == 2 && isMC)
      {
      
      }

      // MuMu Channel
      else if (pType == 3 && isMC)
      {
      
      }

      // EleEle Channel
      else if (pType == 4 && isMC)
      {
      
      }

      theSmallTree.m_trigSF     = (isMC ? trigSF : 1.0);*/


      // ------ OLD METHOD -- 2016 DATA ------
      // FIXME: should I compute a SF for ID-ISO for taus?
      if (pType == 0 && isMC) // mu
	{
	  // trigSF = myScaleFactor[type1][0]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	  // trigSF = 1.0; // no trigger info available in MC

	  float mupt  = tlv_firstLepton.Pt();
	  float muabseta = TMath::Abs(tlv_firstLepton.Eta());
      
	  // to combine SF based on lumi, here is the list of lumi in /fb per run period
	  // B 5.892
	  // C 2.646
	  // D 4.353
	  // E 4.117
	  // F 3.186
	  // G 7.721
	  // H 8.857
	  // TOT: 36.772
      
	  // B-F : 20.194 (frac: 0.5492)
	  // G-H : 16.578 (frac: 0.4508)
	  // cout << "DEBUG: getting content histo 2D : " << hMuPOGSF_ID_BF << " " << hMuPOGSF_ID_GH << " " << hMuPOGSF_ISO_BF << " " << hMuPOGSF_ISO_GH << " " << hMuPOGSF_trig_p3 << " " << hMuPOGSF_trig_p4 << endl;
	  // ID
	  double idsf_BF = getContentHisto2D(hMuPOGSF_ID_BF, mupt, muabseta);
	  double idsf_GH = getContentHisto2D(hMuPOGSF_ID_GH, mupt, muabseta);
	  double idsf = 0.5492*idsf_BF + 0.4508*idsf_GH; 

	  // ISO
	  double isosf_BF = getContentHisto2D(hMuPOGSF_ISO_BF, mupt, muabseta);
	  double isosf_GH = getContentHisto2D(hMuPOGSF_ISO_GH, mupt, muabseta);
	  double isosf = 0.5492*isosf_BF + 0.4508*isosf_GH; 

	  idAndIsoSF = idsf * isosf;

	  // TRIG -- just to compute if I am not reweighting the MC
	  if (applyTriggers)
	    {
	      // NOTE: from mu POG twiki, Period 3: (3/fb) run F post L1 EMFT fix (from run 278167); Period 4: (16/fb) run GH (post HIPs fix).
	      // so normalization is different in this case. TOT LUMI = 16+3 = 19. p3: 0.158, p4: 0.842
	      // NOTE (11/02/17) : SFs have been updated to B-F and GH, so from now we use the same normalization as other chs
	      double trigsf_p3 = getContentHisto2D(hMuPOGSF_trig_p3, mupt, muabseta);
	      double trigsf_p4 = getContentHisto2D(hMuPOGSF_trig_p4, mupt, muabseta);

	      trigSF = 0.5492*trigsf_p3 + 0.4508*trigsf_p4;
	    }

	  // int ptbin = hMuPOGSF_ID->GetXaxis()->FindBin(tlv_firstLepton.Pt());
	  // if (ptbin > hMuPOGSF_ID->GetNbinsX()) ptbin = hMuPOGSF_ID->GetNbinsX();
	  // else if (ptbin < 1) ptbin = 1;

	  // int etabin = hMuPOGSF_ID->GetYaxis()->FindBin(TMath::Abs(tlv_firstLepton.Eta()));
	  // if (etabin > hMuPOGSF_ID->GetNbinsY()) etabin = hMuPOGSF_ID->GetNbinsY();
	  // else if (etabin < 1) etabin = 1;

	  // idAndIsoSF = hMuPOGSF_ID->GetBinContent(ptbin, etabin);

	  // ptbin = hMuPOGSF_ISO->GetXaxis()->FindBin(tlv_firstLepton.Pt());
	  // if (ptbin > hMuPOGSF_ISO->GetNbinsX()) ptbin = hMuPOGSF_ISO->GetNbinsX();
	  // else if (ptbin < 1) ptbin = 1;

	  // etabin = hMuPOGSF_ISO->GetYaxis()->FindBin(TMath::Abs(tlv_firstLepton.Eta()));
	  // if (etabin > hMuPOGSF_ISO->GetNbinsY()) etabin = hMuPOGSF_ISO->GetNbinsY();
	  // else if (etabin < 1) etabin = 1;

	  // idAndIsoSF *= hMuPOGSF_ISO->GetBinContent(ptbin, etabin);
	  // idAndIsoSF = myScaleFactor[type1][1]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	}

      // FIXME: should I compute a SF for ID-ISO for taus?
      else if (pType == 1 && isMC) // ele
	{
	  // trigSF = myScaleFactor[type1][0]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	  // trigSF = 1.0; // no trigger info available in MC
      
	  // FIXME: should we use MU POG SFs?
	  idAndIsoSF = myScaleFactor[type1][1]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
      
	  if (applyTriggers)
	    trigSF = myScaleFactor[type1][0]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	}

      else if (pType == 2 && isMC) // tau tau pair
	{

	  idAndIsoSF = 1.0; // recommendation from tau POG?
  
	  if (applyTriggers)
	    {      
	      double SF1 = tauTrgSF.getSF(tlv_firstLepton.Pt(),  theBigTree.decayMode->at(firstDaughterIndex)) ;
	      double SF2 = tauTrgSF.getSF(tlv_secondLepton.Pt(), theBigTree.decayMode->at(secondDaughterIndex)) ;
	      trigSF = SF1 * SF2;
	    }
	}

      else if (pType == 3 && isMC) // mumu pair
	{
	  // trigSF = myScaleFactor[type1][0]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	  // trigSF = 1.0; // no trigger info available in MC

	  ////////////////// first muon ID and ISO
	  float mupt  = tlv_firstLepton.Pt();
	  float muabseta = TMath::Abs(tlv_firstLepton.Eta());
      
	  // to combine SF based on lumi, here is the list of lumi in /fb per run period
	  // B 5.892
	  // C 2.646
	  // D 4.353
	  // E 4.117
	  // F 3.186
	  // G 7.721
	  // H 8.857
	  // TOT: 36.772
      
	  // B-F : 20.194 (frac: 0.5492)
	  // G-H : 16.578 (frac: 0.4508)
	  // cout << "DEBUG: getting content histo 2D : " << hMuPOGSF_ID_BF << " " << hMuPOGSF_ID_GH << " " << hMuPOGSF_ISO_BF << " " << hMuPOGSF_ISO_GH << " " << hMuPOGSF_trig_p3 << " " << hMuPOGSF_trig_p4 << endl;
	  // ID
	  double idsf_BF = getContentHisto2D(hMuPOGSF_ID_BF, mupt, muabseta);
	  double idsf_GH = getContentHisto2D(hMuPOGSF_ID_GH, mupt, muabseta);
	  double idsf = 0.5492*idsf_BF + 0.4508*idsf_GH; 

	  // ISO
	  double isosf_BF = getContentHisto2D(hMuPOGSF_ISO_BF, mupt, muabseta);
	  double isosf_GH = getContentHisto2D(hMuPOGSF_ISO_GH, mupt, muabseta);
	  double isosf = 0.5492*isosf_BF + 0.4508*isosf_GH; 

	  idAndIsoSF = idsf * isosf;

	  // TRIG -- just to compute if I am not reweighting the MC. Lepton 1 if the one matched to trigger
	  if (applyTriggers)
	    {
	      // NOTE: from mu POG twiki, Period 3: (3/fb) run F post L1 EMFT fix (from run 278167); Period 4: (16/fb) run GH (post HIPs fix).
	      // so normalization is different in this case. TOT LUMI = 16+3 = 19. p3: 0.158, p4: 0.842
	      // NOTE (11/02/17) : SFs have been updated to B-F and GH, so from now we use the same normalization as other chs
	      double trigsf_p3 = getContentHisto2D(hMuPOGSF_trig_p3, mupt, muabseta);
	      double trigsf_p4 = getContentHisto2D(hMuPOGSF_trig_p4, mupt, muabseta);

	      trigSF = (0.5492*trigsf_p3 + 0.4508*trigsf_p4);
	    }


	  ////////////////// second muon ID and ISO
	  mupt  = tlv_secondLepton.Pt();
	  muabseta = TMath::Abs(tlv_secondLepton.Eta());
      
	  // to combine SF based on lumi, here is the list of lumi in /fb per run period
	  // B 5.892
	  // C 2.646
	  // D 4.353
	  // E 4.117
	  // F 3.186
	  // G 7.721
	  // H 8.857
	  // TOT: 36.772
      
	  // B-F : 20.194 (frac: 0.5492)
	  // G-H : 16.578 (frac: 0.4508)
	  // cout << "DEBUG: getting content histo 2D : " << hMuPOGSF_ID_BF << " " << hMuPOGSF_ID_GH << " " << hMuPOGSF_ISO_BF << " " << hMuPOGSF_ISO_GH << " " << hMuPOGSF_trig_p3 << " " << hMuPOGSF_trig_p4 << endl;
	  // ID
	  idsf_BF = getContentHisto2D(hMuPOGSF_ID_BF, mupt, muabseta);
	  idsf_GH = getContentHisto2D(hMuPOGSF_ID_GH, mupt, muabseta);
	  idsf = 0.5492*idsf_BF + 0.4508*idsf_GH; 

	  // ISO
	  isosf_BF = getContentHisto2D(hMuPOGSF_ISO_BF, mupt, muabseta);
	  isosf_GH = getContentHisto2D(hMuPOGSF_ISO_GH, mupt, muabseta);
	  isosf = 0.5492*isosf_BF + 0.4508*isosf_GH; 

	  idAndIsoSF *= (idsf * isosf);
	}

      else if (pType == 4 && isMC) // ee pair
	{
	  // trigSF = myScaleFactor[type1][0]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	  // trigSF = 1.0; // no trigger info available in MC
      
	  // FIXME: should we use MU POG SFs?
	  idAndIsoSF = myScaleFactor[type1][1]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	  idAndIsoSF *= myScaleFactor[type1][1]->get_ScaleFactor(tlv_secondLepton.Pt(),tlv_secondLepton.Eta());
      
	  if (applyTriggers)
	    trigSF = myScaleFactor[type1][0]->get_ScaleFactor(tlv_firstLepton.Pt(),tlv_firstLepton.Eta());
	}

      theSmallTree.m_trigSF     = (isMC ? trigSF : 1.0);
      theSmallTree.m_IdAndIsoSF = (isMC ? idAndIsoSF : 1.0);

      // loop over leptons
      vector<pair<float, int> > thirdLeptons ; // pt, idx
      for (unsigned int iLep = 0 ; (iLep < theBigTree.daughters_px->size ()) ; ++iLep)
	{
	  // skip the H decay candiates
	  if (int (iLep) == firstDaughterIndex || 
	      int (iLep) == secondDaughterIndex) continue ;

	  // remove taus
	  if (theBigTree.particleType->at (iLep) == 2)
	    {
	      continue ;
	    }  
	  else if (theBigTree.particleType->at (iLep) == 0) // muons
	    {
	      if (!oph.muBaseline (&theBigTree, iLep, 10., 2.4, 0.3, OfflineProducerHelper::MuLoose)) continue ;
	    }
	  else if (theBigTree.particleType->at (iLep) == 1) // electrons
	    {
	      if (!oph.eleBaseline (&theBigTree, iLep, 10., 2.5, 0.3, OfflineProducerHelper::EMVAMedium)) continue ;  //FRA: syncFeb2018
	    }
	  TLorentzVector tlv_dummyLepton
	    (
	     theBigTree.daughters_px->at (iLep),
	     theBigTree.daughters_py->at (iLep),
	     theBigTree.daughters_pz->at (iLep),
	     theBigTree.daughters_e->at (iLep)
	     ) ;
	  thirdLeptons.push_back (make_pair(tlv_dummyLepton.Pt(), iLep)) ;
      
	  if(DEBUG)
	    {
	      cout << "** 3rd lep veto passed"
		   << " idx="  << iLep
		   << " type=" << theBigTree.particleType->at(iLep)
		   << " pt="   << tlv_dummyLepton.Pt()
		   << " eta="  << tlv_dummyLepton.Eta()
		   << " phi="  << tlv_dummyLepton.Phi()
		   << " iso="  << getIso (iLep, tlv_dummyLepton.Pt (), theBigTree)
		   << " dxy="  << theBigTree.dxy->at(iLep)
		   << " dz="   << theBigTree.dz->at(iLep)
		   << " elePassConvVeto=" << theBigTree.daughters_passConversionVeto->at(iLep)
		   << " eleMissingHits="  << theBigTree.daughters_eleMissingHits->at(iLep)
		   << endl;
	    }

	} // loop over leptons
      sort (thirdLeptons.begin(), thirdLeptons.end()) ;
      // reverse loop to start from last one == highest pT
      for (int iLep = thirdLeptons.size() -1; (iLep >=0) && (theSmallTree.m_nleps < 2) ; iLep--)
	{
	  TLorentzVector tlv_dummyLepton
	    (
	     theBigTree.daughters_px->at (iLep),
	     theBigTree.daughters_py->at (iLep),
	     theBigTree.daughters_pz->at (iLep),
	     theBigTree.daughters_e->at (iLep)
	     ) ;

	  theSmallTree.m_leps_pt.push_back   (tlv_dummyLepton.Pt ()) ;
	  theSmallTree.m_leps_eta.push_back  (tlv_dummyLepton.Eta ()) ;
	  theSmallTree.m_leps_phi.push_back  (tlv_dummyLepton.Phi ()) ;
	  theSmallTree.m_leps_e.push_back    (tlv_dummyLepton.E ()) ;
	  theSmallTree.m_leps_flav.push_back (theBigTree.particleType->at (iLep)) ;
	  ++theSmallTree.m_nleps ;
	} 

      if(DEBUG)
	{
	  cout << "***** DEBUG: nleps="<< theSmallTree.m_nleps<< endl;
	}

      // ----------------------------------------------------------
      // select jets 
      // ----------------------------------------------------------

      vector <pair <float, int> > jets_and_sortPar ;
      // loop over jets
      TLorentzVector jetVecSum (0,0,0,0);
      for (unsigned int iJet = 0 ; iJet < theBigTree.jets_px->size () ; ++iJet)
      {
        // JET PU ID cut
        if (theBigTree.jets_PUJetID->at (iJet) < PUjetID_minCut) continue ;
        if (theBigTree.PFjetID->at (iJet) < PFjetID_WP) continue; // 0 ; don't pass PF Jet ID; 1: loose, 2: tight, 3: tightLepVeto

        TLorentzVector tlv_jet
        (
         theBigTree.jets_px->at (iJet),
         theBigTree.jets_py->at (iJet),
         theBigTree.jets_pz->at (iJet),
         theBigTree.jets_e->at (iJet)
        ) ;
        if (tlv_jet.Pt () < 20.) continue ;
        if (tlv_jet.DeltaR (tlv_firstLepton) < lepCleaningCone) continue ;
        if (tlv_jet.DeltaR (tlv_secondLepton) < lepCleaningCone) continue ;

        // use these jets for HT
        if (tlv_jet.Pt () > 20)
        {
          ++theSmallTree.m_njets20 ;
          theSmallTree.m_HT20 += tlv_jet.Pt() ;
          jetVecSum += tlv_jet ;
        }

          if (tlv_jet.Pt () > 50)
        {
          ++theSmallTree.m_njets50 ;
          theSmallTree.m_HT50 += tlv_jet.Pt() ;
        }

        // all jets selected as btag cands apart from eta cut
        int ajetHadFlav = abs(theBigTree.jets_HadronFlavour->at(iJet));
        if (ajetHadFlav == 5) ++theSmallTree.m_njetsBHadFlav;
        if (ajetHadFlav == 4) ++theSmallTree.m_njetsCHadFlav;

        if (TMath::Abs(tlv_jet.Eta()) > 2.4) continue; // 2.4 for b-tag

        //float sortPar = (bChoiceFlag == 1 ) ? theBigTree.bCSVscore->at (iJet) : tlv_jet.Pt() ;
        float sortPar = (bChoiceFlag == 1 ) ? theBigTree.bDeepCSV_probb->at(iJet) + theBigTree.bDeepCSV_probbb->at(iJet)  : tlv_jet.Pt() ;
        if (bChoiceFlag != 1 && bChoiceFlag != 2) cout << "** WARNING : bChoiceFlag not known :" << bChoiceFlag << endl;
        jets_and_sortPar.push_back (make_pair (sortPar, iJet) );

      } // loop over jets

      theSmallTree.m_HT20Full = theSmallTree.m_HT20 + tlv_firstLepton.Pt() + tlv_secondLepton.Pt() ;
      theSmallTree.m_jet20centrality = jetVecSum.Pt() / theSmallTree.m_HT20Full ;

      theSmallTree.m_nbjetscand = jets_and_sortPar.size();
      theSmallTree.m_nfatjets = theBigTree.ak8jets_px->size();

      if (!beInclusive && jets_and_sortPar.size () < 2) continue ;
      ec.Increment("TwoJets", EvtW);
      if (isHHsignal && pairType == genHHDecMode) ecHHsig[genHHDecMode].Increment ("TwoJets", EvtW);

      // sort jet collection by deepCSV
      sort (jets_and_sortPar.begin(), jets_and_sortPar.end(), bJetSort); //sort by first parameter, then pt (dummy if pt order chosen)
      if (jets_and_sortPar.size () >= 2)
      {
        bool isVBF = false;
        vector<pair <int, float> > jets_and_BTag;
        for (auto pair : jets_and_sortPar) jets_and_BTag.push_back (make_pair(pair.second, pair.first)); // just for compatibility...

        // NB !!! the following function only works if jets_and_sortPar contains <CVSscore, idx>
        vector<float> bTagWeight = bTagSFHelper.getEvtWeight (jets_and_BTag, theBigTree.jets_px, theBigTree.jets_py, theBigTree.jets_pz, theBigTree.jets_e, theBigTree.jets_HadronFlavour, pType) ;
        theSmallTree.m_bTagweightL = (isMC ? bTagWeight.at(0) : 1.0) ;
        theSmallTree.m_bTagweightM = (isMC ? bTagWeight.at(1) : 1.0) ;
        theSmallTree.m_bTagweightT = (isMC ? bTagWeight.at(2) : 1.0) ;

        bool bPairFound = false;
        int njets = jets_and_sortPar.size();
        if (jets_and_sortPar.at(njets-2).first>0.4941) bPairFound = true; // medium WP is: 0.8484 for 2016 CSV, 0.4941 for 2017 DeepCSV

        const int bjet1idx = jets_and_sortPar.at(njets-1).second ;
        int bjet2idx_temp  = jets_and_sortPar.at(njets-2).second ;

        //VBF tag
        int VBFidx1 = -1;
        int VBFidx2 = -1;
        theSmallTree.m_isVBF = 0;

        // build all the possible VBF-jet pairs
        std::vector< tuple<float, int, int> > VBFcand_Mjj;
        if (theBigTree.jets_px->size ()>1)
        {
          for (unsigned int iJet = 0; (iJet < theBigTree.jets_px->size ()) && (theSmallTree.m_njets < maxNjetsSaved); ++iJet)
          {
            if (theBigTree.jets_PUJetID->at (iJet) < PUjetID_minCut) continue ;
            if (theBigTree.PFjetID->at (iJet) < PFjetID_WP) continue; // 0 ; don't pass PF Jet ID; 1: loose, 2: tight, 3: tightLepVeto
            if (int (iJet) == bjet1idx) continue;
            if(bPairFound && int (iJet) == bjet2idx_temp) continue;
            TLorentzVector ijet;
            ijet.SetPxPyPzE(
                           theBigTree.jets_px->at (iJet),
                           theBigTree.jets_py->at (iJet),
                           theBigTree.jets_pz->at (iJet),
                           theBigTree.jets_e ->at (iJet)
                           );
            if (ijet.DeltaR (tlv_firstLepton) < lepCleaningCone) continue ;
            if (ijet.DeltaR (tlv_secondLepton) < lepCleaningCone) continue ;
            if(ijet.Pt() < 30.) continue;
            if(fabs(ijet.Eta()) > 5.) continue; // keeping the whole HF acceptance for the time being
            for (unsigned int kJet = iJet+1 ;   (kJet < theBigTree.jets_px->size ()) && (theSmallTree.m_njets < maxNjetsSaved) ;  ++kJet)
            {
              if (theBigTree.jets_PUJetID->at (kJet) < PUjetID_minCut) continue ;
              if (theBigTree.PFjetID->at (kJet) < PFjetID_WP) continue; // 0 ; don't pass PF Jet ID; 1: loose, 2: tight, 3: tightLepVeto
              if (int (kJet) == bjet1idx) continue;
              if (bPairFound && int (kJet) == bjet2idx_temp) continue;
              TLorentzVector kjet;
              kjet.SetPxPyPzE(
                             theBigTree.jets_px->at (kJet),
                             theBigTree.jets_py->at (kJet),
                             theBigTree.jets_pz->at (kJet),
                             theBigTree.jets_e ->at (kJet)
                             );
              if (kjet.DeltaR (tlv_firstLepton) < lepCleaningCone) continue ;
              if (kjet.DeltaR (tlv_secondLepton) < lepCleaningCone) continue ;
              if(kjet.Pt() < 30.) continue;
              if(fabs(kjet.Eta()) > 5.) continue;
              TLorentzVector jetPair = ijet+kjet;

              bool VBFjetLegsMatched = true;
              if (isVBFfired) VBFjetLegsMatched = checkVBFjetMatch(DEBUG, iJet, kJet, theBigTree);
              if (isVBFfired && !VBFjetLegsMatched) continue;
              VBFcand_Mjj.push_back(make_tuple(jetPair.M(),iJet,kJet));
            }
          }

          // if is a VBF event (in the tautau channel) but no good candidate is found --> throw away the event
          if (isVBFfired && VBFcand_Mjj.size()<=0) continue;

          if (VBFcand_Mjj.size()>0)
          {
            std::sort(VBFcand_Mjj.begin(),VBFcand_Mjj.end());
            isVBF = true;
            VBFidx1 = std::get<1>(*(VBFcand_Mjj.rbegin()));
            VBFidx2 = std::get<2>(*(VBFcand_Mjj.rbegin()));
          }

        } // all possible VBF jets pairs built

        int bjet2idx_notVBF = bjet2idx_temp;  // assign 2nd jet by CSV in any case, then:
        int bjet2idx_isVBF  = bjet2idx_temp;

        if (!bPairFound)  // if the bjet2 was not definitive yet
        {
          if(isVBF)
          {
            if ((bjet2idx_temp == VBFidx1) || (bjet2idx_temp == VBFidx2))   // and the 2nd jet by CSV was already picked as VBF jet
            {
              for (int bidx = 2; bidx<=int(jets_and_sortPar.size()); bidx++)  // look for the next jet by CSV
              {
                int idxbyCSV = jets_and_sortPar.at(njets-bidx).second;
                if ((idxbyCSV== VBFidx1)||(idxbyCSV== VBFidx2)) continue;
                bPairFound = true;
                bjet2idx_isVBF = idxbyCSV;
                break;
              }
              if(bPairFound == false)  // if there were not enough jets
              {
                isVBF = false;         // discard the jets as VBF jets
                VBFcand_Mjj.clear();
              }
            }
          }
        }

        if (isVBF)
            theSmallTree.m_isVBF = 1;
        else
            theSmallTree.m_isVBF = 0;

        const int bjet2idx = isVBF? bjet2idx_isVBF : bjet2idx_notVBF;

        // Now that I've selected the bjets build the TLorentzVectors
        TLorentzVector tlv_firstBjet (theBigTree.jets_px->at(bjet1idx), theBigTree.jets_py->at(bjet1idx), theBigTree.jets_pz->at(bjet1idx), theBigTree.jets_e->at(bjet1idx));
        TLorentzVector tlv_secondBjet(theBigTree.jets_px->at(bjet2idx), theBigTree.jets_py->at(bjet2idx), theBigTree.jets_pz->at(bjet2idx), theBigTree.jets_e->at(bjet2idx));

        double ptRegr[2] = {tlv_firstBjet.Pt(), tlv_secondBjet.Pt()};
        if (computeBregr)
        {
          for (int iBJet = 0; iBJet <=1; iBJet++)
          {
            int bidx = (iBJet == 0 ? bjet1idx : bjet2idx);
            bjrv.Jet_pt     = (iBJet == 0 ? tlv_firstBjet.Pt()  : tlv_secondBjet.Pt());
            bjrv.Jet_eta    = (iBJet == 0 ? tlv_firstBjet.Eta() : tlv_secondBjet.Eta());
            //bjrv.Jet_corr         = theBigTree.jets_rawPt->at(bidx);
            bjrv.Jet_corr         = theBigTree.jetRawf->at(bidx); // should be 1./jetrawf ??
            bjrv.rho              = theBigTree.rho;
            bjrv.Jet_mt           = theBigTree.jets_mT->at(bidx);
            bjrv.Jet_leadTrackPt  = theBigTree.jets_leadTrackPt->at(bidx);
            bjrv.Jet_leptonPtRel  = theBigTree.jets_leptonPtRel->at(bidx);
            bjrv.Jet_leptonPt     = theBigTree.jets_leptonPt->at(bidx);
            bjrv.Jet_leptonDeltaR = theBigTree.jets_leptonDeltaR->at(bidx);
            bjrv.Jet_neHEF   = theBigTree.jets_nHEF->at(bidx);
            bjrv.Jet_neEmEF  = theBigTree.jets_nEmEF->at(bidx);
            bjrv.Jet_chMult  = theBigTree.jets_chMult->at(bidx);
            bjrv.Jet_vtxPt   = theBigTree.jets_vtxPt->at(bidx);
            bjrv.Jet_vtxMass = theBigTree.jets_vtxMass->at(bidx);
            bjrv.Jet_vtx3dL  = theBigTree.jets_vtx3dL->at(bidx);
            bjrv.Jet_vtxNtrk = theBigTree.jets_vtxNtrk->at(bidx);
            bjrv.Jet_vtx3deL = theBigTree.jets_vtx3deL->at(bidx);

            ptRegr[iBJet] = (bRreader->EvaluateRegression (bRegrMethodName.c_str()))[0];
          }
        }

        // save the b-jets
        TLorentzVector tlv_firstBjet_raw = tlv_firstBjet;
        TLorentzVector tlv_secondBjet_raw = tlv_secondBjet;

        // ----- up/down variation using JEC
        double unc_first = theBigTree.jets_jecUnc->at (bjet1idx);
        TLorentzVector tlv_firstBjet_raw_jetup = tlv_firstBjet_raw;
        TLorentzVector tlv_firstBjet_raw_jetdown = tlv_firstBjet_raw;

        tlv_firstBjet_raw_jetup.SetPtEtaPhiE(
                           (1.+unc_first) *tlv_firstBjet_raw_jetup.Pt(),
                           tlv_firstBjet_raw_jetup.Eta(),
                           tlv_firstBjet_raw_jetup.Phi(),
                           (1.+unc_first) *tlv_firstBjet_raw_jetup.E()
                           );
        tlv_firstBjet_raw_jetdown.SetPtEtaPhiE(
                         (1.-unc_first) *tlv_firstBjet_raw_jetdown.Pt(),
                         tlv_firstBjet_raw_jetdown.Eta(),
                         tlv_firstBjet_raw_jetdown.Phi(),
                         (1.-unc_first) *tlv_firstBjet_raw_jetdown.E()
                         );

        double unc_second = theBigTree.jets_jecUnc->at (bjet2idx);
        TLorentzVector tlv_secondBjet_raw_jetup = tlv_secondBjet_raw;
        TLorentzVector tlv_secondBjet_raw_jetdown = tlv_secondBjet_raw;

        tlv_secondBjet_raw_jetup.SetPtEtaPhiE(
                        (1.+unc_second) *tlv_secondBjet_raw_jetup.Pt(),
                        tlv_secondBjet_raw_jetup.Eta(),
                        tlv_secondBjet_raw_jetup.Phi(),
                        (1.+unc_second) *tlv_secondBjet_raw_jetup.E()
                        );
        tlv_secondBjet_raw_jetdown.SetPtEtaPhiE(
                          (1.-unc_second) *tlv_secondBjet_raw_jetdown.Pt(),
                          tlv_secondBjet_raw_jetdown.Eta(),
                          tlv_secondBjet_raw_jetdown.Phi(),
                          (1.-unc_second) *tlv_secondBjet_raw_jetdown.E()
                          );

        if (DEBUG)
        {
            cout << "-------- JET JEC -------" << endl;
            cout << "jet1 UP: " << tlv_firstBjet_raw_jetup.Pt() <<endl;
            cout << "jet1 DW: " << tlv_firstBjet_raw_jetdown.Pt()<<endl;
            cout << "jet2 UP: " << tlv_secondBjet_raw_jetup.Pt()<<endl;
            cout << "jet2 DW: " << tlv_secondBjet_raw_jetdown.Pt()<<endl;
            cout << endl;
            cout << "b1_deepCSV: " << theBigTree.bDeepCSV_probb->at(bjet1idx) + theBigTree.bDeepCSV_probbb->at(bjet1idx) << endl;
            cout << "b2_deepCSV: " << theBigTree.bDeepCSV_probb->at(bjet2idx) + theBigTree.bDeepCSV_probbb->at(bjet2idx) << endl;
            cout << "------------------------" << endl;
        }

        theSmallTree.m_bjet1_pt_raw = tlv_firstBjet_raw.Pt();
        theSmallTree.m_bjet2_pt_raw = tlv_secondBjet_raw.Pt();
        theSmallTree.m_bjet1_pt_raw_jetup = tlv_firstBjet_raw_jetup.Pt();
        theSmallTree.m_bjet2_pt_raw_jetup = tlv_secondBjet_raw_jetup.Pt();
        theSmallTree.m_bjet1_pt_raw_jetdown = tlv_firstBjet_raw_jetdown.Pt();
        theSmallTree.m_bjet2_pt_raw_jetdown = tlv_secondBjet_raw_jetdown.Pt();

        TLorentzVector tlv_bH_raw = tlv_firstBjet + tlv_secondBjet ;
        TLorentzVector tlv_bH_raw_jetup   = tlv_firstBjet_raw_jetup + tlv_secondBjet_raw_jetup ;
        TLorentzVector tlv_bH_raw_jetdown =  tlv_firstBjet_raw_jetdown + tlv_secondBjet_raw_jetdown ;

        theSmallTree.m_bH_mass_raw = tlv_bH_raw.M();
        theSmallTree.m_bH_mass_raw_jetup   = tlv_bH_raw_jetup.M();
        theSmallTree.m_bH_mass_raw_jetdown = tlv_bH_raw_jetdown.M();

        // FIXME : here mass is manually set to 0, should we change it?
        float ptScale1 = ptRegr[0] / tlv_firstBjet.Pt() ;
        float ptScale2 = ptRegr[1] / tlv_secondBjet.Pt() ;
        tlv_firstBjet.SetPtEtaPhiE (ptRegr[0], tlv_firstBjet.Eta(), tlv_firstBjet.Phi(), ptScale1*tlv_firstBjet.Energy());
        tlv_secondBjet.SetPtEtaPhiE (ptRegr[1], tlv_secondBjet.Eta(), tlv_secondBjet.Phi(), ptScale2*tlv_secondBjet.Energy());

        theSmallTree.m_bjet1_pt   = tlv_firstBjet.Pt () ;
        theSmallTree.m_bjet1_eta  = tlv_firstBjet.Eta () ;
        theSmallTree.m_bjet1_phi  = tlv_firstBjet.Phi () ;
        theSmallTree.m_bjet1_e    = theBigTree.jets_e->at (bjet1idx) ;
        theSmallTree.m_bjet1_bID  = theBigTree.bCSVscore->at (bjet1idx) ;
        theSmallTree.m_bjet1_bID_deepCSV  = theBigTree.bDeepCSV_probb->at(bjet1idx) + theBigTree.bDeepCSV_probbb->at(bjet1idx) ;
        theSmallTree.m_bjet1_bMVAID  = theBigTree.pfCombinedMVAV2BJetTags->at (bjet1idx) ;
        theSmallTree.m_bjet1_flav = theBigTree.jets_HadronFlavour->at (bjet1idx) ;
        double bjet1_JER = theBigTree.jets_JER->at(bjet1idx);
        theSmallTree.m_bjet1_JER = bjet1_JER ;

        theSmallTree.m_bjet2_pt   = tlv_secondBjet.Pt () ;
        theSmallTree.m_bjet2_eta  = tlv_secondBjet.Eta () ;
        theSmallTree.m_bjet2_phi  = tlv_secondBjet.Phi () ;
        theSmallTree.m_bjet2_e    = theBigTree.jets_e->at (bjet2idx) ;
        theSmallTree.m_bjet2_bID  = theBigTree.bCSVscore->at (bjet2idx) ;
        theSmallTree.m_bjet2_bID_deepCSV  = theBigTree.bDeepCSV_probb->at(bjet2idx) + theBigTree.bDeepCSV_probbb->at(bjet2idx) ;
        theSmallTree.m_bjet2_bMVAID  = theBigTree.pfCombinedMVAV2BJetTags->at (bjet2idx) ;
        theSmallTree.m_bjet2_flav = theBigTree.jets_HadronFlavour->at (bjet2idx) ;
        double bjet2_JER = theBigTree.jets_JER->at(bjet2idx);
        theSmallTree.m_bjet2_JER = bjet2_JER ;

        theSmallTree.m_bjets_bID  = theBigTree.bCSVscore->at (bjet1idx) +theBigTree.bCSVscore->at (bjet2idx) ;
        theSmallTree.m_bjets_bID_deepCSV  = theBigTree.bDeepCSV_probb->at(bjet1idx) + theBigTree.bDeepCSV_probbb->at(bjet1idx) + theBigTree.bDeepCSV_probb->at(bjet2idx) + theBigTree.bDeepCSV_probbb->at(bjet2idx);
        
        // Save gen info for b-jets
        bool hasgj1 = false;
        bool hasgj2 = false;

        if (isMC)
        {
          int mcind = theBigTree.jets_genjetIndex->at(bjet1idx);
          if (mcind>=0)
          {
            TLorentzVector gen(theBigTree.genjet_px->at(mcind),theBigTree.genjet_py->at(mcind),theBigTree.genjet_pz->at(mcind),theBigTree.genjet_e->at(mcind));
            theSmallTree.m_genjet1_pt = gen.Pt();
            theSmallTree.m_genjet1_eta = gen.Eta();
            theSmallTree.m_genjet1_phi = gen.Phi();
            theSmallTree.m_genjet1_e = gen.E();
            if (gen.Pt() > 8) hasgj1 = true;
          }

          mcind = theBigTree.jets_genjetIndex->at(bjet2idx);
          if (mcind>=0)
          {
            TLorentzVector gen(theBigTree.genjet_px->at(mcind),theBigTree.genjet_py->at(mcind),theBigTree.genjet_pz->at(mcind),theBigTree.genjet_e->at(mcind));
            theSmallTree.m_genjet2_pt = gen.Pt();
            theSmallTree.m_genjet2_eta = gen.Eta();
            theSmallTree.m_genjet2_phi = gen.Phi();
            theSmallTree.m_genjet2_e = gen.E();
            if (gen.Pt() > 8) hasgj2 = true;
          }
        }
        theSmallTree.m_bjet1_hasgenjet = hasgj1 ;
        theSmallTree.m_bjet2_hasgenjet = hasgj2 ;

        // Save HT_20, HT_50 and HT_20_BDT(with cut on |eta|<4.7)
        TLorentzVector jetVecSum (0,0,0,0);
        if (DEBUG) cout << "----- BDT HT debug ------" << endl;
        for (unsigned int iJet = 0 ; iJet < theBigTree.jets_px->size () ; ++iJet)
        {
          // JET PU ID cut
          if (theBigTree.jets_PUJetID->at (iJet) < PUjetID_minCut) continue ;
          if (theBigTree.PFjetID->at (iJet) < PFjetID_WP) continue; // 0 ; don't pass PF Jet ID; 1: loose, 2: tight, 3: tightLepVeto

          // Build the jet TLorentzVector
          TLorentzVector tlv_jet(theBigTree.jets_px->at(iJet), theBigTree.jets_py->at(iJet), theBigTree.jets_pz->at(iJet), theBigTree.jets_e->at(iJet)) ;

          // Pt cut for jets
          if (tlv_jet.Pt () < 20.) continue ;

          // Lepton and b-jet cleaning
          if (tlv_jet.DeltaR (tlv_firstLepton) < lepCleaningCone) continue ;
          if (tlv_jet.DeltaR (tlv_secondLepton) < lepCleaningCone) continue ;
          if ( (int) iJet == bjet1idx || (int) iJet == bjet2idx ) continue ;

          // use these jets for HT
          if (tlv_jet.Pt () > 20)
          {
            ++theSmallTree.m_njets20 ;
            theSmallTree.m_HT20 += tlv_jet.Pt() ;
            jetVecSum += tlv_jet ;

            if (TMath::Abs(tlv_jet.Eta()) < 4.7)
              theSmallTree.m_BDT_HT20 += tlv_jet.Pt() ;
              if (DEBUG) cout << " ---> Jet " << iJet << " - pt: " << tlv_jet.Pt() << " - HT: " << theSmallTree.m_BDT_HT20 << endl;
          }

          if (tlv_jet.Pt () > 50)
          {
            ++theSmallTree.m_njets50 ;
            theSmallTree.m_HT50 += tlv_jet.Pt() ;
          }
        }
        theSmallTree.m_HT20Full = theSmallTree.m_HT20 + tlv_firstLepton.Pt() + tlv_secondLepton.Pt() ;
        theSmallTree.m_jet20centrality = jetVecSum.Pt() / theSmallTree.m_HT20Full ;


        if (DEBUG) cout << "  HT = " << theSmallTree.m_BDT_HT20 << endl;
        if (DEBUG) cout << "---------------------" << endl;

        float METx = theBigTree.METx->at (chosenTauPair) ;
        float METy = theBigTree.METy->at (chosenTauPair) ;
        //float METpt = 0;//TMath::Sqrt (METx*METx + METy*METy) ;

        TLorentzVector tlv_bH = tlv_firstBjet + tlv_secondBjet ;
        TLorentzVector tlv_neutrinos =  tlv_bH - tlv_bH_raw;
        theSmallTree.m_met_et_corr = theBigTree.met - tlv_neutrinos.Et() ;

        const TVector2 ptmiss = TVector2(METx, METy) ;
        TMatrixD metcov (2, 2) ;
        metcov (0,0) = theBigTree.MET_cov00->at (chosenTauPair) ;
        metcov (1,0) = theBigTree.MET_cov10->at (chosenTauPair) ;
        metcov (0,1) = theBigTree.MET_cov01->at (chosenTauPair) ;
        metcov (1,1) = theBigTree.MET_cov11->at (chosenTauPair) ;
        const TMatrixD stableMetCov = metcov;

        // MET shifted for JES
        // This will be useful when splitting JECs
        //const TVector2 ptmiss_jetup   = getShiftedMET(+1., ptmiss, theBigTree);
        //const TVector2 ptmiss_jetdown = getShiftedMET(-1., ptmiss, theBigTree);
        // For now we use the total shift already stored in LLR ntuples
        const TVector2 ptmiss_jetup   (theBigTree.METx_UP->at(chosenTauPair) , theBigTree.METy_UP->at(chosenTauPair));
        const TVector2 ptmiss_jetdown (theBigTree.METx_DOWN->at(chosenTauPair) , theBigTree.METy_DOWN->at(chosenTauPair));

        // MET shifted for TES
        const TVector2 ptmiss_tauup   (theBigTree.METx_UP_TES->at(chosenTauPair) , theBigTree.METy_UP_TES->at(chosenTauPair));
        const TVector2 ptmiss_taudown (theBigTree.METx_DOWN_TES->at(chosenTauPair) , theBigTree.METy_DOWN_TES->at(chosenTauPair));

        theSmallTree.m_bH_pt = tlv_bH.Pt () ;
        theSmallTree.m_bH_eta = tlv_bH.Eta () ;
        theSmallTree.m_bH_phi = tlv_bH.Phi () ;
        theSmallTree.m_bH_e = tlv_bH.E () ;
        theSmallTree.m_bH_mass = tlv_bH.M () ;

        theSmallTree.m_bH_MET_deltaEta    = std::abs(tlv_bH.Eta()); // since MET.Eta()==0 by definition, dEta(bH,MET)=|bH.Eta()|
        theSmallTree.m_bH_MET_deltaR      = tlv_bH.DeltaR(tlv_MET);
        theSmallTree.m_bH_tauH_MET_deltaR = tlv_bH.DeltaR(tlv_tauH + tlv_MET);
        theSmallTree.m_BDT_bHMet_deltaPhi = ROOT::Math::VectorUtil::DeltaPhi(tlv_bH, tlv_MET);
        theSmallTree.m_BDT_topPairMasses  = Calculate_topPairMasses(getLVfromTLV(tlv_firstLepton), getLVfromTLV(tlv_secondLepton), getLVfromTLV(tlv_firstBjet), getLVfromTLV(tlv_secondBjet), getLVfromTLV(tlv_MET)).first;
        theSmallTree.m_BDT_MX             = Calculate_MX(tlv_firstLepton, tlv_secondLepton, tlv_firstBjet, tlv_secondBjet, tlv_MET);
        theSmallTree.m_BDT_bH_tauH_MET_InvMass = ROOT::Math::VectorUtil::InvariantMass(tlv_bH, tlv_tauH + tlv_MET);
        theSmallTree.m_BDT_bH_tauH_InvMass     = ROOT::Math::VectorUtil::InvariantMass(tlv_bH, tlv_tauH);
        theSmallTree.m_BDT_MET_bH_cosTheta = Calculate_cosTheta_2bodies(getLVfromTLV(tlv_MET), getLVfromTLV(tlv_bH));
        theSmallTree.m_BDT_b1_bH_cosTheta  = Calculate_cosTheta_2bodies(getLVfromTLV(tlv_firstBjet), getLVfromTLV(tlv_bH));

        TLorentzVector tlv_HH = tlv_bH + tlv_tauH ;
        TLorentzVector tlv_HH_raw = tlv_bH_raw + tlv_tauH ;
        theSmallTree.m_HH_pt = tlv_HH.Pt () ;
        theSmallTree.m_HH_eta = tlv_HH.Eta () ;
        theSmallTree.m_HH_phi = tlv_HH.Phi () ;
        theSmallTree.m_HH_e = tlv_HH.E () ;
        theSmallTree.m_HH_mass = tlv_HH.M () ;
        theSmallTree.m_HH_mass_raw = tlv_HH_raw.M () ;
        theSmallTree.m_HH_deltaR = tlv_bH.DeltaR(tlv_tauH);

        TLorentzVector tlv_HH_raw_tauup = tlv_bH_raw + tlv_firstLepton_tauup + tlv_secondLepton_tauup ;
        TLorentzVector tlv_HH_raw_taudown = tlv_bH_raw + tlv_firstLepton_taudown + tlv_secondLepton_taudown ;

        theSmallTree.m_HH_mass_raw_tauup = tlv_HH_raw_tauup.M();
        theSmallTree.m_HH_mass_raw_taudown = tlv_HH_raw_taudown.M();

        // in case the SVFIT mass is calculated
        if (theBigTree.SVfitMass->at (chosenTauPair) > -900.)
        {
          TLorentzVector tlv_HHsvfit  = tlv_bH + tlv_tauH_SVFIT ;
          theSmallTree.m_HHsvfit_pt   = tlv_HHsvfit.Pt () ;
          theSmallTree.m_HHsvfit_eta  = tlv_HHsvfit.Eta () ;
          theSmallTree.m_HHsvfit_phi  = tlv_HHsvfit.Phi () ;
          theSmallTree.m_HHsvfit_e    = tlv_HHsvfit.E () ;
          theSmallTree.m_HHsvfit_mass = tlv_HHsvfit.M () ;

          theSmallTree.m_BDT_HHsvfit_abs_deltaPhi  = fabs(ROOT::Math::VectorUtil::DeltaPhi(tlv_bH, tlv_tauH_SVFIT));
          theSmallTree.m_BDT_bH_tauH_SVFIT_InvMass = ROOT::Math::VectorUtil::InvariantMass(tlv_bH, tlv_tauH_SVFIT);
          theSmallTree.m_BDT_total_CalcPhi = Calculate_phi (getLVfromTLV(tlv_firstLepton), getLVfromTLV(tlv_secondLepton), getLVfromTLV(tlv_firstBjet), getLVfromTLV(tlv_secondBjet), getLVfromTLV(tlv_tauH_SVFIT), getLVfromTLV(tlv_bH));
          theSmallTree.m_BDT_ditau_CalcPhi = Calculate_phi1(getLVfromTLV(tlv_firstLepton), getLVfromTLV(tlv_secondLepton), getLVfromTLV(tlv_tauH_SVFIT), getLVfromTLV(tlv_bH));
          theSmallTree.m_BDT_dib_CalcPhi   = Calculate_phi1(getLVfromTLV(tlv_firstBjet), getLVfromTLV(tlv_secondBjet), getLVfromTLV(tlv_tauH_SVFIT), getLVfromTLV(tlv_bH));
          theSmallTree.m_BDT_MET_tauH_SVFIT_cosTheta   = Calculate_cosTheta_2bodies(getLVfromTLV(tlv_MET), getLVfromTLV(tlv_tauH_SVFIT));
          theSmallTree.m_BDT_tauH_SVFIT_reson_cosTheta = Calculate_cosTheta_2bodies(getLVfromTLV(tlv_tauH_SVFIT), getLVfromTLV((tlv_firstLepton+tlv_secondLepton+tlv_firstBjet+tlv_secondBjet+tlv_MET)) );

        } // in case the SVFIT mass is calculated

        // compute HHKinFit -- ask a reasonable mass window to suppress most error messages
        bool wrongHHK=false;
        float HHKmass = -999;
        float HHKChi2 = -999;
        // if (runHHKinFit && tlv_HH_raw.M() > 20 && tlv_HH_raw.M() < 200)
        if (runHHKinFit && pairType <= 2 && tlv_bH_raw.M() > 50 && tlv_bH_raw.M() < 200 && theBigTree.SVfitMass->at (chosenTauPair) > 50 && theBigTree.SVfitMass->at (chosenTauPair) < 200) // no kinfit for ee / mumu + very loose mass window
        //if (runHHKinFit && pairType <= 2) // FIXME: temporary
        {
          HHKinFit2::HHKinFitMasterHeavyHiggs kinFits = HHKinFit2::HHKinFitMasterHeavyHiggs(tlv_firstBjet, tlv_secondBjet, tlv_firstLepton, tlv_secondLepton, ptmiss, stableMetCov, bjet1_JER, bjet2_JER) ;
          HHKinFit2::HHKinFitMasterHeavyHiggs kinFitsraw = HHKinFit2::HHKinFitMasterHeavyHiggs(tlv_firstBjet_raw, tlv_secondBjet_raw, tlv_firstLepton, tlv_secondLepton,  ptmiss, stableMetCov, bjet1_JER, bjet2_JER) ;
          HHKinFit2::HHKinFitMasterHeavyHiggs kinFitsraw_tauup = HHKinFit2::HHKinFitMasterHeavyHiggs(tlv_firstBjet_raw, tlv_secondBjet_raw, tlv_firstLepton_tauup, tlv_secondLepton_tauup,  ptmiss_tauup, stableMetCov, bjet1_JER, bjet2_JER) ;
          HHKinFit2::HHKinFitMasterHeavyHiggs kinFitsraw_taudown = HHKinFit2::HHKinFitMasterHeavyHiggs(tlv_firstBjet_raw, tlv_secondBjet_raw, tlv_firstLepton_taudown, tlv_secondLepton_taudown,  ptmiss_taudown, stableMetCov, bjet1_JER, bjet2_JER) ;
          HHKinFit2::HHKinFitMasterHeavyHiggs kinFitsraw_jetup = HHKinFit2::HHKinFitMasterHeavyHiggs(tlv_firstBjet_raw_jetup, tlv_secondBjet_raw_jetup, tlv_firstLepton, tlv_secondLepton,  ptmiss_jetup, stableMetCov, bjet1_JER, bjet2_JER) ;
          HHKinFit2::HHKinFitMasterHeavyHiggs kinFitsraw_jetdown = HHKinFit2::HHKinFitMasterHeavyHiggs(tlv_firstBjet_raw_jetdown, tlv_secondBjet_raw_jetdown, tlv_firstLepton, tlv_secondLepton,  ptmiss_jetdown, stableMetCov, bjet1_JER, bjet2_JER) ;

          //           kinFits.setAdvancedBalance (&ptmiss, metcov) ;
          //           kinFits.setSimpleBalance (ptmiss.Pt (),10) ; //alternative which uses only the absolute value of ptmiss in the fit
          //
          //           kinFits.addMh1Hypothesis (hypo_mh1) ;
          //           kinFits.addMh2Hypothesis (hypo_mh2) ;
          kinFits.   addHypo(hypo_mh1,hypo_mh2);
          kinFitsraw.addHypo(hypo_mh1,hypo_mh2);
          kinFitsraw_tauup.addHypo(hypo_mh1,hypo_mh2);
          kinFitsraw_taudown.addHypo(hypo_mh1,hypo_mh2);

          try{ kinFits.fit();}
          catch(HHKinFit2::HHInvMConstraintException e)
          {
            cout<<"INVME THIS EVENT WAS WRONG, INV MASS CONSTRAIN EXCEPTION"<<endl;
            cout<<"INVME masshypo1 = 125,    masshypo2 = 125"<<endl;
            cout<<"INVME Tau1"<<endl;
            cout<<"INVME (E,Px,Py,Pz,M) "<<tlv_firstLepton.E()<<","<<tlv_firstLepton.Px()<<","<<tlv_firstLepton.Py()<<","<<tlv_firstLepton.Pz()<<","<<tlv_firstLepton.M()<<endl;
            cout<<"INVME Tau2"<<endl;
            cout<<"INVME (E,Px,Py,Pz,M) "<<tlv_secondLepton.E()<<","<<tlv_secondLepton.Px()<<","<<tlv_secondLepton.Py()<<","<<tlv_secondLepton.Pz()<<","<<tlv_secondLepton.M()<<endl;
            cout<<"INVME B1"<<endl;
            cout<<"INVME (E,Px,Py,Pz,M) "<<tlv_firstBjet.E()<<","<<tlv_firstBjet.Px()<<","<<tlv_firstBjet.Py()<<","<<tlv_firstBjet.Pz()<<","<<tlv_firstBjet.M()<<endl;
            cout<<"INVME B2"<<endl;
            cout<<"INVME (E,Px,Py,Pz,M) "<<tlv_secondBjet.E()<<","<<tlv_secondBjet.Px()<<","<<tlv_secondBjet.Py()<<","<<tlv_secondBjet.Pz()<<","<<tlv_secondBjet.M()<<endl;
            cout<<"INVME MET"<<endl;
            cout<<"INVME (E,Px,Py,Pz,M) "<<","<<ptmiss.Px()<<","<<ptmiss.Py()<<endl;
            cout<<"INVME METCOV "<<endl;
            cout<<"INVME "<<metcov (0,0)<<"  "<<metcov (0,1)<<endl;// = theBigTree.MET_cov00->at (chosenTauPair) ;
            cout<<"INVME "<<metcov (1,0)<<"  "<<metcov (1,1)<<endl;// = theBigTree.MET_cov10->at (chosenTauPair) ;
            cout<<"INVME tau1, tau2, b1, b2"<<endl;
            cout<<"INVME ";
            tlv_firstLepton.Print();
            cout<<"INVME ";
            tlv_secondLepton.Print();
            cout<<"INVME ";
            tlv_firstBjet.Print();
            cout<<"INVME ";
            tlv_secondBjet.Print();
            wrongHHK=true;
          }
          catch (HHKinFit2::HHEnergyRangeException e)
          {
            cout<<"ERANGE THIS EVENT WAS WRONG, ENERGY RANGE EXCEPTION"<<endl;
            cout<<"ERANGE masshypo1 = 125,    masshypo2 = 125"<<endl;
            cout<<"ERANGE Tau1"<<endl;
            cout<<"ERANGE (E,Px,Py,Pz,M) "<<tlv_firstLepton.E()<<","<<tlv_firstLepton.Px()<<","<<tlv_firstLepton.Py()<<","<<tlv_firstLepton.Pz()<<","<<tlv_firstLepton.M()<<endl;
            cout<<"ERANGE Tau2"<<endl;
            cout<<"ERANGE (E,Px,Py,Pz,M) "<<tlv_secondLepton.E()<<","<<tlv_secondLepton.Px()<<","<<tlv_secondLepton.Py()<<","<<tlv_secondLepton.Pz()<<","<<tlv_secondLepton.M()<<endl;
            cout<<"ERANGE B1"<<endl;
            cout<<"ERANGE (E,Px,Py,Pz,M) "<<tlv_firstBjet.E()<<","<<tlv_firstBjet.Px()<<","<<tlv_firstBjet.Py()<<","<<tlv_firstBjet.Pz()<<","<<tlv_firstBjet.M()<<endl;
            cout<<"ERANGE B2"<<endl;
            cout<<"ERANGE (E,Px,Py,Pz,M) "<<tlv_secondBjet.E()<<","<<tlv_secondBjet.Px()<<","<<tlv_secondBjet.Py()<<","<<tlv_secondBjet.Pz()<<","<<tlv_secondBjet.M()<<endl;
            cout<<"ERANGE MET"<<endl;
            cout<<"ERANGE (E,Px,Py,Pz,M) "<<","<<ptmiss.Px()<<","<<ptmiss.Py()<<endl;
            cout<<"ERANGE METCOV "<<endl;
            cout<<"ERANGE "<<metcov (0,0)<<"  "<<metcov (0,1)<<endl;// = theBigTree.MET_cov00->at (chosenTauPair) ;
            cout<<"ERANGE "<<metcov (1,0)<<"  "<<metcov (1,1)<<endl;// = theBigTree.MET_cov10->at (chosenTauPair) ;
            cout<<"ERANGE tau1, tau2, b1, b2"<<endl;
            cout<<"ERANGE ";
            tlv_firstLepton.Print();
            cout<<"ERANGE ";
            tlv_secondLepton.Print();
            cout<<"ERANGE ";
            tlv_firstBjet.Print();
            cout<<"ERANGE ";
            tlv_secondBjet.Print();
            wrongHHK=true;
          }
          catch(HHKinFit2::HHEnergyConstraintException e)
          {
            cout<<"ECON THIS EVENT WAS WRONG, ENERGY CONSTRAIN EXCEPTION"<<endl;
            cout<<"ECON masshypo1 = 125,    masshypo2 = 125"<<endl;
            cout<<"ECON Tau1"<<endl;
            cout<<"ECON (E,Px,Py,Pz,M) "<<tlv_firstLepton.E()<<","<<tlv_firstLepton.Px()<<","<<tlv_firstLepton.Py()<<","<<tlv_firstLepton.Pz()<<","<<tlv_firstLepton.M()<<endl;
            cout<<"ECON Tau2"<<endl;
            cout<<"ECON (E,Px,Py,Pz,M) "<<tlv_secondLepton.E()<<","<<tlv_secondLepton.Px()<<","<<tlv_secondLepton.Py()<<","<<tlv_secondLepton.Pz()<<","<<tlv_secondLepton.M()<<endl;
            cout<<"ECON B1"<<endl;
            cout<<"ECON (E,Px,Py,Pz,M) "<<tlv_firstBjet.E()<<","<<tlv_firstBjet.Px()<<","<<tlv_firstBjet.Py()<<","<<tlv_firstBjet.Pz()<<","<<tlv_firstBjet.M()<<endl;
            cout<<"ECON B2"<<endl;
            cout<<"ECON (E,Px,Py,Pz,M) "<<tlv_secondBjet.E()<<","<<tlv_secondBjet.Px()<<","<<tlv_secondBjet.Py()<<","<<tlv_secondBjet.Pz()<<","<<tlv_secondBjet.M()<<endl;
            cout<<"ECON MET"<<endl;
            cout<<"ECON (E,Px,Py,Pz,M) "<<","<<ptmiss.Px()<<","<<ptmiss.Py()<<endl;
            cout<<"ECON METCOV "<<endl;
            cout<<"ECON "<<metcov (0,0)<<"  "<<metcov (0,1)<<endl;// = theBigTree.MET_cov00->at (chosenTauPair) ;
            cout<<"ECON "<<metcov (1,0)<<"  "<<metcov (1,1)<<endl;// = theBigTree.MET_cov10->at (chosenTauPair) ;
            cout<<"ECON tau1, tau2, b1, b2"<<endl;
            cout<<"ECON ";
            tlv_firstLepton.Print();
            cout<<"ECON ";
            tlv_secondLepton.Print();
            cout<<"ECON ";
            tlv_firstBjet.Print();
            cout<<"ECON ";
            tlv_secondBjet.Print();
            wrongHHK=true;
          }
          if(!wrongHHK)
          {
            HHKmass = kinFits.getMH () ;
            HHKChi2 = kinFits.getChi2 () ;
          }
          else
          {
            if(isOS)HHKmass = -333;
          }

          // nominal kinfit raw
          bool wrongHHKraw =false;
          try {kinFitsraw.fit();}
          catch(HHKinFit2::HHInvMConstraintException e){wrongHHKraw=true;}
          catch(HHKinFit2::HHEnergyConstraintException e){wrongHHKraw=true;}
          catch (HHKinFit2::HHEnergyRangeException e){wrongHHKraw=true;}
          if(!wrongHHKraw)
          {
            theSmallTree.m_HHKin_mass_raw = kinFitsraw.getMH();
            theSmallTree.m_HHKin_mass_raw_chi2        = kinFitsraw.getChi2();
            theSmallTree.m_HHKin_mass_raw_convergence = kinFitsraw.getConvergence();
            theSmallTree.m_HHKin_mass_raw_prob        = kinFitsraw.getFitProb();
          }
          else theSmallTree.m_HHKin_mass_raw = -100 ;
          if (theBigTree.SVfitMass->at (chosenTauPair) > -900. && !wrongHHK)
          {
            TLorentzVector b1 = kinFits.getFittedBJet1();
            TLorentzVector b2 = kinFits.getFittedBJet2();
            TLorentzVector bH_HKin = b1 + b2;
            TLorentzVector tlv_HHsvfit = bH_HKin + tlv_tauH_SVFIT ;

            theSmallTree.m_HHkinsvfit_bHmass = bH_HKin.M();
            theSmallTree.m_HHkinsvfit_pt  = tlv_HHsvfit.Pt () ;
            theSmallTree.m_HHkinsvfit_eta = tlv_HHsvfit.Eta () ;
            theSmallTree.m_HHkinsvfit_phi = tlv_HHsvfit.Phi () ;
            theSmallTree.m_HHkinsvfit_e   = tlv_HHsvfit.E () ;
            theSmallTree.m_HHkinsvfit_m   = tlv_HHsvfit.M () ;
          } // in case the SVFIT mass is calculated

          // raw kinfit TES up
          bool wrongHHKraw_tauup =false;
          try {kinFitsraw_tauup.fit();}
          catch(HHKinFit2::HHInvMConstraintException e){wrongHHKraw_tauup=true;}
          catch(HHKinFit2::HHEnergyConstraintException e){wrongHHKraw_tauup=true;}
          catch (HHKinFit2::HHEnergyRangeException e){wrongHHKraw_tauup=true;}
          if(!wrongHHKraw_tauup){theSmallTree.m_HHKin_mass_raw_tauup = kinFitsraw_tauup.getMH();}
          else theSmallTree.m_HHKin_mass_raw_tauup = -100 ;

          // raw kinfit TES down
          bool wrongHHKraw_taudown =false;
          try {kinFitsraw_taudown.fit();}
          catch(HHKinFit2::HHInvMConstraintException e){wrongHHKraw_taudown=true;}
          catch(HHKinFit2::HHEnergyConstraintException e){wrongHHKraw_taudown=true;}
          catch (HHKinFit2::HHEnergyRangeException e){wrongHHKraw_taudown=true;}
          if(!wrongHHKraw_taudown){theSmallTree.m_HHKin_mass_raw_taudown = kinFitsraw_taudown.getMH();}
          else theSmallTree.m_HHKin_mass_raw_taudown = -100 ;

          // raw kinfit JES up
          bool wrongHHKraw_jetup =false;
          try {kinFitsraw_jetup.fit();}
          catch(HHKinFit2::HHInvMConstraintException e){wrongHHKraw_jetup=true;}
          catch(HHKinFit2::HHEnergyConstraintException e){wrongHHKraw_jetup=true;}
          catch (HHKinFit2::HHEnergyRangeException e){wrongHHKraw_jetup=true;}
          if(!wrongHHKraw_jetup){theSmallTree.m_HHKin_mass_raw_jetup = kinFitsraw_jetup.getMH();}
          else theSmallTree.m_HHKin_mass_raw_jetup = -100 ;

          // raw kinfit JES down
          bool wrongHHKraw_jetdown =false;
          try {kinFitsraw_jetdown.fit();}
          catch(HHKinFit2::HHInvMConstraintException e){wrongHHKraw_jetdown=true;}
          catch(HHKinFit2::HHEnergyConstraintException e){wrongHHKraw_jetdown=true;}
          catch (HHKinFit2::HHEnergyRangeException e){wrongHHKraw_jetdown=true;}
          if(!wrongHHKraw_jetdown){theSmallTree.m_HHKin_mass_raw_jetdown = kinFitsraw_jetdown.getMH();}
          else theSmallTree.m_HHKin_mass_raw_jetdown = -100 ;

        } // end if doing HHKinFit

        theSmallTree.m_HHKin_mass_raw_copy = theSmallTree.m_HHKin_mass_raw ; // store twice if different binning needed

        theSmallTree.m_HHKin_mass = HHKmass;//kinFits.getMH () ;
        theSmallTree.m_HHKin_chi2 = HHKChi2;//kinFits.getChi2 () ;

        // Stransverse mass
        if (runMT2)
        {
          double mVisA = tlv_firstBjet_raw.M(); // mass of visible object on side A.  Must be >=0.
          double pxA = tlv_firstBjet_raw.Px();  // x momentum of visible object on side A.
          double pyA = tlv_firstBjet_raw.Py();  // y momentum of visible object on side A.

          double mVisB = tlv_secondBjet_raw.M(); // mass of visible object on side B.  Must be >=0.
          double pxB = tlv_secondBjet_raw.Px();  // x momentum of visible object on side B.
          double pyB = tlv_secondBjet_raw.Py();  // y momentum of visible object on side B.

          double pxMiss = tlv_firstLepton.Px() + tlv_secondLepton.Px() + theBigTree.METx->at(chosenTauPair); // x component of missing transverse momentum.
          double pyMiss = tlv_firstLepton.Py() + tlv_secondLepton.Py() + theBigTree.METy->at(chosenTauPair); // y component of missing transverse momentum.

          double chiA = tlv_firstLepton.M();  // hypothesised mass of invisible on side A.  Must be >=0.
          double chiB = tlv_secondLepton.M(); // hypothesised mass of invisible on side B.  Must be >=0.

          // TES variations
          double pxMiss_tauup = tlv_firstLepton_tauup.Px() + tlv_secondLepton_tauup.Px() + theBigTree.METx_UP_TES->at(chosenTauPair); // shiftedMET for TES
          double pyMiss_tauup = tlv_firstLepton_tauup.Py() + tlv_secondLepton_tauup.Py() + theBigTree.METy_UP_TES->at(chosenTauPair); // shiftedMET for TES
          double chiA_tauup = tlv_firstLepton_tauup.M();  // hypothesised mass of invisible on side A.  Must be >=0.
          double chiB_tauup = tlv_secondLepton_tauup.M(); // hypothesised mass of invisible on side B.  Must be >=0.

          double pxMiss_taudown = tlv_firstLepton_taudown.Px() + tlv_secondLepton_taudown.Px() + theBigTree.METx_DOWN_TES->at(chosenTauPair); // shiftedMET for TES
          double pyMiss_taudown = tlv_firstLepton_taudown.Py() + tlv_secondLepton_taudown.Py() + theBigTree.METy_DOWN_TES->at(chosenTauPair); // shiftedMET for TES
          double chiA_taudown = tlv_firstLepton_taudown.M();  // hypothesised mass of invisible on side A.  Must be >=0.
          double chiB_taudown = tlv_secondLepton_taudown.M(); // hypothesised mass of invisible on side B.  Must be >=0.

          // JES variations
          double mVisA_jetup = tlv_firstBjet_raw_jetup.M();  // mass of visible object on side A.
          double pxA_jetup = tlv_firstBjet_raw_jetup.Px();   // x momentum of visible object on side A.
          double pyA_jetup = tlv_firstBjet_raw_jetup.Py();   // y momentum of visible object on side A.
          double mVisB_jetup = tlv_secondBjet_raw_jetup.M(); // mass of visible object on side B.
          double pxB_jetup = tlv_secondBjet_raw_jetup.Px();  // x momentum of visible object on side B.
          double pyB_jetup = tlv_secondBjet_raw_jetup.Py();  // y momentum of visible object on side B.
          double pxMiss_jetup = tlv_firstLepton.Px() + tlv_secondLepton.Px() + ptmiss_jetup.Px(); // shiftedMET for JES
          double pyMiss_jetup = tlv_firstLepton.Py() + tlv_secondLepton.Py() + ptmiss_jetup.Py(); // shiftedMET for JES

          double mVisA_jetdown = tlv_firstBjet_raw_jetdown.M();  // mass of visible object on side A.
          double pxA_jetdown = tlv_firstBjet_raw_jetdown.Px();   // x momentum of visible object on side A.
          double pyA_jetdown = tlv_firstBjet_raw_jetdown.Py();   // y momentum of visible object on side A.
          double mVisB_jetdown = tlv_secondBjet_raw_jetdown.M(); // mass of visible object on side B.
          double pxB_jetdown = tlv_secondBjet_raw_jetdown.Px();  // x momentum of visible object on side B.
          double pyB_jetdown = tlv_secondBjet_raw_jetdown.Py();  // y momentum of visible object on side B.
          double pxMiss_jetdown = tlv_firstLepton.Px() + tlv_secondLepton.Px() + ptmiss_jetdown.Px(); // shiftedMET for JES
          double pyMiss_jetdown = tlv_firstLepton.Py() + tlv_secondLepton.Py() + ptmiss_jetdown.Py(); // shiftedMET for JES

          double desiredPrecisionOnMt2 = 0; // Must be >=0.  If 0 alg aims for machine precision.  if >0, MT2 computed to supplied absolute precision.

          asymm_mt2_lester_bisect::disableCopyrightMessage();
        
          double MT2 = asymm_mt2_lester_bisect::get_mT2(
                       mVisA, pxA, pyA,
                       mVisB, pxB, pyB,
                       pxMiss, pyMiss,
                       chiA, chiB,
                       desiredPrecisionOnMt2);

          double MT2_tauup = asymm_mt2_lester_bisect::get_mT2(
                             mVisA, pxA, pyA,
                             mVisB, pxB, pyB,
                             pxMiss_tauup, pyMiss_tauup,
                             chiA_tauup, chiB_tauup,
                             desiredPrecisionOnMt2);

          double MT2_taudown = asymm_mt2_lester_bisect::get_mT2(
                               mVisA, pxA, pyA,
                               mVisB, pxB, pyB,
                               pxMiss_taudown, pyMiss_taudown,
                               chiA_taudown, chiB_taudown,
                               desiredPrecisionOnMt2);

          double MT2_jetup = asymm_mt2_lester_bisect::get_mT2(
                             mVisA_jetup, pxA_jetup, pyA_jetup,
                             mVisB_jetup, pxB_jetup, pyB_jetup,
                             pxMiss_jetup, pyMiss_jetup, // shiftedMET
                             chiA, chiB,
                             desiredPrecisionOnMt2);

          double MT2_jetdown = asymm_mt2_lester_bisect::get_mT2(
                               mVisA_jetdown, pxA_jetdown, pyA_jetdown,
                               mVisB_jetdown, pxB_jetdown, pyB_jetdown,
                               pxMiss_jetdown, pyMiss_jetdown, // shiftedMET
                               chiA, chiB,
                               desiredPrecisionOnMt2);

          theSmallTree.m_MT2 = MT2;
          theSmallTree.m_MT2_tauup = MT2_tauup;
          theSmallTree.m_MT2_taudown = MT2_taudown;
          theSmallTree.m_MT2_jetup = MT2_jetup;
          theSmallTree.m_MT2_jetdown = MT2_jetdown;

        } // end calcultion of MT2

        if (DEBUG)  cout << "---------- MT2 DEBUG: " << theSmallTree.m_MT2 << endl;

        theSmallTree.m_HH_deltaPhi = deltaPhi (tlv_bH.Phi (), tlv_tauH.Phi ()) ;
        theSmallTree.m_HH_deltaEta = fabs(tlv_bH.Eta()- tlv_tauH.Eta ()) ;
        theSmallTree.m_HHsvfit_deltaPhi = deltaPhi (tlv_bH.Phi (), tlv_tauH_SVFIT.Phi ()) ;
        theSmallTree.m_bHMet_deltaPhi = deltaPhi (theBigTree.metphi, tlv_bH.Phi ()) ;
        theSmallTree.m_dib_deltaPhi = deltaPhi (tlv_firstBjet.Phi (), tlv_secondBjet.Phi ()) ;
        theSmallTree.m_dib_deltaEta = fabs(tlv_firstBjet.Eta()-tlv_secondBjet.Eta ()) ;
        theSmallTree.m_dib_deltaR = tlv_firstBjet.DeltaR(tlv_secondBjet) ;
        theSmallTree.m_dib_deltaR_per_bHpt = theSmallTree.m_dib_deltaR * tlv_bH_raw.Pt();

        theSmallTree.m_BDT_dib_deltaPhi     = ROOT::Math::VectorUtil::DeltaPhi(tlv_firstBjet, tlv_secondBjet);
        theSmallTree.m_BDT_dib_abs_deltaPhi = fabs(ROOT::Math::VectorUtil::DeltaPhi(tlv_firstBjet, tlv_secondBjet));

        vector <float> dRBTau;
        dRBTau.push_back (tlv_firstLepton.DeltaR(tlv_firstBjet));
        dRBTau.push_back (tlv_firstLepton.DeltaR(tlv_secondBjet));
        dRBTau.push_back (tlv_secondLepton.DeltaR(tlv_firstBjet));
        dRBTau.push_back (tlv_secondLepton.DeltaR(tlv_secondBjet));
        theSmallTree.m_btau_deltaRmin = *std::min_element(dRBTau.begin(), dRBTau.end());
        theSmallTree.m_btau_deltaRmax = *std::max_element(dRBTau.begin(), dRBTau.end());

        // Save other VBF related quantities
        if ( theBigTree.jets_px->size()> 1 && VBFcand_Mjj.size()>0 )
        {
          // save the VBF-jets
          TLorentzVector VBFjet1;
          VBFjet1.SetPxPyPzE( theBigTree.jets_px->at(VBFidx1), theBigTree.jets_py->at(VBFidx1), theBigTree.jets_pz->at(VBFidx1), theBigTree.jets_e->at(VBFidx1) );
          TLorentzVector VBFjet2;
          VBFjet2.SetPxPyPzE( theBigTree.jets_px->at(VBFidx2), theBigTree.jets_py->at(VBFidx2), theBigTree.jets_pz->at(VBFidx2), theBigTree.jets_e->at(VBFidx2) );

          bool hasgj1_VBF = false;
          bool hasgj2_VBF = false;

          // Save gen info for VBF jets
          if (isMC)
          {
            int mcind = theBigTree.jets_genjetIndex->at(VBFidx1);
            if (mcind>=0)
            {
              TLorentzVector thisGenJet(theBigTree.genjet_px->at(mcind),theBigTree.genjet_py->at(mcind),theBigTree.genjet_pz->at(mcind),theBigTree.genjet_e->at(mcind));
              if (thisGenJet.Pt() > 8)
              {
                hasgj1_VBF = true;
                theSmallTree.m_VBFgenjet1_pt =  thisGenJet.Pt() ;
                theSmallTree.m_VBFgenjet1_eta = thisGenJet.Eta();
                theSmallTree.m_VBFgenjet1_phi = thisGenJet.Phi();
                theSmallTree.m_VBFgenjet1_e =   thisGenJet.E();
              }
            }
            mcind = theBigTree.jets_genjetIndex->at(VBFidx2);
            if (mcind>=0)
            {
              TLorentzVector thisGenJet(theBigTree.genjet_px->at(mcind),theBigTree.genjet_py->at(mcind),theBigTree.genjet_pz->at(mcind),theBigTree.genjet_e->at(mcind));
              if (thisGenJet.Pt() > 8)
              {
                hasgj2_VBF = true;
                theSmallTree.m_VBFgenjet2_pt =  thisGenJet.Pt() ;
                theSmallTree.m_VBFgenjet2_eta = thisGenJet.Eta();
                theSmallTree.m_VBFgenjet2_phi = thisGenJet.Phi();
                theSmallTree.m_VBFgenjet2_e =   thisGenJet.E();
              }
            }
          }

          // Save VBF variables
          theSmallTree.m_VBFjj_mass        = std::get<0>(*(VBFcand_Mjj.rbegin()));
          theSmallTree.m_VBFjj_mass_log    = log(std::get<0>(*(VBFcand_Mjj.rbegin())));
          theSmallTree.m_VBFjj_deltaEta    = fabs(VBFjet1.Eta()-VBFjet2.Eta());
          theSmallTree.m_VBFjet1_pt        = VBFjet1.Pt() ;
          theSmallTree.m_VBFjet1_eta       = VBFjet1.Eta();
          theSmallTree.m_VBFjet1_phi       = VBFjet1.Phi();
          theSmallTree.m_VBFjet1_e         = VBFjet1.E();
          theSmallTree.m_VBFjet1_btag      = (theBigTree.bCSVscore->at (VBFidx1)) ;
          theSmallTree.m_VBFjet1_flav      = (theBigTree.jets_HadronFlavour->at (VBFidx1)) ;
          theSmallTree.m_VBFjet1_hasgenjet = hasgj1_VBF ;
          theSmallTree.m_VBFjet2_pt        = VBFjet2.Pt() ;
          theSmallTree.m_VBFjet2_eta       = VBFjet2.Eta();
          theSmallTree.m_VBFjet2_phi       = VBFjet2.Phi();
          theSmallTree.m_VBFjet2_e         = VBFjet2.E();
          theSmallTree.m_VBFjet2_btag      = (theBigTree.bCSVscore->at (VBFidx2)) ;
          theSmallTree.m_VBFjet2_flav      = (theBigTree.jets_HadronFlavour->at (VBFidx2)) ;
          theSmallTree.m_VBFjet2_hasgenjet = hasgj2_VBF ;
          theSmallTree.m_VBFjj_HT          = VBFjet1.Pt()+VBFjet2.Pt();

          theSmallTree.m_dau1_z = getZ(tlv_firstLepton.Eta(),VBFjet1.Eta(),VBFjet2.Eta());
          theSmallTree.m_dau2_z = getZ(tlv_secondLepton.Eta(),VBFjet1.Eta(),VBFjet2.Eta());
          theSmallTree.m_bjet1_z = getZ(tlv_firstBjet.Eta(),VBFjet1.Eta(),VBFjet2.Eta());
          theSmallTree.m_bjet2_z = getZ(tlv_secondBjet.Eta(),VBFjet1.Eta(),VBFjet2.Eta());
          theSmallTree.m_tauH_z = getZ(tlv_tauH.Eta(),VBFjet1.Eta(),VBFjet2.Eta());
          theSmallTree.m_bH_z = getZ(tlv_bH.Eta(),VBFjet1.Eta(),VBFjet2.Eta());
          theSmallTree.m_HH_z = getZ(tlv_HH.Eta(),VBFjet1.Eta(),VBFjet2.Eta());

          //top mass calculation
          TLorentzVector VBFcentral = VBFjet1;
          TLorentzVector VBFforward = VBFjet2;

          if (fabs(VBFjet1.Eta())>fabs(VBFjet2.Eta())) std::swap(VBFcentral,VBFforward);
          TLorentzVector fakeTau = tlv_secondLepton;
          if (pairType == 2 && theSmallTree.m_dau1_iso > theSmallTree.m_dau2_iso)
          {
            fakeTau = tlv_firstLepton;
          }

          TLorentzVector Wc = VBFcentral + fakeTau;
          TLorentzVector Wf = VBFforward + fakeTau;
          TLorentzVector bclose = tlv_firstBjet;
          if(tlv_secondBjet.DeltaR(Wc)<tlv_firstBjet.DeltaR(Wc)) bclose = tlv_secondBjet;
          TLorentzVector top_Wc_bclose = Wc + bclose;
          bclose = tlv_firstBjet;
          if(tlv_secondBjet.DeltaR(Wf)<tlv_firstBjet.DeltaR(Wf)) bclose = tlv_secondBjet;
          TLorentzVector top_Wf_bclose = Wf + bclose;
          TLorentzVector bcentral = tlv_firstBjet;
          TLorentzVector bforward = tlv_secondBjet;
          if (fabs(tlv_firstBjet.Eta())>fabs(tlv_secondBjet.Eta())) std::swap(bcentral,bforward);
          TLorentzVector top_Wc_bcentral = Wc + bcentral;
          TLorentzVector top_Wf_bcentral = Wf + bcentral;
          TLorentzVector top_Wc_bforward = Wc + bforward;
          TLorentzVector top_Wf_bforward = Wf + bforward;

          float Wmass = 80.4;
          TLorentzVector W1 = VBFjet1 + fakeTau;
          TLorentzVector W2 = VBFjet2 + fakeTau;

          theSmallTree.m_top_Wc_bclose_mass = top_Wc_bclose.M();
          theSmallTree.m_top_Wc_bcentral_mass = top_Wc_bcentral.M();
          theSmallTree.m_top_Wc_bforward_mass = top_Wc_bforward.M();
          theSmallTree.m_top_Wf_bclose_mass = top_Wf_bclose.M();
          theSmallTree.m_top_Wf_bcentral_mass = top_Wf_bcentral.M();
          theSmallTree.m_top_Wf_bforward_mass = top_Wf_bforward.M();

          if (fabs(Wmass-W1.M()) > fabs(Wmass-W2.M())) std::swap(W1,W2);
          bclose = tlv_firstBjet;
          if (tlv_firstBjet.DeltaR(W1) > tlv_secondBjet.DeltaR(W1)) bclose = tlv_secondBjet;
          TLorentzVector top_Wmass_bclose = W1+ bclose;
          theSmallTree.m_top_Wmass_bclose_mass = top_Wmass_bclose.M();

          //boson centrality
          float DeltaEta_minus = std::min(tlv_tauH.Eta(), tlv_bH.Eta())- std::min(VBFjet1.Eta(), VBFjet2.Eta());
          float DeltaEta_plus =std::max(VBFjet1.Eta(), VBFjet2.Eta()) - std::max(tlv_tauH.Eta(), tlv_bH.Eta());
          float zV = std::min(DeltaEta_minus, DeltaEta_plus);
          theSmallTree.m_HH_zV = zV;

          //pT balance
          TVector3 v_tauH = tlv_tauH.Vect();
          TVector3 v_bH = tlv_bH.Vect();
          float HH_A = (v_tauH + v_bH).Mag()/(tlv_tauH.Pt() + tlv_bH.Pt());
          theSmallTree.m_HH_A = HH_A;
        }

        // loop over jets
        int genjets = 0;
        int jets  = 0;
        for (unsigned int iJet = 0; (iJet < theBigTree.jets_px->size ()) && (theSmallTree.m_njets < maxNjetsSaved); ++iJet)
        {
          // PG filter jets at will
          if (theBigTree.jets_PUJetID->at (iJet) < PUjetID_minCut) continue ;
          if (theBigTree.PFjetID->at (iJet) < PFjetID_WP) continue; // 0 ; don't pass PF Jet ID; 1: loose, 2: tight, 3: tightLepVeto
      
          // skip the H decay candiates
          if (int (iJet) == bjet1idx ){
            theSmallTree.m_bjet1_jecUnc = theBigTree.jets_jecUnc->at(iJet);
            continue;
          }else if(int (iJet) == bjet2idx){
            theSmallTree.m_bjet2_jecUnc = theBigTree.jets_jecUnc->at(iJet);
            continue ;
          }
          TLorentzVector tlv_dummyJet(
                                      theBigTree.jets_px->at (iJet),
                                      theBigTree.jets_py->at (iJet),
                                      theBigTree.jets_pz->at (iJet),
                                      theBigTree.jets_e->at (iJet)
                                     );

          // remove jets that overlap with the tau selected in the leg 1 and 2
          if (tlv_firstLepton.DeltaR(tlv_dummyJet) < lepCleaningCone){
            theSmallTree.m_dau1_jecUnc = theBigTree.jets_jecUnc->at(iJet);
            continue;
          }
          if (tlv_secondLepton.DeltaR(tlv_dummyJet) < lepCleaningCone){
            theSmallTree.m_dau2_jecUnc = theBigTree.jets_jecUnc->at(iJet);
            continue;
          }

          // find matching gen jet
          bool hasgj = false;
          if (isMC)
          {
            int mcind = theBigTree.jets_genjetIndex->at(iJet);
            if (mcind>=0)
            {
              TLorentzVector thisGenJet(theBigTree.genjet_px->at(mcind),theBigTree.genjet_py->at(mcind),theBigTree.genjet_pz->at(mcind),theBigTree.genjet_e->at(mcind));
              if (thisGenJet.Pt() > 8)
              {
                hasgj = true;
                if(genjets == 0)
                {
                  theSmallTree.m_genjet3_pt =  thisGenJet.Pt() ;
                  theSmallTree.m_genjet3_eta = thisGenJet.Eta();
                  theSmallTree.m_genjet3_phi = thisGenJet.Phi();
                  theSmallTree.m_genjet3_e =   thisGenJet.E();
                }else if(genjets == 1){
                  theSmallTree.m_genjet4_pt =  thisGenJet.Pt() ;
                  theSmallTree.m_genjet4_eta = thisGenJet.Eta();
                  theSmallTree.m_genjet4_phi = thisGenJet.Phi();
                  theSmallTree.m_genjet4_e =   thisGenJet.E();
                }
                genjets ++;
              }
            }
          }

          //if VBF, skip VBF jets candidates and save 5th jet
          if (VBFcand_Mjj.size()>0)
          {
            if(int (iJet) != VBFidx1 && int (iJet) != VBFidx2 and jets == 0)
            {
              theSmallTree.m_jet5_VBF_pt   = tlv_dummyJet.Pt() ;
              theSmallTree.m_jet5_VBF_eta  = tlv_dummyJet.Eta();
              theSmallTree.m_jet5_VBF_phi  = tlv_dummyJet.Phi();
              theSmallTree.m_jet5_VBF_e    = tlv_dummyJet.E();
              theSmallTree.m_jet5_VBF_btag = (theBigTree.bCSVscore->at (iJet)) ;
              theSmallTree.m_jet5_VBF_btag_deepCSV = (theBigTree.bDeepCSV_probb->at(iJet) + theBigTree.bDeepCSV_probbb->at(iJet)) ;
              theSmallTree.m_jet5_VBF_flav = (theBigTree.jets_HadronFlavour->at (iJet)) ;
              theSmallTree.m_jet5_VBF_hasgenjet = hasgj ;
              theSmallTree.m_jet5_VBF_z = getZ(tlv_dummyJet.Eta(),theSmallTree.m_VBFjet1_eta,theSmallTree.m_VBFjet2_eta);
              if (hasgj)
              {
                int mcind = theBigTree.jets_genjetIndex->at(iJet);
                TLorentzVector thisGenJet(theBigTree.genjet_px->at(mcind),theBigTree.genjet_py->at(mcind),theBigTree.genjet_pz->at(mcind),theBigTree.genjet_e->at(mcind));
                theSmallTree.m_genjet5_VBF_pt =  thisGenJet.Pt() ;
                theSmallTree.m_genjet5_VBF_eta = thisGenJet.Eta();
                theSmallTree.m_genjet5_VBF_phi = thisGenJet.Phi();
                theSmallTree.m_genjet5_VBF_e =   thisGenJet.E();
              }
		      jets++;
            }
            if(int (iJet) != VBFidx1 && int (iJet) != VBFidx2) ++ theSmallTree.m_addjets;
	      }

          theSmallTree.m_jets_pt.push_back (tlv_dummyJet.Pt ()) ;
          theSmallTree.m_jets_eta.push_back (tlv_dummyJet.Eta ()) ;
          theSmallTree.m_jets_phi.push_back (tlv_dummyJet.Phi ()) ;
          theSmallTree.m_jets_e.push_back (theBigTree.jets_e->at (iJet)) ;
          theSmallTree.m_jets_btag.push_back (theBigTree.bCSVscore->at (iJet)) ;
          theSmallTree.m_jets_btag_deepCSV.push_back (theBigTree.bDeepCSV_probb->at(iJet) + theBigTree.bDeepCSV_probbb->at(iJet)) ;
          theSmallTree.m_jets_flav.push_back (theBigTree.jets_HadronFlavour->at (iJet)) ;
          theSmallTree.m_jets_jecUnc.push_back (theBigTree.jets_jecUnc->at (iJet)) ;
          theSmallTree.m_jets_hasgenjet.push_back (hasgj) ;
          ++theSmallTree.m_njets ;
        } // loop over jets

        if (theSmallTree.m_jets_pt.size()>0)
        {
          theSmallTree.m_jet3_pt =theSmallTree.m_jets_pt.at(0);
          theSmallTree.m_jet3_eta =theSmallTree.m_jets_eta.at(0);
          theSmallTree.m_jet3_phi =theSmallTree.m_jets_phi.at(0);
          theSmallTree.m_jet3_e =theSmallTree.m_jets_e.at(0);
          theSmallTree.m_jet3_btag= theSmallTree.m_jets_btag.at (0);
          theSmallTree.m_jet3_btag_deepCSV= theSmallTree.m_jets_btag_deepCSV.at (0);
          theSmallTree.m_jet3_flav= theSmallTree.m_jets_flav.at (0);
          theSmallTree.m_jet3_hasgenjet= theSmallTree.m_jets_hasgenjet.at (0);
        }
        if (theSmallTree.m_jets_pt.size()>1)
        {
          theSmallTree.m_jet4_pt =theSmallTree.m_jets_pt.at(1);
          theSmallTree.m_jet4_eta =theSmallTree.m_jets_eta.at(1);
          theSmallTree.m_jet4_phi =theSmallTree.m_jets_phi.at(1);
          theSmallTree.m_jet4_e =theSmallTree.m_jets_e.at(1);
          theSmallTree.m_jet4_btag= theSmallTree.m_jets_btag.at (1);
          theSmallTree.m_jet4_btag_deepCSV= theSmallTree.m_jets_btag_deepCSV.at (1);
          theSmallTree.m_jet4_flav= theSmallTree.m_jets_flav.at (1);
          theSmallTree.m_jet4_hasgenjet= theSmallTree.m_jets_hasgenjet.at (1);
	  
          //invariant mass of first 2 additional jets (skipping H decay) ordered by Pt
          TLorentzVector tlv_jet1;
          TLorentzVector tlv_jet2;
          tlv_jet1.SetPtEtaPhiE(
                  theSmallTree.m_jets_pt.at(0),
                  theSmallTree.m_jets_eta.at(0),
                  theSmallTree.m_jets_phi.at(0),
                  theSmallTree.m_jets_e.at(0)
                  );
          tlv_jet2.SetPtEtaPhiE(
                  theSmallTree.m_jets_pt.at(1),
                  theSmallTree.m_jets_eta.at(1),
                  theSmallTree.m_jets_phi.at(1),
                  theSmallTree.m_jets_e.at(1)
                  );
          TLorentzVector tlv_jetPair = tlv_jet1 + tlv_jet2;

          theSmallTree.m_jj_mass = tlv_jetPair.M();
          theSmallTree.m_jj_mass_log = log(tlv_jetPair.M());
          theSmallTree.m_jj_deltaEta = fabs(tlv_jet1.Eta() - tlv_jet2.Eta());
          theSmallTree.m_jj_HT = tlv_jet1.Pt()+tlv_jet2.Pt();

          TLorentzVector b = tlv_firstBjet;
          if(tlv_secondBjet.Pt() > tlv_firstBjet.Pt()) b = tlv_secondBjet;
          TLorentzVector top_Wjj_b = b + tlv_jetPair; //2 highest pt additional jets + highest pt bjet
          theSmallTree.m_top_Wjj_b_mass = top_Wjj_b.M();

          TLorentzVector bclose = tlv_firstBjet;
          if(tlv_secondBjet.DeltaR(tlv_jetPair) < tlv_firstBjet.DeltaR(tlv_jetPair)) bclose =tlv_secondBjet;
          TLorentzVector top_Wjj_bclose = b + tlv_jetPair; //2 highest pt additional jets + closest bjet
          theSmallTree.m_top_Wjj_bclose_mass = top_Wjj_bclose.M();
        }
        if (theSmallTree.m_jets_pt.size()>2)
        {
          theSmallTree.m_jet5_pt =theSmallTree.m_jets_pt.at(2);
          theSmallTree.m_jet5_eta =theSmallTree.m_jets_eta.at(2);
          theSmallTree.m_jet5_phi =theSmallTree.m_jets_phi.at(2);
          theSmallTree.m_jet5_e =theSmallTree.m_jets_e.at(2);
          theSmallTree.m_jet5_btag= theSmallTree.m_jets_btag.at (2);
          theSmallTree.m_jet5_btag_deepCSV= theSmallTree.m_jets_btag_deepCSV.at (2);
          theSmallTree.m_jet5_flav= theSmallTree.m_jets_flav.at (2);
          theSmallTree.m_jet5_hasgenjet= theSmallTree.m_jets_hasgenjet.at (2);
        }

      if (DEBUG)
      {
        cout << "--- VBF jets ---" << endl;
        cout << "isVBF: " << theSmallTree.m_isVBF << endl;
        cout << "VBF1(pt,eta,phi): " << theSmallTree.m_VBFjet1_pt << " / " << theSmallTree.m_VBFjet1_eta << " / " << theSmallTree.m_VBFjet1_phi << endl;
        cout << "VBF2(pt,eta,phi): " << theSmallTree.m_VBFjet2_pt << " / " << theSmallTree.m_VBFjet2_eta << " / " << theSmallTree.m_VBFjet2_phi << endl;
        cout << "----------------" << endl;
      }

      // Boosted section
	  theSmallTree.m_isBoosted = 0;
	  if (theBigTree.ak8jets_px->size() > 0)
	    {
	      // int idxSub1 = -1;
	      // int idxSub2 = -1;
	      // int idxFatj = -1;

	      vector<pair<float, int>> fatjets_bTag;
	      for (unsigned int ifj = 0; ifj < theBigTree.ak8jets_px->size(); ++ifj)
		{
		  TLorentzVector tlv_fj (theBigTree.ak8jets_px->at(ifj) , theBigTree.ak8jets_py->at(ifj) , theBigTree.ak8jets_pz->at(ifj) , theBigTree.ak8jets_e->at(ifj));
		  if (theBigTree.ak8jets_SoftDropMass -> at(ifj) < 30) continue;
		  if ( theBigTree.ak8jets_nsubjets->at(ifj) < 2 ) continue;
              
		  TLorentzVector tlv_subj1;
		  TLorentzVector tlv_subj2;
		  vector<int> sjIdxs = findSubjetIdxs(ifj, theBigTree);

		  int nSJ = 0;
		  for (int isj : sjIdxs)
		    {
		      ++nSJ;
		      if (nSJ > 2) break; // FIXME: storing first two <--> highest pt, order subjets for b tag?
		      if (nSJ == 1)
			{
			  tlv_subj1.SetPxPyPzE (theBigTree.subjets_px->at(isj), theBigTree.subjets_py->at(isj), theBigTree.subjets_pz->at(isj), theBigTree.subjets_e->at(isj));
			}
		      if (nSJ == 2)
			{
			  tlv_subj2.SetPxPyPzE (theBigTree.subjets_px->at(isj), theBigTree.subjets_py->at(isj), theBigTree.subjets_pz->at(isj), theBigTree.subjets_e->at(isj));
			}

		      if(DEBUG)
			{
			  cout << "- nSJ=" << nSJ << " px=" << theBigTree.subjets_px->at(isj) << endl;
			}

		    }

		  bool A1B2 = (tlv_subj1.DeltaR(tlv_firstBjet) < 0.4)   && (tlv_subj2.DeltaR(tlv_secondBjet) < 0.4 );
		  bool A2B1 = (tlv_subj1.DeltaR(tlv_secondBjet) < 0.4)  && (tlv_subj2.DeltaR(tlv_firstBjet) < 0.4 );

		  if(DEBUG)
		    {
		      cout << " fatjet: idx " << ifj << " nsj=" << sjIdxs.size() 
			   << " sj1pt=" << tlv_subj1.Pt() << " sj1eta=" << tlv_subj1.Eta() << " sj1phi=" << tlv_subj1.Phi()
			   << " sj2pt=" << tlv_subj2.Pt() << " sj2eta=" << tlv_subj2.Eta() << " sj2phi=" << tlv_subj2.Phi()
			   << " !passMatch=" << (!A1B2 && !A2B1) << endl;
		    }

		  if (!A1B2 && !A2B1) continue; // is not matched to resolved jets

		  //fatjets_bTag.push_back(make_pair(theBigTree.ak8jets_CSV->size(), ifj));
		  fatjets_bTag.push_back(make_pair(theBigTree.ak8jets_deepCSV_probb->at(ifj)+theBigTree.ak8jets_deepCSV_probbb->at(ifj), ifj));
		}

	      if(DEBUG)
		{
		  cout << " N selected fatjets : " << fatjets_bTag.size() << endl;
		}

	      if (fatjets_bTag.size() != 0) 
		{
		  theSmallTree.m_isBoosted = 1;
		  sort (fatjets_bTag.begin(), fatjets_bTag.end());
		  int fjIdx = fatjets_bTag.back().second;
		  TLorentzVector tlv_fj (theBigTree.ak8jets_px->at(fjIdx) , theBigTree.ak8jets_py->at(fjIdx) , theBigTree.ak8jets_pz->at(fjIdx) , theBigTree.ak8jets_e->at(fjIdx));
		  theSmallTree.m_fatjet_pt   = tlv_fj.Pt();
		  theSmallTree.m_fatjet_eta  = tlv_fj.Eta();
		  theSmallTree.m_fatjet_phi  = tlv_fj.Phi();
		  theSmallTree.m_fatjet_e    = tlv_fj.E();
		  theSmallTree.m_fatjet_bID  = theBigTree.ak8jets_CSV->at(fjIdx);
		  theSmallTree.m_fatjet_bID_deepCSV  = theBigTree.ak8jets_deepCSV_probb->at(fjIdx) + theBigTree.ak8jets_deepCSV_probbb->at(fjIdx);
		  theSmallTree.m_fatjet_filteredMass = theBigTree.ak8jets_FilteredMass -> at(fjIdx) ;
		  theSmallTree.m_fatjet_prunedMass   = theBigTree.ak8jets_PrunedMass   -> at(fjIdx) ;
		  theSmallTree.m_fatjet_trimmedMass  = theBigTree.ak8jets_TrimmedMass  -> at(fjIdx) ;
		  theSmallTree.m_fatjet_softdropMass = theBigTree.ak8jets_SoftDropMass -> at(fjIdx) ;
		  theSmallTree.m_fatjet_tau1 = theBigTree.ak8jets_tau1->at(fjIdx);
		  theSmallTree.m_fatjet_tau2 = theBigTree.ak8jets_tau2->at(fjIdx);
		  theSmallTree.m_fatjet_tau3 = theBigTree.ak8jets_tau3->at(fjIdx);
		  theSmallTree.m_fatjet_nsubjets = theBigTree.ak8jets_nsubjets->at(fjIdx);
		  // FIXME: redoing this a second time, can be optimized
		  if ( theBigTree.ak8jets_nsubjets->at(fjIdx) < 2) cout << "ERROR: there are not 2 subjets. Should not happen!!" << endl;
		  TLorentzVector tlv_subj1;
		  TLorentzVector tlv_subj2;
		  vector<int> sjIdxs = findSubjetIdxs(fjIdx, theBigTree);
		  int nSJ = 0;
		  for (int isj : sjIdxs)
		    {
		      ++nSJ;
		      if (nSJ > 2) break; // FIXME: storing first two <--> highest pt, order subjets for b tag?
		      if (nSJ == 1)
			{
			  tlv_subj1.SetPxPyPzE (theBigTree.subjets_px->at(isj), theBigTree.subjets_py->at(isj), theBigTree.subjets_pz->at(isj), theBigTree.subjets_e->at(isj));
			  theSmallTree.m_subjetjet1_pt   = tlv_subj1.Pt();
			  theSmallTree.m_subjetjet1_eta  = tlv_subj1.Eta();
			  theSmallTree.m_subjetjet1_phi  = tlv_subj1.Phi();
			  theSmallTree.m_subjetjet1_e    = tlv_subj1.E();
			  theSmallTree.m_subjetjet1_bID  = theBigTree.subjets_CSV->at(isj) ;
			  theSmallTree.m_subjetjet1_bID_deepCSV  = theBigTree.subjets_deepCSV_probb->at(isj) + theBigTree.subjets_deepCSV_probbb->at(isj) ;
			}
		      if (nSJ == 2)
			{
			  tlv_subj2.SetPxPyPzE (theBigTree.subjets_px->at(isj), theBigTree.subjets_py->at(isj), theBigTree.subjets_pz->at(isj), theBigTree.subjets_e->at(isj));
			  theSmallTree.m_subjetjet2_pt   = tlv_subj2.Pt();
			  theSmallTree.m_subjetjet2_eta  = tlv_subj2.Eta();
			  theSmallTree.m_subjetjet2_phi  = tlv_subj2.Phi();
			  theSmallTree.m_subjetjet2_e    = tlv_subj2.E();
			  theSmallTree.m_subjetjet2_bID  = theBigTree.subjets_CSV->at(isj) ;
			  theSmallTree.m_subjetjet2_bID_deepCSV  = theBigTree.subjets_deepCSV_probb->at(isj) + theBigTree.subjets_deepCSV_probbb->at(isj) ;
			}
		      theSmallTree.m_dR_subj1_subj2 = tlv_subj1.DeltaR(tlv_subj2);
		    } 
		}
	    }
	}// if there's two jets in the event, at least

      if (isMC) selectedEvents += theBigTree.aMCatNLOweight ;  //FIXME: probably wrong, but unused up to now
      else selectedEvents += 1 ;
      ++selectedNoWeightsEventsNum ;

      theSmallTree.Fill () ;
    }

  cout << "1: " << totalEvents << endl ;
  cout << "2: " << selectedEvents << endl ;
  cout << "3: " << totalNoWeightsEventsNum << endl ;
  cout << "4: " << selectedNoWeightsEventsNum << endl ;

  if (totalEvents != 0) cout << "efficiency = " << selectedEvents / totalEvents << endl ;
  else                  cout << "NO events found\n" ;
  TH1F h_eff ("h_eff", "h_eff", 4 , 0, 4) ;
  h_eff.SetBinContent (1, totalEvents) ;
  h_eff.SetBinContent (2, selectedEvents) ;
  h_eff.SetBinContent (3, totalNoWeightsEventsNum) ;
  h_eff.SetBinContent (4, selectedNoWeightsEventsNum) ;
  
  // store more detailed eff counter in output
  vector<pair<string, double> > vEffSumm = ec.GetSummary();
  TH1F* h_effSummary = new TH1F ("h_effSummary", "h_effSummary", vEffSumm.size(), 0, vEffSumm.size());
  for (uint isumm = 0; isumm < vEffSumm.size(); ++isumm)
    {
      h_effSummary->SetBinContent(isumm+1, vEffSumm.at(isumm).second);
      h_effSummary->GetXaxis()->SetBinLabel(isumm+1, vEffSumm.at(isumm).first.c_str());
    }

  TH1F* hEffHHSigsSummary [6];
  if (isHHsignal)
    {
      std::vector<string> vNames = {
	"MuTau",
	"ETau",
	"TauTau",
	"MuMu",
	"EE",
	"EMu"
      };
    
    
      for (uint ich = 0; ich < 6; ++ich)
	{
	  string hname = string("h_effSummary_") + vNames.at(ich);
	  vector<pair<string, double> > vEffSummHH = ecHHsig[ich].GetSummary();
	  hEffHHSigsSummary[ich] = new TH1F (hname.c_str(), hname.c_str(), vEffSummHH.size(), 0, vEffSummHH.size());
	  for (uint isumm = 0; isumm < vEffSummHH.size(); ++isumm)
	    {
	      hEffHHSigsSummary[ich]->SetBinContent(isumm+1, vEffSummHH.at(isumm).second);
	      hEffHHSigsSummary[ich]->GetXaxis()->SetBinLabel(isumm+1, vEffSummHH.at(isumm).first.c_str());
	    }
	}

    }

  // for (unsigned int i = 0 ; i < counter.size () ; ++i)
  //   h_eff.SetBinContent (5 + i, counter.at (i)) ;

  smallFile->cd() ;
  h_eff.Write () ;
  h_effSummary->Write() ;
  if (isHHsignal)
    {
      for (uint ich = 0; ich < 6; ++ich)
	hEffHHSigsSummary[ich]->Write();
    }

  smallFile->Write () ;
  smallFile->Close () ;

  // free memory used by histos for eff
  delete h_effSummary;
  
  if (isHHsignal)
    {
      for (uint ich = 0; ich < 6; ++ich)
        delete hEffHHSigsSummary[ich];
    }



  bool computeMVA    = (gConfigParser->isDefined("TMVA::computeMVA")        ? gConfigParser->readBoolOption ("TMVA::computeMVA")        : false);
  bool computeMVARes = (gConfigParser->isDefined("BDTResonant::computeMVA") ? gConfigParser->readBoolOption ("BDTResonant::computeMVA") : false);
  bool computeMVAResHM = (gConfigParser->isDefined("BDTResonantHM::computeMVA") ? gConfigParser->readBoolOption ("BDTResonantHM::computeMVA") : false);
  bool computeMVAResLM = (gConfigParser->isDefined("BDTResonantLM::computeMVA") ? gConfigParser->readBoolOption ("BDTResonantLM::computeMVA") : false);
  bool computeMVANonRes = (gConfigParser->isDefined("BDTNonResonant::computeMVA") ? gConfigParser->readBoolOption ("BDTNonResonant::computeMVA") : false);

  if (computeMVA || computeMVARes || computeMVAResHM || computeMVAResLM)
    {  
      bool doMuTau  = gConfigParser->isDefined("TMVA::weightsMuTau");
      bool doETau   = gConfigParser->isDefined("TMVA::weightsETau");
      bool doTauTau = gConfigParser->isDefined("TMVA::weightsTauTau");
      bool doLepTau = gConfigParser->isDefined("TMVA::weightsLepTau");
      bool doResonant = computeMVARes;
      bool doResonantHM = computeMVAResHM;
      bool doResonantLM = computeMVAResLM;
      bool doNonResonant = computeMVANonRes;

      string TMVAweightsTauTau   = "";
      string TMVAweightsMuTau    = "";
      string TMVAweightsETau     = "";
      string TMVAweightsLepTau   = "";
      string TMVAweightsResonant = "";
      string TMVAweightsResonantHM = "";
      string TMVAweightsResonantLM = "";
      string TMVAweightsNonResonant = "";
    
      if (doMuTau)    TMVAweightsMuTau  = gConfigParser->readStringOption ("TMVA::weightsMuTau");
      if (doETau)     TMVAweightsETau   = gConfigParser->readStringOption ("TMVA::weightsETau");
      if (doTauTau)   TMVAweightsTauTau = gConfigParser->readStringOption ("TMVA::weightsTauTau");
      if (doLepTau)   TMVAweightsLepTau = gConfigParser->readStringOption ("TMVA::weightsLepTau");
      if (doResonant) TMVAweightsResonant = gConfigParser->readStringOption ("BDTResonant::weights");
      if (doResonantHM) TMVAweightsResonantHM = gConfigParser->readStringOption ("BDTResonantHM::weights");
      if (doResonantLM) TMVAweightsResonantLM = gConfigParser->readStringOption ("BDTResonantLM::weights");
      if (doNonResonant) TMVAweightsNonResonant = gConfigParser->readStringOption ("BDTNonResonant::weights");

      // bool TMVAspectatorsIn      = gConfigParser->readBoolOption   ("TMVA::spectatorsPresent");
      vector<string> TMVAspectators = ( computeMVA ? gConfigParser->readStringListOption   ("TMVA::spectators") : vector<string>(0) );
      vector<string> TMVAvariables  = ( computeMVA ? gConfigParser->readStringListOption   ("TMVA::variables") : vector<string>(0) );
      vector<string> TMVAvariablesResonant   = ( doResonant ? gConfigParser->readStringListOption   ("BDTResonant::variables") : vector<string>(0) );
      vector<string> TMVAvariablesResonantHM = ( doResonantHM ? gConfigParser->readStringListOption   ("BDTResonantHM::variables") : vector<string>(0) );
      vector<string> TMVAvariablesResonantLM = ( doResonantLM ? gConfigParser->readStringListOption   ("BDTResonantLM::variables") : vector<string>(0) );
      vector<string> TMVAvariablesNonResonant = ( doNonResonant ? gConfigParser->readStringListOption   ("BDTNonResonant::variables") : vector<string>(0) );

      // split the resonant name in two strings
      vector<pair<string, string>> splitTMVAvariablesResonant;
      for (unsigned int iv = 0 ; iv < TMVAvariablesResonant.size () ; ++iv)
	{
	  // split my_name:BDT_name in two strings
	  std::stringstream packedName(TMVAvariablesResonant.at(iv));
	  std::string segment;
	  std::vector<std::string> unpackedNames;
	  while(std::getline(packedName, segment, ':'))
	    unpackedNames.push_back(segment);

	  splitTMVAvariablesResonant.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1))); 
	} 

      // split the resonant name in two strings
      cout << "BDT resonant HIGH MASS vars:" << endl;
      vector<pair<string, string>> splitTMVAvariablesResonantHM;
      for (unsigned int iv = 0 ; iv < TMVAvariablesResonantHM.size () ; ++iv)
	{
	  // split my_name:BDT_name in two strings
	  std::stringstream packedName(TMVAvariablesResonantHM.at(iv));
	  std::string segment;
	  std::vector<std::string> unpackedNames;
	  while(std::getline(packedName, segment, ':'))
	    unpackedNames.push_back(segment);

	  // replace "internal" names for graphics names -- shitty parser!!
	  boost::replace_all(unpackedNames.at(1), "_T_", "*");
	  boost::replace_all(unpackedNames.at(1), "__", "()");

	  splitTMVAvariablesResonantHM.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1))); 
	  cout << " ... " << iv << " " << unpackedNames.at(0) << " --> " << unpackedNames.at(1) << endl;
	} 
      cout << endl;

      // split the resonant name in two strings
      vector<pair<string, string>> splitTMVAvariablesResonantLM;
      cout << "BDT resonant LOW MASS vars:" << endl;
      for (unsigned int iv = 0 ; iv < TMVAvariablesResonantLM.size () ; ++iv)
	{
	  // split my_name:BDT_name in two strings
	  std::stringstream packedName(TMVAvariablesResonantLM.at(iv));
	  std::string segment;
	  std::vector<std::string> unpackedNames;
	  while(std::getline(packedName, segment, ':'))
	    unpackedNames.push_back(segment);

	  // replace "internal" names for graphics names -- shitty parser!!
	  boost::replace_all(unpackedNames.at(1), "_T_", "*");
	  boost::replace_all(unpackedNames.at(1), "__", "()");

	  splitTMVAvariablesResonantLM.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1))); 
	  cout << " ... " << iv << " " << unpackedNames.at(0) << " --> " << unpackedNames.at(1) << endl;
	} 

      // split the non resonant name in two strings
      vector<pair<string, string>> splitTMVAvariablesNonResonant;
      cout << "BDT non resonant vars:" << endl;
      for (unsigned int iv = 0 ; iv < TMVAvariablesNonResonant.size () ; ++iv)
	{
	  // split my_name:BDT_name in two strings
	  std::stringstream packedName(TMVAvariablesNonResonant.at(iv));
	  std::string segment;
	  std::vector<std::string> unpackedNames;
	  while(std::getline(packedName, segment, ':'))
	    unpackedNames.push_back(segment);

	  // replace "internal" names for graphics names -- shitty parser!!
	  boost::replace_all(unpackedNames.at(1), "_T_", "*");
	  boost::replace_all(unpackedNames.at(1), "__", "()");

	  splitTMVAvariablesNonResonant.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1))); 
	  cout << " ... " << iv << " " << unpackedNames.at(0) << " --> " << unpackedNames.at(1) << endl;
	}


      // now merge all names into a vector to get a list of uniquely needed elements
      std::vector<string> allVars;
      allVars.insert(allVars.end(), TMVAspectators.begin(), TMVAspectators.end());
      allVars.insert(allVars.end(), TMVAvariables.begin(), TMVAvariables.end());
      for (unsigned int iv = 0; iv < splitTMVAvariablesResonant.size(); ++iv)
        allVars.push_back(splitTMVAvariablesResonant.at(iv).first);
      for (unsigned int iv = 0; iv < splitTMVAvariablesResonantHM.size(); ++iv)
        allVars.push_back(splitTMVAvariablesResonantHM.at(iv).first);
      for (unsigned int iv = 0; iv < splitTMVAvariablesResonantLM.size(); ++iv)
        allVars.push_back(splitTMVAvariablesResonantLM.at(iv).first);
      for (unsigned int iv = 0; iv < splitTMVAvariablesNonResonant.size(); ++iv)
        allVars.push_back(splitTMVAvariablesNonResonant.at(iv).first);

      sort(allVars.begin(), allVars.end());
      allVars.erase( unique( allVars.begin(), allVars.end() ), allVars.end() );
      std::map<string, float> allVarsMap;
      for (string var : allVars)
	allVarsMap[var] = 0.0;

      TFile *outFile = TFile::Open(outputFile,"UPDATE");
      TTree *treenew = (TTree*)outFile->Get("HTauTauTree");

      TMVA::Reader * reader = new TMVA::Reader () ;
      TMVA::Reader * readerResonant = new TMVA::Reader () ;
      TMVA::Reader * readerResonantHM = new TMVA::Reader () ;
      TMVA::Reader * readerResonantLM = new TMVA::Reader () ;
      TMVA::Reader * readerNonResonant = new TMVA::Reader () ;
      Float_t mvatautau,mvamutau, mvaetau, mvaleptau, mvaresonant, mvaresonantHM, mvaresonantLM, mvanonresonant;
      TBranch *mvaBranchmutau;
      TBranch *mvaBranchtautau;
      TBranch *mvaBranchetau;
      TBranch *mvaBranchleptau;
      TBranch *mvaBranchResonant;
      TBranch *mvaBranchResonantHM;
      TBranch *mvaBranchResonantLM;
      TBranch *mvaBranchNonResonant;

      for (string var : TMVAvariables)
	{
	  treenew->SetBranchAddress (var.c_str (), &(allVarsMap.at (var))) ;
	  reader->AddVariable (var, &(allVarsMap.at (var))) ;
	}  

      for (string var : TMVAspectators)
	{
	  treenew->SetBranchAddress (var.c_str (), &(allVarsMap.at (var))) ;
	  reader->AddSpectator (var, &(allVarsMap.at (var))) ;
	}  

      for (pair<string, string> vpair : splitTMVAvariablesResonant)
	{
	  treenew->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerResonant->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;      
	}

      for (pair<string, string> vpair : splitTMVAvariablesResonantHM)
	{
	  treenew->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerResonantHM->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;      
	  // cout << "DEBUG HM: " << vpair.second.c_str () <<  " <-- " << vpair.first.c_str () << endl;
	}

      for (pair<string, string> vpair : splitTMVAvariablesResonantLM)
	{
	  treenew->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerResonantLM->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;      
	}

      for (pair<string, string> vpair : splitTMVAvariablesNonResonant)
	{
	  treenew->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerNonResonant->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;      
	}

      if (doMuTau)  mvaBranchmutau = treenew->Branch ("MuTauKine", &mvamutau, "MuTauKine/F") ;
      if (doETau)   mvaBranchetau = treenew->Branch ("ETauKine", &mvaetau, "ETauKine/F") ;
      if (doTauTau) mvaBranchtautau = treenew->Branch ("TauTauKine", &mvatautau, "TauTauKine/F") ;
      if (doLepTau) mvaBranchleptau = treenew->Branch ("LepTauKine", &mvaleptau, "LepTauKine/F") ;
      if (doResonant) mvaBranchResonant = treenew->Branch ("BDTResonant", &mvaresonant, "BDTResonant/F") ;
      if (doResonantHM) mvaBranchResonantHM = treenew->Branch ("BDTResonantHM", &mvaresonantHM, "BDTResonantHM/F") ;
      if (doResonantLM) mvaBranchResonantLM = treenew->Branch ("BDTResonantLM", &mvaresonantLM, "BDTResonantLM/F") ;
      if (doNonResonant) mvaBranchNonResonant = treenew->Branch ("BDTNonResonant", &mvanonresonant, "BDTNonResonant/F") ;
      //}
      if (doMuTau)   reader->BookMVA ("MuTauKine",  TMVAweightsMuTau.c_str ()) ;
      if (doETau)    reader->BookMVA ("ETauKine",  TMVAweightsETau.c_str ()) ;
      if (doTauTau)  reader->BookMVA ("TauTauKine",  TMVAweightsTauTau.c_str ()) ;
      if (doLepTau)  reader->BookMVA ("LepTauKine",  TMVAweightsLepTau.c_str ()) ;
      if (doResonant)  readerResonant->BookMVA ("BDT_full_mass_iso_nodrbbsv",  TMVAweightsResonant.c_str ()) ;
      if (doResonantHM)  readerResonantHM->BookMVA ("500t_PU_mass_newvars_HIGH_oldvars",  TMVAweightsResonantHM.c_str ()) ;
      if (doResonantLM)  readerResonantLM->BookMVA ("500t_PU_mass_newvars_LOW",  TMVAweightsResonantLM.c_str ()) ;
      if (doNonResonant)  readerNonResonant->BookMVA ("BDT_nonres_SM",  TMVAweightsNonResonant.c_str ()) ;

      int nentries = treenew->GetEntries();
      for(int i=0;i<nentries;i++){
	treenew->GetEntry(i);

	if (doMuTau)   mvamutau= reader->EvaluateMVA ("MuTauKine") ;  
	if (doETau)    mvaetau= reader->EvaluateMVA ("ETauKine") ;  
	if (doTauTau)  mvatautau= reader->EvaluateMVA ("TauTauKine") ;  
	if (doLepTau)  mvaleptau= reader->EvaluateMVA ("LepTauKine") ;  
	if (doResonant)  mvaresonant= readerResonant->EvaluateMVA ("BDT_full_mass_iso_nodrbbsv") ;  
	if (doResonantHM)  mvaresonantHM= readerResonantHM->EvaluateMVA ("500t_PU_mass_newvars_HIGH_oldvars") ;  
	if (doResonantLM)  mvaresonantLM= readerResonantLM->EvaluateMVA ("500t_PU_mass_newvars_LOW") ;  
	if (doNonResonant)  mvanonresonant= readerNonResonant->EvaluateMVA ("BDT_nonres_SM") ;  

	if (doMuTau)    mvaBranchmutau->Fill();
	if (doETau)     mvaBranchetau->Fill();
	if (doTauTau)   mvaBranchtautau->Fill();
	if (doLepTau)   mvaBranchleptau->Fill();
	if (doResonant)  mvaBranchResonant->Fill();
	if (doResonantHM)  mvaBranchResonantHM->Fill();
	if (doResonantLM)  mvaBranchResonantLM->Fill();
	if (doNonResonant)  mvaBranchNonResonant->Fill();
      }

      outFile->cd () ;
      h_eff.Write () ;
      treenew->Write ("", TObject::kOverwrite) ;

      delete reader;
      delete readerResonant;
      delete readerResonantHM;
      delete readerResonantLM;
      delete readerNonResonant;
    }


  // NEW BDT
  bool computeBDTsm = (gConfigParser->isDefined("BDTsm::computeMVA") ? gConfigParser->readBoolOption ("BDTsm::computeMVA") : false);
  bool computeBDTlm = (gConfigParser->isDefined("BDTlm::computeMVA") ? gConfigParser->readBoolOption ("BDTlm::computeMVA") : false);
  bool computeBDTmm = (gConfigParser->isDefined("BDTmm::computeMVA") ? gConfigParser->readBoolOption ("BDTmm::computeMVA") : false);
  bool computeBDThm = (gConfigParser->isDefined("BDThm::computeMVA") ? gConfigParser->readBoolOption ("BDThm::computeMVA") : false);

  if (computeBDTsm || computeBDTlm || computeBDTmm || computeBDThm)
  {
    cout << " ------------ ############### ----- NEW BDT ----- ############### ------------ " <<endl;

    bool doSM = computeBDTsm;
    bool doLM = computeBDTlm;
    bool doMM = computeBDTmm;
    bool doHM = computeBDThm;

    // weights file
    string TMVAweightsSM = "";
    string TMVAweightsLM = "";
    string TMVAweightsMM = "";
    string TMVAweightsHM = "";
    vector<float> SM_kl;
    vector<float> LM_mass;
    vector<float> MM_mass;
    vector<float> HM_mass;
    vector<int> LM_spin;
    vector<int> MM_spin;
    vector<int> HM_spin;

    if (doSM)
    {
      TMVAweightsSM = gConfigParser->readStringOption ("BDTsm::weights");
      SM_kl         = gConfigParser->readFloatListOption("BDTsm::kl");
    }
    if (doLM)
    {
      TMVAweightsLM = gConfigParser->readStringOption ("BDTlm::weights");
      LM_mass       = gConfigParser->readFloatListOption ("BDTlm::mass");
      LM_spin       = gConfigParser->readIntListOption ("BDTlm::spin");
    }
    if (doMM)
    {
      TMVAweightsMM = gConfigParser->readStringOption ("BDTmm::weights");
      MM_mass       = gConfigParser->readFloatListOption ("BDTmm::mass");
      MM_spin       = gConfigParser->readIntListOption ("BDTmm::spin");
    }
    if (doHM)
    {
      TMVAweightsHM = gConfigParser->readStringOption ("BDThm::weights");
      HM_mass       = gConfigParser->readFloatListOption ("BDThm::mass");
      HM_spin       = gConfigParser->readIntListOption ("BDThm::spin");
    }

    // Input variables
    vector<string> TMVAvariablesSM = ( doSM ? gConfigParser->readStringListOption ("BDTsm::variables") : vector<string>(0) );
    vector<string> TMVAvariablesLM = ( doLM ? gConfigParser->readStringListOption ("BDTlm::variables") : vector<string>(0) );
    vector<string> TMVAvariablesMM = ( doMM ? gConfigParser->readStringListOption ("BDTmm::variables") : vector<string>(0) );
    vector<string> TMVAvariablesHM = ( doHM ? gConfigParser->readStringListOption ("BDThm::variables") : vector<string>(0) );
    
    // Split the resonant name in two strings
    vector<pair<string, string>> splitTMVAvariablesSM;
    for (unsigned int iv = 0 ; iv < TMVAvariablesSM.size () ; ++iv)
    {
      // Split my_name:BDT_name in two strings
      std::stringstream packedName(TMVAvariablesSM.at(iv));
      std::string segment;
      std::vector<std::string> unpackedNames;
      while(std::getline(packedName, segment, ':'))
        unpackedNames.push_back(segment);

      splitTMVAvariablesSM.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1)));
    }
    
    vector<pair<string, string>> splitTMVAvariablesLM;
    for (unsigned int iv = 0 ; iv < TMVAvariablesLM.size () ; ++iv)
    {
      // Split my_name:BDT_name in two strings
      std::stringstream packedName(TMVAvariablesLM.at(iv));
      std::string segment;
      std::vector<std::string> unpackedNames;
      while(std::getline(packedName, segment, ':'))
        unpackedNames.push_back(segment);

      splitTMVAvariablesLM.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1)));
    }

    vector<pair<string, string>> splitTMVAvariablesMM;
    for (unsigned int iv = 0 ; iv < TMVAvariablesMM.size () ; ++iv)
    {
      // Split my_name:BDT_name in two strings
      std::stringstream packedName(TMVAvariablesMM.at(iv));
      std::string segment;
      std::vector<std::string> unpackedNames;
      while(std::getline(packedName, segment, ':'))
        unpackedNames.push_back(segment);

      splitTMVAvariablesMM.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1)));
    }

    vector<pair<string, string>> splitTMVAvariablesHM;
    for (unsigned int iv = 0 ; iv < TMVAvariablesHM.size () ; ++iv)
    {
      // Split my_name:BDT_name in two strings
      std::stringstream packedName(TMVAvariablesHM.at(iv));
      std::string segment;
      std::vector<std::string> unpackedNames;
      while(std::getline(packedName, segment, ':'))
        unpackedNames.push_back(segment);

      splitTMVAvariablesHM.push_back(make_pair(unpackedNames.at(0), unpackedNames.at(1)));
    }

    // Now merge all names into a vector to get a list of uniquely needed elements
    std::vector<string> allVars;
    for (unsigned int iv = 0; iv < splitTMVAvariablesSM.size(); ++iv)
      allVars.push_back(splitTMVAvariablesSM.at(iv).first);
    for (unsigned int iv = 0; iv < splitTMVAvariablesLM.size(); ++iv)
      allVars.push_back(splitTMVAvariablesLM.at(iv).first);
    for (unsigned int iv = 0; iv < splitTMVAvariablesMM.size(); ++iv)
      allVars.push_back(splitTMVAvariablesMM.at(iv).first);
    for (unsigned int iv = 0; iv < splitTMVAvariablesHM.size(); ++iv)
      allVars.push_back(splitTMVAvariablesHM.at(iv).first);
      
    sort(allVars.begin(), allVars.end());
    allVars.erase( unique( allVars.begin(), allVars.end() ), allVars.end() );
    
    // Create map to contain values of variables
    std::map<string, float> allVarsMap;
    for (string var : allVars)
    allVarsMap[var] = 0.0;

    // Open tree to be updated
    TFile *outFile = TFile::Open(outputFile,"UPDATE");
    TTree *treenew = (TTree*)outFile->Get("HTauTauTree");
    int nentries = treenew->GetEntries();

    // Create vectors to store all the BDT outputs and relative vectors of TBranches
    std::vector<float> outSM (SM_kl.size());
    std::vector<float> outLM (LM_spin.size()*LM_mass.size());
    std::vector<float> outMM (MM_spin.size()*MM_mass.size());
    std::vector<float> outHM (HM_spin.size()*HM_mass.size());
    
    std::vector<TBranch*> branchSM (SM_kl.size());
    std::vector<TBranch*> branchLM (LM_spin.size()*LM_mass.size());
    std::vector<TBranch*> branchMM (MM_spin.size()*MM_mass.size());
    std::vector<TBranch*> branchHM (HM_spin.size()*HM_mass.size());

    // Declare the TMVA readers
    TMVA::Reader * readerSM = new TMVA::Reader () ;
    TMVA::Reader * readerLM = new TMVA::Reader () ;
    TMVA::Reader * readerMM = new TMVA::Reader () ;
    TMVA::Reader * readerHM = new TMVA::Reader () ;

    // Assign variables to SM reader
    for (pair<string, string> vpair : splitTMVAvariablesSM)
	{
	  treenew ->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerSM->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;
	}
    // Add the kl variable to the SM reader
    float kl_var;
    readerSM->AddVariable("kl", &kl_var);


    // Assign variables to LM reader
    for (pair<string, string> vpair : splitTMVAvariablesLM)
	{
	  treenew ->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerLM->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;
	}
    // Add mass, channel and spin to the LM reader
    float mass_LM;
    float channel_LM;
    float spin_LM;
    readerLM->AddVariable("mass", &mass_LM);
    treenew ->SetBranchAddress ("BDT_channel", &channel_LM) ;
    readerLM->AddVariable("channel", &channel_LM);
    readerLM->AddVariable("spin", &spin_LM);


    // Assign variables to MM reader
    for (pair<string, string> vpair : splitTMVAvariablesMM)
	{
	  treenew ->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerMM->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;
	}
    // Add mass, channel and spin to the LM reader
    float mass_MM;
    float channel_MM;
    float spin_MM;
    readerMM->AddVariable("mass", &mass_MM);
    treenew ->SetBranchAddress ("BDT_channel", &channel_MM) ;
    readerMM->AddVariable("channel", &channel_MM);
    readerMM->AddVariable("spin", &spin_MM);


    // Assign variables to HM reader
    for (pair<string, string> vpair : splitTMVAvariablesHM)
	{
	  treenew ->SetBranchAddress (vpair.first.c_str (), &(allVarsMap.at (vpair.first))) ;
	  readerHM->AddVariable (vpair.second.c_str (), &(allVarsMap.at (vpair.first))) ;
	}
    // Add mass, channel and spin to the LM reader
    float mass_HM;
    float channel_HM;
    float spin_HM;
    readerHM->AddVariable("mass", &mass_HM);
    treenew ->SetBranchAddress ("BDT_channel", &channel_HM) ;
    readerHM->AddVariable("channel", &channel_HM);
    readerHM->AddVariable("spin", &spin_HM);


    // Book the MVA methods
    if(doSM) readerSM->BookMVA("Grad_1", TMVAweightsSM.c_str() );
    if(doLM) readerLM->BookMVA("Grad_2", TMVAweightsLM.c_str() );
    if(doMM) readerMM->BookMVA("Grad_3", TMVAweightsMM.c_str() );
    if(doHM) readerHM->BookMVA("Grad_4", TMVAweightsHM.c_str() );

    // Calculate BDT output for SM
    if (doSM)
    {
        int idxSM = 0;
        for (unsigned int ikl = 0; ikl < SM_kl.size(); ++ikl)
        {
            // Declare the BDT output branch
            std::string branch_name = boost::str(boost::format("BDToutSM_kl_%d") % SM_kl.at(ikl));
            branchSM.at(idxSM) = treenew->Branch(branch_name.c_str(), &outSM.at(idxSM));

            // Assign value to parametrization variables
            kl_var = SM_kl.at(ikl);

            // Calculate BDT output
            for(int i=0;i<nentries;i++)
            {
                treenew->GetEntry(i);

                /*allVarsMap.at("BDT_channel") = 2.;
                //cout << "--------  NEW ENTRY  ---------"<< endl;
                cout << " channel: " << allVarsMap.at("BDT_channel") << endl;
                for (pair<string, string> vpair : splitTMVAvariablesSM)
                {
                  cout << "Name: " << vpair.first.c_str () << "  --  val: " << allVarsMap.at (vpair.first) << endl;
                }
                cout << "-------------------------------"<< endl;*/
                outSM.at(idxSM) = readerSM->EvaluateMVA("Grad_1");
                branchSM.at(idxSM)->Fill();
            }
            ++idxSM;
        }
    }


    // Calculate BDT output for LM
    if (doLM)
    {
        int idxLM = 0;
        for (unsigned int ispin = 0; ispin < LM_spin.size(); ++ispin)
        {
            for (unsigned int imass = 0; imass < LM_mass.size(); ++imass)
            {
                // Declare the BDT output branch
                std::string branch_name = boost::str(boost::format("BDToutLM_spin_%d_mass_%d") % LM_spin.at(ispin) % LM_mass.at(imass));
                branchLM.at(idxLM) = treenew->Branch(branch_name.c_str(), &outLM.at(idxLM));

                // Assign value to parametrization variables
                mass_LM = LM_mass.at(imass);
                spin_LM = LM_spin.at(ispin);

                // Calculate BDT output
                for(int i=0;i<nentries;i++)
                {
                    treenew->GetEntry(i);

                    /*cout << "--------  NEW ENTRY LM  ---------"<< endl;
                    for (pair<string, string> vpair : splitTMVAvariablesLM)
                    {
                      cout << "Name: " << vpair.first.c_str () << "  --  val: " << allVarsMap.at (vpair.first) << endl;
                    }
                    cout << "Name: channelLM  --  val: " << channel_LM << endl;
                    cout << "-------------------------------"<< endl;*/

                    outLM.at(idxLM) = readerLM->EvaluateMVA("Grad_2");
                    branchLM.at(idxLM)->Fill();
                }
                ++idxLM;
            }
        }
    }


    // Calculate BDT output for MM
    if (doMM)
    {
        int idxMM = 0;
        for (unsigned int ispin = 0; ispin < MM_spin.size(); ++ispin)
        {
            for (unsigned int imass = 0; imass < MM_mass.size(); ++imass)
            {
                // Declare the BDT output branch
                std::string branch_name = boost::str(boost::format("BDToutMM_spin_%d_mass_%d") % MM_spin.at(ispin) % MM_mass.at(imass));
                branchMM.at(idxMM) = treenew->Branch(branch_name.c_str(), &outMM.at(idxMM));

                // Assign value to parametrization variables
                mass_MM = MM_mass.at(imass);
                spin_MM = MM_spin.at(ispin);

                // Calculate BDT output
                for(int i=0;i<nentries;i++)
                {
                    treenew->GetEntry(i);
                    outMM.at(idxMM) = readerMM->EvaluateMVA("Grad_3");
                    branchMM.at(idxMM)->Fill();
                }
                ++idxMM;
            }
        }
    }


    // Calculate BDT output for HM
    if (doHM)
    {
        int idxHM = 0;
        for (unsigned int ispin = 0; ispin < HM_spin.size(); ++ispin)
        {
            for (unsigned int imass = 0; imass < HM_mass.size(); ++imass)
            {
                // Declare the BDT output branch
                std::string branch_name = boost::str(boost::format("BDToutHM_spin_%d_mass_%d") % HM_spin.at(ispin) % HM_mass.at(imass));
                branchHM.at(idxHM) = treenew->Branch(branch_name.c_str(), &outHM.at(idxHM));

                // Assign value to parametrization variables
                mass_HM = HM_mass.at(imass);
                spin_HM = HM_spin.at(ispin);

                // Calculate BDT output
                for(int i=0;i<nentries;i++)
                {
                    treenew->GetEntry(i);
                    outHM.at(idxHM) = readerHM->EvaluateMVA("Grad_4");
                    branchHM.at(idxHM)->Fill();
                }
                ++idxHM;
            }
        }
    }

    // Update tree and delete readers
    outFile->cd();
    treenew->Write ("", TObject::kOverwrite) ;
    delete readerSM;
    delete readerLM;
    delete readerMM;
    delete readerHM;

  } // End new BDT


  cout << "... SKIM finished, exiting." << endl;
  return 0 ;
}