#ifndef RecoLocalCalo_HGCalRecProducers_HGCalCLUEAlgo_h
#define RecoLocalCalo_HGCalRecProducers_HGCalCLUEAlgo_h

#include "RecoLocalCalo/HGCalRecProducers/interface/HGCalClusteringAlgoBase.h"

#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"

#include "DataFormats/DetId/interface/DetId.h"
#include "DataFormats/HGCRecHit/interface/HGCRecHitCollections.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloGeometry.h"
#include "Geometry/CaloGeometry/interface/CaloSubdetectorGeometry.h"
#include "Geometry/CaloGeometry/interface/TruncatedPyramid.h"
#include "Geometry/CaloTopology/interface/HGCalTopology.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
#include "Geometry/Records/interface/CaloGeometryRecord.h"

#include "DataFormats/EgammaReco/interface/BasicCluster.h"
#include "DataFormats/Math/interface/Point3D.h"

#include "RecoLocalCalo/HGCalRecAlgos/interface/RecHitTools.h"
#include "RecoLocalCalo/HGCalRecAlgos/interface/KDTreeLinkerAlgoT.h"

// C/C++ headers
#include <set>
#include <string>
#include <vector>


class HGCalCLUEAlgo : public HGCalClusteringAlgoBase {
 public:

 HGCalCLUEAlgo(const edm::ParameterSet& ps)
  : HGCalClusteringAlgoBase(
      (HGCalClusteringAlgoBase::VerbosityLevel)ps.getUntrackedParameter<unsigned int>("verbosity",3),
      reco::CaloCluster::undefined),
     thresholdW0_(ps.getParameter<std::vector<double> >("thresholdW0")),
     positionDeltaRho_c_(ps.getParameter<std::vector<double> >("positionDeltaRho_c")),
     vecDeltas_(ps.getParameter<std::vector<double> >("deltac")),
     kappa_(ps.getParameter<double>("kappa")),
     ecut_(ps.getParameter<double>("ecut")),
     dependSensor_(ps.getParameter<bool>("dependSensor")),
     dEdXweights_(ps.getParameter<std::vector<double> >("dEdXweights")),
     thicknessCorrection_(ps.getParameter<std::vector<double> >("thicknessCorrection")),
     fcPerMip_(ps.getParameter<std::vector<double> >("fcPerMip")),
     fcPerEle_(ps.getParameter<double>("fcPerEle")),
     nonAgedNoises_(ps.getParameter<edm::ParameterSet>("noises").getParameter<std::vector<double> >("values")),
     noiseMip_(ps.getParameter<edm::ParameterSet>("noiseMip").getParameter<double>("value")),
     initialized_(false),
     points_(2*(maxlayer+1)),
     minpos_(2*(maxlayer+1),{ {0.0f,0.0f} }),
     maxpos_(2*(maxlayer+1),{ {0.0f,0.0f} }) {}

  ~HGCalCLUEAlgo() override {}

  void populate(const HGCRecHitCollection &hits) override;
  // this is the method that will start the clusterisation (it is possible to invoke this method
  // more than once - but make sure it is with different hit collections (or else use reset)

  void makeClusters() override;

  // this is the method to get the cluster collection out
  std::vector<reco::BasicCluster> getClusters(bool) override;

  // use this if you want to reuse the same cluster object but don't want to accumulate clusters
  // (hardly useful?)
  void reset() override {
    clusters_v_.clear();
    layerClustersPerLayer_.clear();
    for (auto &it : points_) {
      it.clear();
      std::vector<KDNode>().swap(it);
    }
    for (unsigned int i = 0; i < minpos_.size(); i++) {
      minpos_[i][0] = 0.;
      minpos_[i][1] = 0.;
      maxpos_[i][0] = 0.;
      maxpos_[i][1] = 0.;
    }
  }
  void computeThreshold();

  static void fillPSetDescription(edm::ParameterSetDescription& iDesc) {
    iDesc.add<std::vector<double>>("thresholdW0", {
        2.9,
        2.9,
        2.9
        });
    iDesc.add<std::vector<double>>("positionDeltaRho_c", {
        1.3,
        1.3,
        1.3
        });
    iDesc.add<std::vector<double>>("deltac", {
        1.3,
        1.3,
        5.0,
        });
    iDesc.add<bool>("dependSensor", true);
    iDesc.add<double>("ecut", 3.0);
    iDesc.add<double>("kappa", 9.0);
    iDesc.addUntracked<unsigned int>("verbosity", 3);
    iDesc.add<std::vector<double>>("dEdXweights",{});
    iDesc.add<std::vector<double>>("thicknessCorrection",{});
    iDesc.add<std::vector<double>>("fcPerMip",{});
    iDesc.add<double>("fcPerEle",0.0);
    edm::ParameterSetDescription descNestedNoises;
    descNestedNoises.add<std::vector<double> >("values", {});
    iDesc.add<edm::ParameterSetDescription>("noises", descNestedNoises);
    edm::ParameterSetDescription descNestedNoiseMIP;
    descNestedNoiseMIP.add<double>("value", 0 );
    iDesc.add<edm::ParameterSetDescription>("noiseMip", descNestedNoiseMIP);
  }

  /// point in the space
  typedef math::XYZPoint Point;

 private:
  // To compute the cluster position
  std::vector<double> thresholdW0_;
  std::vector<double> positionDeltaRho_c_;

  // The two parameters used to identify clusters
  std::vector<double> vecDeltas_;
  double kappa_;

  // The hit energy cutoff
  double ecut_;

  // various parameters used for calculating the noise levels for a given sensor (and whether to use
  // them)
  bool dependSensor_;
  std::vector<double> dEdXweights_;
  std::vector<double> thicknessCorrection_;
  std::vector<double> fcPerMip_;
  double fcPerEle_;
  std::vector<double> nonAgedNoises_;
  double noiseMip_;
  std::vector<std::vector<double> > thresholds_;
  std::vector<std::vector<double> > v_sigmaNoise_;

  // initialization bool
  bool initialized_;

  struct Hexel {
    double x;
    double y;
    double z;
    bool isHalfCell;
    double weight;
    double fraction;
    DetId detid;
    double rho;
    double delta;
    int nearestHigher;
    int clusterIndex;
    float sigmaNoise;
    float thickness;
    const hgcal::RecHitTools *tools;

    Hexel(const HGCRecHit &hit, DetId id_in, bool isHalf, float sigmaNoise_in, float thickness_in,
          const hgcal::RecHitTools *tools_in)
        : isHalfCell(isHalf),
          weight(0.),
          fraction(1.0),
          detid(id_in),
          rho(0.),
          delta(0.),
          nearestHigher(-1),
          clusterIndex(-1),
          sigmaNoise(sigmaNoise_in),
          thickness(thickness_in),
          tools(tools_in) {
      const GlobalPoint position(tools->getPosition(detid));
      weight = hit.energy();
      x = position.x();
      y = position.y();
      z = position.z();
    }
    Hexel()
        : x(0.),
          y(0.),
          z(0.),
          isHalfCell(false),
          weight(0.),
          fraction(1.0),
          detid(),
          rho(0.),
          delta(0.),
          nearestHigher(-1),
          clusterIndex(-1),
          sigmaNoise(0.),
          thickness(0.),
          tools(nullptr) {}
    bool operator>(const Hexel &rhs) const { return (rho > rhs.rho); }
  };

  typedef KDTreeLinkerAlgo<Hexel, 2> KDTree;
  typedef KDTreeNodeInfoT<Hexel, 2> KDNode;

  std::vector<std::vector<std::vector<KDNode> > > layerClustersPerLayer_;

  std::vector<size_t> sort_by_delta(const std::vector<KDNode> &v) const {
    std::vector<size_t> idx(v.size());
    std::iota(std::begin(idx), std::end(idx), 0);
    sort(idx.begin(), idx.end(),
         [&v](size_t i1, size_t i2) { return v[i1].data.delta > v[i2].data.delta; });
    return idx;
  }

  std::vector<std::vector<KDNode> > points_;  // a vector of vectors of hexels, one for each layer
  //@@EM todo: the number of layers should be obtained programmatically - the range is 1-n instead
  //of 0-n-1...

  std::vector<std::array<float, 2> > minpos_;
  std::vector<std::array<float, 2> > maxpos_;

  // these functions should be in a helper class.
  inline double distance2(const Hexel &pt1, const Hexel &pt2) const {  // distance squared
    const double dx = pt1.x - pt2.x;
    const double dy = pt1.y - pt2.y;
    return (dx * dx + dy * dy);
  }  // distance squaredq
  inline double distance(const Hexel &pt1,
                         const Hexel &pt2) const {  // 2-d distance on the layer (x-y)
    return std::sqrt(distance2(pt1, pt2));
  }
  double calculateLocalDensity(std::vector<KDNode> &, KDTree &,
                               const unsigned int) const;  // return max density
  double calculateDistanceToHigher(std::vector<KDNode> &) const;
  int findAndAssignClusters(std::vector<KDNode> &, KDTree &, double, KDTreeBox &,
                            const unsigned int, std::vector<std::vector<KDNode> > &) const;
  math::XYZPoint calculatePosition(std::vector<KDNode> &) const;
};

#endif
