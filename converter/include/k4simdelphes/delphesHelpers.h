#ifndef DELPHESEDM4HEP_DELPHESHELPERS
#define DELPHESEDM4HEP_DELPHESHELPERS

#include "classes/DelphesClasses.h"

#include "TRefArray.h"

#include <set>

// TODO: If CLHEP ever gets part of edm4hep, take this from there.
static constexpr double c_light = 2.99792458e+8;

// TODO: check whether the set is necessary or if this could also be a vector
// since all the IDs are already deduplicated
template<typename DelphesT>
std::set<UInt_t> getAllParticleIDs(DelphesT* delphesCand) {
  const auto& refArray = delphesCand->Particles;
  std::set<UInt_t> relatedParticles;
  for (int i = 0; i < refArray.GetEntries(); ++i) {
    relatedParticles.insert(refArray.At(i)->GetUniqueID());
  }

  return relatedParticles;
}

// Following what is done in TreeWriter::FillParticles
std::set<UInt_t> getAllParticleIDs(Candidate* candidate) {
  std::set<UInt_t> relatedParticles;
  TIter it1(candidate->GetCandidates());
  it1.Reset();

  while((candidate = static_cast<Candidate*>(it1.Next()))) {
    TIter it2(candidate->GetCandidates());

    // particle
    if (candidate->GetCandidates()->GetEntriesFast() == 0) {
      relatedParticles.insert(candidate->GetUniqueID());
      continue;
    }

    // track
    candidate = static_cast<Candidate*>(candidate->GetCandidates()->At(0));
    if (candidate->GetCandidates()->GetEntriesFast() == 0) {
      relatedParticles.insert(candidate->GetUniqueID());
      continue;
    }

    // tower
    it2.Reset();
    while((candidate = static_cast<Candidate*>(it2.Next()))) {
      relatedParticles.insert(candidate->GetCandidates()->At(0)->GetUniqueID());
    }
  }

  return relatedParticles;
}

#endif
